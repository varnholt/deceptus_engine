# Splash animation

The `SplashAnimation` engine player is `src/framework/image/splash_animation.*`. It renders configurable
sprite layers and has no knowledge of the studio, PSD layer names, colors or typography.
`game/splashscreen.*` plays the animation once at startup and logs asset failures.
The game passes `data/config/splash.json` after applying its window/display settings.
Sound cues play through the sound-effects mixer with the master and sound-effects volume
settings. The player stops any active splash sounds when the animation exits.

## Change the design

- Edit `data/config/splash.json` for the canvas, duration, layers, timing and sound cues.
- Edit `SPLASH BACKGROUND`, `SPLASH LOGO` and `SPLASH LETTERING` in `data/game/splash.psd`.
  Keep the artwork in named raster layers in an 8-bit RGB PSD with raw or RLE layer compression.
  The hidden `ORIGINAL ARTWORK` group retains the original editable layers.
- Put replacement Ogg clips in `data/sounds` and reference them in the sound cues.

## Configuration

`canvas` is a positive `[width, height]` reference size; `duration` is positive seconds.
`clear_color` optionally specifies three integer RGB channels (default black). The canvas
fits the actual target with letterboxing and is recalculated after resizing.

The optional `sounds` array schedules clips by `sample` filename relative to `data/sounds`
and `start` time in seconds. Start defaults to zero and must be within the animation. Each cue
plays on the first frame at or after its start time. Clips can overlap, and the same sample can
be scheduled more than once.

Set `psd` to the PSD asset path and use `psd_layer` to select a named raster layer. The
player uses the PSD layer's top-left position by default and multiplies its opacity by the
configured opacity. Layers are drawn in array order.
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
a zero duration produces an instantaneous transition. Scale and spread default to `[1, 1]`.

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
Invalid configuration or missing artwork throws from the player. The startup wrapper
logs the problem and allows startup to continue. The sound mixer logs missing clips while
the animation continues. The player itself is replayable and
`drawFrame(target, seconds)` supports deterministic frame sampling, preserving the
caller's view. Native playback uses the window's VSync/frame limit; browser playback
yields to the browser between frames.
