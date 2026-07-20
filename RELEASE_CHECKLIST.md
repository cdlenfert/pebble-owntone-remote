Release publish checklist
=========================

This checklist describes the steps to perform before publishing a new release to Pebble / Rebble app stores. Save this file in the repo so automated agents can find and follow it.

1. Bump version
   - Update `appinfo.json`:
     - increment `versionCode` (integer)
     - update `versionLabel` (string), e.g. "1.18"

2. Update release notes
   - Prepend a new entry to `store-assets/release_notes.txt` with the new version and date.

3. Commit and push
   - Commit the changes and push to `origin/main`.
   - Create an annotated tag matching the version, e.g. `v1.18` and push it.

4. Build artifacts
   - Run the build to produce the PBW(s):

```bash
mise build
```

 - Verify `build/pebble-owntone-remote.pbw` exists and matches the expected version.

5. Smoke test on device/emulator
   - Ensure the phone's Pebble developer connection is enabled.
   - Install the PBW to a test phone or emulator.
     - Use `./scripts/install-phone.sh <phone-ip>` (replace `<phone-ip>`)
   - Verify basic flows: startup, config page pre-population, vibration settings, and player controls.

6. Create GitHub release (optional)
   - On GitHub create a release using the tag (e.g. `v1.18`).
   - Attach `build/pebble-owntone-remote.pbw` to the release assets.
   - Paste the release notes from `store-assets/release_notes.txt` into the release body.

7. Publish to Pebble / Rebble app stores
   - Follow the store-specific upload steps (login, upload PBW, add screenshots, set release notes).

8. Announce and docs
   - Update README/store pages if needed.
   - Announce release on project channels.

Notes for automation
 - The build artifact path is `build/pebble-owntone-remote.pbw`.
 - The `owntone_vibration` preference is stored in webview `localStorage` and sent via AppMessage key `VIBRATION` (192) at app startup.
 - Ensure the device has developer connection enabled when automating install steps.
