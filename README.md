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

### v1.5 (2026-03-17)
- **Player layout redesign**: Dynamic text measurement using `text_layer_get_content_size()` for accurate sizing
- Text containers now sized exactly to rendered content height — no extra padding inside boxes
- Divider lines between track/artist/album 
- Entire text block vertically centered on screen
- Overflow handling: album truncates first, then artist, then track
- Splash screen minimum display time halved (2000ms → 1000ms)

### v1.4 and earlier
- Initial release with player controls, search, random, outputs management

## Phase 2 Roadmap

- **Favorites Management**: Configure favorite playlists/artists/albums via web interface
- **Output Defaults**: Set default volumes for each output
- **Track Search**: Add individual tracks without clearing queue
- **Queue Viewer**: See upcoming tracks (next 10)
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

## Credits

Built from scratch to replace buggy WIP implementation with:
- Cleaner architecture
- Predictable UI flow
- Extensible design
- Better UX (no blank screens!)
