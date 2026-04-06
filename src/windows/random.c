#include <pebble.h>
#include "random.h"
#include "search.h"
#include "player.h"
#include "../message_keys.h"
#include "../messaging.h"

static Window *s_window;
static MenuLayer *s_menu_layer;

static const char *s_content_types[] = {"Playlist", "Artist", "Album"};

// Custom light vibration pattern (20ms pulse)
static void light_vibe(void) {
  uint32_t segments[] = { 20 };
  VibePattern pat = {
    .durations = segments,
    .num_segments = 1,
  };
  vibes_enqueue_custom_pattern(pat);
}

// Forward declaration
static void random_results_handler(int count, char *titles[], char *uris[]);

static uint16_t menu_get_num_rows(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return 3;
}

static void menu_draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  menu_cell_basic_draw(ctx, cell_layer, s_content_types[cell_index->row], NULL, NULL);
}

static void random_results_handler(int count, char *titles[], char *uris[]) {
  // Unregister our callback
  message_set_results_callback(NULL);
  
  // Get the type from the currently selected menu item
  MenuIndex index = menu_layer_get_selected_index(s_menu_layer);
  ContentType type = (ContentType)index.row;
  
  // Push results window with the data
  results_window_push(count, titles, uris, type);
}

static void menu_select(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  ContentType type = (ContentType)cell_index->row;
  
  // Register callback to receive results
  message_set_results_callback(random_results_handler);
  
  // Send random request
  message_send_random(type);
  light_vibe();
}

#ifndef PBL_PLATFORM_APLITE
static void menu_select_long(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  player_window_push();
}
#endif

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  s_menu_layer = menu_layer_create(bounds);
#ifdef PBL_ROUND
  menu_layer_set_center_focused(s_menu_layer, true);
#endif
  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks){
    .get_num_rows = menu_get_num_rows,
    .draw_row = menu_draw_row,
    .select_click = menu_select,
#ifndef PBL_PLATFORM_APLITE
    .select_long_click = menu_select_long
#else
    .select_long_click = NULL
#endif
  });
  menu_layer_set_click_config_onto_window(s_menu_layer, window);
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
}

static void window_unload(Window *window) {
  // Clear any pending results callback
  message_set_results_callback(NULL);
  menu_layer_destroy(s_menu_layer);
  window_destroy(window);
  s_window = NULL;
}

void random_window_push(void) {
  if (!s_window) {
    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers){
      .load = window_load,
      .unload = window_unload
    });
  }
  window_stack_push(s_window, true);
}
