#include <pebble.h>
#include "splash.h"
#include "../message_keys.h"
#include "../messaging.h"
#include "main_menu.h"
#include "player.h"

// Timers and retry configuration
#define SPLASH_MIN_MS 2000
#define SPLASH_RETRY_MS 2000
#define SPLASH_MAX_ATTEMPTS 5

static Window *s_window;
static BitmapLayer *s_logo_layer;
static GBitmap *s_logo = NULL;
static TextLayer *s_status_layer;
static AppTimer *s_retry_timer = NULL;
static AppTimer *s_min_timer = NULL;
static int s_attempts = 0;
static bool s_connected = false;
static bool s_min_elapsed = false;

static void cancel_retry_timer(void) {
  if (s_retry_timer) {
    app_timer_cancel(s_retry_timer);
    s_retry_timer = NULL;
  }
}

static void cancel_min_timer(void) {
  if (s_min_timer) {
    app_timer_cancel(s_min_timer);
    s_min_timer = NULL;
  }
}

static void proceed_to_app(void);

static void splash_player_callback(PlayerState state, const char *track, const char *artist, const char *album, int volume) {
  // Any player state response indicates the JS could reach the server
  if (s_connected) return;
  s_connected = true;
  text_layer_set_text(s_status_layer, "Connected");
  cancel_retry_timer();
  // If min display time elapsed, proceed now
  if (s_min_elapsed) {
    proceed_to_app();
  }
}

static void min_timer_callback(void *data) {
  s_min_timer = NULL;
  s_min_elapsed = true;
  if (s_connected) {
    proceed_to_app();
  }
}

static void send_ping(void) {
  if (s_connected) return;
  if (s_attempts >= SPLASH_MAX_ATTEMPTS) {
    // Give up
    text_layer_set_text(s_status_layer, "Server not found");
    cancel_retry_timer();
    return;
  }
  s_attempts++;
  message_set_player_callback(splash_player_callback);
  message_send_command(CMD_GET_PLAYER_STATE);
  // Schedule next retry if still not connected
  cancel_retry_timer();
  s_retry_timer = app_timer_register(SPLASH_RETRY_MS, (AppTimerCallback)send_ping, NULL);
}

static void proceed_to_app(void) {
  // Clean up timers and callbacks
  cancel_retry_timer();
  cancel_min_timer();
  message_set_player_callback(NULL);

  // Push main menu (so it's underneath) then player window
  main_menu_push();
  player_window_push();

  // Remove splash window from stack
  if (s_window) {
    window_stack_remove(s_window, false);
  }
}

static void back_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Allow user to exit the app if connection failed
  window_stack_pop_all(true);
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_BACK, back_click_handler);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Ensure white background for splash
  window_set_background_color(window, GColorWhite);

  // Choose resource based on platform: prefer BW on Aplite (classic)
#if defined(PBL_PLATFORM_APLITE)
  s_logo = gbitmap_create_with_resource(RESOURCE_ID_LOGO_OWNTONE_BW);
#else
  s_logo = gbitmap_create_with_resource(RESOURCE_ID_LOGO_OWNTONE);
  if (!s_logo) {
    s_logo = gbitmap_create_with_resource(RESOURCE_ID_LOGO_OWNTONE_BW);
  }
#endif

  if (s_logo) {
    GSize bmp_size = gbitmap_get_bounds(s_logo).size;
    APP_LOG(APP_LOG_LEVEL_INFO, "Splash bounds: %d x %d, logo: %d x %d", bounds.size.w, bounds.size.h, bmp_size.w, bmp_size.h);
    GRect bmp_frame = GRect((bounds.size.w - bmp_size.w) / 2, (bounds.size.h - bmp_size.h) / 2 - 10, bmp_size.w, bmp_size.h);
    s_logo_layer = bitmap_layer_create(bmp_frame);
    bitmap_layer_set_bitmap(s_logo_layer, s_logo);
    bitmap_layer_set_compositing_mode(s_logo_layer, GCompOpSet);
    layer_add_child(window_layer, bitmap_layer_get_layer(s_logo_layer));
  }

  // Status text below logo
  s_status_layer = text_layer_create(GRect(4, bounds.size.h - 40, bounds.size.w - 8, 36));
  text_layer_set_font(s_status_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(s_status_layer, GTextAlignmentCenter);
  text_layer_set_text(s_status_layer, "Checking for server...");
  text_layer_set_background_color(s_status_layer, GColorClear);
  text_layer_set_text_color(s_status_layer, GColorBlack);
  layer_add_child(window_layer, text_layer_get_layer(s_status_layer));

  // Start min timer and first ping
  s_min_elapsed = false;
  s_connected = false;
  s_attempts = 0;
  s_min_timer = app_timer_register(SPLASH_MIN_MS, min_timer_callback, NULL);
  // Small delay before first ping to ensure JS bridge is ready
  s_retry_timer = app_timer_register(500, (AppTimerCallback)send_ping, NULL);
}

static void window_unload(Window *window) {
  message_set_player_callback(NULL);
  cancel_retry_timer();
  cancel_min_timer();

  if (s_logo_layer) bitmap_layer_destroy(s_logo_layer);
  if (s_logo) gbitmap_destroy(s_logo);
  if (s_status_layer) text_layer_destroy(s_status_layer);

  window_destroy(s_window);
  s_window = NULL;
}

void splash_window_push(void) {
  if (!s_window) {
    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers){
      .load = window_load,
      .unload = window_unload
    });
    window_set_click_config_provider(s_window, click_config_provider);
  }
  window_stack_push(s_window, true);
}
