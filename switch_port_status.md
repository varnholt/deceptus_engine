# Nintendo Switch homebrew port — status

Living status document. Mirrors the convention of `wasm_port_status.md`.

**Last updated:** 2026-08-14
**Branch:** `feat/switch-port` (pushed to origin, branched off `feat/harpoon` —
rebase onto `master` later, nothing here overlaps other work)

**Verified working:** `build_switch.bat lab/switch_smoke build_switch` produces
`build_switch/switch_smoke.nro` — valid `NRO0` magic, AArch64 PIE, 5.9 MB.
Toolchain, EGL/mesa/nouveau link, and `.nro` packaging are all confirmed end to end.
The smoke test has **not** been run on hardware yet (needs a CFW Switch + `nxlink`).

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

**Still worth re-verifying the WASM build** before trusting this, even though the table
above argues it is a no-op there.

### Lua on devkitPro

Lua failed with `#error "Compiler does not support 'long long'"`. `luaconf.h` uses the
presence of `LLONG_MAX` as its proxy for C99 compliance, and devkitPro's newlib does not
define it in C++ mode — verified by compiling a probe rather than assumed.

**First attempt was wrong:** setting `-DLUA_INT_TYPE=LUA_INT_LONG` has no effect, because
`luaconf.h` *unconditionally* defines `LUA_INT_TYPE` in its else branch rather than
honouring a command-line value, which just produces "redefinition" errors on top.

The supported knob is **`LUA_C89_NUMBERS`**, which selects `long` + `double`. On aarch64
both are 64-bit, so this matches the default `long long` + `double` exactly — nothing is
narrowed. It is set `PUBLIC` on the `lua` target deliberately: every translation unit
including `lua.hpp` must agree with the library about `lua_Integer`, or the ABI silently
disagrees.

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
| 3 | Implement SDL3 Switch video backend | **done** (compiles; unrun) |
| 4 | Implement SDL3 Switch joystick backend | **done** (compiles; unrun) |
| 5 | Implement SDL3 Switch audio backend | **done** (compiles; unrun) |
| 6 | Build VRSFML against Switch SDL3 | **done except audio** — all modules build; see `patches/switch-vrsfml-backend.patch` |
| 7 | Add `NINTENDO_SWITCH` branch to project CMakeLists | pending |
| 8 | Audit shaders for GL 4.3 core / GLES 3.2 | pending |
| 9 | Package `data/` into romfs, produce first `.nro` | pending |

---

## Files added so far

- `build_switch.bat` — Docker entry point
- `docker/build_switch.sh` — in-container CMake build
- `lab/switch_smoke/` — EGL + core-GL smoke test (also the runtime GL-capability probe)
- `switch_port_status.md` — this file
