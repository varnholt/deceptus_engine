# WASM music reverb / "hall effect" — investigation status

**Date:** 2026-07-16 (root cause isolated), 2026-07-18 (**FIXED + confirmed clean in browser**)
**Status:** **RESOLVED.** Two coupled bugs, both fixed; music now plays clean (no reverb, no
glitches) on the WASM build.

## TL;DR — two bugs, two fixes

1. **Reverb** (music has a phasey "hall" wash): VRSFML built miniaudio on the **deprecated
   ScriptProcessorNode** (main-thread; broken under `-pthread`). Fix: define
   `MA_ENABLE_AUDIO_WORKLETS` → modern **AudioWorklet** backend (dedicated audio thread).
2. **Missing music** (exposed once #1 was fixed — SFX fine, music silent): VRSFML's `sf::Music`
   **decodes OGG and reads the file on the audio thread**. On AudioWorklet that thread is a WASM
   Worker where **filesystem syscalls are proxied and can't be awaited**, so file-backed streaming
   yields silence. SFX are pre-decoded (pure memcpy) so they survive. Fix (app-side): on WASM read
   the whole track into memory on the main thread and use `MusicReader::openFromMemory`, so the
   worklet decodes without touching the filesystem. (`openFromMemory` *references* the buffer, so
   deceptus keeps the compressed bytes alive per slot: `MusicPlayer::_music_data`.)

## Resolution (2026-07-18)

**Root cause:** VRSFML compiles miniaudio **without `MA_ENABLE_AUDIO_WORKLETS`**, so on Emscripten
miniaudio silently falls back to the **deprecated `ScriptProcessorNode`** backend. That backend runs
its audio callback on the **main browser thread**; under `-pthread` it is not serviced reliably, so
output buffers get repeated/overlapped — a comb-filter/reverb that is strongest on sustained tonal
content (music) and nearly inaudible on short transient SFX. This matches every observation,
including the standalone-`main()` repro. (Proof the bad path was active: the built `deceptus.js`
contained `createScriptProcessor`.)

**Fix:** define `MA_ENABLE_AUDIO_WORKLETS` for the miniaudio TU so it uses the modern **AudioWorklet**
backend (dedicated real-time audio thread). All required emscripten link flags
(`-sAUDIO_WORKLET=1 -sWASM_WORKERS=1 -sASYNCIFY=1 -pthread`) were already present in the deceptus link
line. The device struct in `miniaudio.h` is gated by `MA_SUPPORT_WEBAUDIO` (identical in both paths),
so there is no ABI/ODR change.

- **Deceptus (durable):** `CMakeLists.txt`, Emscripten branch:
  `target_compile_definitions(sfml-audio PRIVATE MA_ENABLE_AUDIO_WORKLETS)` — survives clean
  re-fetches, no edits to fetched source.
- **Verified at binary level:** after rebuild, `deceptus.js` has **zero** `createScriptProcessor`
  and now contains `createWasmAudioWorkletProcessor` + `emscriptenGetAudioObject` and registers the
  `miniaudio` worklet processor.

**Upstream VRSFML PR (two parts, both required)** — draft in the scratchpad
`vrsfml-audio-worklets.md`:
1. `src/SFML/Audio/CMakeLists.txt`: `if(SFML_OS_EMSCRIPTEN) target_compile_definitions(sfml-audio
   PRIVATE MA_ENABLE_AUDIO_WORKLETS) endif()`
2. `cmake/Config.cmake` `SFML_EMSCRIPTEN_TARGET_LINK_OPTIONS`: add `-sAUDIO_WORKLET=1 -sWASM_WORKERS=1`
   (VRSFML's own examples don't link these today, so even upstream examples run on ScriptProcessorNode).

**Final check for the user:** load http://localhost:9080/deceptus.html in a fresh browser window and
listen — the music reverb should be gone.

---

## Original investigation (2026-07-16)

**Status:** ROOT CAUSE ISOLATED (not yet fixed) — it is a **VRSFML / miniaudio audio-output bug on
the Emscripten build**, NOT anything in the Deceptus code.

## Symptom
On the WASM/VRSFML build, **music** has a constant reverb / phasey "hall" wash on top. Short **SFX
sound crisp** (the artifact is likely present on all output but only audible on sustained audio).
The desktop build is fine.

## Definitive conclusion
A **completely standalone `main()`** — audio context + one `sf::Sound.play()` of an ogg, and
**nothing else** (no `Game`, no `Preloader`, no `game.loop()`, no WebGL, MusicPlayer never
constructed) — **still reverbs.** That is the minimal repro. There is nothing in our code to fix.

## Everything ruled out (each tested in isolation, cache-confirmed via console build markers)
- **Browser cache** — confirmed fresh builds load (console `MUSIC-BUILD-MARKER rev-XX`; serve.py
  sends `Cache-Control: no-store`).
- **Source files** — the raw `.ogg` plays perfectly clean in a plain browser `<audio>` element.
- **MusicPlayer** — disabled via `queueTrack` no-op; then bypassed entirely (standalone `main`).
- **Streaming vs buffered** — `sf::Music` (streamed) AND `sf::Sound` (buffered) both reverb.
- **Decoder** — OGG (libvorbis) AND WAV (miniaudio-native, same track) both reverb.
  (Note: `coin.wav` short SFX via `sf::Sound` is clean — consistent with "only audible when
  sustained".)
- **Spatialization** — `setSpatializationEnabled(false)` on a fully isolated bare `sf::Sound`: no
  change.
- **Channels / sample rate** — mono and stereo both reverb; `coin.wav` is also stereo 44.1 kHz.
- **Playback device count** — one shared device vs a second device: no change.
- **Listener movement** — bare test uses its own device with a default (unmoving) listener.
- **Game activity (GPU/workers/CPU load)** — standalone `main` runs none of it and still reverbs.
- **Memory / threads** — `INITIAL_MEMORY` 64MB→512MB: no change.
- **Start-up timing** — delaying `play()` 5 s after load: no change.

## Minimal repro (for a VRSFML upstream report — vittorioromeo/VRSFML)
An Emscripten `main()` that, after `sf::AudioContext::create()`, loads a music-length ogg into an
`sf::SoundBuffer`, plays it via one `sf::Sound` (`setSpatializationEnabled(false)`,
`setLooping(true)`), and keeps the runtime alive with `emscripten_set_main_loop([]{}, 0, 1)` — the
sustained playback has a constant reverb/comb-filter wash. Build flags (see repo `CMakeLists.txt`
EMSCRIPTEN block): `-pthread -sAUDIO_WORKLET=1 -sWASM_WORKERS=1 -sASYNCIFY=1 -sALLOW_MEMORY_GROWTH=1`
(+ VRSFML forces `-pthread`/`--shared-memory`). emcc image: `emscripten/emsdk` 6.0.1.

## Next steps
1. File the minimal repro with **VRSFML upstream**; likely a miniaudio AudioWorklet / resampler /
   buffer interaction on Emscripten with pthreads. Ask whether a different device config, buffer
   size, or a non-AUDIO_WORKLET path avoids it.
2. Meanwhile the game's WASM music is affected but SFX are fine; no local code change helps.

## Build / test loop (reference)
- Rebuild: `docker run --rm -v <winpath>:/workspace -w /workspace emscripten/emsdk bash -c "cd
  build_wasm && make -j$(nproc)"` — **run in background** (a foreground run hits a 2-min cap and is
  killed mid-link, silently leaving a stale binary — this bit us repeatedly).
- Serve: `python lab/wasm_browser_test/serve.py` (:9080, now sends `no-store`). Full clean build
  needs serve.py stopped first (it chdir's into build_wasm and locks it).
- Verify a build is live: grep the `rev-XX` marker in the served/on-disk `deceptus.wasm`, and check
  it in the browser console. Test in a **fresh Incognito window**.
- Headless Selenium could NOT capture the game's audio (silent in automation) — parked.

## Kept (not reverted — legit, unrelated to reverb)
- `CMakeLists.txt`: `-sINITIAL_MEMORY` 64MB → 512MB.
- `lab/wasm_browser_test/serve.py`: `Cache-Control: no-store` (prevents stale-build confusion).
