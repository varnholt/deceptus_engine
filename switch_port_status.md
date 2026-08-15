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

---

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
| 2 | Get SDL3 configuring/building for the Switch toolchain | pending |
| 3 | Implement SDL3 Switch video backend | pending |
| 4 | Implement SDL3 Switch joystick backend | pending |
| 5 | Implement SDL3 Switch audio backend | pending |
| 6 | Build VRSFML against Switch SDL3 | pending |
| 7 | Add `NINTENDO_SWITCH` branch to project CMakeLists | pending |
| 8 | Audit shaders for GL 4.3 core / GLES 3.2 | pending |
| 9 | Package `data/` into romfs, produce first `.nro` | pending |

---

## Files added so far

- `build_switch.bat` — Docker entry point
- `docker/build_switch.sh` — in-container CMake build
- `lab/switch_smoke/` — EGL + core-GL smoke test (also the runtime GL-capability probe)
- `switch_port_status.md` — this file
