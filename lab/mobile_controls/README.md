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

Then open `http://localhost:9090/` on the same machine. Tap **TAP TO PLAY**.

Requires the WASM build to exist (`build_wasm/deceptus.{js,wasm,data}`).

## Playing on a phone (HTTPS is required)

The WASM build is compiled with pthreads (`-pthread`, `WASM_WORKERS`,
`AUDIO_WORKLET`), so it needs **`SharedArrayBuffer`**. Browsers only expose
`SharedArrayBuffer` when the page is *both* cross-origin isolated (the COOP/COEP
headers `serve.py` sends) **and** in a **secure context** — i.e. `https://` or
`http://localhost`.

That is why the plain `http://<your-pc-ip>:9090/` URL works on the laptop (via
`localhost`) but **fails on a phone**: a bare LAN IP over plain HTTP is not a
secure context, so `SharedArrayBuffer` is unavailable and the game refuses to
start. The page now shows a "CAN'T RUN HERE" screen explaining this instead of
failing silently.

Pick one:

**A. Self-signed HTTPS (no extra tools):**
```
uv run --extra https python serve.py --https
```
On the phone open `https://<your-pc-ip>:9090/` and accept the certificate
warning. (A self-signed cert `cert.pem`/`key.pem` is generated next to
`serve.py` on first run; both are git-ignored.)

**B. A real-HTTPS tunnel (most reliable — no cert warning):** keep the plain
HTTP server running and put a tunnel in front of it:
```
cloudflared tunnel --url http://localhost:9090
# or:  ngrok http 9090
```
Open the `https://…` URL the tunnel prints on the phone.

## Button mapping (mirrors `data/config/controls.json`)

| On-screen | Key         | Game action |
|-----------|-------------|-------------|
| ▲ ▼ ◀ ▶   | Arrow keys  | move / look |
| A         | Space       | jump        |
| B         | Enter       | action      |
| X         | Left Ctrl   | slot 1      |
| Y         | Left Alt    | slot 2      |

Edit the `KEYS` map in `mobile.html` to remap.
