#include <pebble.h>
#include "list_window.h"
#include "player.h"
#include "../message_keys.h"
#include "../messaging.h"

static char *s_titles[MAX_QUEUE_ITEMS];
static char *s_artists[MAX_QUEUE_ITEMS];
static int s_item_ids[MAX_QUEUE_ITEMS];
static int s_queue_count = 0;
static int s_selected_index = 0;

static void cleanup_queue(void) {
  for (int i = 0; i < MAX_QUEUE_ITEMS; i++) {
    if (s_titles[i])  { free(s_titles[i]);  s_titles[i]  = NULL; }
    if (s_artists[i]) { free(s_artists[i]); s_artists[i] = NULL; }
    s_item_ids[i] = 0;
  }
  s_queue_count = 0;
  s_selected_index = 0;
}

static void queue_data_handler(int count, char *titles[], char *artists[],
                                int item_ids[], int selected_index);

// ── ListWindowConfig callbacks ───────────────────────────────────────────────

static uint16_t queue_get_num_rows(void *ctx) {
  (void)ctx;
  return s_queue_count;
}

static void queue_draw_row(GContext *ctx, const Layer *cell_layer,
                            MenuIndex *cell_index, void *_ctx) {
  (void)_ctx;
  const char *title  = s_titles[cell_index->row]  ? s_titles[cell_index->row]  : "Unknown";
  const char *artist = s_artists[cell_index->row] ? s_artists[cell_index->row] : "";
  menu_cell_basic_draw(ctx, cell_layer, title, artist, NULL);
}

static void queue_on_select(MenuIndex *cell_index, void *_ctx) {
  (void)_ctx;
  if (cell_index->row < s_queue_count && s_item_ids[cell_index->row] > 0) {
    message_send_play_queue_item(s_item_ids[cell_index->row]);
    player_set_launch_state_playing();
    player_window_push();
  }
}

static void queue_on_select_long(MenuIndex *cell_index, void *_ctx) {
  (void)cell_index; (void)_ctx;
  player_window_push();
}

static void queue_on_appear(void *_ctx) {
  (void)_ctx;
  message_set_queue_callback(queue_data_handler);
  message_send_command(CMD_GET_QUEUE);
}

static void queue_on_unload(void *_ctx) {
  (void)_ctx;
  message_set_queue_callback(NULL);
  cleanup_queue();
}

// ── Data handler ─────────────────────────────────────────────────────────────

static void queue_data_handler(int count, char *titles[], char *artists[],
                                int item_ids[], int selected_index) {
  cleanup_queue();
  s_queue_count    = (count > MAX_QUEUE_ITEMS) ? MAX_QUEUE_ITEMS : count;
  s_selected_index = (selected_index >= 0 && selected_index < s_queue_count)
                     ? selected_index : 0;

  for (int i = 0; i < s_queue_count; i++) {
    if (titles[i]) {
      s_titles[i] = malloc(strlen(titles[i]) + 1);
      if (s_titles[i]) strcpy(s_titles[i], titles[i]);
    }
    if (artists[i]) {
      s_artists[i] = malloc(strlen(artists[i]) + 1);
      if (s_artists[i]) strcpy(s_artists[i], artists[i]);
    }
    s_item_ids[i] = item_ids[i];
  }

  list_window_reload();
  list_window_set_selected(MenuIndex(0, s_selected_index), MenuRowAlignCenter,
                            false);
}

// ── Public API ───────────────────────────────────────────────────────────────

void queue_window_push(void) {
  list_window_push(&(ListWindowConfig){
    .get_num_rows     = queue_get_num_rows,
    .draw_row         = queue_draw_row,
    .on_select        = queue_on_select,
    .on_select_long   = queue_on_select_long,
    .on_appear        = queue_on_appear,
    .on_unload        = queue_on_unload,
  });
}

