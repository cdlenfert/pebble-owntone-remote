#include <pebble.h>
#include "queue.h"
#include "player.h"
#include "../message_keys.h"
#include "../messaging.h"

static Window *s_window;
static MenuLayer *s_menu_layer;

#define MAX_QUEUE_ITEMS 10

static char *s_titles[MAX_QUEUE_ITEMS];
static char *s_artists[MAX_QUEUE_ITEMS];
static int s_item_ids[MAX_QUEUE_ITEMS];
static int s_queue_count = 0;
static int s_selected_index = 0;

static void cleanup_queue(void) {
  for (int i = 0; i < MAX_QUEUE_ITEMS; i++) {
    if (s_titles[i]) {
      free(s_titles[i]);
      s_titles[i] = NULL;
    }
    if (s_artists[i]) {
      free(s_artists[i]);
      s_artists[i] = NULL;
    }
    s_item_ids[i] = 0;
  }
  s_queue_count = 0;
  s_selected_index = 0;
}

static void queue_handler(int count, char *titles[], char *artists[], int item_ids[], int selected_index) {
  cleanup_queue();
  
  s_queue_count = (count > MAX_QUEUE_ITEMS) ? MAX_QUEUE_ITEMS : count;
  s_selected_index = (selected_index >= 0 && selected_index < s_queue_count) ? selected_index : 0;
  
  for (int i = 0; i < s_queue_count; i++) {
    if (titles[i]) {
      s_titles[i] = malloc(strlen(titles[i]) + 1);
      if (s_titles[i]) strcpy(s_titles[i], titles[i]);
    }
    if (artists[i]) {
      s_artists[i] = malloc(strlen(artists[i]) + 1);
      if (s_artists[i]) strcpy(s_artists[i], artists[i]);
    }
    s_item_ids[i] = item_ids[i];
  }
  
  if (s_menu_layer) {
    menu_layer_reload_data(s_menu_layer);
    // Set selection to currently playing item
    menu_layer_set_selected_index(s_menu_layer, MenuIndex(0, s_selected_index), MenuRowAlignCenter, false);
  }
}

static uint16_t menu_get_num_rows(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return s_queue_count;
}

static void menu_draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  const char *title = (s_titles[cell_index->row]) ? s_titles[cell_index->row] : "Unknown";
  const char *artist = (s_artists[cell_index->row]) ? s_artists[cell_index->row] : "";
  
  menu_cell_basic_draw(ctx, cell_layer, title, artist, NULL);
}

static void menu_select(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  if (cell_index->row < s_queue_count && s_item_ids[cell_index->row] > 0) {
    // Jump to this queue item
    message_send_play_queue_item(s_item_ids[cell_index->row]);
    
    // Set optimistic UI state before pushing window
    player_set_launch_state_playing();
    
    // Open player window
    player_window_push();
  }
}

static void menu_select_long(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  player_window_push();
}

static void window_appear(Window *window) {
  // Refresh queue data when returning from player
  message_send_command(CMD_GET_QUEUE);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  s_menu_layer = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks){
    .get_num_rows = menu_get_num_rows,
    .draw_row = menu_draw_row,
    .select_click = menu_select,
    .select_long_click = menu_select_long
  });
  menu_layer_set_click_config_onto_window(s_menu_layer, window);
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
  
  // Request queue data
  message_set_queue_callback(queue_handler);
  message_send_command(CMD_GET_QUEUE);
}

static void window_unload(Window *window) {
  message_set_queue_callback(NULL);
  menu_layer_destroy(s_menu_layer);
  s_menu_layer = NULL;
  cleanup_queue();
  
  window_destroy(window);
  s_window = NULL;
}

void queue_window_push(void) {
  if (!s_window) {
    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers){
      .load = window_load,
      .appear = window_appear,
      .unload = window_unload
    });
  }
  window_stack_push(s_window, true);
}
