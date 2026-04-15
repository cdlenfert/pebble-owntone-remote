#include <pebble.h>
#include "list_window.h"

// Magic value stored in ListWindowState to verify a Window is a list window
// before dereferencing its user-data pointer.
#define LIST_WINDOW_MAGIC 0x4C495354u  // "LIST"

typedef struct {
  uint32_t        magic;
  ListWindowConfig config;   // copy of caller's config
  MenuLayer      *menu_layer;
} ListWindowState;

// ── Private helpers ──────────────────────────────────────────────────────────

static ListWindowState *priv_get_state(Window *window);
static uint16_t priv_get_num_rows(MenuLayer *ml, uint16_t section_index, void *data);
static void priv_draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data);
static void priv_select(MenuLayer *ml, MenuIndex *cell_index, void *data);
static void priv_select_long(MenuLayer *ml, MenuIndex *cell_index, void *data);
static void priv_selection_changed(MenuLayer *ml, MenuIndex new_idx, MenuIndex old_idx, void *data);

static ListWindowState *priv_get_state(Window *window) {
  if (!window) return NULL;
  ListWindowState *s = (ListWindowState *)window_get_user_data(window);
  if (!s || s->magic != LIST_WINDOW_MAGIC) return NULL;
  return s;
}

static void priv_make_menu_layer(Window *window) {
  ListWindowState *s = priv_get_state(window);
  if (!s || s->menu_layer) return;

  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);

  s->menu_layer = menu_layer_create(bounds);
  menu_layer_set_callbacks(s->menu_layer, s, (MenuLayerCallbacks){
    .get_num_rows       = priv_get_num_rows,
    .draw_row           = priv_draw_row,
    .select_click       = priv_select,
    .select_long_click  = s->config.on_select_long      ? priv_select_long        : NULL,
    .selection_changed  = s->config.on_selection_changed ? priv_selection_changed : NULL,
  });
  menu_layer_set_click_config_onto_window(s->menu_layer, window);
  layer_add_child(root, menu_layer_get_layer(s->menu_layer));
}

// ── MenuLayer callbacks (forward-declared so priv_make_menu_layer can ref) ──

static uint16_t priv_get_num_rows(MenuLayer *ml, uint16_t section_index,
                                   void *data) {
  (void)ml; (void)section_index;
  ListWindowState *s = (ListWindowState *)data;
  return s->config.get_num_rows(s->config.context);
}

static void priv_draw_row(GContext *ctx, const Layer *cell_layer,
                           MenuIndex *cell_index, void *data) {
  ListWindowState *s = (ListWindowState *)data;
  s->config.draw_row(ctx, cell_layer, cell_index, s->config.context);
}

static void priv_select(MenuLayer *ml, MenuIndex *cell_index, void *data) {
  (void)ml;
  ListWindowState *s = (ListWindowState *)data;
  if (s->config.on_select) s->config.on_select(cell_index, s->config.context);
}

static void priv_select_long(MenuLayer *ml, MenuIndex *cell_index, void *data) {
  (void)ml;
  ListWindowState *s = (ListWindowState *)data;
  if (s->config.on_select_long)
    s->config.on_select_long(cell_index, s->config.context);
}

static void priv_selection_changed(MenuLayer *ml, MenuIndex new_idx,
                                    MenuIndex old_idx, void *data) {
  (void)ml;
  ListWindowState *s = (ListWindowState *)data;
  if (s->config.on_selection_changed)
    s->config.on_selection_changed(new_idx, old_idx, s->config.context);
}

// ── Window lifecycle ─────────────────────────────────────────────────────────

static void priv_window_load(Window *window) {
  priv_make_menu_layer(window);
}

static void priv_window_appear(Window *window) {
  // Recreate MenuLayer if it was freed while a child window was on top.
  priv_make_menu_layer(window);

  ListWindowState *s = priv_get_state(window);
  if (s && s->config.on_appear)
    s->config.on_appear(s->config.context);
}

static void priv_window_disappear(Window *window) {
  // Free MenuLayer when hidden so child windows get more heap for bitmaps.
  // priv_window_appear recreates it when this window returns to the top.
  ListWindowState *s = priv_get_state(window);
  if (!s) return;
  if (s->menu_layer) {
    menu_layer_destroy(s->menu_layer);
    s->menu_layer = NULL;
  }
}

static void priv_window_unload(Window *window) {
  ListWindowState *s = priv_get_state(window);
  if (s) {
    if (s->menu_layer) {
      menu_layer_destroy(s->menu_layer);
      s->menu_layer = NULL;
    }
    if (s->config.on_unload)
      s->config.on_unload(s->config.context);
    free(s);
  }
  window_destroy(window);
}

// ── Public API ───────────────────────────────────────────────────────────────

void list_window_push(const ListWindowConfig *config) {
  ListWindowState *state = (ListWindowState *)malloc(sizeof(ListWindowState));
  if (!state) return;

  state->magic      = LIST_WINDOW_MAGIC;
  state->config     = *config;   // shallow copy of config struct
  state->menu_layer = NULL;

  Window *window = window_create();
  window_set_user_data(window, state);
  window_set_window_handlers(window, (WindowHandlers){
    .load       = priv_window_load,
    .unload     = priv_window_unload,
    .appear     = priv_window_appear,
    .disappear  = priv_window_disappear,
  });
  window_stack_push(window, true);
}

void list_window_reload(void) {
  Window *top = window_stack_get_top_window();
  if (!top) return;
  ListWindowState *s = priv_get_state(top);
  if (s && s->menu_layer)
    menu_layer_reload_data(s->menu_layer);
}

void list_window_set_selected(MenuIndex index, MenuRowAlign align,
                               bool animated) {
  Window *top = window_stack_get_top_window();
  if (!top) return;
  ListWindowState *s = priv_get_state(top);
  if (s && s->menu_layer)
    menu_layer_set_selected_index(s->menu_layer, index, align, animated);
}
