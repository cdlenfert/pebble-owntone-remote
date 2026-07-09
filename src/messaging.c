#include <pebble.h>
#include "message_keys.h"
#include "messaging.h"
#include "windows/player.h"
#include "app_auto_close.h"

// Forward declarations
static void inbox_received_callback(DictionaryIterator *iterator, void *context);
static void inbox_dropped_callback(AppMessageResult reason, void *context);
static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context);
static void outbox_sent_callback(DictionaryIterator *iterator, void *context);

static PlayerStateCallback s_player_callback = NULL;
static SearchResultsCallback s_results_callback = NULL;
static OutputsCallback s_outputs_callback = NULL;
static StatusCallback s_status_callback = NULL;
static FavoritesCallback s_favorites_callback = NULL;
static QueueCallback s_queue_callback = NULL;

// Cached player state (helps avoid race where JS replies while UI not yet registered)
static bool s_have_cached_player = false;
static PlayerState s_cached_state = PLAYER_STATE_STOPPED;
static char s_cached_track[MAX_STRING_LENGTH] = {0};
static char s_cached_artist[MAX_STRING_LENGTH] = {0};
static char s_cached_album[MAX_STRING_LENGTH] = {0};
static int s_cached_volume = 50;

void message_init(void) {
  APP_LOG(APP_LOG_LEVEL_INFO, "messaging: message_init() called");
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  app_message_open(PLATFORM_INBOX_SIZE, PLATFORM_OUTBOX_SIZE);
  APP_LOG(APP_LOG_LEVEL_INFO, "messaging: app_message_open() done");
}

void message_deinit(void) {
  app_message_deregister_callbacks();
}

void message_set_player_callback(PlayerStateCallback callback) {
  s_player_callback = callback;
}

void message_set_results_callback(SearchResultsCallback callback) {
  s_results_callback = callback;
}

void message_set_outputs_callback(OutputsCallback callback) {
  s_outputs_callback = callback;
}

void message_set_status_callback(StatusCallback callback) {
  s_status_callback = callback;
}

void message_set_favorites_callback(FavoritesCallback callback) {
  s_favorites_callback = callback;
}

void message_set_queue_callback(QueueCallback callback) {
  s_queue_callback = callback;
}

void message_send_command(CommandType cmd) {
  DictionaryIterator *out;
  if (app_message_outbox_begin(&out) == APP_MSG_OK) {
    dict_write_uint8(out, KEY_CMD, cmd);
    app_message_outbox_send();
  }
}

void message_send_search(ContentType type, const char *query) {
  DictionaryIterator *out;
  if (app_message_outbox_begin(&out) == APP_MSG_OK) {
    dict_write_uint8(out, KEY_CMD, CMD_SEARCH);
    dict_write_uint8(out, KEY_TYPE, type);
    dict_write_cstring(out, KEY_QUERY, query);
    app_message_outbox_send();
  }
}

void message_send_random(ContentType type) {
  DictionaryIterator *out;
  if (app_message_outbox_begin(&out) == APP_MSG_OK) {
    dict_write_uint8(out, KEY_CMD, CMD_RANDOM);
    dict_write_uint8(out, KEY_TYPE, type);
    app_message_outbox_send();
  }
}

void message_send_add_to_queue(const char *uri, ContentType type) {
  DictionaryIterator *out;
  if (app_message_outbox_begin(&out) == APP_MSG_OK) {
    dict_write_uint8(out, KEY_CMD, CMD_ADD_TO_QUEUE);
    dict_write_cstring(out, KEY_URI, uri);
    dict_write_uint8(out, KEY_TYPE, type);
    app_message_outbox_send();
  }
}

void message_send_set_volume(int volume) {
  DictionaryIterator *out;
  if (app_message_outbox_begin(&out) == APP_MSG_OK) {
    dict_write_uint8(out, KEY_CMD, CMD_SET_VOLUME);
    dict_write_uint8(out, KEY_VOLUME, volume);
    app_message_outbox_send();
  }
}

void message_send_set_output_exclusive(const char *output_id) {
  DictionaryIterator *out;
  if (app_message_outbox_begin(&out) == APP_MSG_OK) {
    dict_write_uint8(out, KEY_CMD, CMD_SET_OUTPUT_EXCLUSIVE);
    dict_write_cstring(out, KEY_OUTPUT_ID, output_id);
    app_message_outbox_send();
  }
}

void message_send_toggle_output(const char *output_id) {
  DictionaryIterator *out;
  if (app_message_outbox_begin(&out) == APP_MSG_OK) {
    dict_write_uint8(out, KEY_CMD, CMD_TOGGLE_OUTPUT);
    dict_write_cstring(out, KEY_OUTPUT_ID, output_id);
    app_message_outbox_send();
  }
}

void message_send_set_output_volume(const char *output_id, int volume) {
  DictionaryIterator *out;
  if (app_message_outbox_begin(&out) == APP_MSG_OK) {
    dict_write_uint8(out, KEY_CMD, CMD_SET_OUTPUT_VOLUME);
    dict_write_cstring(out, KEY_OUTPUT_ID, output_id);
    dict_write_uint8(out, KEY_VOLUME, volume);
    app_message_outbox_send();
  }
}

void message_send_get_favorites(void) {
  DictionaryIterator *out;
  if (app_message_outbox_begin(&out) == APP_MSG_OK) {
    dict_write_uint8(out, KEY_CMD, CMD_GET_FAVORITES);
    app_message_outbox_send();
  }
}

void message_send_play_queue_item(int item_id) {
  DictionaryIterator *out;
  if (app_message_outbox_begin(&out) == APP_MSG_OK) {
    dict_write_uint8(out, KEY_CMD, CMD_PLAY_QUEUE_ITEM);
    dict_write_int32(out, KEY_QUEUE_ITEM_ID, item_id);
    app_message_outbox_send();
  }
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  Tuple *t;
  
  // Check for status response (e.g., play/pause confirmation)
  t = dict_find(iterator, KEY_STATUS);
  if (t && s_status_callback) {
    s_status_callback(t->value->uint8);
    return; // Status is standalone message
  }
  
  // Check for player state update
  t = dict_find(iterator, KEY_PLAYER_STATE);
  if (t) {
    PlayerState state = (PlayerState)t->value->uint8;
    const char *track = "";
    const char *artist = "";
    const char *album = "";
    int volume = 50;

    Tuple *track_t = dict_find(iterator, KEY_PLAYER_TRACK);
    if (track_t) track = track_t->value->cstring;

    Tuple *artist_t = dict_find(iterator, KEY_PLAYER_ARTIST);
    if (artist_t) artist = artist_t->value->cstring;

    Tuple *album_t = dict_find(iterator, KEY_PLAYER_ALBUM);
    if (album_t) album = album_t->value->cstring;

    Tuple *vol_t = dict_find(iterator, KEY_PLAYER_VOLUME);
    if (vol_t) volume = vol_t->value->uint8;

    // Cache the latest player state so UI can pick it up after a race
    s_cached_state = state;
    strncpy(s_cached_track, track, sizeof(s_cached_track)-1);
    s_cached_track[sizeof(s_cached_track)-1] = '\0';
    strncpy(s_cached_artist, artist, sizeof(s_cached_artist)-1);
    s_cached_artist[sizeof(s_cached_artist)-1] = '\0';
    strncpy(s_cached_album, album, sizeof(s_cached_album)-1);
    s_cached_album[sizeof(s_cached_album)-1] = '\0';
    s_cached_volume = volume;
    s_have_cached_player = true;

    

    // Deliver to registered callback if any. Use our cached copies so the
    // TextLayer doesn't end up referencing the transient AppMessage buffer
    // (which becomes invalid after this callback returns).
    if (s_player_callback) {
      s_player_callback(state, s_cached_track, s_cached_artist, s_cached_album, volume);
    }
  }
  
  // Check for search/random results
  t = dict_find(iterator, KEY_RESULT_COUNT);
  if (t && s_results_callback) {
    int count = t->value->uint8;
    static char *titles[MAX_RESULTS];
    static char *uris[MAX_RESULTS];
    
    for (int i = 0; i < MAX_RESULTS; i++) {
      if (titles[i]) {
        free(titles[i]);
        titles[i] = NULL;
      }
      if (uris[i]) {
        free(uris[i]);
        uris[i] = NULL;
      }
    }
    
    for (int i = 0; i < count && i < MAX_RESULTS; i++) {
      Tuple *title_t = dict_find(iterator, KEY_RESULT_TITLE_BASE + i);
      if (title_t) {
        titles[i] = malloc(strlen(title_t->value->cstring) + 1);
        if (titles[i]) strcpy(titles[i], title_t->value->cstring);
      }
      
      Tuple *uri_t = dict_find(iterator, KEY_RESULT_URI_BASE + i);
      if (uri_t) {
        uris[i] = malloc(strlen(uri_t->value->cstring) + 1);
        if (uris[i]) strcpy(uris[i], uri_t->value->cstring);
      }
    }
    
    s_results_callback(count < MAX_RESULTS ? count : MAX_RESULTS, titles, uris);
    // Free our copies; the callback already duplicated what it needs
    for (int i = 0; i < MAX_RESULTS; i++) {
      if (titles[i]) { free(titles[i]); titles[i] = NULL; }
      if (uris[i])   { free(uris[i]);   uris[i]   = NULL; }
    }
  }
  
  // Check for outputs list
  t = dict_find(iterator, KEY_OUTPUT_COUNT);
  if (t && s_outputs_callback) {
    int count = t->value->uint8;
    static char *names[MAX_OUTPUTS];
    static char *ids[MAX_OUTPUTS];
    static int volumes[MAX_OUTPUTS];
    static bool enabled[MAX_OUTPUTS];
    
    for (int i = 0; i < MAX_OUTPUTS; i++) {
      if (names[i]) {
        free(names[i]);
        names[i] = NULL;
      }
      if (ids[i]) {
        free(ids[i]);
        ids[i] = NULL;
      }
      volumes[i] = 0;
      enabled[i] = false;
    }
    
    for (int i = 0; i < count && i < MAX_OUTPUTS; i++) {
      Tuple *name_t = dict_find(iterator, KEY_OUTPUT_NAME_BASE + i);
      if (name_t) {
        names[i] = malloc(strlen(name_t->value->cstring) + 1);
        if (names[i]) strcpy(names[i], name_t->value->cstring);
      }
      
      Tuple *id_t = dict_find(iterator, KEY_OUTPUT_ID_BASE + i);
      if (id_t) {
        ids[i] = malloc(strlen(id_t->value->cstring) + 1);
        if (ids[i]) strcpy(ids[i], id_t->value->cstring);
      }
      
      Tuple *vol_t = dict_find(iterator, KEY_OUTPUT_VOLUME_BASE + i);
      if (vol_t) volumes[i] = vol_t->value->uint8;
      
      Tuple *en_t = dict_find(iterator, KEY_OUTPUT_ENABLED_BASE + i);
      if (en_t) enabled[i] = en_t->value->uint8 != 0;
    }
    
    s_outputs_callback(count, names, ids, volumes, enabled);
    // Free our copies; the callback already duplicated what it needs
    for (int i = 0; i < MAX_OUTPUTS; i++) {
      if (names[i]) { free(names[i]); names[i] = NULL; }
      if (ids[i])   { free(ids[i]);   ids[i]   = NULL; }
    }
  }
  
  // Check for favorites list
  t = dict_find(iterator, KEY_FAVORITE_COUNT);
  if (t && s_favorites_callback) {
    int count = t->value->uint8;
    static char *names[MAX_FAVORITES];
    static int types[MAX_FAVORITES];
    
    for (int i = 0; i < MAX_FAVORITES; i++) {
      if (names[i]) {
        free(names[i]);
        names[i] = NULL;
      }
      types[i] = 0;
    }
    
    for (int i = 0; i < count && i < MAX_FAVORITES; i++) {
      Tuple *name_t = dict_find(iterator, favorite_name_key(i));
      if (name_t) {
        names[i] = malloc(strlen(name_t->value->cstring) + 1);
        if (names[i]) strcpy(names[i], name_t->value->cstring);
      }
      
      Tuple *type_t = dict_find(iterator, favorite_type_key(i));
      if (type_t) {
        // Handle both integer and string-encoded types coming from JS
        if (type_t->type == TUPLE_CSTRING) {
          types[i] = atoi(type_t->value->cstring);
        } else {
          types[i] = type_t->value->uint8;
        }
      }
    }

    // Favorites received; hand off to callback
    s_favorites_callback(count, names, types);
    // Free our copies; the callback already duplicated what it needs
    for (int i = 0; i < MAX_FAVORITES; i++) {
      if (names[i]) { free(names[i]); names[i] = NULL; }
    }
  }
  
  // Check for queue list
  t = dict_find(iterator, KEY_QUEUE_COUNT);
  if (t && s_queue_callback) {
    int count = t->value->uint8;
    int selected_index = 0;
    
    // Get selected index if available
    Tuple *selected_t = dict_find(iterator, KEY_QUEUE_SELECTED);
    if (selected_t) {
      selected_index = selected_t->value->int8;
    }
    
    static char *titles[MAX_QUEUE_ITEMS];
    static char *artists[MAX_QUEUE_ITEMS];
    static int item_ids[MAX_QUEUE_ITEMS];
    
    for (int i = 0; i < MAX_QUEUE_ITEMS; i++) {
      if (titles[i]) {
        free(titles[i]);
        titles[i] = NULL;
      }
      if (artists[i]) {
        free(artists[i]);
        artists[i] = NULL;
      }
      item_ids[i] = 0;
    }
    
    for (int i = 0; i < count && i < MAX_QUEUE_ITEMS; i++) {
      Tuple *title_t = dict_find(iterator, KEY_QUEUE_TITLE_BASE + i);
      if (title_t) {
        titles[i] = malloc(strlen(title_t->value->cstring) + 1);
        if (titles[i]) strcpy(titles[i], title_t->value->cstring);
      }
      
      Tuple *artist_t = dict_find(iterator, KEY_QUEUE_ARTIST_BASE + i);
      if (artist_t) {
        artists[i] = malloc(strlen(artist_t->value->cstring) + 1);
        if (artists[i]) strcpy(artists[i], artist_t->value->cstring);
      }
      
      Tuple *id_t = dict_find(iterator, KEY_QUEUE_ITEM_ID_BASE + i);
      if (id_t) item_ids[i] = id_t->value->int32;
    }
    
    s_queue_callback(count, titles, artists, item_ids, selected_index);
    // Free our copies; the callback already duplicated what it needs
    for (int i = 0; i < MAX_QUEUE_ITEMS; i++) {
      if (titles[i])  { free(titles[i]);  titles[i]  = NULL; }
      if (artists[i]) { free(artists[i]); artists[i] = NULL; }
    }
  }
  
  // Check for player auto-close timeout setting
  t = dict_find(iterator, KEY_PLAYER_AUTO_CLOSE_TIMEOUT);
  if (t) {
    int timeout_seconds = t->value->int32;
    APP_LOG(APP_LOG_LEVEL_INFO, "Received player auto-close timeout: %d seconds", timeout_seconds);
    player_set_auto_close_timeout(timeout_seconds);
  }

  // Check for app auto-close timeout setting
  t = dict_find(iterator, KEY_APP_AUTO_CLOSE_TIMEOUT);
  if (t) {
    int timeout_seconds = t->value->int32;
    APP_LOG(APP_LOG_LEVEL_INFO, "Received app auto-close timeout: %d seconds", timeout_seconds);
    app_auto_close_set_timeout(timeout_seconds);
    // Start the app auto-close timer immediately so the setting takes effect
    // even if the main menu already appeared before the JS message arrived.
    app_auto_close_start();
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped: %d", (int)reason);
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed: %d", (int)reason);
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Outbox send success");
}

// Expose cached-player helpers to avoid race where JS replies before UI registers
bool message_has_cached_player_state(void) {
  return s_have_cached_player;
}

void message_get_cached_player_state(PlayerState *state, char *track, char *artist, char *album, int *volume) {
  if (!s_have_cached_player) return;
  if (state) *state = s_cached_state;
  if (track) strncpy(track, s_cached_track, MAX_STRING_LENGTH-1);
  if (artist) strncpy(artist, s_cached_artist, MAX_STRING_LENGTH-1);
  if (album) strncpy(album, s_cached_album, MAX_STRING_LENGTH-1);
  if (volume) *volume = s_cached_volume;
}

const char *message_get_cached_track(void)  { return s_cached_track; }
const char *message_get_cached_artist(void) { return s_cached_artist; }
const char *message_get_cached_album(void)  { return s_cached_album; }
