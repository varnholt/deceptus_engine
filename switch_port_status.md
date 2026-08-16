# Nintendo Switch homebrew port — status

Living status document. Mirrors the convention of `wasm_port_status.md`.

**Last updated:** 2026-08-15
**Branch:** `feat/switch-port` (pushed to origin, branched off `feat/harpoon` —
rebase onto `master` later, nothing here overlaps other work)

> ## State in one paragraph
>
> **The game is playable on the Switch.** `build_switch.bat` produces
> a 122.6 MB `deceptus.nro` with the full `data/` tree embedded as romfs. Driven in Ryujinx
> by `lab/switch_smoke/drive_ryujinx.py` it boots to the main menu, takes controller input
> through the menu and the file select screen, loads the catacombs save and plays: the
> player walks and jumps, the camera follows through room transitions, and the lighting,
> shadows, parallax background, tilemaps and HUD all render correctly at 60 fps. **No
> errors are logged at any point.**
>
> Four things got it there, each documented in its own section below: the Switch EGL driver
> silently ignores context sharing (fixed by giving the platform a single GL context), the
> shaders selected their legacy branch on a core profile (fixed by selecting on
> `__VERSION__` rather than `GL_ES`), VRSFML's libbacktrace stack traces hang the console
> (fixed by turning stack traces off for this target), and controller hotplug detection
> never ran on the VRSFML path at all (fixed by polling the device list from the main loop).
>
> **It runs on real hardware.** The first hardware run crashed during asset loading; the
> cause was applet mode, not the port. Relaunched in **title takeover mode** — hold R while
> starting a game from the HOME menu — the same binary works. See "The first hardware run"
> below; that distinction is the single most important thing to know about running this on a
> console.
>
> Remaining: **audio is silent** (miniaudio resolves to its null backend), and debug
> scaffolding is still in the tree, listed under "How to pick this up from scratch".

Read "How to pick this up from scratch" near the bottom first — it has the working-copy
locations, the build/run commands, and the iteration loop, which is easy to get wrong.

---

## Decision taken

Port **SDL3 to Switch**, then run **VRSFML** on top of it. Vanilla SFML 3 is a dead end
on this platform (see "The blocker" below). This reuses the existing dual-SFML
architecture: WASM already runs VRSFML, so Switch becomes a third branch of the same path
rather than a new rendering stack.

Rejected alternatives:

- *Vanilla SFML 3 + a Switch window backend* — blocked, SFML 3 needs a GL compatibility context.
- *Fork VRSFML with a direct libnx+EGL backend* — smaller surface, but a permanent second fork to maintain.

---

## Build environment

Everything runs in Docker via the official **`devkitpro/devkita64`** image (705 MB), mirroring
the `build_wasm.bat` → `emscripten/emsdk` pattern.

Do **not** install devkitPro natively on Windows: it plants a global MSYS2 tree at
`C:\devkitPro`, registers system-wide `DEVKITPRO` / `DEVKITA64` env vars, and its CMake
toolchain files assume POSIX paths.

```
build_switch.bat [source_dir] [build_dir]   -> docker/build_switch.sh
```

Defaults to building `lab/switch_smoke` into `build_switch/`.

### Toolchain probe results

| Item | Result |
|---|---|
| Compiler | devkitA64 **GCC 15.2.0** |
| C++23 | **Verified working** — `std::expected`, `std::print`, `std::ranges` all compile |
| Toolchain file | `$DEVKITPRO/cmake/Switch.cmake` |
| Platform module | `Platform/NintendoSwitch.cmake` sets `NINTENDO_SWITCH TRUE`, defines `__SWITCH__` |
| Packaging | `nx_create_nro(target)` + `nx_generate_nacp()` |
| pkg-config | `aarch64-none-elf-pkg-config` present |

### Portlibs already installed in the image

`switch-mesa 20.1.0`, `switch-libdrm_nouveau`, `switch-glad`, `switch-glm`,
`switch-sdl2 2.28.5`, `switch-box2d 2.4.1`, `switch-freetype`, `switch-libogg`,
`switch-libvorbis`, `switch-flac`, `switch-libpng`, `switch-zlib`.

Every SFML dependency is already packaged. **There is no `switch-sdl3`** — SDL2 only.
That absence is the reason task 2–5 exist.

---

## The blocker (why vanilla SFML 3 is out)

Switch Mesa exposes **core profile / GLES only**:

- `switch-glad` is generated `--api="gl=4.3" --profile="core"`.
- Libs present: `libEGL.a`, `libGLESv2.a`, `libGLESv1_CM.a`, `libglapi.a`, `libglfw3.a`.
  **There is no `libGL.a`.**
- `nm` across every Mesa lib finds **zero** fixed-function symbols — no `glVertexPointer`,
  `glMatrixMode`, `glEnableClientState`, `glTexCoordPointer`, `glBegin`.

Vanilla SFML 3.0.2 renders through the fixed-function pipeline — 27 call sites in
`RenderTarget.cpp` alone. It requires a compatibility context this stack does not provide.

VRSFML is modern-GL and needs no fixed function, hence the decision above.

---

## Useful facts found along the way

**EGL init matches SFML's existing `EglContext` almost exactly.** devkitPro's examples do:

```c
eglGetDisplay(EGL_DEFAULT_DISPLAY);
eglBindAPI(EGL_OPENGL_API);                 // note: OPENGL, not OPENGL_ES
eglCreateWindowSurface(display, config, nwindowGetDefault(), nullptr);
```

and SFML's `EglContext::createSurface(EGLNativeWindowType)` makes the identical
`eglCreateWindowSurface` call. SFML's DRM backend is *not* directly reusable — it needs
`gbm.h`, which Switch does not have — but it is the right structural template.

**SDL3 3.2.4 already ships homebrew console backends in-tree:** `vita`, `n3ds`, `psp`, `ps2`
for video, audio *and* joystick. `src/video/vita/` is the closest analog (EGL-based console)
and is the model for `src/video/switch/`. Video drivers register via the `bootstrap[]` array
in `src/video/SDL_video.c`.

**Audio:** SFML 3 uses miniaudio, which has no Switch backend. libnx offers `audout`/`audren`.
Doing it at the SDL3 layer (task 5) is what unblocks it.

**Local source trees already fetched** (no need to re-clone):
`build_rel/_deps/sdl3-src`, `build_rel/_deps/sfml-src`.

---

## SDL3 port plan (tasks 2–5)

### Target the right SDL3 — this one is easy to get wrong

The project's root `CMakeLists.txt` pins `release-3.2.4`, **but that pin only applies to the
desktop branch.** On the VRSFML path SDL3 is fetched internally by VRSFML via CPM (see the
comment at `CMakeLists.txt:895`), and VRSFML currently pins:

> **SDL 3.5.0**, git rev `e205361fb` — local checkout at `build_vrsfml/_deps/sdl-src`

Since Switch goes through VRSFML, **the backend must target 3.5.0, not 3.2.4.** Backend
directory layout is identical between the two, so notes taken against either tree transfer.

Neither version has any `NINTENDO_SWITCH` / `__SWITCH__` support.

### How SDL3 platform support is structured

devkitPro's `Platform/NintendoSwitch.cmake` already sets `NINTENDO_SWITCH TRUE`, which lines
up exactly with how SDL3 detects the 3DS — also a devkitPro platform, and the closest
precedent in the tree:

```cmake
# cmake/sdlplatform.cmake
elseif(NINTENDO_3DS)
  set(sdl_cmake_platform n3ds)     # uppercased into N3DS TRUE
```

So the changes are:

1. `cmake/sdlplatform.cmake` — add `elseif(NINTENDO_SWITCH)` → `set(sdl_cmake_platform switch)`
2. `CMakeLists.txt` — add an `elseif(SWITCH)` subsystem block, modelled on the `elseif(VITA)`
   block at ~line 2571 (sets `SDL_VIDEO_DRIVER_*`, `SDL_AUDIO_DRIVER_*`, `SDL_JOYSTICK_*`,
   `SDL_FILESYSTEM_*`, `SDL_THREAD_*`, `SDL_TIMER_*`), plus `__SWITCH__` compile definition
3. `include/SDL3/SDL_platform_defines.h` — add `SDL_PLATFORM_SWITCH` keyed off `__SWITCH__`
4. Backends under `src/video/switch/`, `src/audio/switch/`, `src/joystick/switch/`

**Simplification vs Vita/3DS:** libnx ships newlib with real pthreads, so SDL's generic
`src/thread/pthread` backend should work as-is rather than needing a custom one — unlike
Vita and 3DS, which both carry bespoke thread implementations. Same likely applies to
timers and filesystem.

### Reference backends to copy from

`src/video/vita/` is the closest analog (EGL-based console). SDL 3.5.0 also ships `n3ds`,
`psp`, `ps2` for video, audio *and* joystick. Video drivers register through the
`bootstrap[]` array in `src/video/SDL_video.c`.

### Working copy

Develop against a durable clone of SDL at `e205361fb` **outside** the repo, and commit the
result as `patches/switch-sdl3-backend.patch` — matching how the repo already carries
`patches/vrsfml-webgl2-uniforms.patch` and `patches/sfml-ostream-include.patch`.
Do not develop directly in `build_vrsfml/_deps/sdl-src`; it is build output and can be wiped.

> **Working copy lives at `D:/deceptus/sdl3_switch`, branch `switch-backend`,
> based on `e205361fb`.** It is a local clone (no network needed to recreate:
> `git clone --no-hardlinks build_vrsfml/_deps/sdl-src`). Build it with:
>
> ```
> docker run --rm -v "D:/deceptus/sdl3_switch:/sdl" -w /sdl devkitpro/devkita64 bash -c \
>   'cmake -S /sdl -B /sdl/build_switch -DCMAKE_TOOLCHAIN_FILE=$DEVKITPRO/cmake/Switch.cmake \
>    -DCMAKE_BUILD_TYPE=Release -DSDL_SHARED=OFF -DSDL_STATIC=ON && cmake --build /sdl/build_switch -- -j$(nproc)'
> ```

### Progress on task 2 — SDL3 configures for Switch

**`cmake` configure now completes cleanly** (`Revision: SDL-3.5.0-e205361fb`). Four edits,
all in the working copy:

1. `include/SDL3/SDL_platform_defines.h` — `SDL_PLATFORM_SWITCH` keyed off `__SWITCH__`
   (the devkitPro platform module already passes `-D__SWITCH__`)
2. `cmake/sdlplatform.cmake` — `elseif(NINTENDO_SWITCH)` → `set(sdl_cmake_platform switch)`,
   which uppercases into `SWITCH TRUE`
3. `CMakeLists.txt` — an `elseif(SWITCH)` platform block using `src/main/generic`,
   `CheckPTHREAD()`, POSIX fsops, and the unix time/timer backends
4. `CMakeLists.txt` + `cmake/sdlchecks.cmake` — two pthread fixes:
   - `SDL_PTHREADS_DEFAULT` was OFF because Switch is not `UNIX_OR_MAC_SYS`; now
     `if ((UNIX_OR_MAC_SYS OR SWITCH) AND NOT EMSCRIPTEN)`
   - `CheckPTHREAD()` fell through to the generic `else()` branch and tried to link
     `-lpthread`, which does not exist on libnx; added a `SWITCH` branch alongside
     `ANDROID` for "pthreads are baked into libc"

**The libnx pthread bet paid off:** `HAVE_PTHREADS`, `HAVE_PTHREADS_SEM` and `pthread.h`
all probe successfully, so SDL uses its stock `src/thread/pthread` backend with no
Switch-specific thread code. `pthread_setname_np` and `pthread_np.h` are absent, which SDL
already handles.

Video, audio and joystick currently fall through to SDL's automatic dummy backends — that
is deliberate, so the library builds and links before tasks 3–5 replace them.

### Compile fixes on top of configure

Everything that broke was in SDL's *generic* backends meeting newlib's limits. Each fix
follows a precedent already in the tree rather than inventing anything:

| File | Problem | Fix |
|---|---|---|
| `src/dynapi/SDL_dynapi.h` | `#error Please define your platform` | `SDL_DYNAMIC_API 0` for Switch, alongside the existing Vita and 3DS entries — devkitA64 has no dynamic linking |
| `src/thread/pthread/SDL_systhread.c` | libnx has no `pthread_getschedparam` / `pthread_setschedparam` / `sched_get_priority_min`/`max` | added `SDL_PLATFORM_SWITCH` to the existing `SDL_PLATFORM_RISCOS` guard that no-ops `SDL_SYS_SetThreadPriority` |
| `src/time/unix/SDL_systime.c` | newlib's `struct tm` has no `tm_gmtoff` | route Switch down the strictly POSIX.1-2008 branch, same as Solaris |
| `src/time/unix/SDL_systime.c` | newlib names the global `_timezone`, not `timezone` | `#ifdef SDL_PLATFORM_SWITCH` → `_timezone` |

**Result: `libSDL3.a` builds for AArch64** (~15.7 MB). The whole of "SDL3 builds for
Switch" is 57 added lines across 7 files, captured as
**`patches/switch-sdl3-backend.patch`** — apply it the same way the root `CMakeLists.txt`
already applies `vrsfml-webgl2-uniforms.patch`.

Build SDL's own `test/` targets are disabled (`-DSDL_TESTS=OFF -DSDL_EXAMPLES=OFF`); they
fail to link because they expect a real video driver and libnx entry point. The library
target is unaffected.

### Tasks 3–5 — the three real backends

All three compile into `libSDL3.a`. **None has been run**, on hardware or otherwise.

**Video — `src/video/switch/`.** Modelled on `src/video/vivante/`, which is the closest
analog in the tree: embedded EGL, no window manager, and it drives everything through
SDL's shared `SDL_EGL_*` helpers. Switch is simpler still, because libnx owns the one
window — `SWITCH_CreateWindow` takes `nwindowGetDefault()` and sizes it with
`nwindowSetDimensions()` rather than creating anything, and `SWITCH_DestroyWindow`
deliberately does not destroy it. `SWITCH_PumpEvents` turns `appletMainLoop()` going
false (user quitting from the home menu) into `SDL_SendQuit()`. Display modes advertise
both 1280x720 handheld and 1920x1080 docked.

Three findings worth keeping:

1. **SDL's EGL layer already supports desktop GL.** `SDL_egl.c` binds `EGL_OPENGL_API`
   with `EGL_OPENGL_BIT` whenever the profile is not ES, chosen at runtime from
   `gl_config.profile_mask`. So the same backend serves GL 4.3 core and GLES 3.2 — the
   port never has to pick one at build time.
2. **Static EGL is a solved problem in SDL — Vita already does it.** `SDL_egl.c` has five
   `SDL_VIDEO_DRIVER_VITA` guards that swap `dlopen` for direct symbol references; Switch
   joins four of them. It deliberately does **not** join the fifth (~line 1154), because
   that one gates desktop-GL support that Vita lacks and the Switch has.
3. **SDL's bundled Khronos `eglplatform.h` had no Switch branch** and failed with
   `#error "Platform not recognized"`. The added branch mirrors devkitPro's switch-mesa
   header exactly — `void *` display and window, `khronos_uint8_t *` pixmap — verified by
   preprocessing the system header rather than guessed, since SDL calls straight into
   that statically linked libEGL and the types must agree.

**Joystick — `src/joystick/switch/`.** libnx `pad` API, one controller,
`HidNpadStyleSet_NpadStandard` so a Joy-Con pair reports as a single gamepad. Sticks come
out of libnx already in signed 16-bit range, so only the Y axes get flipped (libnx points
up positive, SDL up negative). Buttons are exposed at their `HidNpadButton` bit positions.
**Note the A/B and X/Y swap in the gamepad mapping:** SDL's gamepad roles are positional,
and Nintendo's physical layout puts A where a standard pad puts B, so `.a` maps to
`HidNpadButton_B` and `.x` to `HidNpadButton_Y`. ZL/ZR are digital here, so they map as
buttons rather than axes.

**Audio — `src/audio/switch/`.** Built on libnx `audout` rather than `audren`: audout is
plain PCM out and needs no voice/mempool setup, which is all this game requires. Format is
not negotiated — audout only ever does 48 kHz stereo S16, so the backend reports the
hardware's terms via `SDL_UpdatedAudioDeviceFormat()` and lets SDL convert. Double
buffered, both address and size page-aligned to 0x1000 as audout demands, with
`audoutWaitPlayFinish()` pacing the audio thread. Recording is unimplemented (`audin`).

**Correction — this does *not* give SFML audio.** An earlier note here (and commit
`423ac4ea`) claimed it did. It doesn't: `src/SFML/Window/CMakeLists.txt` in VRSFML sets
**`SDL_AUDIO OFF`** and VRSFML brings its own miniaudio (`Audio/MiniaudioUnity.cpp`), so
SFML audio never travels through SDL's audio driver.

**Second correction, from actually testing it:** a follow-up note here suggested pointing
miniaudio at its SDL backend (`ma_backend_sdl`). **That backend does not exist** — this
miniaudio has zero references to `MA_SUPPORT_SDL` / `ma_backend_sdl`. Do not go looking
for it.

What the backend-selection block at `extlibs/headers/miniaudio/miniaudio.h:2877` actually
does on Switch: none of the platform arms match (`MA_WIN32`, `MA_UNIX`, `MA_ANDROID`,
`MA_APPLE`, `MA_EMSCRIPTEN` are all false), leaving only the two unconditional ones —
`MA_SUPPORT_CUSTOM` and `MA_SUPPORT_NULL`.

So the state of audio is:

- `libsfml-audio-s.a` **compiles clean** for Switch, and CMake even finds devkitPro's
  Vorbis and FLAC portlibs — decoding is fine.
- At runtime miniaudio will have **only the null backend**, so the game will be **silent**.
  Building is not the same as making sound.

The fix is a **custom miniaudio backend over libnx `audout`**, which is what
`MA_SUPPORT_CUSTOM` exists for and is available on every platform.

**Consequence for task 5:** the SDL audio backend is likely **unused** by the game, since
SFML audio bypasses SDL entirely. It stays in the patch because it makes SDL's own audio
subsystem work on Switch and costs nothing, but it should not be counted as progress
toward getting sound out of this game.

**Known limitation to revisit:** on Switch the real timezone lives behind libnx's time
service (`timeGetDeviceLocationName`, `timeToCalendarTimeWithMyRule`), so newlib's
`_timezone` will read 0 unless `TZ` is set. Wall-clock UTC offset will therefore be wrong
until a proper Switch time backend exists. It does not block anything the game needs.

---

## Task 6 — VRSFML on Switch

Working copy: **`D:/deceptus/vrsfml_switch`, branch `switch-backend`, based on `9c272d601`**
(cloned locally from `build_vrsfml/_deps/sfml-src`, no network needed).

**Good news on scope: VRSFML's window layer is entirely SDL-based.** `src/SFML/Window/`
is `SDLWindowImpl.cpp` / `SDLGlContext.cpp` / `SDLLayer.cpp`, with only Android, iOS,
Win32 and a `Stub` directory as per-OS extras. So there is no new renderer or window
backend to write — the Switch inherits everything through the SDL3 work in tasks 2–5.

**Injecting the patched SDL is easy.** `src/SFML/Window/CMakeLists.txt` checks
`if(EXISTS "${PROJECT_SOURCE_DIR}/../SDL/CMakeLists.txt")` and prefers that over its CPM
pin. Mounting the patched tree as a sibling `SDL/` directory is all it takes:

```
docker run --rm \
  -v "D:/deceptus/vrsfml_switch:/work/VRSFML" \
  -v "D:/deceptus/sdl3_switch:/work/SDL" \
  -w /work/VRSFML devkitpro/devkita64 bash -c \
  'cmake -S /work/VRSFML -B /work/VRSFML/build_switch \
     -DCMAKE_TOOLCHAIN_FILE=$DEVKITPRO/cmake/Switch.cmake -DCMAKE_BUILD_TYPE=Release \
     -DSFML_BUILD_AUDIO=OFF -DSFML_BUILD_NETWORK=OFF -DSFML_BUILD_EXAMPLES=OFF -DSFML_BUILD_TEST_SUITE=OFF'
```

The only source change needed so far is a `NINTENDO_SWITCH` branch in
`cmake/Config.cmake`, which otherwise hard-fails with "Unsupported operating system or
environment". It sets `SFML_OS_SWITCH 1` and `OPENGL_ES 0` (desktop GL, per the driver
findings above).

**Configure succeeds**, and SDL's own summary confirms the backends registered:

```
Video drivers:    dummy offscreen switch
Joystick drivers: switch
Audio drivers:    dummy          <-- SDL_AUDIO OFF, see the audio correction above
```

### VRSFML changes so far

Each is a platform gate that had no Switch case, not new functionality:

| File | Gate | Change |
|---|---|---|
| `cmake/Config.cmake` | falls through to `FATAL_ERROR "Unsupported operating system"` | `elseif(NINTENDO_SWITCH)` → `SFML_OS_SWITCH 1`, `OPENGL_ES 0` |
| `include/SFML/Config.hpp` | `#error This operating system is not supported` | `#elif defined(__SWITCH__)` → `SFML_SYSTEM_SWITCH`. Note the Switch is **not** a `__unix__` platform despite libnx providing newlib+pthreads, so it needs its own branch rather than joining the UNIX tree |
| `include/SFML/System/Path.hpp` | `value_type` defaulted to `wchar_t` because Switch matched neither Emscripten nor `LINUX_OR_BSD` | added `SFML_SYSTEM_SWITCH` to the narrow-`char` branch — newlib paths are narrow |
| `src/SFML/System/Thread.cpp` | `#error "no thread backend implemented for this platform"` | added `SFML_SYSTEM_SWITCH` to the POSIX list; libnx pthreads means the existing backend works unchanged |
| `extlibs/headers/moodycamel/lightweightsemaphore.h` | `#error Unsupported platform! (No semaphore wrapper available)` | added `__SWITCH__` to the two `__unix__` branches; libnx has `<semaphore.h>` with `sem_t`, already proven by SDL's `HAVE_PTHREADS_SEM` probe |

One more, found later and worth its own note: `SDLWindowImpl::getNativeHandle()` has a
per-platform `#if` chain that Switch fell off the end of, leaving an empty `static_cast`.
Rather than stubbing it to `nullptr`, it now works the way every other platform does — a
`SDL_PROP_WINDOW_SWITCH_NWINDOW_POINTER` property added to SDL's property list, set in
`SWITCH_CreateWindow` and read back here — so the handle is genuinely the libnx `NWindow`.

### Result: VRSFML builds for the Switch

With `SFML_BUILD_AUDIO=OFF`, the whole library set builds clean:

```
libsfml-system-s.a   libsfml-window-s.a   libsfml-graphics-s.a
libsfml-glutils-s.a  libsfml-imgui-s.a    libimgui.a   libSDL3.a
```

The entire VRSFML side is **29 added lines across 8 files**, carried as
`patches/switch-vrsfml-backend.patch`. Nothing needed rewriting — every change was a
platform gate with no Switch case.

**With `SFML_BUILD_AUDIO=ON` it also builds clean** — `libsfml-audio-s.a` appears and
nothing else changes. But see the audio correction above before reading that as working
sound: miniaudio resolves to the null backend on Switch, so it will be silent until
someone writes a custom miniaudio backend over `audout`.

## Task 7 — `NINTENDO_SWITCH` branch in the project CMakeLists

Added as a third arm alongside `EMSCRIPTEN` and desktop, reusing the VRSFML path.

**Both dependencies are pinned to exact revisions**, not `master`:
VRSFML `9c272d60134d568f35fbad9891f3b436de87cc28` and SDL
`e205361fb67ff53868dbc333eb2c491e11ff1a51`. The Switch patches were generated against
those trees; `master` would drift and they would stop applying.

**The ordering problem, and how it is solved.** The SDL patch changes SDL's *own CMake
logic* (platform detection, the `elseif(SWITCH)` subsystem block), so it has to land
before VRSFML configures SDL — patching afterwards would be too late. VRSFML's
`src/SFML/Window/CMakeLists.txt` prefers a sibling `../SDL` directory over its CPM pin,
so SDL3 is fetched deliberately into `${CMAKE_BINARY_DIR}/_deps/SDL`, patched there, and
only then is VRSFML populated, patched and added. Hence `FetchContent_Populate` +
explicit `add_subdirectory` rather than `FetchContent_MakeAvailable` for those two.

Patch application goes through a new `deceptus_apply_patch()` helper that follows the
same rule as the existing inline blocks: skip only when a reverse-apply proves the patch
is already applied, otherwise fail loudly. The two existing inline blocks were left
untouched.

Linking swaps glew for glad (`glad EGL glapi drm_nouveau`, in that order), and
`nx_generate_nacp()` / `nx_create_nro()` produce the homebrew binary.

### The include shim, and the `__EMSCRIPTEN__` problem behind it

First engine build produced only **4 errors**, all `SFML/Graphics.hpp: No such file or
directory` — VRSFML has no monolithic umbrella headers. The WASM target already solves
this with a shim tree at `src/wasm/SFML/`, and **those shims are VRSFML compatibility
headers, not Emscripten-specific**: they re-export VRSFML's split headers and supply
`sf::Drawable` and `sf::VertexArray`, both of which VRSFML dropped. So Switch reuses them.

Added `src/switch/opengl/glew.h`, the counterpart to `src/wasm/opengl/glew.h`, mapping
`glewInit()` onto `gladLoadGL()`. The difference from the WASM stub is that this one
exposes **desktop** GL via glad rather than GLES3. Both `src/switch` and `src/wasm` go on
the include path in a **single** `target_include_directories(... BEFORE ...)` call so the
order holds — two separate `BEFORE` calls would prepend `src/wasm` last and let the GLES3
stub shadow the glad one.

**The larger issue this exposes.** The dual-SFML architecture selects between VRSFML and
vanilla SFML APIs with `#ifdef __EMSCRIPTEN__`, across ~161 files. On Switch that macro is
not defined, so every one of those sites takes the *vanilla SFML* branch while compiling
against *VRSFML* headers. The first build only surfaced 4 errors because it failed on the
umbrella headers before reaching any of that.

Measured with a keep-going build: **1575 errors across 124 files**, and the taxonomy is
uniform — `Sprite::setPosition` vs VRSFML's `position` member, `Texture()` vs
`Optional<Texture>`, `Time::Zero`, `RenderStates::Default`, `RenderTarget::setView`,
`Rect::findIntersection`, `Shader::loadFromFile` signatures. All vanilla-SFML calls
against VRSFML headers, exactly as predicted.

### The `DECEPTUS_VRSFML` migration

What made this tractable: of the 160 files carrying `__EMSCRIPTEN__` guards, only **4**
touch genuinely Emscripten-only APIs (`emscripten.h`, `EM_ASM`, `EM_JS`) —
`gamepaths.cpp`, `gameconfiguration.cpp`, `game.cpp`, `main.cpp`. The other 156 are pure
SFML-flavour selection.

So `DECEPTUS_VRSFML` is now defined by CMake exactly when `EMSCRIPTEN OR NINTENDO_SWITCH`,
and the flavour guards were migrated onto it. **This cannot change the existing targets:**

| target | `__EMSCRIPTEN__` | `DECEPTUS_VRSFML` | effect |
|---|---|---|---|
| WASM | defined | defined | every guard evaluates identically — no behaviour change |
| desktop | undefined | undefined | no behaviour change |
| Switch | undefined | defined | takes the VRSFML branch, which is the point |

Because both macros are defined together on WASM, it does not matter which one any given
site uses there — which is what makes iterating on this safe rather than risky.

Hand-reviewed sites, all four files:

- `gamepaths.cpp` — guards are genuinely Emscripten (IDBFS mount, `EM_ASM` syncfs), left
  alone. Added a Switch branch instead: romfs is read-only, so saves go to
  `sdmc:/switch/deceptus`.
- `gameconfiguration.cpp` — the `getDesktopMode()` seeding and `clampResolutionToDesktop()`
  now exclude Switch as well as web: neither has a desktop, the Switch scans out at a fixed
  720p handheld / 1080p docked.
- `game.cpp` — 35 of 36 guards migrated; only the `emscripten.h` / `html5.h` include kept.
- `main.cpp` — 1 of 3 migrated (the `GraphicsContext`/`AudioContext` creation); the IDBFS
  mount and header include kept.

**Desktop is verified — visually, not just by reasoning.** Rebuilt `build/Release` with the
migration in place and drove it with the existing harness:

```
cd lab/map_render
uv run --with pywin32 --with pillow python drive_desktop.py build/Release
```

It played through the catacombs, teleported, opened the inventory, revealed the map and
wrote the visited rooms back to the save state. Screenshots in `lab/map_render/out/` show
correct rendering — lighting and the lantern glow, shadow gradients, HUD, tilemaps, and
the full map page with legend and fonts. `out/game.log` has **zero errors or warnings**.

**WASM has still not been rebuilt.** The macro table above argues it is a no-op there too,
but that remains reasoning rather than evidence.

### Lua on devkitPro

Lua failed with `#error "Compiler does not support 'long long'"`. `luaconf.h` uses the
presence of `LLONG_MAX` as its proxy for C99 compliance, and devkitPro's newlib does not
define it in C++ mode — verified by compiling a probe rather than assumed.

**Two attempts failed before the right fix, both for the same reason.** `luaconf.h`
unconditionally defines these macros a few lines before it tests them, so neither
`-DLUA_INT_TYPE=LUA_INT_LONG` nor `-DLUA_C89_NUMBERS=1` survives — the command-line value
is simply overwritten, and the only visible symptom is extra "redefinition" errors on top
of the original ones. The knob Lua's own error message tells you to set is not settable.

`patches/switch-lua-c89-numbers.patch` makes it overridable; that is the entire patch.
With it, `LUA_C89_NUMBERS=1` selects `long` + `double`, and on aarch64 both are 64-bit, so
this matches the default `long long` + `double` exactly — nothing is narrowed. It is set
`PUBLIC` on the `lua` target deliberately: every translation unit including `lua.hpp` must
agree with the library about `lua_Integer`, or the ABI silently disagrees.

Unlike the SDL patch, this one is applied *after* `FetchContent_MakeAvailable`, which is
fine because `luaconf.h` is only read at compile time rather than by CMake.

### Gotcha: never reset a fetched dependency from the Windows host

If a patch needs regenerating, do **not** `git checkout` / `git clean` the fetched tree
under `build_switch_engine/_deps/` from the host. With `core.autocrlf=true` the host
rewrites those files as CRLF, and the LF patch then fails to apply *inside the container*
— while `git apply --check` still passes on the host, which makes it look fine.

Two related traps in the same area:

- `git checkout .` alone is not enough: the patch adds new files (`src/video/switch/` and
  friends) which are untracked, so they survive and the patch fails as "already exists".
- The right move is simply `rm -rf build_switch_engine/_deps/SDL
  build_switch_engine/_deps/switch_sdl-subbuild` and let CMake re-clone it in the
  container, where the checkout stays LF.

## Assets, and how to check the build

`data/` is embedded in the `.nro` as romfs, so the binary is self-contained at 122.6 MB
(15 MB code + 104 MB assets).

The engine reaches for assets through relative paths — `"data/sprites/x.png"` and
similar, 134 literals — so romfs carries a **nested `data/` directory**, not the contents
of `data/` at its root. CMake stages into `build_switch_engine/romfs/data` using
`copy_directory_if_different`, so only the first build moves the whole tree. `main()`
mounts romfs and `chdir("romfs:/")` before anything reads a config or texture. Saves still
go to `sdmc:/switch/deceptus`, since romfs is read-only.

ImGui is excluded from this build entirely, as on WASM — clean, because the three debug UI
files were already compiled out on VRSFML targets.

### `lab/switch_smoke/test_switch_build.py`

```
uv run --with pytest pytest lab/switch_smoke/test_switch_build.py -v
```

24 tests, all passing. They cover the failures that would otherwise be **silent**:

- romfs really embedded, measured as bytes appended past the NRO image
- the Switch backends really linked — SDL quietly substitutes dummy drivers when a
  backend is missing, so a broken port still looks like a clean build
- romfs nested as `data/` rather than at the root, which would break every asset path
- the staged tree compared file-by-file against source `data/`, so stale incremental
  staging cannot ship old assets unnoticed

Two of these caught mistakes in the *tests* rather than the build, both worth remembering:
`romfsInit()` is an inline wrapper in libnx so it never appears as a symbol (check the
mount machinery it pulls in instead), and `SWITCHAUD_bootstrap` is legitimately absent
because VRSFML sets `SDL_AUDIO OFF` — the Switch audio backend is not compiled into the
game at all. That is now asserted as a documented limitation, so it fails loudly if anyone
wires SDL audio in.

**What these tests cannot tell you:** whether it runs. For that, see below.

## Running it in Ryujinx

**Correction to an earlier claim in this document:** I argued an emulator would not be a
useful test, because emulators implement NVN while this port goes through mesa/nouveau and
`libdrm_nouveau`. That was wrong. Ryujinx emulates **nvservices**, so the homebrew GL path
works, and it turned out to be the single most valuable test available.

Setup (Ryujinx 1.3.2 at `D:\games\ryujinx-1.3.2-win_x64\publish`):

- `prod.keys` from `D:\games\nsw_keys_firmware\keys-21\` into `%APPDATA%\Ryujinx\system\`
- set `"update_checker_type": "Off"` in `%APPDATA%\Ryujinx\Config.json`, otherwise a
  GitHub 404 dialog blocks startup — the project was removed from GitHub
- `Ryujinx.exe <path-to-nro>`

**`lab/switch_smoke` renders correctly.** Its clear colour comes out **pixel exact**:
sampled R=38 G=25 B=64 against the 0.15/0.10/0.25 passed to `glClearColor`. EGL bring-up,
a desktop-GL context, `glClear` and `eglSwapBuffers` all work.

### Seeing the engine's own log

`stderr` reaches `svcOutputDebugString` through `consoleDebugInit(debugDevice_SVC)` and
appears in Ryujinx's guest log. **`std::cout` does not** — its filebuf caches the original
`FILE*`, so the usual `stdout = stderr;` idiom moves `printf` but not the engine's logging,
which is what `Log::` uses. Startup tracing in `main()` uses `fprintf(stderr, ...)` for
this reason.

### Three startup bugs it found

All three would have failed on hardware too:

1. **`std::filesystem::copy_file` cannot cross devoptab devices.** Seeding the bundled
   default config copies `romfs:/data/config/game.json` → `sdmc:/…/settings/game.json`.
   newlib created the target, copied nothing, and reported it through the `error_code`
   that call discards. The resulting empty config then killed startup, because
   `GameConfiguration::deserialize` calls `json::parse` **outside** its try block. Fixed by
   copying through streams and deleting a truncated target rather than leaving it to
   poison the next launch.
2. **`nwindowSetDimensions` failure was treated as fatal.** It overrides a size the NWindow
   already reports and is rejected in some operation modes. Now falls back to
   `nwindowGetDimensions`.
3. **One NWindow, several SDL windows.** EGL allows one surface per native window, and
   VRSFML creates a context window before the real render window. The second
   `eglCreateWindowSurface` returned `EGL_NO_SURFACE` while reporting **`EGL_SUCCESS`**,
   which reads like a driver fault. The surface is now created once, shared, and destroyed
   with the last window.

### A startup stall in VRSFML's window setup — and a wrong conclusion

**Superseded, and kept because the wrong turn is instructive.** The stall was real, but the
diagnosis below blames the joystick backend and it is wrong: what hangs is VRSFML's *error
path*, through libbacktrace, and the joystick was merely what happened to log an error first.
See "Solved: an SFML error message freezes the console" further down. Controller input works.

Traces run through the SDL backend now, so the sequence is exact:

```
video: create window 1x1          <- VRSFML's context window
video: surface ready (refs 1)
egl: creating context / context created
graphics context created
audio context created
game constructed
initialize: before initializeWindow
video: create window 1920x1080    <- the real render window
video: surface ready (refs 2)
video: show window / show window done
<stall - "egl: creating context" is never reached for this window>
```

`SWITCH_CreateWindow` and `SWITCH_ShowWindow` both complete, and
`SWITCH_GLES_CreateContext` is never entered, so the stall sits **inside VRSFML's window
setup**, not in this backend.

**Resolved: it was the joystick backend.** Not window events — sending
`SDL_EVENT_WINDOW_SHOWN`/`_EXPOSED`/`_FOCUS_GAINED` from `SWITCH_ShowWindow` made no
difference. Reporting a device from `SWITCH_JoystickGetCount` is what hangs start-up:
enumerating it makes callers query properties this driver does not supply
(`SDL_GetJoystickVendor` fails first) and something in that path never returns. The
explicit `SDL_PrivateJoystickAdded` in `Init` is *not* the cause — both halves were tested
separately.

`SWITCH_JoystickGetCount` therefore returns **0** for now, with a TODO.
**Controller input does not work yet.** The GUID from `SDL_CreateJoystickGUIDForName`
carries no vendor or product id, which is the most likely thing to fix first.

Two real fixes came out of narrowing this down:

- **The window size is dictated by the system.** `nwindowSetDimensions` is no longer called
  at all; the size comes from `nwindowGetDimensions` and is reported back to SDL. There is
  one NWindow, so honouring an application's requested size resized the *shared* native
  window — and because VRSFML asks for a 1x1 context window first, every later window
  inherited a 1x1 surface.
- **`SDL_WINDOW_FULLSCREEN` is no longer forced.** The window does cover the screen, but
  claiming fullscreen makes SDL run a transition and wait for an event that no window
  manager exists to send.

The `SWITCH_GLES_CreateContext` / `MakeCurrent` wrappers are written out by hand rather
than generated by `SDL_EGL_*_impl` so they can carry traces. **All the tracing is
scaffolding and should be removed once this is solved.**

### Solved: the render texture FBO — switch-mesa cannot share GL contexts

The symptom was `glCheckFramebufferStatus` coming back incomplete for the very first
render target:

```
[[SFML ERROR]]: Impossible to create render texture
                (failed to link the target texture to the framebuffer)
```

The stencil hypothesis recorded here earlier was **wrong**, and guessing further would have
cost hours. `lab/switch_fbo_probe` settled it in one run by replicating VRSFML's exact
sequence and varying one ingredient at a time. Every single-context combination is fine —
colour-only, `STENCIL_INDEX8` through either storage entry point, `DEPTH24_STENCIL8` on
either attachment point, `DEPTH_COMPONENT16`, sized and unsized colour formats — all
`GL_FRAMEBUFFER_COMPLETE`. The one that fails is a texture created on a *different,
shared* context:

```
GL_VENDOR nouveau / GL_RENDERER NV120 / GL_VERSION 4.3 (Core Profile) Mesa 20.1.0-rc3
colour only (GL_RGBA texture)                  : GL_FRAMEBUFFER_COMPLETE
colour + STENCIL_INDEX8 (multisample entry)    : GL_FRAMEBUFFER_COMPLETE
...
glIsTexture on secondary context: NO (sharing is broken)
  gl error after glFramebufferTexture2D: 0x0502   <- GL_INVALID_OPERATION
colour only, texture from other context        : GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT
```

**Root cause: devkitPro's switch-mesa does not implement EGL context sharing, and reports
no error for it.** Its EGL driver lives in `src/egl/drivers/switch/egl_switch.c` (a
Switch-specific `_EGLDriver`, not the shared dri2 one). `switch_create_context` takes the
`share_list` argument and then calls:

```c
context->stctx = display->stapi->create_context(display->stapi, display->stmgr, &attribs, &error, NULL);
                                                                        the shared context ^^^^
```

`share_list` is never used. **Every devkitPro mesa branch does this** — checked `switch`,
`switch-20.1.0-rc3`, `switch-21.x` and `switch-new`; the shipped portlib is
`switch-mesa 20.1.0-5`. So a context created with a share list is silently unshared, and
anything created on one context does not exist on the other. VRSFML creates *every* texture
on its shared context (`Texture::create` runs under `GLSharedContextGuard`) and every FBO on
whichever context is current, so nothing that needs a render target could ever have worked.

**Fix: replace sharing with sameness.** `SWITCH_GLES_CreateContext` now hands out one
refcounted `SDL_GLContext` to every caller, with `SWITCH_GLES_DestroyContext` tearing it
down only when the last reference goes. This mirrors what the backend already does one
level down — one NWindow, one EGL surface, and now one context — and is safe because a GL
context can only be current on one thread at a time and nothing in this engine drives GL
from a second thread (`LazyTexture` and `Game::loadLevel` are both synchronous on the
`DECEPTUS_VRSFML` path).

Rebuilding mesa was rejected as the alternative: it is a one-line driver fix, but it would
mean cross-compiling mesa 20.1 with meson in the container and shipping a patched portlib
that the Docker image does not carry, which breaks "the image is the toolchain".

Also seeded the Switch video mode at **1280x720** rather than 1920x1080: that is the
handheld scan-out size and what the NWindow reports by default. Docked mode hands out
1080p, and the window reports its real size at creation time.

### Solved: every shader took its legacy branch on a core profile

With render targets working, the next failure was shader compilation:

```
[[SFML ERROR]]: Failed to compile fragment shader:
0:114(29): error: `gl_TexCoord' undeclared
0:117(15): error: no matching function for call to `texture2D()'
```

The shaders under `data/shaders` carry two flavours and were selecting between them with
`#ifdef GL_ES`. VRSFML prepends its own `#version` line to everything it compiles, and
which line depends on the target: `300 es` on WASM, **`430 core` on the Switch**. Only the
WASM preamble defines `GL_ES`, so the Switch took the *legacy* branch — `gl_TexCoord`,
`gl_FragColor`, `varying`, `texture2D` — none of which exist in a core profile.

**Fix: select on `#if __VERSION__ >= 300` instead**, in all 21 shaders that had the guard.
That covers both modern targets and leaves desktop, which gets no preamble at all and so
reports `__VERSION__` 110, on exactly the branch it has always used:

| target | preamble | `__VERSION__` | branch |
|---|---|---|---|
| WASM | `#version 300 es` | 300 | modern |
| Switch | `#version 430 core` | 430 | modern |
| desktop (vanilla SFML 3) | none | 110 | legacy |

`GL_ES` cannot simply be defined for the Switch instead: GLSL reserves the `GL_` prefix and
the driver rejects any attempt to define such a macro. The reasoning is written up at the
top of `src/framework/tools/sfmlshader.h`, which is where a reader of the shader sources
will look.

The modern branches needed no other change — they are plain modern GLSL (`in`/`out`,
`texture()`, `layout(location=…)`), all valid under 430 core, and they already target
VRSFML's `sf_a_*` / `sf_u_*` interface, which is identical on both targets.

Deliberately untouched: `texture.fs` / `texture.vs` (the raw-GL 3D menu background, whose
`GL_ES` guard really is about GLES vs desktop sampler bindings) and the handful of shaders
that still open with their own `#version 120`, which are desktop-only paths.

### Solved: an SFML error message freezes the console

Enumerating a joystick was recorded here as stalling start-up "somewhere inside
`SDL_GetJoystickVendor`". It is not the joystick, and the SDL backend was never at fault —
traces showed `SWITCH_JoystickOpen`, the gamepad mapping and a full `SWITCH_JoystickUpdate`
all completing. What hangs is **VRSFML's error path itself**.

`sf::priv::emitErr` prints the message and then, when `SFML_ENABLE_STACK_TRACES` is on,
calls `printStackTrace()`. That starts with a `std::puts("")` — the mysterious blank line
right after the error in the log — and then calls `backtrace_full` on a state built by
`backtrace_create_state(nullptr, ...)`, which asks libbacktrace to locate the running
executable by itself. That has no meaning for an NRO, and the call never returns.

This matters far beyond the joystick: **any** error VRSFML logs would have frozen the game,
which makes it worth knowing about. `SFML_ENABLE_STACK_TRACES` defaults to a generator
expression that CMake evaluates as true even in Release, so it was on. It is now forced OFF
for `NINTENDO_SWITCH` in the root `CMakeLists.txt`, alongside the existing Emscripten
exclusion in VRSFML itself.

### Solved: controller input

With the freeze gone, the joystick backend was re-enabled and works. Two changes:

- `SWITCH_JoystickGetCount` returns **1** again. libnx presents the Joy-Con pair, a Pro
  Controller and the handheld unit through one `PadState`, so there is always exactly one
  controller as far as this driver is concerned.
- `SWITCH_JoystickGetDeviceGUID` now builds the GUID with `SDL_CreateJoystickGUID` carrying
  Nintendo's vendor id `0x057E` and the Pro Controller product id `0x2009`, instead of
  `SDL_CreateJoystickGUIDForName`. SDL decodes vendor and product back *out of* the GUID, so
  a name-only GUID makes `SDL_GetJoystickVendor` and `SDL_GetJoystickProduct` fail outright
  — which is what produced the error that triggered the freeze above. These are also the ids
  SDL's own HIDAPI driver reports for this hardware, so gamepad databases keyed on them
  match.

That got input as far as SDL, but the game still ignored it, because **nothing on the
`DECEPTUS_VRSFML` path ever told the engine a controller existed.** `GameControllerDetection`
detects hotplug by running `SDL_WaitEvent` on a worker thread, and `start()` is
`#ifndef DECEPTUS_VRSFML` — reasonably so, since VRSFML owns the SDL event queue and pumps
it from the main loop, and a second thread would fight it for events. But nothing replaced
it, so `processEvent` was never called, no controller was ever opened, and
`isControllerConnected()` was permanently false. On desktop and web that is invisible
because there is a keyboard; on a console it means no input at all.

`GameControllerDetection::update()` now polls `SDL_GetJoysticks()` once a frame from
`GameControllerIntegration::update()` and diffs it against the previous frame. Polling
rather than reading events is deliberate: taking joystick events out of the queue here
would hide them from VRSFML, and a device-list diff is idempotent and needs no ownership of
the queue. The body is `#ifdef DECEPTUS_VRSFML`, so desktop keeps its event thread
untouched. Note the SDL 3 detail this relies on: `SDL_GetJoysticks` returns *instance* ids,
which is exactly what `GameController::activate` and the add/remove callbacks want.

## The first hardware run

**Confirmed: it was applet mode.** Relaunching the *same* binary in title takeover mode made
it work on the console. The diagnosis below is kept because the symptom is so unhelpful that
anyone hitting it again will need it — and because the whole thing was read off a photo of
the fatal screen, which is a technique worth reusing.

It crashed. Atmosphère's fatal screen, on firmware 22.5.0 / Atmosphère 1.11.2:

```
Error Code: 2168-0002 (0x4a8)      <- data abort, i.e. an invalid memory access
Program:    010000000000100D       <- the Album applet
```

Two things are readable straight off that screen without a crash report or symbols.

**The program id says how it was launched.** `010000000000100D` is the Album applet, which
is what the homebrew menu runs inside when it is opened from the album. That is *applet
mode*, and applet-mode homebrew lives inside that applet's memory pool, which is a fraction
of the application pool a game gets. Title takeover mode — hold R while starting a game from
the HOME menu — hands over the full pool instead.

**The register dump says what it was doing.** Decoded little-endian, X6 through X13 spell:

```
X6  742064656C696146   "Failed t"
X7  692064616F6C206F   "o load i"
X8  6F7266206567616D   "mage fro"
X9  79726F6D656D206D   "m memory"
X13 203A6E6F73616552   "Reason: "
```

which is VRSFML's `"Failed to load image from memory. Reason: {}"`, from `Image.cpp:300`,
where the `{}` is `stbi_failure_reason()`. So an image failed to decode, and the crash is
immediately after: `TexturePool::createResource` returns `nullptr` on a failed load and the
caller dereferences it.

Why an image would fail to decode on hardware but not under emulation is answered by the
number the emulator reports: **3285 MB total**, because Ryujinx runs an NRO as an
application. Applet mode has nothing like that, `stbi`'s allocation fails with `outofmem`,
and the null propagates into a data abort. The engine embeds 104 MB of assets and decodes
much of that to RGBA, so it is not a marginal case.

**The fix was therefore how it is launched, not a code change** — which is what relaunching
in title takeover mode confirmed. These changes went in alongside, to make the next hardware
failure readable rather than another register dump:

- **The Switch build now writes a log to the SD card**, `sdmc:/switch/deceptus/logs/`.
  `LogThread` was compiled out for every `DECEPTUS_VRSFML` target because the web build has
  no filesystem worth logging to; the Switch does, and it is the only artefact a hardware run
  leaves behind. `stderr` reaches `svcOutputDebugString`, which needs a debugger or an
  emulator attached, so it is no use on a console by itself.
- **VRSFML's own errors are routed into that log** through `sf::priv::setErrSink`, installed
  in `main()` on the VRSFML path. Otherwise exactly the message that mattered here — the one
  sitting in the registers — would never appear in it.
- Start-up logs the applet type and the memory pool, and warns explicitly when it is running
  in applet mode. Under Ryujinx that line reads
  `switch: applet type 0, memory 3281 MB used of 3285 MB`; applet type 0 is
  `AppletType_Application`, which is what title takeover looks like.

Still worth fixing regardless of the outcome: a failed texture load should be a logged error,
not a null dereference. Which callers need hardening is not yet known, and the log now names
the asset, so the next run says.


## Open: the physics runs slow on hardware

The rendering looks fine on the console but the simulation drags. The coupling is not in
doubt — `Level::update` is the only place a Box2D world is stepped anywhere in the codebase,
and it steps a fixed amount exactly once per frame:

```cpp
_world->Step(PhysicsConfiguration::getInstance()._time_step, 8, 3);   // level.cpp:1762
```

`data/config/physics.json` sets that step to **1/35**, not the 1/60 the C++ default uses, so
world speed is `fps x timestep`. Two candidates follow, and they produce the *same* symptom
in game, because animations, timers and tweens advance on real `dt` and stay correct either
way:

1. the frame rate is low — at 30 fps the world runs at half
2. the timestep is not what we think it is on this target

**Ruled out so far**, each checked rather than assumed:

- **Resolution.** It is played handheld, which is 1280x720 — exactly what Ryujinx rendered.
  Note handheld is also the *lowest* GPU clock mode, so it is not the lighter case.
- **`setFramerateLimit(60)` fighting vsync.** VRSFML's `ThisThread::sleepFor` returns
  immediately on a non-positive duration, so under vsync the limiter never sleeps.
- **A swap interval of 2.** SDL sets `eglSwapInterval(1)`.
- **The Switch forcing 30 fps.** It does not; it scans out at 60 Hz in both modes.
- **`physics.json` failing to load.** `levels.json` is read by the *identical* byte-by-byte
  `ifstream` loop from the same `data/config/` relative path on romfs and demonstrably works,
  and nothing touches `PhysicsConfiguration` before `main()` runs `chdir("romfs:/")`.

**What it needs next is a measurement, not more theory:** the frame rate, and the update
versus draw split, which says whether a low one is cpu or gpu bound. The engine already
measures both under `DEVELOPMENT_MODE`; on a console they simply have nowhere to go, and the
sd-card log is now the place to put them.

One free check before instrumenting anything: the **main menu** starmap is continuous
rendering with no physics behind it. If it is visibly less smooth than the desktop build, the
frame rate is the answer.

**Also worth knowing: Ryujinx is not a performance proxy.** It JIT-recompiles the guest
AArch64 onto a desktop cpu and translates the Tegra command buffers to Vulkan on a desktop
gpu. Sixty frames a second there says nothing whatsoever about what the console can do.

## How to pick this up from scratch

Everything below is what a fresh session needs; there is no state left in anyone's head.

### Working copies

| What | Where | Branch / base |
|---|---|---|
| SDL3 (vittorioromeo fork) | `D:/deceptus/sdl3_switch` | `switch-backend`, based on `e205361fb` |
| VRSFML | `D:/deceptus/vrsfml_switch` | `switch-backend`, based on `9c272d601` |

Both are **outside** the repo on purpose: the committed artefacts are the patches under
`patches/`, which are the single source of truth. The trees are scratch space — about a
gigabyte of it — and deleting them costs nothing:

```
powershell -File lab/switch_smoke/setup_working_copies.ps1
```

clones both at those revisions, puts each on its `switch-backend` branch and applies its
patch. The round trip is exact: recreating from the patches and regenerating the patches from
the result gives byte-identical files, which is checked rather than assumed.

Note it clones with `core.autocrlf=false` on purpose. Cloning them normally on this host
would produce CRLF trees, and everything downstream — patch regeneration, mirroring into
`_deps/` — then fights the line endings.

### The iteration loop — this is the part that is easy to get wrong

The engine build uses its **own** fetched copies under `build_switch_engine/_deps/`, not
the working copies. Editing `D:/deceptus/sdl3_switch` alone changes nothing. Worse, CMake
refuses to configure when a patch under `patches/` neither applies nor reverse-applies, so
a mirrored edit without a regenerated patch fails the build before anything compiles.

Both halves are scripted:

```
powershell -File lab/switch_smoke/sync_switch_patches.ps1
build_switch.bat
```

`sync_switch_patches.ps1` regenerates `patches/switch-sdl3-backend.patch` and
`patches/switch-vrsfml-backend.patch` from the working copies, then mirrors every file the
patch touches into `_deps/`. Two things it gets right that a plain `Copy-Item` does not:

- the patch is written through `cmd` redirection, because git emits LF and PowerShell's
  `Set-Content` would re-encode it — a CRLF patch does not apply inside the container
- the mirrored files are **converted to LF on the way in**. The working copies live on a
  host with `core.autocrlf=true`, so every file git has checked out there is CRLF, while
  the tree under `_deps/` was cloned inside the container and is LF. Copying verbatim drags
  CRLF into `_deps/` and the patch then fails in the container — the exact trap described
  above, hit again while writing this script.

The **clean loop** is still what proves a patch really applies: `rm -rf
build_switch_engine/_deps/SDL build_switch_engine/_deps/switch_sdl-subbuild` and let CMake
re-clone and re-apply. **Never** reset those trees from the Windows host.

### Build and run

```
build_switch.bat          # -> build_switch_engine/deceptus.nro
uv run --with pytest pytest lab/switch_smoke/test_switch_build.py -v
powershell -File lab/switch_smoke/run_ryujinx.ps1 `
  -NroPath D:\deceptus\deceptus_engine\build_switch_engine\deceptus.nro `
  -OutputPath out.png -SettleSeconds 60
```

`run_ryujinx.ps1` launches the emulator, waits, screenshots the window, prints the guest
log and kills it. Guest trace lines appear as
`KernelSvc OutputDebugString: [switch-trace] …`. Only the last 40 lines are printed; the
whole log is at `%TEMP%\ryujinx_stdout.txt`, and pulling the guest lines out of it is worth
doing every time:

```
Get-Content "$env:TEMP\ryujinx_stdout.txt" | Select-String "OutputDebugString" |
  ForEach-Object { ($_ -split "OutputDebugString: ")[-1].TrimEnd() }
```

**Ryujinx maps the keyboard to Player 1** as a Pro Controller, so input can be driven from
a script. Note the layout: Nintendo's B (which SDL reports as the positional `A` button,
and what the game treats as confirm) is bound to **X** on the keyboard; Nintendo's A is
bound to Z. D-pad is the arrow keys, left stick is WASD.

`lab/switch_smoke/drive_ryujinx.py` does exactly that — launch, wait, confirm through the
main menu and file select, wait for the level, walk and jump, capturing each step into
`lab/switch_smoke/out/`:

```
uv run --with pywin32 --with pillow python lab/switch_smoke/drive_ryujinx.py
```

Two things it borrows from `lab/map_render/drive_desktop.py` for the same reasons: real
`keybd_event` presses rather than posted messages, because the emulator reads key state
rather than window messages, and `PrintWindow` with `PW_RENDERFULLCONTENT`, because a plain
screen grab of a hardware-accelerated surface comes back blank or cropped. The screenshots
it produces are also much better than `run_ryujinx.ps1`'s, which captures the screen region
and therefore only what is not covered.

### `lab/switch_fbo_probe`

```
build_switch.bat lab/switch_fbo_probe build_switch_fbo_probe
```

A standalone `.nro` that dumps the GL driver strings and then walks a matrix of framebuffer
configurations, printing `glCheckFramebufferStatus` for each — including the cross-context
case that turned out to be the answer. It builds in seconds and runs in seconds, against a
122 MB engine build, so reach for it before instrumenting the engine whenever a GL question
can be phrased as "does this combination work here".

### Debug scaffolding — removed

The start-up tracing is gone: the `SWITCH_TRACE` macro and its sixteen call sites across
`src/main.cpp` and `src/game/game.cpp`, and the eight `fprintf` traces in
`SDL_switchvideo.c` / `SDL_switchopengl.c`. The sd-card log covers the same ground, persists,
and works on hardware, which stderr does not. `SWITCH_GLES_MakeCurrent` was only written out
by hand so it could carry a trace and is back to `SDL_EGL_MakeCurrent_impl(SWITCH)`;
`CreateContext` and `DestroyContext` stay expanded because they hold the reference count.

What is deliberately kept: `consoleDebugInit(debugDevice_SVC)` and the `stdout = stderr`
aliasing in `main()`, because they cost nothing and are the only way to see start-up output
under an emulator, and `lab/switch_fbo_probe`, which earned its place.

### On threading, and what the `DECEPTUS_VRSFML` guards actually mean

Worth stating because the macro's name invites the wrong conclusion: **threads work on the
Switch.** libnx ships real pthreads, SDL uses its stock pthread backend, and `LogThread` runs
on a thread there. Three `DECEPTUS_VRSFML` guards look like "this target cannot thread" and
each is really about something else:

| Site | Why the VRSFML path is single-threaded |
|---|---|
| `Game::loadLevel` | The loader brings up an `sf::Context` of its own and needs it to **share GL objects** with the render context. WebGL has no sharing; switch-mesa accepts a share list and ignores it. Hard blocker on both — must stay. |
| `GameControllerDetection::start` | VRSFML owns the SDL event queue and pumps it from the main loop; a second thread in `SDL_WaitEvent` would fight it for events. Must stay; replaced by polling. |
| `LazyTexture::loadTexture` | **Only inherited.** That thread decodes an image and does no GL at all — the upload happens on the main thread in `uploadTexture()`. Nothing stops the Switch using it. Left alone for now rather than changed unverified before a merge. |

The last row is the one real opportunity: turning that decode back on for the Switch would
help texture streaming and costs nothing structurally. It needs runtime on hardware first.

## Open questions

- **What GL version/profile does the Switch actually report at runtime?** Static analysis says
  core 4.3. The `lab/switch_smoke` test prints `GL_VERSION` / `GL_RENDERER` /
  `GL_SHADING_LANGUAGE_VERSION` and compiles a `#version 430 core` shader to confirm on
  real hardware. **Needs a CFW Switch to run.**
- Whether `data/` (104 MB) goes in romfs or on sdmc, and how the 65 files using
  `std::filesystem` behave under libnx path semantics.

---

## Task list

| # | Task | Status |
|---|---|---|
| 1 | Add Docker Switch build harness | **done** (commit `6a6a28ea`) |
| 2 | Get SDL3 configuring/building for the Switch toolchain | **done** — `libSDL3.a` builds for AArch64; see `patches/switch-sdl3-backend.patch` |
| 3 | Implement SDL3 Switch video backend | **done and exercised** — window, EGL surface and GL context all work in Ryujinx |
| 4 | Implement SDL3 Switch joystick backend | **done** — enumerates and reports state; the stall was VRSFML's stack traces, not this |
| 5 | Implement SDL3 Switch audio backend | **compiles, unused** — VRSFML sets `SDL_AUDIO OFF`, see task 10 |
| 6 | Build VRSFML against Switch SDL3 | **done except audio** — all modules build; see `patches/switch-vrsfml-backend.patch` |
| 7 | Add `NINTENDO_SWITCH` branch to project CMakeLists | **done — the engine builds; `deceptus.nro` is produced** |
| 8 | Audit shaders for GL 4.3 core / GLES 3.2 | **done** — all 21 dual-flavour shaders now select on `__VERSION__`; every one compiles |
| 9 | Package `data/` into romfs, produce first `.nro` | **done** — 122.6 MB `.nro`, 24/24 validation tests pass |
| 10 | Custom miniaudio backend over libnx `audout` | pending — audio is silent until then |
| 11 | Run it on hardware or an emulator | **done for Ryujinx** — boots, plays, no errors logged; hardware still untested |
| 12 | Fix the incomplete render texture FBO | **done** — switch-mesa ignores EGL context sharing; the platform now uses one GL context |
| 13 | Restore controller input | **done** — count back to 1, GUID carries vendor/product, and detection now polls on the VRSFML path |
| 14 | Play past the menu — start a level and verify gameplay | **done** — menu, file select, catacombs load, walking and jumping all verified in Ryujinx |
| 15 | Run it on real hardware | **done** — runs on a console in title takeover mode; how it plays there is still to be reported |
| 16 | Strip the debug scaffolding | pending — the per-frame traces are gone, the start-up ones remain |
| 17 | Make a failed asset load an error rather than a crash | pending — see "The first hardware run" |
| 18 | CI for the Switch target | **done** — `.github/workflows/switch.yml`, builds and validates in the devkitPro container |
| 19 | Physics runs slow on hardware | open — see the frame-rate note below |

---

## Files added so far

- `build_switch.bat` — Docker entry point
- `docker/build_switch.sh` — in-container CMake build
- `lab/switch_smoke/source/main.cpp` — EGL + core-GL smoke test; also the runtime
  GL-capability probe, printing `GL_VERSION`, profile bits and a `#version 430 core`
  compile result over nxlink
- `lab/switch_smoke/test_switch_build.py` — 24 structural tests over the built `.nro`
- `lab/switch_smoke/run_ryujinx.ps1` — launches the emulator, screenshots, dumps the guest log
- `lab/switch_smoke/sync_switch_patches.ps1` — the iteration loop: regenerates the patches
  from the working copies and mirrors them into `_deps/`, converting to LF
- `lab/switch_smoke/setup_working_copies.ps1` — recreates both working copies from the patches
- `doc/switch_build.md` — the how-to: prerequisites, build, run, hardware, iteration loop
- `lab/switch_smoke/drive_ryujinx.py` — launches the emulator, sends controller input
  through the menus into a level, and captures each step; the counterpart to
  `lab/map_render/drive_desktop.py`
- `lab/switch_fbo_probe/` — standalone framebuffer-configuration probe; found the
  context-sharing bug
- `src/switch/opengl/glew.h`, `src/switch/SFML/OpenGL.hpp` — include shims
- `patches/switch-sdl3-backend.patch` — the SDL3 Switch backend
- `patches/switch-vrsfml-backend.patch` — VRSFML platform gates
- `patches/switch-lua-c89-numbers.patch` — makes Lua's own knob settable
- `switch_port_status.md` — this file
