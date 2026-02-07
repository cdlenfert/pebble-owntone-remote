# Artwork Feature Investigation

**Status**: Abandoned (2026-02-07)  
**Reason**: Technical limitations of PebbleKit JS environment

## What We Tried to Build

Display album artwork on the Pebble watch when long-pressing the play/pause button in the player window.

## Technical Challenges Discovered

### 1. **Image Format Incompatibility**
- **Problem**: OwnTone serves album artwork as JPEG (industry standard for music)
- **Pebble Requirement**: Only supports PNG format via `gbitmap_create_from_png_data()`
- **Why Pebble doesn't support JPEG**:
  - Memory constraints (24-64KB RAM on watch)
  - JPEG decoder is large and complex (DCT operations)
  - PNG decoder is simpler and smaller
  - Basalt only displays 64 colors anyway (JPEG's millions of colors are wasted)
  - Pebble was designed for UI graphics, not photo viewing

### 2. **PebbleKit JS Limitations**
- **Discovery**: PebbleKit JS is a very minimal JavaScript environment
- **Missing APIs**:
  - No `Blob` constructor
  - No `URL.createObjectURL()`
  - No `FileReader`
  - No `Image` element
  - No `Canvas` API
  - No `document.createElement()`
- **Impact**: Cannot perform client-side JPEG→PNG conversion on the phone

### 3. **Solution Attempted: Conversion Proxy**
We created a Node.js proxy server that:
- Sits between Pebble app and OwnTone
- Fetches JPEG from OwnTone
- Converts JPEG→PNG using `sharp` library
- Returns optimized PNG (120×120, 64 colors, palette mode)

**Why this wasn't pursued**:
- Requires additional infrastructure (Node.js server)
- Must run on same machine as OwnTone
- Adds deployment complexity
- Network reliability concerns
- Not worth the effort for a secondary feature

## What We Built (that worked)

### Messaging Infrastructure
- **AppMessage protocol**: Extended to 148 message keys
- **Chunking protocol**: Successfully transmitted large binary data
  - Metadata message (size, dimensions)
  - 100-byte chunks with delays (100ms between chunks, 200ms initial delay)
  - All chunks verified arriving and acknowledged
- **Track ID propagation**: Full chain from OwnTone API → JavaScript → C watch app

### Artwork Window (UI)
- ScrollLayer with artwork display area
- BitmapLayer (120×120 pixels, centered)
- Track/artist/album text below artwork
- Long-press SELECT handler on player window
- Multi-line debug status display (invaluable for debugging without watch logs)

### Debugging Techniques
- **On-screen status display**: Showed hex dumps of received data
- **Format detection**: Verified PNG vs JPEG signatures in real-time
- **Chunk assembly verification**: Proved data integrity through entire pipeline
- **Phone log analysis**: Tracked JavaScript execution and XHR requests

## Code Artifacts Created

### Successfully Working
1. **Message protocol** (keys 145-148):
   - `ARTWORK_CHUNK_INDEX`
   - `ARTWORK_CHUNK_TOTAL`
   - `ARTWORK_CHUNK_DATA`
   - `ARTWORK_SIZE`

2. **Chunking implementation**:
   - JavaScript: `sendArtworkChunks()` function
   - C: `artwork_window_handle_chunk()` with buffer assembly

3. **Artwork proxy server** (`artwork-proxy/`):
   - Node.js HTTP server
   - Sharp library for JPEG→PNG conversion
   - Palette optimization (64 colors for Pebble)
   - Ready to deploy if infrastructure allows

### Key Files Modified
- `src/js/pebble-js-app.js`: Artwork fetching and chunking
- `src/messaging.c`: Message routing for artwork
- `src/windows/artwork.c`: Artwork display window (complete)
- `src/windows/artwork.h`: Window interface
- `src/windows/player.c`: Long-press handler
- `appinfo.json`: Message key definitions

## Lessons Learned

### 1. **PebbleKit JS is not a browser**
- Always check API availability before implementing
- ES5 only (no modern JavaScript features)
- Very limited standard library
- No DOM, no browser APIs

### 2. **Image processing requires infrastructure**
- Client-side conversion not feasible
- Server-side conversion is the only option
- Consider format requirements early in design

### 3. **Debugging without logs is possible**
- On-screen status displays are powerful
- Hex dumps reveal format issues
- Phone logs provide JavaScript execution visibility

### 4. **Pebble's design constraints make sense**
- 64-color displays don't need complex image formats
- Memory limitations force simple decoders
- UI graphics (PNG) vs photos (JPEG) use case mismatch

## Alternative Approaches (if revisited)

### Option 1: OwnTone Plugin/Extension
Modify OwnTone to serve PNG instead of JPEG for album art.
- **Pros**: Clean solution, no proxy needed
- **Cons**: Requires OwnTone modification, may not be accepted upstream

### Option 2: Pre-converted Artwork Cache
Background process that pre-converts all album art to PNG.
- **Pros**: Fast serving, no runtime conversion
- **Cons**: Storage overhead, syncing complexity

### Option 3: Dedicated Proxy Service
Our implemented solution (artwork-proxy).
- **Pros**: Works today, transparent to OwnTone
- **Cons**: Extra infrastructure, deployment complexity

### Option 4: Accept Limitation
Display track info without artwork.
- **Pros**: Simple, reliable, no dependencies
- **Cons**: Less visual appeal

## Recommendation

**Don't implement artwork feature.**

Reasons:
1. PebbleKit JS cannot perform conversion
2. Proxy adds too much complexity for a watch app
3. OwnTone modification is out of scope
4. Track info display is already valuable without artwork
5. 64-color Pebble display won't show artwork well anyway

The player window already provides:
- Track name
- Artist
- Album
- Playback controls
- Volume control

This is sufficient for a music remote control app.

## Reusable Components

If artwork is ever revisited, these components are ready:

### 1. Binary Data Chunking Protocol
The chunking implementation works perfectly and could be used for other binary data transfers (playlists with images, lyrics, visualizations).

### 2. Artwork Proxy Server
The proxy is complete and functional. Just needs deployment to OwnTone server and firewall configuration.

### 3. On-Screen Debugging Pattern
The multi-line status display with hex dumps is a valuable debugging technique for any Pebble development without log access.

### 4. Track ID Infrastructure
Full propagation chain is working, could be used for other per-track features (ratings, lyrics, etc).

## Conclusion

We successfully:
- ✅ Implemented binary data chunking protocol
- ✅ Built artwork display window
- ✅ Created JPEG→PNG conversion proxy
- ✅ Verified data integrity through entire pipeline
- ✅ Discovered PebbleKit JS limitations

We learned that artwork display is technically feasible but requires infrastructure (proxy server) that adds too much complexity for the value provided.

The investigation was valuable for understanding Pebble's constraints and capabilities, even though the feature won't be implemented.
