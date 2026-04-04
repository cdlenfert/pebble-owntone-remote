#include <pebble.h>
#include "app_auto_close.h"

static int s_timeout_seconds = 0;
static AppTimer *s_timer = NULL;

static void timer_callback(void *data) {
  s_timer = NULL;
  APP_LOG(APP_LOG_LEVEL_INFO, "App auto-close timeout reached, exiting app");
  window_stack_pop_all(true);
}

void app_auto_close_cancel(void) {
  if (s_timer) {
    app_timer_cancel(s_timer);
    s_timer = NULL;
  }
}

void app_auto_close_start(void) {
  app_auto_close_cancel();
  if (s_timeout_seconds > 0) {
    s_timer = app_timer_register(s_timeout_seconds * 1000, timer_callback, NULL);
    APP_LOG(APP_LOG_LEVEL_INFO, "App auto-close timer started for %d seconds", s_timeout_seconds);
  }
}

void app_auto_close_reset(void) {
  if (s_timeout_seconds > 0) {
    app_auto_close_start();
  }
}

void app_auto_close_set_timeout(int timeout_seconds) {
  s_timeout_seconds = timeout_seconds;
  APP_LOG(APP_LOG_LEVEL_INFO, "App auto-close timeout set to %d seconds", timeout_seconds);
}

void app_auto_close_init(void) {
  s_timeout_seconds = 0;
  s_timer = NULL;
}

void app_auto_close_deinit(void) {
  app_auto_close_cancel();
}
