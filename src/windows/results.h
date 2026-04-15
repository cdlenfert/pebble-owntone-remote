#pragma once
#include "../message_keys.h"

// Push a results list window populated with the given search/random results.
void results_window_push(int count, char *titles[], char *uris[],
                          ContentType type);
