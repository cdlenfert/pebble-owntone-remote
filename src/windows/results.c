#include <pebble.h>
#include "list_window.h"
#include "player.h"
#include "../message_keys.h"
#include "../messaging.h"

static ContentType s_current_type;
static char *s_titles[MAX_RESULTS];
static char *s_uris[MAX_RESULTS];
static int s_result_count = 0;

// Use centralized vibration helper from messaging.c

static void cleanup_results(void) {
  for (int i = 0; i < MAX_RESULTS; i++) {
    if (s_titles[i]) { free(s_titles[i]); s_titles[i] = NULL; }
    if (s_uris[i])   { free(s_uris[i]);   s_uris[i]   = NULL; }
  }
  s_result_count = 0;
}

// ── ListWindowConfig callbacks ───────────────────────────────────────────────

static uint16_t results_get_num_rows(void *ctx) {
  (void)ctx;
  return s_result_count > 0 ? s_result_count : 1;
}

static void results_draw_row(GContext *ctx, const Layer *cell_layer,
                              MenuIndex *cell_index, void *_ctx) {
  (void)_ctx;
  if (s_result_count > 0 && cell_index->row < s_result_count) {
    menu_cell_basic_draw(ctx, cell_layer, s_titles[cell_index->row], NULL, NULL);
  } else {
    menu_cell_basic_draw(ctx, cell_layer, "No results", NULL, NULL);
  }
}

static void results_on_select(MenuIndex *cell_index, void *_ctx) {
  (void)_ctx;
  if (s_result_count > 0 && cell_index->row < s_result_count &&
      s_uris[cell_index->row]) {
    message_send_add_to_queue(s_uris[cell_index->row], s_current_type);
    message_vibrate_light();
    window_stack_pop(true);
  }
}

#ifndef PBL_PLATFORM_APLITE
static void results_on_select_long(MenuIndex *cell_index, void *_ctx) {
  (void)cell_index; (void)_ctx;
  player_window_push();
}
#endif

static void results_data_handler(int count, char *titles[], char *uris[]);

static void results_on_appear(void *_ctx) {
  (void)_ctx;
  message_set_results_callback(results_data_handler);
}

static void results_on_unload(void *_ctx) {
  (void)_ctx;
  message_set_results_callback(NULL);
  cleanup_results();
}

// Called when the phone sends updated results while the window is open.
static void results_data_handler(int count, char *titles[], char *uris[]) {
  cleanup_results();
  s_result_count = count < MAX_RESULTS ? count : MAX_RESULTS;
  for (int i = 0; i < s_result_count; i++) {
    if (titles[i]) {
      s_titles[i] = malloc(strlen(titles[i]) + 1);
      if (s_titles[i]) strcpy(s_titles[i], titles[i]);
    }
    if (uris[i]) {
      s_uris[i] = malloc(strlen(uris[i]) + 1);
      if (s_uris[i]) strcpy(s_uris[i], uris[i]);
    }
  }
  list_window_reload();
}

// ── Public API ───────────────────────────────────────────────────────────────

void results_window_push(int count, char *titles[], char *uris[],
                          ContentType type) {
  s_current_type = type;
  cleanup_results();
  s_result_count = count < MAX_RESULTS ? count : MAX_RESULTS;
  for (int i = 0; i < s_result_count; i++) {
    if (titles[i]) {
      s_titles[i] = malloc(strlen(titles[i]) + 1);
      if (s_titles[i]) strcpy(s_titles[i], titles[i]);
    }
    if (uris[i]) {
      s_uris[i] = malloc(strlen(uris[i]) + 1);
      if (s_uris[i]) strcpy(s_uris[i], uris[i]);
    }
  }

  list_window_push(&(ListWindowConfig){
    .get_num_rows      = results_get_num_rows,
    .draw_row          = results_draw_row,
    .on_select         = results_on_select,
#ifndef PBL_PLATFORM_APLITE
    .on_select_long    = results_on_select_long,
#endif
    .on_appear         = results_on_appear,
    .on_unload         = results_on_unload,
  });
}

