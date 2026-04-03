# eye position detector

detects player eye positions in sprite sheets with automatic interpolation for missing frames.

## features

- **automatic eye detection** using color matching
- **smart interpolation** for frames where eyes can't be detected (e.g., blink animations)
- **spatial fallback** uses adjacent frames in sprite sheet when interpolation fails
- **debug visualization** with color-coded overlays:
  - green: eye successfully detected
  - red: eye interpolated/inferred
  - alternating light/dark shades for frame boundaries

## usage

```bash
# analyze sprite sheet and print results
uv run detect_eye_positions.py

# generate debug image with overlays
uv run detect_eye_positions.py --mark output.png

# write animations.json with eye positions data
uv run detect_eye_positions.py --output animations_with_eyes.json

# combine both outputs
uv run detect_eye_positions.py --mark debug.png --output animations_with_eyes.json
```

## output formats

### json output (`--output`)
adds `eye_positions_x` and `eye_positions_y` arrays to each animation:
```json
{
   "player_idle_l": {
      "frame_offset": [0, 0],
      "frame_size": [72, 48],
      "sprite_count": 4,
      "eye_positions_x": [24, 24, 24, 24],
      "eye_positions_y": [30, 31, 30, 29]
   }
}
```

### console output
detailed per-frame analysis with interpolation markers

### debug image (`--mark`)
sprite sheet with colored overlays and eye position markers

## algorithm

1. **pass 1**: scan all animations and detect eyes using color matching
2. **pass 2**: interpolate missing positions using:
   - linear interpolation between detected frames
   - previous/next frame relative position
   - spatial fallback to left neighbor in sprite sheet

## output

- **console**: detailed eye positions per animation (absolute and relative coordinates)
- **json file** (`--output`): original animations.json with `eye_positions_x` and `eye_positions_y` arrays added
- **debug image** (`--mark`): sprite sheet with colored overlays and eye position markers
