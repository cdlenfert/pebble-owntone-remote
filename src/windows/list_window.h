#pragma once
#include <pebble.h>

// Generic single-section MenuLayer window.
//
// Callers provide a config struct with callback pointers and a context pointer.
// list_window_push() allocates a fresh Window each time (config is copied
// internally so a stack-allocated config is fine).  The Window is destroyed
// when it is popped from the stack.
//
// Memory pattern: MenuLayer is created in window_appear and destroyed in
// window_disappear, so child windows (e.g. player) always get a clean heap.

typedef struct {
  // Required: row count and row rendering.
  uint16_t (*get_num_rows)(void *ctx);
  void     (*draw_row)(GContext *gctx, const Layer *cell_layer,
                       MenuIndex *cell_index, void *context);

  // Selection callbacks — set to NULL to disable.
  void (*on_select)(MenuIndex *cell_index, void *ctx);
  void (*on_select_long)(MenuIndex *cell_index, void *ctx);
  void (*on_selection_changed)(MenuIndex new_idx, MenuIndex old_idx, void *ctx);

  // Lifecycle callbacks — set to NULL if not needed.
  void (*on_appear)(void *ctx);   // good place to (re-)request data from phone
  void (*on_unload)(void *ctx);   // good place to unregister messaging callbacks

  void *context;
} ListWindowConfig;

// Push a new list window.  The config struct is copied; caller may use
// a compound-literal or stack-allocated variable.
void list_window_push(const ListWindowConfig *config);

// Reload the MenuLayer of the top-most list window.
// Must only be called when the top window IS a list window.
void list_window_reload(void);

// Scroll the top-most list window to a specific row.
void list_window_set_selected(MenuIndex index, MenuRowAlign align,
                              bool animated);
