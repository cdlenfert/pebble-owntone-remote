Store assets collected for OwnTone Remote

Found image files in the repository (paths shown relative to repo root):

- resources/images/menu_icon.png
- resources/images/icon_play.png
- resources/images/icon_pause.png
- resources/images/icon_next.png
- resources/images/icon_prev.png
- resources/images/icon_volume_up.png
- resources/images/icon_volume_down.png
- resources/images/icon_ellipsis.png
- resources/images/owntone_logo-bw.png
- resources/images/owntone_logo-bw-25.png
- resources/images/owntone_logo.png
- resources/images/official-icon-pack/* (additional small icons)

What I created here:
- `short_description.txt` — one-line store summary
- `long_description.md` — longer store description (markdown)
- `release_notes.txt` — release notes for this version

How to copy image files into this folder for upload (run from repo root):

```bash
mkdir -p store-assets/images
cp resources/images/menu_icon.png store-assets/images/
cp resources/images/owntone_logo.png store-assets/images/
cp resources/images/owntone_logo-bw.png store-assets/images/
cp resources/images/icon_play.png store-assets/images/
cp resources/images/icon_pause.png store-assets/images/
cp resources/images/icon_next.png store-assets/images/
cp resources/images/icon_prev.png store-assets/images/
cp resources/images/icon_volume_up.png store-assets/images/
cp resources/images/icon_volume_down.png store-assets/images/
cp resources/images/icon_ellipsis.png store-assets/images/
```

Screenshot guidance:
- Provide at least one screenshot showing the app UI running on a Pebble device (or emulator).
- For Aplite devices use the Aplite resolution; for Basalt (Pebble Time) use its native resolution. If you can't run the emulator, capture screenshots on-device using the phone app or your phone's screenshot tool.

Recommended store uploads:
- One square high-resolution icon (e.g., 512×512 PNG) for store listing.
- 2–4 screenshots (device-native resolutions), landscape or portrait as appropriate.
- Short description (<=140 chars) and a longer description (detailed usage and features).

Next steps I can do for you:
- Copy the selected images into `store-assets/images` for you (I can run the `cp` commands).
- Draft `short_description.txt` and `long_description.md` content.
- Produce example screenshots from the emulator if you want (requires Pebble SDK/emulator available).
