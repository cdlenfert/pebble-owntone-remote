## Dev Iteration Flow

After you've added a feature run `mise build` to verify it builds.

To install to a phone: `mise install` (uses `IP` from `.env`).

If you need runtime logs from the emulator: `mise emu` to install, then `pebble logs --emulator basalt` in a separate terminal.

## Debugging

- C: `APP_LOG(APP_LOG_LEVEL_DEBUG, "msg", args)`
- JS: `console.log("msg")`

## Pebble Memory Tips

- Avoid allocating large buffers on the stack; use heap with explicit free.
- Prefer drawing directly in an update proc over creating extra layer objects when a simple render path is enough.
- Destroy windows and layers when popped from the stack to free memory.
