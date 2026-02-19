#pragma once
#include <pebble.h>

void player_window_push(void);
void player_set_launch_state_playing(void);
void player_set_transient_playing_state(void);
bool player_is_transient_playing(void);
void player_set_auto_close_timeout(int timeout_seconds);
