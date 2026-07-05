# OwnTone Remote for Pebble

A modern, extensible remote control for OwnTone music server on Pebble smartwatches.

## Features (Phase 1)

### Player Control
- View current track information (title, artist, album)
- Play/Pause toggle
- Skip to next/previous track
- Volume control (±5% increments)
- Action bar with standard Pebble music controls
- Quick navigation with long-press shortcuts
- Configurable player window auto-close (battery optimization)
- Configurable app auto-close after returning to main menu

### Search
- Voice search for playlists, artists, and albums
- View search results
- Add selected items to queue (clears queue, starts playback, shuffles by default)

### Random Selection
- Get random playlists, artists, or albums
- View results and select items
- Same queue behavior as search

### Audio Output Management
- View all available outputs
- Single press on **active** output: Open volume controls directly (no state change)
- Single press on **inactive** output: Enable exclusively + open volume controls
- Long press: Toggle output on/off (no playback change)
- Per-output volume control (±5% increments)
- Pause playback from volume screen

## Requirements

- Pebble Time (Basalt) or compatible watch
- OwnTone server running at `owntone.local:3689`
- Pebble SDK 3.x for building

## Compatibility

- **Basalt (Pebble Time)**: Full feature support with 64 colors
- **Aplite (Pebble Classic)**: Full functionality with memory-optimized UI
  - Voice/search hidden (no microphone)
  - Black & white splash logo (96x96 PNG8)
  - Memory-efficient icon loading with automatic retry
  - All core features available: player, random, outputs, favorites

## Architecture

### Modular Design
```
src/
├── main.c                  # App entry point
├── message_keys.h          # Message protocol definitions
├── messaging.c/h           # AppMessage handling
└── windows/
    ├── main_menu.c/h       # Main navigation
    ├── player.c/h          # Player controls
    ├── search.c/h          # Voice search
    ├── random.c/h          # Random selection
    ├── results.c           # Search/random results
    └── outputs.c/h         # Output management
```

### Message Protocol
Clean command-based system with typed messages:
- Commands: GET_PLAYER_STATE, PLAY_PAUSE, SEARCH, etc.
- Callbacks: Player state, search results, outputs list
- Extensible for future features

### JavaScript Layer
```
src/js/
└── app.js                  # OwnTone API integration
```

## Building

```bash
cd owntone-remote-new
pebble build
pebble install --phone <phone-ip>
```

## Usage
Quick Navigation
**Long-press SELECT button (500ms) for instant navigation:**
- From **any menu** (Main Menu, Favorites, Search, Random, Results): Jump to **Player**
- From **Player**: Jump to **Outputs** list
- From **Output Volume window**: Jump to **Player**
- **Outputs list** preserves existing behavior: Long-press toggles output on/off

This provides fast access to the player from anywhere in the app.

###  (long press: jump to Outputs)
### Main Menu
1. **Player** - Current track + playback controls
2. **Search** - Voice search by content type
3. **Random** - Random content by type
4. **Outputs** - Manage audio outputs

### Player Wind (long press SELECT: jump to Player)ow
- **UP button**: Next track (long press: volume up)
- **SELECT button**: Play/Pause
- **DOWN button**: Previous track (long press: volume down)
- Icons update based on playback state

### Search/Random Flow
1. Select content type (Playlist, Artist, Album) (long press: jump to Player)
2. Speak your search query (Search only)
3. View results
4. Select an item to add to queue

### Outputs
1. View list of outputs (shows ON/OFF status and volume)
2. Single press on **active** output: Open volume controls directly
3. Single press on **inactive** output: Enable exclusively + open volume controls
4. Long press: Toggle output on/off
5. Volume screen: UP/DOWN to adjust, SELECT to pause, BACK to return

## Changelog

### v1.14 (2026-04-30)
- **Player:** Fix missing music info text in the player window; track/artist/album now display reliably and a placeholder is shown when metadata is unavailable.

### v1.15 (2026-07-05)
- **Platform:** Add support for Pebble 2 Duo (`flint`) and Pebble Time 2 (`emery`). Voice search (dictation) now functions on these devices.
- **Haptics:** Fix missing vibration on Flint when long-pressing volume controls by using `vibes_short_pulse()` for consistent feedback.
- **Misc:** Continued improvements to search query sanitization and cross-platform UI memory handling.


### v1.13 (2026-04-15)
- Player: volume level displayed as a number in the center action bar slot when volume mode is active and UP or DOWN is pressed; updates on each press and disappears when volume mode times out (both Aplite and Basalt)
- Queue: fix current track appearing as the last item with no upcoming tracks on Aplite; now correctly shows up to 4 previous and 5 upcoming items on all platforms
- Memory: major internal refactor to reduce peak heap usage on all platforms — list-based windows (queue, results, outputs, random) now share a single reusable window layer instead of each owning a persistent MenuLayer, freeing significant RAM when navigating between screens; standalone splash window removed from Basalt (replaced by a brief logo overlay in the player screen)

### v1.12 (2026-04-11)
- Search: strip punctuation and normalize whitespace before querying the server, improving match quality
- Search: skip the API call and return empty results immediately if the cleaned query is empty

### v1.11 (2026-04-09)
- **Fix all player action bar icons missing on Aplite**: Aplite's PNG decoder only handles 1-bit images; the 7 action bar icons were stored as 8-bit RGBA PNGs, causing `gbitmap_create_with_resource` to return NULL for every icon at runtime. Fixed by changing all action bar icon resources from `"type": "png"` to `"type": "bitmap"` in `appinfo.json` — the SDK now pre-converts them to Pebble binary format at build time, bypassing the runtime decoder entirely.
- **Fix action bar icon colors on Aplite**: All 7 icons had incorrect black/white polarity for `ActionBarLayer` rendering (which uses `GCompOpAssignInverted`). Corrected all icons to the expected format.
- **Fix MenuLayer heap leaks across all list windows on Aplite**: Added `window_disappear` (free `MenuLayer`) and `window_appear` (recreate `MenuLayer`) to `main_menu`, `favorites`, `queue`, `random`, and `results` windows. On Aplite, Pebble's lifecycle order (B.`window_load` → A.`window_disappear` → B.`window_appear`) meant parent MenuLayers stayed allocated while child windows loaded, exhausting the 24KB heap. The disappear/appear pattern ensures the heap is free before child window allocations.
- **Fix Basalt startup flash**: On Basalt, the main menu no longer briefly appears before the player window. `splash.c` now calls `main_menu_push_silent()` (no animation) followed immediately by `player_window_push()`.
- Tune Aplite `AppMessage` buffers to 1536/256 bytes, recovering heap vs. the previous 2048/512 allocation.

### v1.10 (2026-04-04)
- **App auto-close**: New configurable setting — after the player window closes, the app exits to the watchface if no buttons are pressed within the selected time (default: 5 minutes). Resets on any button press in the main menu or outputs windows. Configurable via the Battery Optimization section of the settings page.
- Add mise dev environment: `mise.toml`, `scripts/`, `.env.example`, `AGENTS.md`
- Modernize `wscript`: replace deprecated `pbl_program()` with `pbl_build(bin_type='app')`

### v1.9 (2026-03-21)
- Fix radio streams in player window: show track title at the top and station name at the bottom

### v1.8 (2026-03-20)
- Fix output volume window icons missing on Aplite: free outputs MenuLayer before volume window loads to stay within 24KB heap
- Free player window bitmaps when hidden on all platforms, reducing peak heap usage

### v1.7 (2026-03-17)
- Fix player layout not reflowing correctly when track changes mid-playback (switch to `graphics_text_layout_get_content_size()` to avoid stale layer render cache)

### v1.6 (2026-03-17)
- Fix descender clipping on track/artist text (G, g, p, y no longer cut off) using `layer_set_clips(false)`

### v1.5 (2026-03-17)
- **Player layout redesign**: Dynamic text measurement using `text_layer_get_content_size()` for accurate sizing
- Text containers now sized exactly to rendered content height — no extra padding inside boxes
- Divider lines between track/artist/album 
- Entire text block vertically centered on screen
- Overflow handling: album truncates first, then artist, then track
- Splash screen minimum display time halved (2000ms → 1000ms)

### v1.4 (2026-03-10)
- Raise max search/random results on Aplite from 4 to 8 (matching other platforms)
- Remove long-press shortcut to player window on Aplite (icons displayed incorrectly when accessed this way)

### v1.3 (2026-02-24)
- Outputs: single-pressing an active speaker now opens volume controls directly, without changing its state
- Outputs: single-pressing an inactive speaker still enables it exclusively and opens volume controls

### v1.2 (2026-02-23)
- Fix crash on Aplite when scrolling the Random results list

### v1.1 (2026-02-23)
- RAM optimizations for Aplite (Classic/Pebble Steel) to reduce memory usage
- Skip splash screen on Aplite to free additional heap
- Reduce max result and queue list sizes on Aplite to stay within memory limits

## Phase 2 Roadmap

- **Favorites Management**: Configure favorite playlists/artists/albums via web interface
- **Output Defaults**: Set default volumes for each output
- **Track Search**: Add individual tracks without clearing queue
- **Smart Playlists**: Quick access to OwnTone smart playlists

## Technical Details

### Default Behaviors
- **Add to Queue**: Clears queue, starts playback, enables shuffle (except albums)
- **Exclusive Output**: Disables all others, enables selected, starts playback (only triggered when tapping an inactive output)
- **Toggle Output**: Changes state without affecting playback
- **Volume Steps**: 5% increments (0-100%)

### OwnTone API Integration
- Uses JSON API endpoints
- Player: `/api/player`
- Queue: `/api/queue/items/add`
- Search: `/api/search`
- Outputs: `/api/outputs`

## License

Personal project - see LICENSE file