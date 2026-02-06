#include <pebble.h>
#include "message_keys.h"
#include "messaging.h"

// Forward declarations
static void inbox_received_callback(DictionaryIterator *iterator, void *context);
static void inbox_dropped_callback(AppMessageResult reason, void *context);
static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context);
static void outbox_sent_callback(DictionaryIterator *iterator, void *context);

// Message callbacks - to be implemented by specific handlers
typedef void (*PlayerStateCallback)(PlayerState state, const char *track, const char *artist, const char *album, int volume);
typedef void (*SearchResultsCallback)(int count, char *titles[], char *uris[]);
typedef void (*OutputsCallback)(int count, char *names[], char *ids[], int volumes[], bool enabled[]);
typedef void (*StatusCallback)(int status);
typedef void (*FavoritesCallback)(int count, char *names[], int types[]);

static PlayerStateCallback s_player_callback = NULL;
static SearchResultsCallback s_results_callback = NULL;
static OutputsCallback s_outputs_callback = NULL;
static StatusCallback s_status_callback = NULL;
static FavoritesCallback s_favorites_callback = NULL;

void message_init(void) {
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  app_message_open(2048, 512);
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
  if (t && s_player_callback) {
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
    
    s_player_callback(state, track, artist, album, volume);
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
    
    s_results_callback(count, titles, uris);
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
      Tuple *name_t = dict_find(iterator, KEY_FAVORITE_NAME_BASE + i);
      if (name_t) {
        names[i] = malloc(strlen(name_t->value->cstring) + 1);
        if (names[i]) strcpy(names[i], name_t->value->cstring);
      }
      
      Tuple *type_t = dict_find(iterator, KEY_FAVORITE_TYPE_BASE + i);
      if (type_t) types[i] = type_t->value->uint8;
    }
    
    s_favorites_callback(count, names, types);
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
