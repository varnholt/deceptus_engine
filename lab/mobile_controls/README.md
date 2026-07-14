# Mobile touch-control shell (WASM)

An alternative index page for the WASM build that plays on a phone: it goes
fullscreen, fills the whole screen width (16:9, letterboxed on taller screens),
and overlays very transparent touch buttons — a D-pad (▲▼◀▶) and an A/B/X/Y
action cluster — that drive the game via synthetic keyboard events.

Self-contained: this folder does **not** modify the game source or the
`build_wasm/` output. `serve.py` serves the emscripten artifacts read-only from
`build_wasm/` and injects `mobile.html` at `/`.

## Run

```
uv run python serve.py      # or: python serve.py
```

Then open `http://localhost:9090/` — or, from a phone on the same network,
`http://<your-pc-ip>:9090/`. Tap **TAP TO PLAY** to go fullscreen.

Requires the WASM build to exist (`build_wasm/deceptus.{js,wasm,data}`).

## Button mapping (mirrors `data/config/controls.json`)

| On-screen | Key         | Game action |
|-----------|-------------|-------------|
| ▲ ▼ ◀ ▶   | Arrow keys  | move / look |
| A         | Space       | jump        |
| B         | Enter       | action      |
| X         | Left Ctrl   | slot 1      |
| Y         | Left Alt    | slot 2      |

Edit the `KEYS` map in `mobile.html` to remap.
