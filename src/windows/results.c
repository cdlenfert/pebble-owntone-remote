#include <pebble.h>
#include "search.h"
#include "../message_keys.h"
#include "../messaging.h"

// Forward declarations
static void results_handler(int count, char *titles[], char *uris[]);

static Window *s_window;
static MenuLayer *s_menu_layer;
static ContentType s_current_type;

static char *s_titles[MAX_RESULTS];
static char *s_uris[MAX_RESULTS];
static int s_result_count = 0;

static void cleanup_results(void) {
  for (int i = 0; i < MAX_RESULTS; i++) {
    if (s_titles[i]) {
      free(s_titles[i]);
      s_titles[i] = NULL;
    }
    if (s_uris[i]) {
      free(s_uris[i]);
      s_uris[i] = NULL;
    }
  }
  s_result_count = 0;
}

static void results_handler(int count, char *titles[], char *uris[]) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "results_handler: received %d results", count);
  cleanup_results();
  s_result_count = count;
  
  for (int i = 0; i < count && i < MAX_RESULTS; i++) {
    if (titles[i]) {
      s_titles[i] = malloc(strlen(titles[i]) + 1);
      if (s_titles[i]) strcpy(s_titles[i], titles[i]);
    }
    if (uris[i]) {
      s_uris[i] = malloc(strlen(uris[i]) + 1);
      if (s_uris[i]) strcpy(s_uris[i], uris[i]);
    }
  }
  
  if (s_menu_layer && window_stack_contains_window(s_window)) {
    menu_layer_reload_data(s_menu_layer);
  } else {
    results_window_push(count, titles, uris, s_current_type);
  }
}

static uint16_t menu_get_num_rows(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return s_result_count > 0 ? s_result_count : 1;
}

static void menu_draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  if (s_result_count > 0 && cell_index->row < s_result_count) {
    menu_cell_basic_draw(ctx, cell_layer, s_titles[cell_index->row], NULL, NULL);
  } else {
    menu_cell_basic_draw(ctx, cell_layer, "No results", NULL, NULL);
  }
}

static void menu_select(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  if (s_result_count > 0 && cell_index->row < s_result_count && s_uris[cell_index->row]) {
    message_send_add_to_queue(s_uris[cell_index->row], s_current_type);
    vibes_short_pulse();
    window_stack_pop(true);
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
  
  message_set_results_callback(results_handler);
}

static void window_unload(Window *window) {
  message_set_results_callback(NULL);
  menu_layer_destroy(s_menu_layer);
  s_menu_layer = NULL;
  cleanup_results();
}

static void window_disappear(Window *window) {
  window_destroy(window);
  s_window = NULL;
}

void results_window_push(int count, char *titles[], char *uris[], ContentType type) {
  s_current_type = type;
  
  // Copy the results data
  cleanup_results();
  s_result_count = count;
  
  for (int i = 0; i < count && i < MAX_RESULTS; i++) {
    if (titles[i]) {
      s_titles[i] = malloc(strlen(titles[i]) + 1);
      if (s_titles[i]) strcpy(s_titles[i], titles[i]);
    }
    if (uris[i]) {
      s_uris[i] = malloc(strlen(uris[i]) + 1);
      if (s_uris[i]) strcpy(s_uris[i], uris[i]);
    }
  }
  
  if (!s_window) {
    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers){
      .load = window_load,
      .unload = window_unload,
      .disappear = window_disappear
    });
  }
  
  window_stack_push(s_window, true);
}
