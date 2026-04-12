#include <pebble.h>
#include "message_keys.h"
#include "messaging.h"
#include "app_auto_close.h"
#include "windows/main_menu.h"
#include "windows/player.h"

static void push_player(void *data) {
  player_window_push();
}

static void init(void) {
  APP_LOG(APP_LOG_LEVEL_INFO, "main: init() start");
  app_auto_close_init();
  message_init();
  APP_LOG(APP_LOG_LEVEL_INFO, "main: message_init() returned");
  // Push main menu silently (no slide animation) so it sits beneath the
  // player, then push the player on top after a short delay.  This matches
  // the startup stack the old splash screen produced on Basalt but skips the
  // connectivity-gate UI entirely — early commands simply fail silently until
  // the JS bridge responds.
  main_menu_push_silent();
  app_timer_register(100, push_player, NULL);
  APP_LOG(APP_LOG_LEVEL_INFO, "main: init() window pushed");
}

static void deinit(void) {
  app_auto_close_deinit();
  message_deinit();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
