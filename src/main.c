#include <pebble.h>
#include "message_keys.h"
#include "messaging.h"
#include "windows/splash.h"

static void init(void) {
  message_init();
  splash_window_push();
}

static void deinit(void) {
  message_deinit();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
