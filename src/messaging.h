#pragma once
#include <pebble.h>
#include "message_keys.h"

// Initialize/deinitialize messaging system
void message_init(void);
void message_deinit(void);

// Callback types
typedef void (*PlayerStateCallback)(PlayerState state, const char *track, const char *artist, const char *album, int volume);
typedef void (*SearchResultsCallback)(int count, char *titles[], char *uris[]);
typedef void (*OutputsCallback)(int count, char *names[], char *ids[], int volumes[], bool enabled[]);
typedef void (*StatusCallback)(int status);
typedef void (*FavoritesCallback)(int count, char *names[], int types[]);

// Register callbacks
void message_set_player_callback(PlayerStateCallback callback);
void message_set_results_callback(SearchResultsCallback callback);
void message_set_outputs_callback(OutputsCallback callback);
void message_set_status_callback(StatusCallback callback);
void message_set_favorites_callback(FavoritesCallback callback);

// Cached player-state helpers (avoid race where JS responds before UI is ready)
bool message_has_cached_player_state(void);
void message_get_cached_player_state(PlayerState *state, char *track, char *artist, char *album, int *volume);

// Send messages to phone
void message_send_command(CommandType cmd);
void message_send_search(ContentType type, const char *query);
void message_send_random(ContentType type);
void message_send_add_to_queue(const char *uri, ContentType type);
void message_send_set_volume(int volume);
void message_send_set_output_exclusive(const char *output_id);
void message_send_toggle_output(const char *output_id);
void message_send_set_output_volume(const char *output_id, int volume);
void message_send_get_favorites(void);
