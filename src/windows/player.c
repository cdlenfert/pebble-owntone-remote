#include <pebble.h>
#include "player.h"
#include "../message_keys.h"
#include "../messaging.h"
#include "outputs.h"

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
static GBitmap *s_icon_ellipsis;

static PlayerState s_player_state = PLAYER_STATE_STOPPED;
static int s_current_volume = 50;
static char s_track_text[MAX_STRING_LENGTH];
static char s_artist_text[MAX_STRING_LENGTH];
static char s_album_text[MAX_STRING_LENGTH];

// Optimistic UI flag for launching from queue
static bool s_force_initial_playing = false;
static bool s_delay_next_appear_retry = false;

// Transient flag to enforce "Playing" state visually during track changes
// This prevents the icon from flickering to "Play" (Stopped) if the server momentarily reports stopped state
static bool s_transient_playing_state = false;
static AppTimer *s_transient_playing_timer = NULL;

// Mode tracking
typedef enum {
  CONTROL_MODE_TRANSPORT,  // Normal mode: prev/play-pause/next
  CONTROL_MODE_VOLUME      // Volume mode: vol up/play-pause/vol down
} ControlMode;

static ControlMode s_control_mode = CONTROL_MODE_TRANSPORT;
static AppTimer *s_mode_timer = NULL;
static AppTimer *s_status_check_timer = NULL;
static AppTimer *s_poll_timer = NULL;
static AppTimer *s_state_retry_timer = NULL;
static int s_state_retry_attempts = 0;
static AppTimer *s_volume_repeat_timer = NULL;
static bool s_volume_up_held = false;
static bool s_volume_down_held = false;

// Auto-close configuration for battery optimization
static int s_auto_close_timeout_seconds = 0; // 0 = never auto-close
static AppTimer *s_auto_close_timer = NULL;

// Custom light vibration pattern (20ms pulse)
static void light_vibe(void) {
  uint32_t segments[] = { 20 };
  VibePattern pat = {
    .durations = segments,
    .num_segments = 1,
  };
  vibes_enqueue_custom_pattern(pat);
}

static void cancel_volume_repeat_timer(void) {
  if (s_volume_repeat_timer) {
    app_timer_cancel(s_volume_repeat_timer);
    s_volume_repeat_timer = NULL;
  }
  s_volume_up_held = false;
  s_volume_down_held = false;
}

static void volume_repeat_callback(void *data) {
  if (s_volume_up_held) {
    s_current_volume = (s_current_volume >= 95) ? 100 : s_current_volume + 5;
    message_send_set_volume(s_current_volume);
    light_vibe();
    s_volume_repeat_timer = app_timer_register(500, volume_repeat_callback, NULL);
  } else if (s_volume_down_held) {
    s_current_volume = (s_current_volume <= 5) ? 0 : s_current_volume - 5;
    message_send_set_volume(s_current_volume);
    light_vibe();
    s_volume_repeat_timer = app_timer_register(500, volume_repeat_callback, NULL);
  }
}

static void revert_to_transport_mode(void *data) {
  s_mode_timer = NULL;
  s_control_mode = CONTROL_MODE_TRANSPORT;
  
  // Restore transport controls
  action_bar_layer_set_icon(s_action_bar, BUTTON_ID_UP, s_icon_prev);
  action_bar_layer_set_icon(s_action_bar, BUTTON_ID_DOWN, s_icon_next);
  // Show play icon when paused, ellipsis when playing
  if (s_player_state == PLAYER_STATE_PLAYING) {
    action_bar_layer_set_icon(s_action_bar, BUTTON_ID_SELECT, s_icon_ellipsis);
  } else {
    action_bar_layer_set_icon(s_action_bar, BUTTON_ID_SELECT, s_icon_play);
  }
}

static void cancel_mode_timer(void) {
  if (s_mode_timer) {
    app_timer_cancel(s_mode_timer);
    s_mode_timer = NULL;
  }
}

static void cancel_status_check_timer(void) {
  if (s_status_check_timer) {
    app_timer_cancel(s_status_check_timer);
    s_status_check_timer = NULL;
  }
}

static void cancel_poll_timer(void) {
  if (s_poll_timer) {
    app_timer_cancel(s_poll_timer);
    s_poll_timer = NULL;
  }
}

static void cancel_auto_close_timer(void) {
  if (s_auto_close_timer) {
    app_timer_cancel(s_auto_close_timer);
    s_auto_close_timer = NULL;
  }
}

static void start_auto_close_timer(void);

static void reset_auto_close_timer(void) {
  // User interaction detected, restart the timer
  if (s_auto_close_timeout_seconds > 0) {
    cancel_auto_close_timer();
    start_auto_close_timer();
  }
}

// Retry/backoff for initial player state requests
#define STATE_RETRY_INITIAL_MS 100
#define STATE_RETRY_MAX_ATTEMPTS 5
#define STATE_RETRY_DELAYS_MS { 100, 200, 500, 1000, 2000 }

static void cancel_state_retry(void) {
  if (s_state_retry_timer) {
    app_timer_cancel(s_state_retry_timer);
    s_state_retry_timer = NULL;
  }
  s_state_retry_attempts = 0;
}

static void state_retry_callback(void *data) {
  s_state_retry_timer = NULL;

  // If we've exhausted attempts, give up
  if (s_state_retry_attempts >= STATE_RETRY_MAX_ATTEMPTS) {
    return;
  }

  // Send another request
  message_send_command(CMD_GET_PLAYER_STATE);
  s_state_retry_attempts++;

  // Use predefined retry delays
  static const int retry_delays[] = STATE_RETRY_DELAYS_MS;
  if (s_state_retry_attempts < STATE_RETRY_MAX_ATTEMPTS) {
    int delay_ms = retry_delays[s_state_retry_attempts];
    s_state_retry_timer = app_timer_register(delay_ms, state_retry_callback, NULL);
  }
}

static void start_state_retry(void) {
  cancel_state_retry();
  s_state_retry_attempts = 0;
  s_state_retry_timer = app_timer_register(STATE_RETRY_INITIAL_MS, state_retry_callback, NULL);
}

static void poll_callback(void *data) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "poll_callback running");
  s_poll_timer = NULL;
  message_send_command(CMD_GET_PLAYER_STATE);
  
  // Self-sustaining polling - reschedule based on current state
  // This runs independently of state updates to avoid reset races
  if (s_player_state == PLAYER_STATE_PLAYING) {
    s_poll_timer = app_timer_register(3000, poll_callback, NULL);
  } else {
    s_poll_timer = app_timer_register(10000, poll_callback, NULL);
  }
}

static void defer_polling(int delay_ms) {
  cancel_poll_timer();
  s_poll_timer = app_timer_register(delay_ms, poll_callback, NULL);
}

static void transient_playing_timer_callback(void *data) {
  s_transient_playing_timer = NULL;
  s_transient_playing_state = false;
}

void player_set_transient_playing_state(void) {
  s_transient_playing_state = true;
  if (s_transient_playing_timer) {
    app_timer_cancel(s_transient_playing_timer);
  }
  // Enforce "Playing" visual state for 8 seconds (allow for slow server response)
  s_transient_playing_timer = app_timer_register(8000, transient_playing_timer_callback, NULL);
}

bool player_is_transient_playing(void) {
  return s_transient_playing_state;
}

static void start_mode_timer(void) {
  cancel_mode_timer();
  s_mode_timer = app_timer_register(2000, revert_to_transport_mode, NULL);
}

static void auto_close_timer_callback(void *data) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Auto-close timeout reached, closing player window");
  s_auto_close_timer = NULL;
  
  // Stop all polling before closing window
  cancel_poll_timer();
  cancel_status_check_timer();
  cancel_mode_timer();
  cancel_state_retry();
  
  // Pop the window to return to main menu
  if (s_window) {
    window_stack_pop(true);
  }
}

static void start_auto_close_timer(void) {
  APP_LOG(APP_LOG_LEVEL_INFO, "start_auto_close_timer called, timeout=%d", s_auto_close_timeout_seconds);
  if (s_auto_close_timeout_seconds > 0) {
    cancel_auto_close_timer();
    s_auto_close_timer = app_timer_register(s_auto_close_timeout_seconds * 1000, auto_close_timer_callback, NULL);
    APP_LOG(APP_LOG_LEVEL_INFO, "Auto-close timer started for %d seconds", s_auto_close_timeout_seconds);
  } else {
    APP_LOG(APP_LOG_LEVEL_INFO, "Auto-close timer NOT started (timeout=0)");
  }
}

static void update_action_bar(void) {
  // Don't update if icons aren't loaded yet
  if (!s_icon_play || !s_icon_pause || !s_icon_next || !s_icon_prev) {
    return;
  }
  
  if (s_control_mode == CONTROL_MODE_TRANSPORT) {
    action_bar_layer_set_icon(s_action_bar, BUTTON_ID_UP, s_icon_prev);
    action_bar_layer_set_icon(s_action_bar, BUTTON_ID_DOWN, s_icon_next);
    // Show play icon when paused, ellipsis when playing
    if (s_player_state == PLAYER_STATE_PLAYING) {
      action_bar_layer_set_icon(s_action_bar, BUTTON_ID_SELECT, s_icon_ellipsis);
    } else {
      action_bar_layer_set_icon(s_action_bar, BUTTON_ID_SELECT, s_icon_play);
    }
  } else {
    // Volume mode
    if (!s_icon_volume_up || !s_icon_volume_down) {
      return;
    }
    action_bar_layer_set_icon(s_action_bar, BUTTON_ID_UP, s_icon_volume_up);
    action_bar_layer_set_icon(s_action_bar, BUTTON_ID_DOWN, s_icon_volume_down);
    if (s_player_state == PLAYER_STATE_PLAYING) {
      action_bar_layer_set_icon(s_action_bar, BUTTON_ID_SELECT, s_icon_pause);
    } else {
      action_bar_layer_set_icon(s_action_bar, BUTTON_ID_SELECT, s_icon_play);
    }
  }
}

static void player_state_handler(PlayerState state, const char *track, const char *artist, const char *album, int volume) {
  // Cancel any outstanding retries once we receive a valid state
  cancel_state_retry();

  // If we are in a transient "Forced Playing" state (e.g. just clicked Next),
  // override the state to Playing to prevent UI flicker if server reports Stopped momentarily.
  if (s_transient_playing_state) {
    s_player_state = PLAYER_STATE_PLAYING;
  } else {
    s_player_state = state;
  }
  
  s_current_volume = volume;
  
  snprintf(s_track_text, sizeof(s_track_text), "%s", track ? track : "No track");
  snprintf(s_artist_text, sizeof(s_artist_text), "%s", artist ? artist : "");
  snprintf(s_album_text, sizeof(s_album_text), "%s", album ? album : "");
  
  text_layer_set_text(s_track_layer, s_track_text);
  text_layer_set_text(s_artist_layer, s_artist_text);
  text_layer_set_text(s_album_layer, s_album_text);
  
  // Update UI immediately on state change
  update_action_bar();
  
  // Don't restart polling here - let it run independently to avoid timer reset races
  // The polling callback will adapt intervals based on s_player_state
}

static void player_status_handler(int status) {
  // Status response from JS bridge. A value of 1 indicates success/acknowledgement.
  // If we receive a generic acknowledgement, cancel any outstanding state retries
  // since the JS bridge has received our request and will (or already did) send state.
  if (status == 1) {
    cancel_state_retry();
    return;
  }

  // Non-OK status for play/pause toggles: revert optimistic icon change
  if (status != 1) {
    if (s_player_state == PLAYER_STATE_PLAYING) {
      s_player_state = PLAYER_STATE_PAUSED;
    } else {
      s_player_state = PLAYER_STATE_PLAYING;
    }
    update_action_bar();
  }
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  reset_auto_close_timer();
  if (s_control_mode == CONTROL_MODE_VOLUME) {
    // Volume up
    s_current_volume = (s_current_volume >= 95) ? 100 : s_current_volume + 5;
    message_send_set_volume(s_current_volume);
    light_vibe();
    start_mode_timer();
  } else {
    // Previous track - optimistic UI update, will sync on response
    s_player_state = PLAYER_STATE_PLAYING;
    update_action_bar();
    message_send_command(CMD_PREVIOUS);
    light_vibe();
    defer_polling(1000); // Delay polling to allow server state to update
    player_set_transient_playing_state();
  }
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  reset_auto_close_timer();
  if (s_control_mode == CONTROL_MODE_TRANSPORT) {
    if (s_player_state == PLAYER_STATE_PLAYING) {
      // Playing - switch to volume mode (no vibration for ellipsis)
      s_control_mode = CONTROL_MODE_VOLUME;
      update_action_bar();
      start_mode_timer();
    } else {
      // Paused - play the music
      s_player_state = PLAYER_STATE_PLAYING;
      update_action_bar();
      message_send_command(CMD_PLAY);
      light_vibe();
      defer_polling(1000); // Delay polling
      player_set_transient_playing_state();
    }
  } else {
    // Volume mode: Play/Pause toggle
    if (s_player_state == PLAYER_STATE_PLAYING) {
      s_player_state = PLAYER_STATE_PAUSED;
      update_action_bar();
      message_send_command(CMD_PAUSE);
    } else {
      s_player_state = PLAYER_STATE_PLAYING;
      update_action_bar();
      message_send_command(CMD_PLAY);
      player_set_transient_playing_state();
    }
    light_vibe();
    start_mode_timer();
    defer_polling(1000); // Delay polling
  }
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  reset_auto_close_timer();
  if (s_control_mode == CONTROL_MODE_VOLUME) {
    // Volume down
    s_current_volume = (s_current_volume <= 5) ? 0 : s_current_volume - 5;
    message_send_set_volume(s_current_volume);
    light_vibe();
    start_mode_timer();
  } else {
    // Next track - optimistic UI update, will sync on response
    s_player_state = PLAYER_STATE_PLAYING;
    update_action_bar();
    message_send_command(CMD_NEXT);
    light_vibe();
    defer_polling(1000); // Delay polling to allow server state to update
    player_set_transient_playing_state();
  }
}

static void up_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  reset_auto_close_timer();
  // Show volume icons during long press
  action_bar_layer_set_icon(s_action_bar, BUTTON_ID_UP, s_icon_volume_up);
  action_bar_layer_set_icon(s_action_bar, BUTTON_ID_DOWN, s_icon_volume_down);
  
  // Volume up (first press)
  s_current_volume = (s_current_volume >= 95) ? 100 : s_current_volume + 5;
  message_send_set_volume(s_current_volume);
  light_vibe();
  
  // Start repeating
  s_volume_up_held = true;
  s_volume_repeat_timer = app_timer_register(500, volume_repeat_callback, NULL);
}

static void up_long_click_release_handler(ClickRecognizerRef recognizer, void *context) {
  // Stop repeating and restore transport mode icons
  cancel_volume_repeat_timer();
  update_action_bar();
}

static void down_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  reset_auto_close_timer();
  // Show volume icons during long press
  action_bar_layer_set_icon(s_action_bar, BUTTON_ID_UP, s_icon_volume_up);
  action_bar_layer_set_icon(s_action_bar, BUTTON_ID_DOWN, s_icon_volume_down);
  
  // Volume down (first press)
  s_current_volume = (s_current_volume <= 5) ? 0 : s_current_volume - 5;
  message_send_set_volume(s_current_volume);
  light_vibe();
  
  // Start repeating
  s_volume_down_held = true;
  s_volume_repeat_timer = app_timer_register(500, volume_repeat_callback, NULL);
}

static void down_long_click_release_handler(ClickRecognizerRef recognizer, void *context) {
  // Stop repeating and restore transport mode icons
  cancel_volume_repeat_timer();
  update_action_bar();
}

static void player_select_long_click(ClickRecognizerRef recognizer, void *context) {
  reset_auto_close_timer();
  // Long-press Select on Player -> go to Outputs
  outputs_window_push();
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_long_click_subscribe(BUTTON_ID_UP, 500, up_long_click_handler, up_long_click_release_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 500, player_select_long_click, NULL);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_long_click_subscribe(BUTTON_ID_DOWN, 500, down_long_click_handler, down_long_click_release_handler);
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
  s_icon_ellipsis = gbitmap_create_with_resource(RESOURCE_ID_ICON_ELLIPSIS);
  
  // Start in transport mode with play icon (default assumes paused)
  s_control_mode = CONTROL_MODE_TRANSPORT;
  action_bar_layer_set_icon(s_action_bar, BUTTON_ID_UP, s_icon_prev);
  action_bar_layer_set_icon(s_action_bar, BUTTON_ID_DOWN, s_icon_next);
  action_bar_layer_set_icon(s_action_bar, BUTTON_ID_SELECT, s_icon_play);
  
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
  
  // Set callbacks and request player state. Use any cached state first to
  // avoid the race where JS replies while the player UI isn't registered.
  message_set_player_callback(player_state_handler);
  
  // Handle launch from queue (Optimistic UI)
  if (s_force_initial_playing) {
    s_player_state = PLAYER_STATE_PLAYING;
    s_force_initial_playing = false;
    
    // Maintain playing state even if server initially reports stopped
    player_set_transient_playing_state();
    
    // Show "Loading..." to confirm action
    text_layer_set_text(s_track_layer, "Loading...");
    text_layer_set_text(s_artist_layer, "");
    text_layer_set_text(s_album_layer, "");
    update_action_bar();
    
    // Request fresh state immediately, skip cache
    message_send_command(CMD_GET_PLAYER_STATE);
    start_state_retry();
  }
  
  message_set_status_callback(player_status_handler);
  
  // Always start polling - it will detect state changes
  s_poll_timer = app_timer_register(3000, poll_callback, NULL);

  if (message_has_cached_player_state()) {
    PlayerState cs = PLAYER_STATE_STOPPED;
    char track[MAX_STRING_LENGTH] = {0};
    char artist[MAX_STRING_LENGTH] = {0};
    char album[MAX_STRING_LENGTH] = {0};
    int vol = 50;
    message_get_cached_player_state(&cs, track, artist, album, &vol);
    APP_LOG(APP_LOG_LEVEL_INFO, "player: using cached player state on load");
    player_state_handler(cs, track, artist, album, vol);
    // Request fresh state since cache might be stale
    message_send_command(CMD_GET_PLAYER_STATE);
  } else {
    // No cached state - request immediate state and start retry
    message_send_command(CMD_GET_PLAYER_STATE);
    start_state_retry();
  }
  // Don't call update_action_bar here - it's already called by player_state_handler
  // or will be called when state arrives
}

static void window_unload(Window *window) {
  message_set_player_callback(NULL);
  message_set_status_callback(NULL);
  
  cancel_mode_timer();
  cancel_status_check_timer();
  cancel_state_retry();
  cancel_poll_timer();
  cancel_volume_repeat_timer();
  
  text_layer_destroy(s_track_layer);
  text_layer_destroy(s_artist_layer);
  text_layer_destroy(s_album_layer);
  
  gbitmap_destroy(s_icon_play);
  gbitmap_destroy(s_icon_pause);
  gbitmap_destroy(s_icon_next);
  gbitmap_destroy(s_icon_prev);
  gbitmap_destroy(s_icon_volume_up);
  gbitmap_destroy(s_icon_volume_down);
  gbitmap_destroy(s_icon_ellipsis);
  
  // Set to NULL so window_appear knows to reload them
  s_icon_play = NULL;
  s_icon_pause = NULL;
  s_icon_next = NULL;
  s_icon_prev = NULL;
  s_icon_volume_up = NULL;
  s_icon_volume_down = NULL;
  s_icon_ellipsis = NULL;
  
  action_bar_layer_destroy(s_action_bar);
}

static void window_appear(Window *window) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Player window appearing, timeout=%d", s_auto_close_timeout_seconds);
  // Reload any icons that were destroyed or failed to load
  if (!s_icon_play) s_icon_play = gbitmap_create_with_resource(RESOURCE_ID_ICON_PLAY);
  if (!s_icon_pause) s_icon_pause = gbitmap_create_with_resource(RESOURCE_ID_ICON_PAUSE);
  if (!s_icon_next) s_icon_next = gbitmap_create_with_resource(RESOURCE_ID_ICON_NEXT);
  if (!s_icon_prev) s_icon_prev = gbitmap_create_with_resource(RESOURCE_ID_ICON_PREV);
  if (!s_icon_volume_up) s_icon_volume_up = gbitmap_create_with_resource(RESOURCE_ID_ICON_VOLUME_UP);
  if (!s_icon_volume_down) s_icon_volume_down = gbitmap_create_with_resource(RESOURCE_ID_ICON_VOLUME_DOWN);
  if (!s_icon_ellipsis) s_icon_ellipsis = gbitmap_create_with_resource(RESOURCE_ID_ICON_ELLIPSIS);

  // Callbacks already set in window_load, just ensure they're still set
  message_set_player_callback(player_state_handler);
  message_set_status_callback(player_status_handler);
  
  // Reset to transport mode
  cancel_mode_timer();
  s_control_mode = CONTROL_MODE_TRANSPORT;
  update_action_bar();
  
  if (s_transient_playing_timer) {
    app_timer_cancel(s_transient_playing_timer);
    s_transient_playing_timer = NULL;
  }
  // Start polling if not already running
  if (!s_poll_timer) {
    s_poll_timer = app_timer_register(3000, poll_callback, NULL);
  }
  
  // Start auto-close timer for battery optimization
  start_auto_close_timer();
  
  // Request fresh state with retry mechanism
  if (s_delay_next_appear_retry) {
    // We just launched from queue, delay the check to let server update
    s_delay_next_appear_retry = false;
    cancel_state_retry();
    s_state_retry_attempts = 0;
    // Delay 1 second before first check
    s_state_retry_timer = app_timer_register(1000, state_retry_callback, NULL); 
  } else {
    start_state_retry();
  }
}

static void window_disappear(Window *window) {
  // Stop polling when window is not visible
  cancel_poll_timer();
  cancel_status_check_timer();
  cancel_mode_timer();
  cancel_volume_repeat_timer();
  cancel_state_retry();
  cancel_auto_close_timer();
}

void player_set_launch_state_playing(void) {
  s_force_initial_playing = true;
}

void player_set_auto_close_timeout(int timeout_seconds) {
  s_auto_close_timeout_seconds = timeout_seconds;
  APP_LOG(APP_LOG_LEVEL_INFO, "Player auto-close timeout set to %d seconds", timeout_seconds);
  
  // If player window is currently visible, restart timer with new timeout
  bool window_exists = (s_window != NULL);
  bool is_top = window_exists && (window_stack_get_top_window() == s_window);
  APP_LOG(APP_LOG_LEVEL_INFO, "Window state: exists=%d, is_top=%d", window_exists, is_top);
  
  if (is_top) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Player window is visible, restarting timer");
    cancel_auto_close_timer();
    start_auto_close_timer();
  } else {
    APP_LOG(APP_LOG_LEVEL_INFO, "Player window not visible yet, timer will start on appear");
  }
}

void player_window_push(void) {

  if (!s_window) {
    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers){
      .load = window_load,
      .unload = window_unload,
      .appear = window_appear,
      .disappear = window_disappear
    });
  }
  
  // If player window is already on top, do nothing
  if (window_stack_get_top_window() == s_window) {
    return;
  }
  
  // Otherwise push it - this may create a duplicate in the stack, but pressing Back will work correctly
  window_stack_push(s_window, true);
}
