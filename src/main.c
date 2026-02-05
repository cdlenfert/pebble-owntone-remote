#include <pebble.h>
#include "message_keys.h"
#include "messaging.h"
#include "windows/main_menu.h"

static void init(void) {
  message_init();
  main_menu_push();
}

static void deinit(void) {
  message_deinit();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
