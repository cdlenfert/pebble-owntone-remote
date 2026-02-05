#include <pebble.h>
#include "search.h"
#include "../message_keys.h"
#include "../messaging.h"

static Window *s_window;
static MenuLayer *s_menu_layer;
static DictationSession *s_dictation;

static const char *s_content_types[] = {"Playlist", "Artist", "Album"};
static ContentType s_selected_type;

static void dictation_callback(DictationSession *session, DictationSessionStatus status, char *transcription, void *context) {
  if (status == DictationSessionStatusSuccess && transcription) {
    message_send_search(s_selected_type, transcription);
  } else {
    vibes_double_pulse();
  }
}

static uint16_t menu_get_num_rows(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return 3;
}

static void menu_draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  menu_cell_basic_draw(ctx, cell_layer, s_content_types[cell_index->row], NULL, NULL);
}

static void menu_select(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  s_selected_type = (ContentType)cell_index->row;
  
  if (s_dictation) {
    dictation_session_start(s_dictation);
  }
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  s_menu_layer = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks){
    .get_num_rows = menu_get_num_rows,
    .draw_row = menu_draw_row,
    .select_click = menu_select
  });
  menu_layer_set_click_config_onto_window(s_menu_layer, window);
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
  
  s_dictation = dictation_session_create(128, dictation_callback, NULL);
}

static void window_unload(Window *window) {
  if (s_dictation) {
    dictation_session_destroy(s_dictation);
    s_dictation = NULL;
  }
  menu_layer_destroy(s_menu_layer);
  window_destroy(window);
  s_window = NULL;
}

void search_window_push(void) {
  if (!s_window) {
    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers){
      .load = window_load,
      .unload = window_unload
    });
  }
  window_stack_push(s_window, true);
}
