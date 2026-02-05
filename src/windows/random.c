#include <pebble.h>
#include "random.h"
#include "search.h"
#include "../message_keys.h"
#include "../messaging.h"

static Window *s_window;
static MenuLayer *s_menu_layer;

static const char *s_content_types[] = {"Playlist", "Artist", "Album"};

static uint16_t menu_get_num_rows(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return 3;
}

static void menu_draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  menu_cell_basic_draw(ctx, cell_layer, s_content_types[cell_index->row], NULL, NULL);
}

static void menu_select(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  ContentType type = (ContentType)cell_index->row;
  message_send_random(type);
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
}

static void window_unload(Window *window) {
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
