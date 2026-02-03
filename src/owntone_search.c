#include <pebble.h>
#include <string.h>
#include <stdlib.h>

// Message keys
enum {
  KEY_CMD = 0,         // 1 = search, 2 = add
  KEY_TYPE = 1,        // string: playlist/artist/album/track
  KEY_QUERY = 2,       // string: query
  KEY_RESULT_COUNT = 3,
  KEY_TITLE_BASE = 10, // 10..17
  KEY_URI_BASE = 20,   // 20..27
  KEY_STATUS = 40      // string status from JS
 
};

// message key for sending last action from watch to phone
#define KEY_LAST_ACTION 41
// debug string key (used to send response bodies back to the watch for inspection)
#define KEY_DEBUG 42

// forward declare click config provider used before its definition
static void confirm_click_config_provider(void *context);
static void show_error_popup(const char *msg);

// persist key for storing last action across crashes
#define PERSIST_KEY_LAST_ACTION 200

static Window *s_main_window;
static MenuLayer *s_menu_layer;
static Window *s_results_window;
static MenuLayer *s_results_menu;
static Window *s_confirm_window = NULL;
static TextLayer *s_confirm_text = NULL;

static const char *s_content_types[] = {"Playlist","Artist","Album","Play/Pause"};
static const int NUM_CONTENT_TYPES = 4;

// results storage
static int s_result_count = 0;
static char *s_titles[8];
static char *s_uris[8];
static DictationSession *s_dictation = NULL;
static char s_dictation_buf[256];
static char s_pending_type[16] = "playlist";
static char s_confirm_uri[128];
static bool s_request_in_flight = false;
static bool s_received_search_results = false;

// UI
static Window *s_error_window = NULL;
static TextLayer *s_error_text = NULL;
static AppTimer *s_ack_timer = NULL;

static void cancel_ack_timer(void) {
  if (s_ack_timer) {
    app_timer_cancel(s_ack_timer);
    s_ack_timer = NULL;
  }
}

static void ack_timeout_handler(void *data) {
  s_ack_timer = NULL;
  show_error_popup("No phone response");
}

static char *my_strdup(const char *s) {
  if (!s) return NULL;
  size_t n = strlen(s) + 1;
  char *r = malloc(n);
  if (r) memcpy(r, s, n);
  return r;
}

static void show_error_popup(const char *msg) {
  if (!s_error_window) {
    s_error_window = window_create();
    window_set_background_color(s_error_window, GColorBlack);
    Layer *layer = window_get_root_layer(s_error_window);
    GRect bounds = layer_get_bounds(layer);
    s_error_text = text_layer_create(GRect(6, 24, bounds.size.w - 12, bounds.size.h - 48));
    text_layer_set_font(s_error_text, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    text_layer_set_text_color(s_error_text, GColorWhite);
    text_layer_set_background_color(s_error_text, GColorClear);
    text_layer_set_overflow_mode(s_error_text, GTextOverflowModeWordWrap);
    layer_add_child(layer, text_layer_get_layer(s_error_text));
  }
  text_layer_set_text(s_error_text, msg);
  window_stack_push(s_error_window, true);
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  Tuple *t = dict_read_first(iterator);
  int count = -1;
  const char *status_msg = NULL;
  const char *debug_msg = NULL;
  bool has_result_count = false;
  while (t) {
    if (t->key == KEY_RESULT_COUNT) {
      count = (int)t->value->int32;
      s_result_count = count;
      has_result_count = true;
      s_received_search_results = true;
    } else if (t->key >= KEY_TITLE_BASE && t->key < KEY_TITLE_BASE + 8) {
      int idx = t->key - KEY_TITLE_BASE;
      if (s_titles[idx]) free(s_titles[idx]);
      s_titles[idx] = my_strdup(t->value->cstring);
    } else if (t->key >= KEY_URI_BASE && t->key < KEY_URI_BASE + 8) {
      int idx = t->key - KEY_URI_BASE;
      if (s_uris[idx]) free(s_uris[idx]);
      s_uris[idx] = my_strdup(t->value->cstring);
    } else if (t->key == KEY_STATUS) {
      status_msg = t->value->cstring;
    } else if (t->key == KEY_DEBUG) {
      debug_msg = t->value->cstring;
    }
    t = dict_read_next(iterator);
  }

  /* Cancel any pending ACK timeout since we got something from the phone */
  cancel_ack_timer();

  if (status_msg) {
    if (strcmp(status_msg, "OK") == 0) {
      /* Operation completed successfully — clear persisted breadcrumb */
      if (persist_exists(PERSIST_KEY_LAST_ACTION)) persist_delete(PERSIST_KEY_LAST_ACTION);
      s_request_in_flight = false;
    } else if (strncmp(status_msg, "Error", 5) == 0) {
      s_request_in_flight = false;
      show_error_popup(status_msg);
      return;
    }
  }

  if (debug_msg) {
    // show the debug response body in a popup so user can read it
    show_error_popup(debug_msg);
  }

  // Only show result/confirm windows if this message contained search results
  if (!has_result_count) {
    return;
  }

  // Always show results window (even for single result)
  window_stack_push(s_results_window, true);
  menu_layer_reload_data(s_results_menu);
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped: %d", (int)reason);
  show_error_popup("Message dropped");
}
static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed: %d", (int)reason);
  show_error_popup("Send failed");
  cancel_ack_timer();
}
static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success");
}

static void send_search_to_js(const char *type, const char *query) {
  // persist breadcrumb before sending
  char lastbuf[128];
  snprintf(lastbuf, sizeof(lastbuf), "SENT_SEARCH:%s:%s", type ? type : "", query ? query : "");
  persist_write_string(PERSIST_KEY_LAST_ACTION, lastbuf);

  DictionaryIterator *out; 
  AppMessageResult r = app_message_outbox_begin(&out);
  if (r != APP_MSG_OK || !out) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox begin failed: %d", (int)r);
    show_error_popup("Failed to start search");
    return;
  }
  dict_write_uint8(out, KEY_CMD, 1);
  dict_write_cstring(out, KEY_TYPE, type);
  dict_write_cstring(out, KEY_QUERY, query);
  dict_write_end(out);
  app_message_outbox_send();
  /* start ACK timeout: if phone doesn't reply within 8s, show error */
  cancel_ack_timer();
  s_ack_timer = app_timer_register(8000, ack_timeout_handler, NULL);
}

static void dictation_session_callback(DictationSession *session, DictationSessionStatus status, char *transcription, void *context) {
  if (status == DictationSessionStatusSuccess && transcription) {
    const char *type = s_pending_type;
    send_search_to_js(type, transcription);
  } else {
    vibes_double_pulse();
  }
}

// Menu callbacks for main menu
static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *context) {
  return NUM_CONTENT_TYPES;
}
static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *context) {
  int idx = cell_index->row;
  menu_cell_basic_draw(ctx, cell_layer, s_content_types[idx], NULL, NULL);
}
static void menu_long_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  int idx = cell_index->row;
  // Only support random results for Playlist, Artist, Album (not Play/Pause)
  if (idx >= 3) return;
  
  snprintf(s_pending_type, sizeof(s_pending_type), "%s", s_content_types[idx]);
  for (int i=0; s_pending_type[i]; ++i) {
    if (s_pending_type[i] >= 'A' && s_pending_type[i] <= 'Z') s_pending_type[i] = s_pending_type[i] - 'A' + 'a';
  }
  
  // Send random search (using offset in JS)
  DictionaryIterator *out;
  AppMessageResult r = app_message_outbox_begin(&out);
  if (r != APP_MSG_OK || !out) {
    show_error_popup("Failed to start random search");
    return;
  }
  dict_write_uint8(out, KEY_CMD, 7); // New command for random search
  dict_write_cstring(out, KEY_TYPE, s_pending_type);
  dict_write_end(out);
  app_message_outbox_send();
}

static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  int idx = cell_index->row;
  snprintf(s_pending_type, sizeof(s_pending_type), "%s", s_content_types[idx]);
  for (int i=0; s_pending_type[i]; ++i) {
    if (s_pending_type[i] >= 'A' && s_pending_type[i] <= 'Z') s_pending_type[i] = s_pending_type[i] - 'A' + 'a';
  }

  // Play/Pause toggle — send a simple toggle command to the phone
  if (strcmp(s_pending_type, "play/pause") == 0) {
    DictionaryIterator *out;
    AppMessageResult r = app_message_outbox_begin(&out);
    if (r == APP_MSG_OK && out) {
      dict_write_uint8(out, KEY_CMD, 6);
      dict_write_end(out);
      app_message_outbox_send();
      cancel_ack_timer();
      s_ack_timer = app_timer_register(8000, ack_timeout_handler, NULL);
    } else {
      show_error_popup("Failed to send play/pause");
    }
    return;
  }

  if (!s_dictation) {
    show_error_popup("Dictation unavailable");
    return;
  }
  dictation_session_start(s_dictation);
}

// (Logs UI removed — phone opens a copyable logs page instead)

// Results menu
static uint16_t results_get_num_rows(MenuLayer *menu_layer, uint16_t section_index, void *context) {
  return (uint16_t)((s_result_count < 2) ? 2 : s_result_count);
}
static void results_draw_row(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *context) {
  int row = cell_index->row;
  if (s_result_count == 0) {
    if (row == 0) menu_cell_basic_draw(ctx, cell_layer, "Try Again", NULL, NULL);
    else menu_cell_basic_draw(ctx, cell_layer, "Cancel", NULL, NULL);
  } else {
    if (row < s_result_count && s_titles[row]) menu_cell_basic_draw(ctx, cell_layer, s_titles[row], NULL, NULL);
  }
}
static void results_select(MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  int row = cell_index->row;
  if (s_result_count == 0) {
    if (row == 0) {
      if (!s_dictation) {
        show_error_popup("Dictation unavailable");
      } else {
        dictation_session_start(s_dictation);
      }
    } else {
      window_stack_pop(true);
    }
  } else {
    if (row < s_result_count && s_uris[row] && s_uris[row][0] != '\0') {
        if (s_request_in_flight) {
          show_error_popup("Request in flight");
          return;
        }
        // persist breadcrumb for add
        char lastbuf[128];
        snprintf(lastbuf, sizeof(lastbuf), "SENT_ADD:%s:%s", s_pending_type, s_uris[row] ? s_uris[row] : "");
        persist_write_string(PERSIST_KEY_LAST_ACTION, lastbuf);
        // Send add request immediately
        DictionaryIterator *out;
        AppMessageResult r = app_message_outbox_begin(&out);
        if (r == APP_MSG_OK && out) {
          dict_write_uint8(out, KEY_CMD, 2);
          dict_write_cstring(out, KEY_URI_BASE, s_uris[row]);
          dict_write_cstring(out, KEY_TYPE, s_pending_type);
          dict_write_end(out);
          app_message_outbox_send();
          s_request_in_flight = true;
          vibes_short_pulse();
          show_error_popup("Sent to phone");
          cancel_ack_timer();
          s_ack_timer = app_timer_register(8000, ack_timeout_handler, NULL);
        } else {
          show_error_popup("Failed to send add");
        }
    }
  }
}

static void confirm_select_click(ClickRecognizerRef recognizer, void *context) {
  if (s_request_in_flight) {
    show_error_popup("Request in flight");
    return;
  }
  if (s_confirm_uri[0] == '\0') {
    show_error_popup("No URI to add");
    return;
  }
  DictionaryIterator *out;
  AppMessageResult r = app_message_outbox_begin(&out);
  if (r == APP_MSG_OK && out) {
    dict_write_uint8(out, KEY_CMD, 2);
    dict_write_cstring(out, KEY_URI_BASE, s_confirm_uri);
    dict_write_cstring(out, KEY_TYPE, s_pending_type);
    dict_write_end(out);
    app_message_outbox_send();
    s_request_in_flight = true;
    cancel_ack_timer();
    s_ack_timer = app_timer_register(8000, ack_timeout_handler, NULL);
  } else {
    show_error_popup("Failed to send add command");
  }
  window_stack_pop(true);
}

static void confirm_back_click(ClickRecognizerRef recognizer, void *context) {
  // Cancel
  window_stack_pop(true);
}

static void confirm_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, confirm_select_click);
  window_single_click_subscribe(BUTTON_ID_BACK, confirm_back_click);
}

static void init(void) {
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {.unload = NULL});

  Layer *window_layer = window_get_root_layer(s_main_window);
  GRect bounds = layer_get_bounds(window_layer);

  // Main menu uses full screen (no status bar)
  s_menu_layer = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks){
    .get_num_rows = menu_get_num_rows_callback,
    .draw_row = menu_draw_row_callback,
    .select_click = menu_select_callback,
    .select_long_click = menu_long_select_callback
  });
  menu_layer_set_click_config_onto_window(s_menu_layer, s_main_window);
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));

  // results window and menu (full screen, no status bar)
  s_results_window = window_create();
  Layer *rw_layer = window_get_root_layer(s_results_window);
  GRect rbounds = layer_get_bounds(rw_layer);
  s_results_menu = menu_layer_create(rbounds);
  menu_layer_set_callbacks(s_results_menu, NULL, (MenuLayerCallbacks){
    .get_num_rows = results_get_num_rows,
    .draw_row = results_draw_row,
    .select_click = results_select
  });
  menu_layer_set_click_config_onto_window(s_results_menu, s_results_window);
  layer_add_child(rw_layer, menu_layer_get_layer(s_results_menu));

  // Create confirmation window once during init to avoid window creation during callbacks
  s_confirm_window = window_create();
  window_set_click_config_provider(s_confirm_window, confirm_click_config_provider);
  window_set_background_color(s_confirm_window, GColorWhite);
  Layer *confirm_layer = window_get_root_layer(s_confirm_window);
  GRect confirm_bounds = layer_get_bounds(confirm_layer);
  s_confirm_text = text_layer_create(GRect(4, 4, confirm_bounds.size.w - 8, confirm_bounds.size.h - 8));
  text_layer_set_font(s_confirm_text, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_color(s_confirm_text, GColorBlack);
  text_layer_set_background_color(s_confirm_text, GColorWhite);
  text_layer_set_overflow_mode(s_confirm_text, GTextOverflowModeWordWrap);
  text_layer_set_text_alignment(s_confirm_text, GTextAlignmentLeft);
  layer_add_child(confirm_layer, text_layer_get_layer(s_confirm_text));

  // dictation
  s_dictation = dictation_session_create(sizeof(s_dictation_buf), dictation_session_callback, NULL);

  // app message
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  app_message_open(1024, 256);

  // If we have a persisted last action (breadcrumb) from before a crash, send it to the phone
  if (persist_exists(PERSIST_KEY_LAST_ACTION)) {
    char buf[128];
    int len = persist_read_string(PERSIST_KEY_LAST_ACTION, buf, sizeof(buf));
    if (len > 0) {
      DictionaryIterator *out;
      AppMessageResult r = app_message_outbox_begin(&out);
      if (r == APP_MSG_OK && out) {
        dict_write_cstring(out, KEY_LAST_ACTION, buf);
        dict_write_end(out);
        app_message_outbox_send();
      }
    }
  }

  window_stack_push(s_main_window, true);
}

static void deinit(void) {
  if (s_dictation) dictation_session_destroy(s_dictation);
  if (s_error_text) text_layer_destroy(s_error_text);
  if (s_error_window) window_destroy(s_error_window);
  if (s_confirm_text) text_layer_destroy(s_confirm_text);
  if (s_confirm_window) window_destroy(s_confirm_window);
  menu_layer_destroy(s_menu_layer);
  window_destroy(s_main_window);
  menu_layer_destroy(s_results_menu);
  window_destroy(s_results_window);
  for (int i=0;i<8;i++) { if (s_titles[i]) free(s_titles[i]); if (s_uris[i]) free(s_uris[i]); }
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
