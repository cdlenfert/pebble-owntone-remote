OwnTone Search — Pebble (Basalt)

Simple Pebble app to search an Owntone server via voice and add results to the queue.

Defaults:
- Server: http://owntone.local:3689
- Result limit: 8
- Platforms: Basalt only

How it works:
- Select content type on the watch (Playlist, Artist, Album, Track)
- Speak when prompted ("what playlist" etc.)
- App searches Owntone via the JSON API and returns up to 8 matches
- If one match — it will be added to the queue automatically
- If multiple matches — choose one on the watch

Notes:
- This scaffold keeps keys/UUID unique and avoids colliding with the existing HTTP-Push app.
- To build/install: use your usual Pebble SDK tooling from the repo root.
