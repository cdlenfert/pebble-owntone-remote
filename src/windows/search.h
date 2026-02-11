#pragma once
#include <pebble.h>
#include "../message_keys.h"

#if !defined(PBL_PLATFORM_APLITE)
void search_window_push(void);
#endif
void random_window_push(void);
void results_window_push(int count, char *titles[], char *uris[], ContentType type);
