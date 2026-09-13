# Rope render check

Drives the desktop build around every rope in the catacombs and grabs a frame at each one, so a
change to the rope sprite sheet can be checked against the whole level rather than against one hand
picked spot.

```
uv run --with pywin32 --with pillow python drive_rope_check.py build_rel
uv run --with pywin32 --with pillow python drive_rope_check.py build_rel --only 893 1556
uv run --with pillow python montage.py 5
```

The rope positions come out of `catacombs.tmx` at run time, so the run covers whatever is in the
level. Nothing is hard coded. `--only` re-shoots individual ropes into the same `out/` directory,
and `montage.py` rebuilds the contact sheet from whatever is there, so a couple of bad frames cost
a minute rather than another full sweep.

`smoke_console.py` is the same harness cut down to a single teleport, for checking the input path
before committing to a full run.

Output lands in `out/`: one full frame per rope, the contact sheet, and a strip of consecutive
frames showing the lamp flicker.

## Why every step is verified instead of timed

Each of these failed silently and produced a run full of plausible looking screenshots of the wrong
thing.

- **`PostMessage` does not drive this build at all.** Posted keys do not even move the main menu
  selection, so input has to be `keybd_event`, which means the game has to be in the foreground.
- **`tpp` needs the space after the comma.** Its handler splits on whitespace and wants three
  tokens, so `tpp 208,118` is dropped without a word. It takes tile coordinates, not pixels.
- **The console has to be confirmed open before anything is typed.** If it is not, the letters go
  to the global hotkeys instead, where the two p's of `tpp` toggle the pause menu. A run that got
  this wrong toggled the pause menu sixty times and screenshotted the same frozen room thirty times.
- **The level is paused while the console is open**, so a frame grabbed then is frozen.
- **The camera pans to a teleport rather than snapping to it.** Grabbing the first clean frame
  photographs the room the player just left. The harness teleports twice: the first move lets the
  camera travel while the console is driven for the second, which re-seats the player with the
  camera already there.
- **The player falls the moment the console closes.** Verifying the close first costs about 0.7s,
  and he drops some 200px in that time, which drags the rope out of frame. So the frames are burst
  captured the instant the closing F12 goes out, and the close is verified afterwards.
- **The polyline does not always start at `0,0`.** Object 893 starts at `0,15.04`. Matching the
  first point as a literal read that rope as zero length and aimed the camera at the wrong end.

## The log is the oracle, not the screen

`pwatch 200` makes the game log the player tile to stdout, and it only logs while the level
updates. That one signal covers both things the harness needs to know: whether the console is open
(the log stalls) and whether a teleport landed (the tile it reports).

Two pixel probes were tried first and both report nonsense:

- Counting the console's green help headers. The catacombs have green lit rooms of their own, so a
  clean frame there counts as high as an open console, and the help panel filters down to a single
  topic as soon as a command is typed, so the green disappears with the console still wide open.
  It survives only as a second opinion next to the paused level, never on its own.
- Template matching the lamp sprite against the frame. The game tints sprites even with the
  lighting pass bypassed, so an exact match finds nothing, including in frames where the lamp is
  plainly visible to the eye.

Picking the first frame after the console closes is done by differencing against a reference
grabbed while it was still open, with the bar taken from the burst rather than fixed: how much the
frame changes when the overlay goes depends on how bright the room behind it is, and in the darkest
rooms a fixed bar is never met.

`lighting disable` is sent before the sweep. Several ropes hang in rooms so dim they read as solid
black, which proves nothing either way about the sprites.

## Framing

`montage.py` lays out whole frames at half size. The capture is 1280x720 for a 640x360 game, so
half is the native resolution and the pixel art lands unscaled. Cropping in on the middle of the
window loses the ropes whose room clamps the camera, because near a room edge the player is not in
the middle of the window at all.

## What it leaves behind

The run backs up and restores `savestate.json` and `game.json` in `%APPDATA%\deceptus\settings`,
and forces windowed mode for the duration because `PrintWindow` only ever returns black frames
while the game is fullscreen. The game rewrites `data/locale/en.json` on startup, so expect that
file to come back dirty.
