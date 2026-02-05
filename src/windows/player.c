#include <pebble.h>
#include "player.h"
#include "../message_keys.h"
#include "../messaging.h"

static Window *s_window;
static TextLayer *s_track_layer;
static TextLayer *s_artist_layer;
static TextLayer *s_album_layer;
static ActionBarLayer *s_action_bar;
static GBitmap *s_icon_play;
static GBitmap *s_icon_pause;
static GBitmap *s_icon_next;
static GBitmap *s_icon_prev;
static GBitmap *s_icon_volume_up;
static GBitmap *s_icon_volume_down;

static PlayerState s_player_state = PLAYER_STATE_STOPPED;
static int s_current_volume = 50;
static char s_track_text[MAX_STRING_LENGTH];
static char s_artist_text[MAX_STRING_LENGTH];
static char s_album_text[MAX_STRING_LENGTH];

static void update_action_bar(void) {
  if (s_player_state == PLAYER_STATE_PLAYING) {
    action_bar_layer_set_icon(s_action_bar, BUTTON_ID_SELECT, s_icon_pause);
  } else {
    action_bar_layer_set_icon(s_action_bar, BUTTON_ID_SELECT, s_icon_play);
  }
}

static void player_state_handler(PlayerState state, const char *track, const char *artist, const char *album, int volume) {
  s_player_state = state;
  s_current_volume = volume;
  
  snprintf(s_track_text, sizeof(s_track_text), "%s", track ? track : "No track");
  snprintf(s_artist_text, sizeof(s_artist_text), "%s", artist ? artist : "");
  snprintf(s_album_text, sizeof(s_album_text), "%s", album ? album : "");
  
  text_layer_set_text(s_track_layer, s_track_text);
  text_layer_set_text(s_artist_layer, s_artist_text);
  text_layer_set_text(s_album_layer, s_album_text);
  
  update_action_bar();
}

static void player_status_handler(int status) {
  // Status response from play/pause command - icon already toggled optimistically
  if (status != 1) {
    // Failed - revert the icon (shouldn't happen often)
    if (s_player_state == PLAYER_STATE_PLAYING) {
      s_player_state = PLAYER_STATE_PAUSED;
    } else {
      s_player_state = PLAYER_STATE_PLAYING;
    }
    update_action_bar();
  }
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Next track
  message_send_command(CMD_NEXT);
  vibes_short_pulse();
}

static void up_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Volume up
  s_current_volume = (s_current_volume >= 95) ? 100 : s_current_volume + 5;
  message_send_set_volume(s_current_volume);
  vibes_short_pulse();
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Play/Pause toggle - update icon immediately for responsive UI
  if (s_player_state == PLAYER_STATE_PLAYING) {
    s_player_state = PLAYER_STATE_PAUSED;
  } else {
    s_player_state = PLAYER_STATE_PLAYING;
  }
  update_action_bar();
  
  message_send_command(CMD_PLAY_PAUSE);
  vibes_short_pulse();
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Previous track
  message_send_command(CMD_PREVIOUS);
  vibes_short_pulse();
}

static void down_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Volume down
  s_current_volume = (s_current_volume <= 5) ? 0 : s_current_volume - 5;
  message_send_set_volume(s_current_volume);
  vibes_short_pulse();
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_long_click_subscribe(BUTTON_ID_UP, 500, up_long_click_handler, NULL);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_long_click_subscribe(BUTTON_ID_DOWN, 500, down_long_click_handler, NULL);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // Create action bar
  s_action_bar = action_bar_layer_create();
  action_bar_layer_add_to_window(s_action_bar, window);
  action_bar_layer_set_click_config_provider(s_action_bar, click_config_provider);
  
  // Load icons
  s_icon_play = gbitmap_create_with_resource(RESOURCE_ID_ICON_PLAY);
  s_icon_pause = gbitmap_create_with_resource(RESOURCE_ID_ICON_PAUSE);
  s_icon_next = gbitmap_create_with_resource(RESOURCE_ID_ICON_NEXT);
  s_icon_prev = gbitmap_create_with_resource(RESOURCE_ID_ICON_PREV);
  s_icon_volume_up = gbitmap_create_with_resource(RESOURCE_ID_ICON_VOLUME_UP);
  s_icon_volume_down = gbitmap_create_with_resource(RESOURCE_ID_ICON_VOLUME_DOWN);
  
  action_bar_layer_set_icon(s_action_bar, BUTTON_ID_UP, s_icon_next);
  action_bar_layer_set_icon(s_action_bar, BUTTON_ID_DOWN, s_icon_prev);
  
  // Adjust bounds for action bar
  bounds.size.w -= ACTION_BAR_WIDTH;
  
  // Create text layers
  s_track_layer = text_layer_create(GRect(4, 15, bounds.size.w - 8, 50));
  text_layer_set_font(s_track_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_overflow_mode(s_track_layer, GTextOverflowModeTrailingEllipsis);
  text_layer_set_text(s_track_layer, "No track");
  layer_add_child(window_layer, text_layer_get_layer(s_track_layer));
  
  s_artist_layer = text_layer_create(GRect(4, 70, bounds.size.w - 8, 40));
  text_layer_set_font(s_artist_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_overflow_mode(s_artist_layer, GTextOverflowModeTrailingEllipsis);
  layer_add_child(window_layer, text_layer_get_layer(s_artist_layer));
  
  s_album_layer = text_layer_create(GRect(4, 115, bounds.size.w - 8, 40));
  text_layer_set_font(s_album_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_overflow_mode(s_album_layer, GTextOverflowModeTrailingEllipsis);
  layer_add_child(window_layer, text_layer_get_layer(s_album_layer));
  
  // Set callbacks and request player state
  message_set_player_callback(player_state_handler);
  message_set_status_callback(player_status_handler);
  message_send_command(CMD_GET_PLAYER_STATE);
  
  update_action_bar();
}

static void window_unload(Window *window) {
  message_set_player_callback(NULL);
  message_set_status_callback(NULL);
  
  text_layer_destroy(s_track_layer);
  text_layer_destroy(s_artist_layer);
  text_layer_destroy(s_album_layer);
  
  gbitmap_destroy(s_icon_play);
  gbitmap_destroy(s_icon_pause);
  gbitmap_destroy(s_icon_next);
  gbitmap_destroy(s_icon_prev);
  gbitmap_destroy(s_icon_volume_up);
  gbitmap_destroy(s_icon_volume_down);
  
  action_bar_layer_destroy(s_action_bar);
}

static void window_appear(Window *window) {
  // Refresh player state when window appears
  message_set_player_callback(player_state_handler);
  message_set_status_callback(player_status_handler);
  message_send_command(CMD_GET_PLAYER_STATE);
}

void player_window_push(void) {
  if (!s_window) {
    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers){
      .load = window_load,
      .unload = window_unload,
      .appear = window_appear
    });
  }
  window_stack_push(s_window, true);
}
