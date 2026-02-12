# OwnTone Remote for Pebble

A modern, extensible remote control for OwnTone music server on Pebble smartwatches.

## Features (Phase 1)

### Player Control
- View current track information (title, artist, album)
- Play/Pause toggle
- Skip to next/previous track
- Volume control (±5% increments)
- Action bar with standard Pebble music controls

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
- Single press: Enable output exclusively + start playback
- Long press: Toggle output on/off (no playback change)
- Per-output volume control (±5% increments)
- Pause playback from volume screen

## Requirements

- Pebble Time (Basalt) or compatible watch
- OwnTone server running at `owntone.local:3689`
- Pebble SDK 3.x for building

## Compatibility

- Supported platforms: Basalt (Pebble Time) and Aplite (Pebble Classic). On Aplite the voice/search UI is hidden because the device lacks a microphone; all other features (player, random, outputs, favorites) remain available.

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

### Main Menu
1. **Player** - Current track + playback controls
2. **Search** - Voice search by content type
3. **Random** - Random content by type
4. **Outputs** - Manage audio outputs

### Player Window
- **UP button**: Next track (long press: volume up)
- **SELECT button**: Play/Pause
- **DOWN button**: Previous track (long press: volume down)
- Icons update based on playback state

### Search/Random Flow
1. Select content type (Playlist, Artist, Album)
2. Speak your search query (Search only)
3. View results
4. Select an item to add to queue

### Outputs
1. View list of outputs (shows ON/OFF status and volume)
2. Single press: Exclusive mode + play
3. Long press: Toggle output
4. Volume screen: UP/DOWN to adjust, SELECT to pause, BACK to return

## Phase 2 Roadmap

- **Favorites Management**: Configure favorite playlists/artists/albums via web interface
- **Output Defaults**: Set default volumes for each output
- **Track Search**: Add individual tracks without clearing queue
- **Queue Viewer**: See upcoming tracks (next 10)
- **Smart Playlists**: Quick access to OwnTone smart playlists

## Technical Details

### Default Behaviors
- **Add to Queue**: Clears queue, starts playback, enables shuffle (except albums)
- **Exclusive Output**: Disables all others, enables selected, starts playback
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
