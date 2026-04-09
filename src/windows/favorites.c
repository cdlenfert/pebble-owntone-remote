#include <pebble.h>
#include "favorites.h"
#include "search.h"
#include "player.h"
#include "../message_keys.h"
#include "../messaging.h"

static Window *s_window;
static MenuLayer *s_menu_layer;

static int s_count = 0;
// Heap-allocated on demand so the ~960 byte name buffer is only resident
// while the favorites window is open (important on Aplite's limited RAM).
static char *s_names[MAX_FAVORITES];
static int s_types[MAX_FAVORITES];

// Custom light vibration pattern (20ms pulse)
static void light_vibe(void) {
  uint32_t segments[] = { 20 };
  VibePattern pat = {
    .durations = segments,
    .num_segments = 1,
  };
  vibes_enqueue_custom_pattern(pat);
}

// State for category browsing
static bool s_showing_categories = true;
static int s_selected_category = 0; // 0=playlists, 1=artists, 2=albums

static void search_results_handler(int count, char *titles[], char *uris[]) {
  // When search results come back, show them using the selected category's type
  if (count > 0) {
    // Use the selected category as the content type (0=playlist, 1=artist, 2=album)
    results_window_push(count, titles, uris, s_selected_category);
  } else {
    light_vibe();
  }
}

static void favorites_data_handler(int count, char *names[], int types[]) {
  favorites_set_data(count, names, types);
}

static uint16_t menu_get_num_rows(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  if (s_showing_categories) {
    return 3; // Playlists, Artists, Albums
  }
  // Count favorites of the selected type
  int count = 0;
  for (int i = 0; i < s_count; i++) {
    if (s_types[i] == s_selected_category) {
      count++;
    }
  }
  return count;
}

static void menu_draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  if (s_showing_categories) {
    // Show category names
    const char *category_names[] = {"Playlists", "Artists", "Albums"};
    menu_cell_basic_draw(ctx, cell_layer, category_names[cell_index->row], NULL, NULL);
  } else {
    // Show favorites of selected type
    int current_row = 0;
    for (int i = 0; i < s_count; i++) {
      if (s_types[i] == s_selected_category) {
        if (current_row == cell_index->row) {
          menu_cell_basic_draw(ctx, cell_layer, s_names[i] ? s_names[i] : "?", NULL, NULL);
          return;
        }
        current_row++;
      }
    }
  }
}

static void menu_select(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  if (s_showing_categories) {
    // Selected a category, show its favorites
    s_selected_category = cell_index->row;
    s_showing_categories = false;
    menu_layer_reload_data(s_menu_layer);
  } else {
    // Selected a favorite, trigger search
    int current_row = 0;
    for (int i = 0; i < s_count; i++) {
      if (s_types[i] == s_selected_category) {
        if (current_row == cell_index->row) {
          message_set_results_callback(search_results_handler);
          message_send_search(s_types[i], s_names[i]);
          return;
        }
        current_row++;
      }
    }
  }
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  MenuIndex index = menu_layer_get_selected_index(s_menu_layer);
  menu_select(s_menu_layer, &index, NULL);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  menu_layer_set_selected_next(s_menu_layer, true, MenuRowAlignCenter, true);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  menu_layer_set_selected_next(s_menu_layer, false, MenuRowAlignCenter, true);
}

static void back_handler(ClickRecognizerRef recognizer, void *context) {
  if (!s_showing_categories) {
    // Go back to categories
    s_showing_categories = true;
    menu_layer_reload_data(s_menu_layer);
  } else {
    // Exit window
    window_stack_pop(true);
  }
}

#ifndef PBL_PLATFORM_APLITE
static void favorites_select_long_click(ClickRecognizerRef recognizer, void *context) {
  player_window_push();
}
#endif

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
#ifndef PBL_PLATFORM_APLITE
  window_long_click_subscribe(BUTTON_ID_SELECT, 500, favorites_select_long_click, NULL);
#endif
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 100, up_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 100, down_click_handler);
  window_single_click_subscribe(BUTTON_ID_BACK, back_handler);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  s_showing_categories = true;
  
  s_menu_layer = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks){
    .get_num_rows = menu_get_num_rows,
    .draw_row = menu_draw_row,
    .select_click = menu_select
  });
  
  window_set_click_config_provider(window, click_config_provider);
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
  
  // Request favorites from phone
  message_set_favorites_callback(favorites_data_handler);
  message_send_get_favorites();
}

static void window_appear(Window *window) {
  // Recreate MenuLayer if it was freed while another window was on top.
  if (!s_menu_layer) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);
    s_menu_layer = menu_layer_create(bounds);
    menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks){
      .get_num_rows = menu_get_num_rows,
      .draw_row = menu_draw_row,
      .select_click = menu_select
    });
    window_set_click_config_provider(window, click_config_provider);
    layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
  }
  // Reset to categories view when window appears
  s_showing_categories = true;
  if (s_menu_layer) {
    menu_layer_reload_data(s_menu_layer);
  }
}

static void window_disappear(Window *window) {
  // Free MenuLayer so child windows (player, etc.) have more heap for bitmaps.
  // window_appear recreates it when this window returns to the top.
  if (s_menu_layer) {
    menu_layer_destroy(s_menu_layer);
    s_menu_layer = NULL;
  }
}

static void cleanup_favorites_data(void) {
  for (int i = 0; i < MAX_FAVORITES; i++) {
    if (s_names[i]) {
      free(s_names[i]);
      s_names[i] = NULL;
    }
  }
  s_count = 0;
}

static void window_unload(Window *window) {
  message_set_favorites_callback(NULL);
  if (s_menu_layer) {
    menu_layer_destroy(s_menu_layer);
    s_menu_layer = NULL;
  }
  cleanup_favorites_data();
  window_destroy(window);
  s_window = NULL;
}

void favorites_window_push(void) {
  if (!s_window) {
    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers){
      .load = window_load,
      .appear = window_appear,
      .disappear = window_disappear,
      .unload = window_unload
    });
  }
  window_stack_push(s_window, true);
}

void favorites_set_data(int count, char *names[], int types[]) {
  cleanup_favorites_data();
  s_count = count > MAX_FAVORITES ? MAX_FAVORITES : count;
  
  for (int i = 0; i < s_count; i++) {
    if (names[i]) {
      s_names[i] = malloc(strlen(names[i]) + 1);
      if (s_names[i]) strcpy(s_names[i], names[i]);
    }
    s_types[i] = types[i];
  }

  if (s_menu_layer) {
    menu_layer_reload_data(s_menu_layer);
  }
}
