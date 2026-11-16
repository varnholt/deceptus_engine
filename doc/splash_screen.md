# Splash animation

The `SplashAnimation` engine player is `src/framework/image/splash_animation.*`. It renders configurable
sprite layers and has no knowledge of the studio, PSD layer names, colors or typography.
`game/splashscreen.*` only handles startup's play-once policy and logs asset failures.
The game passes `data/config/splash.json` after applying its window/display settings.

## Change the design

- Edit `data/config/splash.json` for the canvas, duration, layer order and animation.
- Edit `data/game/splash/source.json` for PSD layer selection, colors and grading.
- Run `uv run --with psd-tools --with numpy python tools/export_splash.py` to regenerate
  the PNGs after changing the PSD or color recipe. Runtime playback needs only the JSON
  animation and referenced PNGs; PSD decoding and color grading happen during export.

## Configuration

`canvas` is a positive `[width, height]` reference size; `duration` is positive seconds.
`clear_color` optionally specifies three integer RGB channels (default black). The canvas
fits the actual target with letterboxing and is recalculated after resizing.

Each entry in `layers` references a `texture` asset. Layers are drawn in array order.
`position`, `scale` and `spread` accept either a constant `[x, y]` pair or an object:

```json
{
  "from": [0.8, 0.8],
  "to": [1, 1],
  "start": 0,
  "duration": 1.5,
  "easing": "ease_out_quad"
}
```

Supported easing names are `linear`, `ease_out_quad` and `smoothstep`. Optional `power`
raises the easing result to a positive exponent. Curves clamp outside their interval;
a zero duration produces an instantaneous transition. Default position is `[0, 0]`;
default scale and spread are `[1, 1]`.

`origin` is an optional normalized anchor within each sprite region, default `[0, 0]`.
`opacity` is a constant from 0 to 1, default 1. Optional `fade_in` and `fade_out` objects
use the same start, duration, easing and power fields. Fades multiply the opacity;
fade-out applies its power after reversing the easing result.

By default a layer draws its complete texture. Optional `parts` list texture rectangles
as `[x, y, width, height]`, with an optional local `offset` pair for each. `spread`
animation changes these offsets without stretching the individual regions. This lets
this game's title animate its spacing while other designs can use arbitrary sprites.
Offsets are canvas units, independent of the sprite scale.

Textures are shared within an animation instance and released with that instance.
Invalid configuration or missing assets throw from the player; the startup wrapper
logs the problem and allows startup to continue. The player itself is replayable and
`drawFrame(target, seconds)` supports deterministic frame sampling, preserving the
caller's view. Native playback uses the window's VSync/frame limit; browser playback
yields to the browser between frames.

## Verification

Configure a desktop build with `-DDECEPTUS_BUILD_SPLASH_TESTS=ON`, build
`splash_animation_tests`, then run `ctest --test-dir <build> -R splash_animation --output-on-failure`.
These rendering checks use synthetic red/green artwork and exercise interpolation,
fades, sprite regions, multiple instances, aspect ratios, view restoration and invalid
configuration. They require a working graphics context.
