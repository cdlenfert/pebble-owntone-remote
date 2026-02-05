#pragma once
#include <pebble.h>
#include "../message_keys.h"

void search_window_push(void);
void random_window_push(void);
void results_window_push(int count, char *titles[], char *uris[], ContentType type);
