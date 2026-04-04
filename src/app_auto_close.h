#pragma once
#include <pebble.h>

void app_auto_close_init(void);
void app_auto_close_deinit(void);
void app_auto_close_set_timeout(int timeout_seconds);
void app_auto_close_start(void);   // start/restart timer (call when player window closes)
void app_auto_close_cancel(void);  // cancel timer (call when player window opens)
void app_auto_close_reset(void);   // cancel + restart if timeout > 0 (call on any button press)
