#include <pebble.h>
#include "message_keys.h"
#include "messaging.h"
#include "app_auto_close.h"
#include "windows/splash.h"
#include "windows/main_menu.h"
#include "windows/player.h"

#if defined(PBL_PLATFORM_APLITE)
static void aplite_push_player(void *data) {
  player_window_push();
}
#endif

static void init(void) {
  APP_LOG(APP_LOG_LEVEL_INFO, "main: init() start");
  app_auto_close_init();
  message_init();
  APP_LOG(APP_LOG_LEVEL_INFO, "main: message_init() returned");
#if defined(PBL_PLATFORM_APLITE)
  // Skip splash on Aplite: saves the logo bitmap allocation entirely.
  // Push main menu first so it sits underneath, then player on top —
  // mirrors what splash's proceed_to_app() does on other platforms.
  main_menu_push();
  app_timer_register(100, aplite_push_player, NULL);
#else
  splash_window_push();
#endif
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
