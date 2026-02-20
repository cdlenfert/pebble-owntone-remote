#include <pebble.h>
#include "message_keys.h"
#include "messaging.h"
#include "windows/splash.h"
#include "windows/main_menu.h"

static void init(void) {
  APP_LOG(APP_LOG_LEVEL_INFO, "main: init() start");
  message_init();
  APP_LOG(APP_LOG_LEVEL_INFO, "main: message_init() returned");
#if defined(PBL_PLATFORM_APLITE)
  // Skip splash on Aplite: saves the logo bitmap allocation and avoids
  // holding a GBitmap in memory during the connection check phase.
  main_menu_push();
#else
  splash_window_push();
#endif
  APP_LOG(APP_LOG_LEVEL_INFO, "main: init() window pushed");
}

static void deinit(void) {
  message_deinit();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
