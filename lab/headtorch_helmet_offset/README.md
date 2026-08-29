# Head torch helmet offset

Tools for the head torch helmet: where it lands on the player for every animation cycle, and the
eye position data it is placed from.

## generate_eye_positions.py

Regenerates `data/sprites/eye_positions.json` from `data/sprites/player.png` and
`data/sprites/animations.json`. This replaces `lab/eye_position_detector` for that file.

```
uv run python generate_eye_positions.py            # dry run, reports detection rates
uv run python generate_eye_positions.py --write    # writes data/sprites/eye_positions.json
```

The eye is found structurally rather than by an exact colour: a dark pixel two pixels tall, next to
skin, with an identically coloured partner two or three pixels to the side. The sheet was authored in
blocks whose palettes drifted, so the eye reads `(51, 20, 35)` in the oldest rows, `(51, 23, 37)` in
the sword rows and `(54, 23, 50)` in the double jump sword row.

Cycles the game builds by reversing another one (`player_bend_up_*`, `player_dash_stop_*`) are not in
`animations.json`; they are derived from their source cycle, see `REVERSED_CYCLES`.

Cycles with no entry afterwards, and why that is correct:

| cycle | reason |
| --- | --- |
| `player_death*` | the head torch is switched off while the player is dead |
| `player_jump_{init,up,midair,down}_attack_sword_legs_*` | legs only art, the head comes from the auxiliary cycle |
| `player_jump_dust*`, `player_wallslide_dust`, `player_water_splash` | effects, never the player cycle |
| `player_bend_down_attack_sword_2_r`, `player_standing_attack_sword_3_r` | no art in that region of the sheet |

## render_helmet_offsets.py

Reproduces the engine's helmet placement offline and writes one contact sheet per cycle: the frame,
the eye position in cyan and the helmet in its 24x24 box.

```
uv run python render_helmet_offsets.py --generalized-origin --out sheets
uv run python render_helmet_offsets.py --out sheets_old   # without the origin correction
```

`--generalized-origin` applies the animation origin the engine now accounts for; leaving it off
reproduces the old behaviour, which only lined up for the 72x48 cycles.

`montage.py` stacks several of those sheets into one image for review.

Note this only covers the helmet. The beam is placed from the same eye position and needed the same
origin correction; the cycles that were affected are exactly those whose origin is not (36, 48):

```
uv run python - <<'EOF'
import json
anims = json.load(open("../../data/sprites/animations.json"))
for name, animation in sorted(anims.items()):
    if animation["origin"] != [36.0, 48.0] and name.endswith(("_l", "_r")):
        print(name, animation["origin"])
EOF
```

`review_strips.py` crops the captured in game strips down to the head by locating the lamp. The
level has its own yellow green markers and candle flames, so it takes the blob nearest the middle of
the crop - it still picks a candle when the player has walked out of frame.

## drive_helmet_check.py

Runs the desktop build with the head torch and the sword equipped, walks a few animation cycles and
saves an upscaled crop strip per cycle to `out/`.

```
uv run --with pywin32 --with pillow python drive_helmet_check.py build_rel
uv run --with pywin32 --with pillow python drive_helmet_check.py build_rel --only 01_run --full-frames
```

It sweeps every cycle reachable from flat ground in both directions, once with the sword and once
without. `--only <substring>` narrows it to one capture, `--full-frames` saves the whole window
rather than a crop - worth reaching for before believing something is missing, since the crop
happily cuts the beam off when the player runs out of it.

Notes, each of which cost a run:

- `PrintWindow` returns black frames while the game is fullscreen, so the run flips
  `%APPDATA%\deceptus\settings\game.json` to windowed and restores it afterwards.
- A fresh save state starts in the intro level even with `levelindex: 0`, so the catacombs are loaded
  from the console.
- The save state already fills both inventory slots. They have to be cleared before the head torch
  and the sword go in, otherwise the sword never reaches a slot and the attack button does nothing.
- Console commands are typed as real key events. If the console is not actually open the letters hit
  the global hotkeys - `m` in "item" starts the frame recorder and floods the repo root with bmp
  dumps, `s` in "sword" writes the whole render target set there. The run deletes both on the way
  out. `n` loads the next level and `r` resets, so a badly timed one can also derail the run.
- The camera does not centre the player at checkpoint 1, hence `CROP_CENTER_OFFSET_X/Y`.
