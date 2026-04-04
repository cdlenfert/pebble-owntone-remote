#include <pebble.h>
#include "main_menu.h"
#include "player.h"
#include "search.h"
#include "random.h"
#include "outputs.h"
#include "favorites.h"
#include "queue.h"
#include "../app_auto_close.h"

static Window *s_window;
static MenuLayer *s_menu_layer;

#if defined(PBL_PLATFORM_APLITE)
#define NUM_MENU_ITEMS 5

static const char *menu_items[] = {
  "Player",
  "Favorites",
  "Random",
  "Outputs",
  "Queue"
};
#else
#define NUM_MENU_ITEMS 6

static const char *menu_items[] = {
  "Player",
  "Favorites",
  "Search",
  "Random",
  "Outputs",
  "Queue"
};
#endif

static uint16_t menu_get_num_rows(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return NUM_MENU_ITEMS;
}

static void menu_draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  menu_cell_basic_draw(ctx, cell_layer, menu_items[cell_index->row], NULL, NULL);
}

static void menu_select(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  app_auto_close_reset();
#if defined(PBL_PLATFORM_APLITE)
  switch (cell_index->row) {
    case 0:
      player_window_push();
      break;
    case 1:
      favorites_window_push();
      break;
    case 2:
      random_window_push();
      break;
    case 3:
      outputs_window_push();
      break;
    case 4:
      queue_window_push();
      break;
  }
#else
  switch (cell_index->row) {
    case 0:
      player_window_push();
      break;
    case 1:
      favorites_window_push();
      break;
    case 2:
      search_window_push();
      break;
    case 3:
      random_window_push();
      break;
    case 4:
      outputs_window_push();
      break;
    case 5:
      queue_window_push();
      break;
  }
#endif
}

#ifndef PBL_PLATFORM_APLITE
static void menu_select_long(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  player_window_push();
}
#endif

static void menu_selection_changed(MenuLayer *menu_layer, MenuIndex new_index, MenuIndex old_index, void *data) {
  (void)menu_layer; (void)new_index; (void)old_index; (void)data;
  app_auto_close_reset();
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  s_menu_layer = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks){
    .get_num_rows = menu_get_num_rows,
    .draw_row = menu_draw_row,
    .select_click = menu_select,
    .selection_changed = menu_selection_changed,
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
  menu_layer_destroy(s_menu_layer);
  window_destroy(window);
  s_window = NULL;
}

void main_menu_push(void) {
  if (!s_window) {
    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers){
      .load = window_load,
      .unload = window_unload
    });
  }
  window_stack_push(s_window, true);
}
