#include <pebble.h>
#include "message_keys.h"
#include "messaging.h"
#include "windows/splash.h"

static void init(void) {
  APP_LOG(APP_LOG_LEVEL_INFO, "main: init() start");
  message_init();
  APP_LOG(APP_LOG_LEVEL_INFO, "main: message_init() returned");
  splash_window_push();
  APP_LOG(APP_LOG_LEVEL_INFO, "main: splash_window_push() returned");
}

static void deinit(void) {
  message_deinit();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
