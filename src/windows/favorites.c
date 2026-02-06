#include <pebble.h>
#include "favorites.h"
#include "search.h"
#include "../message_keys.h"
#include "../messaging.h"

#define MAX_FAVORITES 10

static Window *s_window;
static MenuLayer *s_menu_layer;

static int s_count = 0;
static char s_names[MAX_FAVORITES][32];
static int s_types[MAX_FAVORITES];

static void search_results_handler(int count, char *titles[], char *uris[]) {
  // When search results come back, show them using the stored type for the selected favorite
  if (count > 0) {
    // Find the type to use - use the first type in our list for now (could be more sophisticated)
    ContentType type = CONTENT_TYPE_PLAYLIST;
    for (int i = 0; i < s_count; i++) {
      type = s_types[i];
      break;
    }
    results_window_push(count, titles, uris, type);
  } else {
    vibes_short_pulse();
  }
}

static void favorites_data_handler(int count, char *names[], int types[]) {
  favorites_set_data(count, names, types);
}

static uint16_t menu_get_num_rows(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return s_count;
}

static void menu_draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  if (cell_index->row < s_count) {
    // Add type indicator before name
    static char display_text[40];
    const char *type_label = "";
    
    switch (s_types[cell_index->row]) {
      case 0: type_label = "♫ "; break;  // Playlist
      case 1: type_label = "♪ "; break;  // Artist
      case 2: type_label = "◉ "; break;  // Album
    }
    
    snprintf(display_text, sizeof(display_text), "%s%s", type_label, s_names[cell_index->row]);
    menu_cell_basic_draw(ctx, cell_layer, display_text, NULL, NULL);
  }
}

static void menu_select(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  if (cell_index->row < s_count) {
    // Trigger a search for this favorite
    message_set_results_callback(search_results_handler);
    message_send_search(s_types[cell_index->row], s_names[cell_index->row]);
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
  
  // Request favorites from phone
  message_set_favorites_callback(favorites_data_handler);
  message_send_get_favorites();
}

static void window_unload(Window *window) {
  menu_layer_destroy(s_menu_layer);
  window_destroy(window);
  s_window = NULL;
}

void favorites_window_push(void) {
  if (!s_window) {
    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers){
      .load = window_load,
      .unload = window_unload
    });
  }
  window_stack_push(s_window, true);
}

void favorites_set_data(int count, char *names[], int types[]) {
  s_count = count > MAX_FAVORITES ? MAX_FAVORITES : count;
  
  for (int i = 0; i < s_count; i++) {
    strncpy(s_names[i], names[i], sizeof(s_names[i]) - 1);
    s_names[i][sizeof(s_names[i]) - 1] = '\0';
    s_types[i] = types[i];
  }
  
  if (s_menu_layer) {
    menu_layer_reload_data(s_menu_layer);
  }
}
