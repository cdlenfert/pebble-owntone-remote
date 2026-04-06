#include <pebble.h>
#include "outputs.h"
#include "player.h"
#include "../message_keys.h"
#include "../messaging.h"
#include "../app_auto_close.h"

static Window *s_window;
static MenuLayer *s_menu_layer;
static Window *s_volume_window = NULL;

static char *s_names[MAX_OUTPUTS];
static char *s_ids[MAX_OUTPUTS];
static int s_volumes[MAX_OUTPUTS];
static bool s_enabled[MAX_OUTPUTS];
static int s_output_count = 0;

// Custom light vibration pattern (20ms pulse)
static void light_vibe(void) {
  uint32_t segments[] = { 20 };
  VibePattern pat = {
    .durations = segments,
    .num_segments = 1,
  };
  vibes_enqueue_custom_pattern(pat);
}

// Forward declare volume window
static void output_volume_window_push(const char *name, const char *id, int volume);

static void cleanup_outputs(void) {
  for (int i = 0; i < MAX_OUTPUTS; i++) {
    if (s_names[i]) {
      free(s_names[i]);
      s_names[i] = NULL;
    }
    if (s_ids[i]) {
      free(s_ids[i]);
      s_ids[i] = NULL;
    }
    s_volumes[i] = 0;
    s_enabled[i] = false;
  }
  s_output_count = 0;
}

static void outputs_handler(int count, char *names[], char *ids[], int volumes[], bool enabled[]) {
  cleanup_outputs();
  s_output_count = count;
  
  for (int i = 0; i < count && i < MAX_OUTPUTS; i++) {
    if (names[i]) {
      s_names[i] = malloc(strlen(names[i]) + 1);
      if (s_names[i]) strcpy(s_names[i], names[i]);
    }
    if (ids[i]) {
      s_ids[i] = malloc(strlen(ids[i]) + 1);
      if (s_ids[i]) strcpy(s_ids[i], ids[i]);
    }
    s_volumes[i] = volumes[i];
    s_enabled[i] = enabled[i];
  }
  
  if (s_menu_layer) {
    menu_layer_reload_data(s_menu_layer);
  }
}

static uint16_t menu_get_num_rows(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return s_output_count > 0 ? s_output_count : 1;
}

static void menu_draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  if (s_output_count > 0 && cell_index->row < s_output_count) {
    char subtitle[16];
    snprintf(subtitle, sizeof(subtitle), "%s %d%%", s_enabled[cell_index->row] ? "ON" : "OFF", s_volumes[cell_index->row]);
    menu_cell_basic_draw(ctx, cell_layer, s_names[cell_index->row], subtitle, NULL);
  } else {
    menu_cell_basic_draw(ctx, cell_layer, "Loading...", NULL, NULL);
  }
}

static void menu_select(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  app_auto_close_reset();
  if (s_output_count > 0 && cell_index->row < s_output_count && s_ids[cell_index->row]) {
    if (s_enabled[cell_index->row]) {
      // Output is already ON: just open volume controls, no state change
      light_vibe();
      output_volume_window_push(s_names[cell_index->row], s_ids[cell_index->row], s_volumes[cell_index->row]);
    } else {
      // Output is OFF: set as exclusive output and open volume controls
      message_send_set_output_exclusive(s_ids[cell_index->row]);
      light_vibe();
      output_volume_window_push(s_names[cell_index->row], s_ids[cell_index->row], s_volumes[cell_index->row]);
    }
  }
}

static void menu_select_long(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  app_auto_close_reset();
  if (s_output_count > 0 && cell_index->row < s_output_count && s_ids[cell_index->row]) {
    // Long press: toggle output
    bool was_enabled = s_enabled[cell_index->row];
    message_send_toggle_output(s_ids[cell_index->row]);
    light_vibe();
    
    // Only show volume control if we're turning the output ON (was off, will be on)
    if (!was_enabled) {
      output_volume_window_push(s_names[cell_index->row], s_ids[cell_index->row], s_volumes[cell_index->row]);
    }
  }
}

static void outputs_selection_changed(MenuLayer *menu_layer, MenuIndex new_index, MenuIndex old_index, void *data) {
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
    .selection_changed = outputs_selection_changed,
    .select_long_click = menu_select_long
  });
  menu_layer_set_click_config_onto_window(s_menu_layer, window);
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
  
  message_set_outputs_callback(outputs_handler);
  message_send_command(CMD_GET_OUTPUTS);
}

static void window_unload(Window *window) {
  message_set_outputs_callback(NULL);
  if (s_menu_layer) {
    menu_layer_destroy(s_menu_layer);
    s_menu_layer = NULL;
  }
  cleanup_outputs();
}

static void window_appear(Window *window) {
  // Recreate the menu layer if it was freed while hidden (e.g. volume window on top)
  if (!s_menu_layer) {
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
      .selection_changed = outputs_selection_changed,
      .select_long_click = menu_select_long
    });
    menu_layer_set_click_config_onto_window(s_menu_layer, window);
    layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
  }
  // Refresh outputs data
  message_set_outputs_callback(outputs_handler);
  message_send_command(CMD_GET_OUTPUTS);
}

static void window_disappear(Window *window) {
  // Free the MenuLayer so the volume window can allocate its bitmaps without
  // heap pressure. window_appear recreates it when this window returns.
  if (s_menu_layer) {
    menu_layer_destroy(s_menu_layer);
    s_menu_layer = NULL;
  }
}

void outputs_window_push(void) {
  if (!s_window) {
    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers){
      .load = window_load,
      .unload = window_unload,
      .appear = window_appear,
      .disappear = window_disappear
    });
  }
  window_stack_push(s_window, true);
}

// Output Volume Control Window
static TextLayer *s_name_layer;
static TextLayer *s_volume_layer;
static ActionBarLayer *s_volume_action_bar;
static GBitmap *s_icon_vol_up;
static GBitmap *s_icon_vol_down;
static GBitmap *s_icon_play;
static GBitmap *s_icon_pause;
static AppTimer *s_state_request_timer = NULL;
static char s_current_output_id[MAX_STRING_LENGTH];
static char s_current_output_name[MAX_STRING_LENGTH];
static int s_current_output_volume;
static PlayerState s_output_player_state = PLAYER_STATE_STOPPED; // Default to stopped (shows play icon)

static void update_volume_display(void) {
  static char volume_text[16];
  snprintf(volume_text, sizeof(volume_text), "%d%%", s_current_output_volume);
  text_layer_set_text(s_volume_layer, volume_text);
}

static void update_output_play_pause_icon(void) {
  if (s_volume_action_bar) {
    if (s_output_player_state == PLAYER_STATE_PLAYING) {
      action_bar_layer_set_icon(s_volume_action_bar, BUTTON_ID_SELECT, s_icon_pause);
    } else {
      action_bar_layer_set_icon(s_volume_action_bar, BUTTON_ID_SELECT, s_icon_play);
    }
  }
}

static void output_volume_player_state_handler(PlayerState state, const char *track, const char *artist, const char *album, int volume) {
  if (player_is_transient_playing()) {
    s_output_player_state = PLAYER_STATE_PLAYING;
  } else {
    s_output_player_state = state;
  }
  update_output_play_pause_icon();
}

static void output_volume_status_handler(int status) {
  // Status response from play/pause command - icon already toggled optimistically
  if (status != 1) {
    // Failed - revert the icon
    if (s_output_player_state == PLAYER_STATE_PLAYING) {
      s_output_player_state = PLAYER_STATE_PAUSED;
    } else {
      s_output_player_state = PLAYER_STATE_PLAYING;
    }
    update_output_play_pause_icon();
  }
}

static void volume_up_click(ClickRecognizerRef recognizer, void *context) {
  app_auto_close_reset();
  s_current_output_volume = (s_current_output_volume >= 95) ? 100 : s_current_output_volume + 5;
  message_send_set_output_volume(s_current_output_id, s_current_output_volume);
  update_volume_display();
  light_vibe();
}

static void volume_down_click(ClickRecognizerRef recognizer, void *context) {
  app_auto_close_reset();
  s_current_output_volume = (s_current_output_volume <= 5) ? 0 : s_current_output_volume - 5;
  message_send_set_output_volume(s_current_output_id, s_current_output_volume);
  update_volume_display();
  light_vibe();
}

static void volume_select_click(ClickRecognizerRef recognizer, void *context) {
  app_auto_close_reset();
  // Toggle playback - update icon immediately for responsive UI
  if (s_output_player_state == PLAYER_STATE_PLAYING) {
    s_output_player_state = PLAYER_STATE_PAUSED;
  } else {
    s_output_player_state = PLAYER_STATE_PLAYING;
    player_set_transient_playing_state();
  }
  update_output_play_pause_icon();
  
  message_send_command(CMD_PLAY_PAUSE);
  light_vibe();
}

#ifndef PBL_PLATFORM_APLITE
static void volume_select_long_click(ClickRecognizerRef recognizer, void *context) {
  player_window_push();
}
#endif

static void volume_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, volume_up_click);
  window_single_click_subscribe(BUTTON_ID_DOWN, volume_down_click);
  window_single_click_subscribe(BUTTON_ID_SELECT, volume_select_click);
#ifndef PBL_PLATFORM_APLITE
  window_long_click_subscribe(BUTTON_ID_SELECT, 500, volume_select_long_click, NULL);
#endif
}

static void volume_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // Create action bar
  s_volume_action_bar = action_bar_layer_create();
  action_bar_layer_add_to_window(s_volume_action_bar, window);
  action_bar_layer_set_click_config_provider(s_volume_action_bar, volume_click_config_provider);
  
  // Load icons from resources
  s_icon_vol_up = gbitmap_create_with_resource(RESOURCE_ID_ICON_VOLUME_UP);
  s_icon_vol_down = gbitmap_create_with_resource(RESOURCE_ID_ICON_VOLUME_DOWN);
  s_icon_play = gbitmap_create_with_resource(RESOURCE_ID_ICON_PLAY);
  s_icon_pause = gbitmap_create_with_resource(RESOURCE_ID_ICON_PAUSE);
  
  action_bar_layer_set_icon(s_volume_action_bar, BUTTON_ID_UP, s_icon_vol_up);
  action_bar_layer_set_icon(s_volume_action_bar, BUTTON_ID_DOWN, s_icon_vol_down);
  action_bar_layer_set_icon(s_volume_action_bar, BUTTON_ID_SELECT, s_icon_play); // Default to play icon
  
  // Adjust bounds for action bar
  bounds.size.w -= ACTION_BAR_WIDTH;
  
  s_name_layer = text_layer_create(PBL_IF_ROUND_ELSE(
    GRect(20, 30, bounds.size.w - 40, 40),
    GRect(4,  30, bounds.size.w - 8,  40)));
  text_layer_set_font(s_name_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_name_layer, GTextAlignmentCenter);
  text_layer_set_overflow_mode(s_name_layer, GTextOverflowModeTrailingEllipsis);
  text_layer_set_text(s_name_layer, s_current_output_name);
  layer_add_child(window_layer, text_layer_get_layer(s_name_layer));
  
  s_volume_layer = text_layer_create(GRect(4, 80, bounds.size.w - 8, 50));
  text_layer_set_font(s_volume_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_volume_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_volume_layer));
  
  update_volume_display();
}

static void volume_window_unload(Window *window) {
  if (s_state_request_timer) {
    app_timer_cancel(s_state_request_timer);
    s_state_request_timer = NULL;
  }
  
  message_set_player_callback(NULL);
  message_set_status_callback(NULL);
  text_layer_destroy(s_name_layer);
  text_layer_destroy(s_volume_layer);
  action_bar_layer_destroy(s_volume_action_bar);
  gbitmap_destroy(s_icon_vol_up);
  gbitmap_destroy(s_icon_vol_down);
  gbitmap_destroy(s_icon_play);
  gbitmap_destroy(s_icon_pause);
  
  // Destroy the window itself
  window_destroy(window);
  s_volume_window = NULL;
}

static void request_player_state_timer_callback(void *data) {
  s_state_request_timer = NULL;
  message_send_command(CMD_GET_PLAYER_STATE);
}

static void volume_window_appear(Window *window) {
  // Register callbacks and get current state when window becomes visible
  message_set_player_callback(output_volume_player_state_handler);
  message_set_status_callback(output_volume_status_handler);
  
  // Delay the player state request to avoid outbox collision with SET_OUTPUT_EXCLUSIVE
  if (s_state_request_timer) {
    app_timer_cancel(s_state_request_timer);
  }
  s_state_request_timer = app_timer_register(200, request_player_state_timer_callback, NULL);
}

static void volume_window_disappear(Window *window) {
  // Don't destroy - let parent outputs window manage lifecycle
}

static void output_volume_window_push(const char *name, const char *id, int volume) {
  snprintf(s_current_output_name, sizeof(s_current_output_name), "%s", name);
  snprintf(s_current_output_id, sizeof(s_current_output_id), "%s", id);
  s_current_output_volume = volume;
  
  if (!s_volume_window) {
    s_volume_window = window_create();
    window_set_window_handlers(s_volume_window, (WindowHandlers){
      .load = volume_window_load,
      .unload = volume_window_unload,
      .appear = volume_window_appear,
      .disappear = volume_window_disappear
    });
  }
  
  window_stack_push(s_volume_window, true);
}
