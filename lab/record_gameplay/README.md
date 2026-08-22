# record_gameplay

Launches the game, waits for the level to finish loading, captures the window
via ffmpeg gdigrab at 60 fps, and writes two files:

- `output/master.mkv` — the lossless RGB capture, exactly the pixels the engine
  drew. This is the master; keep it and derive everything else from it.
- `output/gameplay.gif` — a straight GIF of the master, no dither.

Web-friendly media for the README and for itch.io is produced from the master by
[`lab/media_assets`](../media_assets/README.md).

## Prerequisites

- [uv](https://docs.astral.sh/uv/) installed
- [ffmpeg](https://ffmpeg.org/download.html) on `PATH` (for capture and GIF encoding)
- A built `deceptus.exe`
- Windows (uses `win32gui` and ffmpeg `gdigrab`)

`gdigrab` records a screen region rather than a window, so the game has to be the
window on top. The script makes it topmost for the duration of the capture and
puts it back afterwards, and it verifies that nothing covers the capture region
before recording a single frame — a covered window aborts the run with the name
of whatever was in the way instead of quietly recording your desktop.

It captures the client area only, queried through `GetClientRect` and
`ClientToScreen`, so the title bar and border never make it into the master and
nothing downstream has to know their size. The process is made DPI aware first,
because a DPI unaware process is handed scaled window coordinates while `gdigrab`
works in physical pixels — on a 125% display that mismatch records an entirely
different part of the screen.

## Setup

Edit `config.json` to point at your local paths:

```json
{
  "game_executable": "D:/deceptus/build/deceptus.exe",
  "working_directory": "D:/deceptus/deceptus_engine",
  "capture_duration_seconds": 8,
  "gif_fps": 25
}
```

`game_executable` — path to the built executable.  
`working_directory` — the repo root; the game reads `data/` relative to this.  
`capture_duration_seconds` — (optional) how many seconds to record; defaults to 8.  
`gif_fps` — (optional) output GIF framerate; defaults to 25. Must divide evenly into 100 for stable timing (valid: 50, 25, 20, 10). 60fps is not a valid GIF rate.  
`teleport_x` / `teleport_y` — (optional) tile coordinates to teleport to before recording starts. Omit both keys to skip teleportation.

Available teleport commands (opened via F12 in-game):

| Command | Description |
|---|---|
| `tpp <x> <y>` | Teleport to tile position |
| `tpr <room>` | Teleport to room by name |
| `tpc <n>` | Teleport to checkpoint index |
| `tps` | Teleport to level start position |

## Run

From anywhere:

```powershell
uv run --project lab/record_gameplay pytest ../../tests/desktop/test_record_gameplay.py -s
```

The `-s` flag lets the game's stdout pass through so you can see loading
progress. The files land in `lab/record_gameplay/output/`. The lossless master is
large — expect a few hundred MB for 8 seconds — so the whole `output/` directory
is git-ignored.

## Tuning

| Constant | Default | Description |
|---|---|---|
| `CAPTURE_FPS` | 60 | Frames captured per second (source quality) |
| `CAPTURE_DURATION_SECONDS` | 8 | How long to record (also settable via `config.json`) |
| `GIF_FPS` | 25 | Output GIF framerate — must divide evenly into 100 (also settable via `config.json`) |
| `LOAD_TIMEOUT_SECONDS` | 30 | How long to wait for the level to load |
