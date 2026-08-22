# media_assets

Derives every publishable media asset from one lossless gameplay master.

[`lab/record_gameplay`](../record_gameplay/README.md) records
`output/master.mkv`, a lossless RGB capture of the game window. This script turns
that master into the things the project actually publishes.

| Asset | What it is | Used for |
|---|---|---|
| `stills/still_<t>s.png` | full resolution PNG frames at chosen timestamps | picking a hero image, itch.io screenshots, bug reports |
| `screenshot.png` | the chosen hero still, full resolution | README hero image |
| `thumbnail.png` | 640x360, the engine's native view size | small previews |
| `gameplay.mp4` | H.264, full resolution | README `<video>`, websites, Discord |
| `gameplay.webm` | VP9, full resolution | pages that prefer WebM |
| `gameplay.gif` | animation under a size budget | README, anywhere that only takes images |
| `gameplay_itch.gif` | animation under 3 MB | itch.io, which accepts images only and caps uploads at 3 MB |

Only what `README.md` references needs to be committed. `--install` copies that
subset into `doc/screenshots/`; everything else stays in the git-ignored
`output/` directory.

## Prerequisites

- [uv](https://docs.astral.sh/uv/) installed
- [ffmpeg](https://ffmpeg.org/download.html) and `ffprobe` on `PATH`
- a master from `lab/record_gameplay`

## Run

From the repository root:

```powershell
uv run --project lab/media_assets lab/media_assets/make_media_assets.py
```

Or `lab\media_assets\make.bat`, which does the same from anywhere.

A typical full pass — record, look at what you caught, then cut the assets from
the best window:

```powershell
uv run --project lab/record_gameplay pytest tests/desktop/test_record_gameplay.py -s

ffmpeg -i lab/record_gameplay/output/master.mkv -vf "scale=426:-2,fps=2,tile=4x4" -frames:v 1 contact.png

uv run --project lab/media_assets lab/media_assets/make_media_assets.py ^
    --start 4 --duration 4 --stills 4,5,6,7 --hero-still 6 --install
```

The contact sheet in the middle is two frames per second of the master, which
makes it easy to pick `--start`, `--duration` and the hero timestamp.

## What the engine actually renders

`Game::initializeRenderTargets` sizes the render texture to
`size_ratio * view`, where `size_ratio` is the integer part of window / view. At
a 1280x720 window with the default 640x360 view that is a full **1280x720**
target. Tile art is drawn at 2x, so it holds no detail beyond 640x360 — but
`sf::Text` glyphs, the HUD filigree and every shader rasterise at the full
1280x720.

So downscaling to 640x360 is lossy in a way that matters: hairline serifs, 1
pixel ornaments and light gradients go. That is why every asset here is full
resolution by default, and why the GIF ladder only drops to 640 as a last
resort.

## How the GIF hits a size budget

- **`palettegen=stats_mode=diff`.** The palette is chosen from what changes
  between frames, so it is spent on the moving parts.
- **`paletteuse=diff_mode=rectangle`.** Each written frame is limited to the
  rectangle that actually changed.
- **`dither=none`.** Ordered dither hides banding but changes every pixel of
  every frame, which defeats both GIF compression and any later re-encode.
- **`--scale-flags area`.** When a downscale is unavoidable, averaging each 2x2
  block returns the identical pixel for 2x scaled tile art while antialiasing
  text and shader output. `neighbor` picks one pixel per block and aliases both.

If the budget is still missed, the ladder gives up palette size first, then
frame rate, and only then resolution. When nothing fits it says so and keeps the
smallest attempt rather than pretending it succeeded.

## Options

| Option | Default | Description |
|---|---|---|
| `--input` | `lab/record_gameplay/output/master.mkv` | the lossless master |
| `--output-directory` | `lab/media_assets/output` | where assets land |
| `--assets` | `stills,thumbnail,mp4,webm,gif,itch-gif` | subset to produce |
| `--stills` | `1,2,3,4` | timestamps in seconds for the PNG frames |
| `--hero-still` | first `--stills` entry | timestamp that becomes `screenshot.png` |
| `--fps` | `20` | animation frame rate; GIF needs 50, 25, 20, 10 or 5 |
| `--start` / `--duration` | none | trim the animations, in seconds |
| `--crop` | `none` | crop expression, for sources that carry window chrome |
| `--scale-flags` | `area` | scaler used when downscaling |
| `--dither` | `none` | `paletteuse` dither mode |
| `--gif-width` | source width | width the GIF ladder starts at |
| `--gif-megabytes` | `8.0` | budget for `gameplay.gif` |
| `--itch-megabytes` | `3.0` | budget for `gameplay_itch.gif` |
| `--video-crf` | `18` | MP4 and WebM quality, lower is better |
| `--install` | off | copy the README assets into `doc/screenshots/` |

A crop is skipped automatically when the source turns out to be smaller than the
crop rectangle, so running the script on an already processed file works.

## Measured on the current master

A 1280x720 clip of the player running and hopping across two rooms, so the
camera pans throughout:

| Asset | Size |
|---|---|
| `screenshot.png`, 1280x720 | 0.34 MB |
| `gameplay.mp4`, 3.5 s, 1280x720 | 0.70 MB |
| `gameplay.gif`, 3.5 s, 640x360, 20 fps, 256 colors | 6.06 MB |
| `gameplay.gif`, 5 s, 1280x720, 20 fps, 256 colors | 18.69 MB |
| `gameplay_itch.gif`, 3.5 s, 640x360, 10 fps, 128 colors | 2.69 MB |

Two things are worth knowing before picking numbers.

**Camera motion dominates.** The same five seconds costs 6.95 MB as a full
resolution GIF when the camera holds still and 18.69 MB when it pans, because
`diff_mode=rectangle` has nothing to reuse once every pixel moves. A panning clip
is where H.264 wins by a factor of about 18 at equal resolution.

**The master's encoding matters more than any GIF flag.** The same clip cut from
the old dithered capture needed ~31 MB for *four* seconds at this quality. A
GIF from the lossless master measures 39 dB PSNR against it, so the 256 colour
quantisation is visually transparent.

Note that GitHub's markdown sanitiser strips `<video>` tags, verified against
its rendering API, so an MP4 cannot be embedded in the README no matter how the
`src` is written. Images are the only medium that renders, which is why the
front page pairs a full resolution still with a 640x360 GIF.
