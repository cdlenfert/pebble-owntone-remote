# Build Instructions

## Two Build Methods

### Option 1: Docker (Recommended - No Setup Required!)

**Easiest method** - Uses Docker to build without installing Pebble SDK locally.

```bash
cd owntone-remote-new
./docker-build.sh
./docker-install.sh <your-phone-ip>
```

**See [DOCKER.md](DOCKER.md) for complete Docker guide.**

---

### Option 2: Local Pebble SDK

Use if you already have Pebble SDK installed.

## Icon Resources (Important!)

Before building, you need to add icon files to `resources/images/`:

- `icon_play.png` (25x25 pixels, white on transparent)
- `icon_pause.png` (25x25 pixels, white on transparent)
- `icon_next.png` (25x25 pixels, white on transparent)
- `icon_prev.png` (25x25 pixels, white on transparent)

### Quick Solution:

You can temporarily copy icons from the old WIP project or use simple placeholder images. See `resources/images/README.md` for details.

Alternatively, you can find free icons at:
- https://www.flaticon.com/ (check license)
- https://icons8.com/ (check license)
- Or create simple geometric shapes

## Build Steps

```bash
# Navigate to project directory
cd owntone-remote-new

# Clean previous builds (optional)
pebble clean

# Build the project
pebble build

# Install to watch (phone must be on same network)
pebble install --phone <your-phone-ip>

# Or use Pebble app developer connection
pebble install --cloudpebble
```

## Development Tips

### Live Logs

```bash
pebble logs --phone <your-phone-ip>
```

### Emulator Testing

```bash
# Install to emulator
pebble install --emulator basalt

# View emulator logs
pebble logs --emulator basalt
```

### Common Issues

**"Resource not found" error:**
- Make sure all 4 icon PNG files exist in `resources/images/`
- Icons must be exactly 25x25 pixels
- Icons should be PNG format

**"Phone not found" error:**
- Ensure phone and computer are on same WiFi network
- Enable Developer Connection in Pebble app settings
- Check phone IP address matches

**JavaScript errors:**
- Check that OwnTone server is running at `owntone.local:3689`
- Verify network connectivity from phone to server
- Check `pebble logs` for JavaScript console output

## Next Steps

1. Build and install the app
2. Test each feature:
   - Player controls
   - Voice search
   - Random selection
   - Output management
3. Report any bugs or issues
4. Move to Phase 2 (Favorites + Configuration)

## File Structure

```
owntone-remote-new/
├── appinfo.json          # App metadata and message keys
├── wscript               # Build configuration
├── config.html           # Settings page (Phase 2)
├── README.md             # Project overview
├── BUILD.md              # This file
├── src/
│   ├── main.c            # App entry point
│   ├── message_keys.h    # Message protocol
│   ├── messaging.c/h     # AppMessage handling
│   ├── windows/          # UI windows
│   └── js/
│       └── app.js        # JavaScript companion
└── resources/
    └── images/           # Icon resources
```
