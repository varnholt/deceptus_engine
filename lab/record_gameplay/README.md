# record_gameplay

Launches the game, waits for the level to finish loading, captures the window
via ffmpeg gdigrab at 60 fps, and writes `output/gameplay.gif`.

## Prerequisites

- [uv](https://docs.astral.sh/uv/) installed
- [ffmpeg](https://ffmpeg.org/download.html) on `PATH` (for capture and GIF encoding)
- A built `deceptus.exe`
- Windows (uses `win32gui` and ffmpeg `gdigrab`)
- The game window must remain visible and unobscured during capture

## Setup

Edit `config.json` to point at your local paths:

```json
{
  "game_executable": "D:/deceptus/build/deceptus.exe",
  "working_directory": "D:/deceptus/deceptus_engine",
  "capture_duration_seconds": 8
}
```

`game_executable` — path to the built executable.  
`working_directory` — the repo root; the game reads `data/` relative to this.  
`capture_duration_seconds` — (optional) how many seconds to record; defaults to 8.  
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
uv run --project lab/record_gameplay pytest lab/record_gameplay -s
```

The `-s` flag lets the game's stdout pass through so you can see loading
progress. The GIF is written to `lab/record_gameplay/output/gameplay.gif`.

## Tuning

| Constant | Default | Description |
|---|---|---|
| `CAPTURE_FPS` | 60 | Frames captured per second |
| `CAPTURE_DURATION_SECONDS` | 8 | How long to record (also settable via `config.json`) |
| `LOAD_TIMEOUT_SECONDS` | 30 | How long to wait for the level to load |
