#include <pebble.h>
#include "list_window.h"
#include "results.h"
#include "player.h"
#include "../message_keys.h"
#include "../messaging.h"

static const char *s_content_types[] = { "Playlist", "Artist", "Album" };

// Stores the type selected by the user so the results handler can use it
// when the async response arrives from the phone.
static ContentType s_pending_type = CONTENT_TYPE_PLAYLIST;

// Custom light vibration pattern (20ms pulse)
static void light_vibe(void) {
  uint32_t segments[] = { 20 };
  VibePattern pat = { .durations = segments, .num_segments = 1 };
  vibes_enqueue_custom_pattern(pat);
}

static void random_results_handler(int count, char *titles[], char *uris[]) {
  message_set_results_callback(NULL);
  results_window_push(count, titles, uris, s_pending_type);
}

// ── ListWindowConfig callbacks ───────────────────────────────────────────────

static uint16_t random_get_num_rows(void *ctx) {
  (void)ctx;
  return 3;
}

static void random_draw_row(GContext *ctx, const Layer *cell_layer,
                             MenuIndex *cell_index, void *_ctx) {
  (void)_ctx;
  menu_cell_basic_draw(ctx, cell_layer, s_content_types[cell_index->row],
                       NULL, NULL);
}

static void random_on_select(MenuIndex *cell_index, void *_ctx) {
  (void)_ctx;
  s_pending_type = (ContentType)cell_index->row;
  message_set_results_callback(random_results_handler);
  message_send_random(s_pending_type);
  light_vibe();
}

#ifndef PBL_PLATFORM_APLITE
static void random_on_select_long(MenuIndex *cell_index, void *_ctx) {
  (void)cell_index; (void)_ctx;
  player_window_push();
}
#endif

static void random_on_unload(void *_ctx) {
  (void)_ctx;
  // Discard a pending results callback if the window closes before data arrives.
  message_set_results_callback(NULL);
}

// ── Public API ───────────────────────────────────────────────────────────────

void random_window_push(void) {
  list_window_push(&(ListWindowConfig){
    .get_num_rows   = random_get_num_rows,
    .draw_row       = random_draw_row,
    .on_select      = random_on_select,
#ifndef PBL_PLATFORM_APLITE
    .on_select_long = random_on_select_long,
#endif
    .on_unload      = random_on_unload,
  });
}

