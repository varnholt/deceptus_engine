# Building and Running for the Nintendo Switch

The Switch build is a **homebrew** build: an unsigned `.nro` that runs from the homebrew menu
on a console with custom firmware, or in an emulator. It is not a retail Switch title and
there is no signing, eShop packaging or Nintendo SDK involved anywhere.

It reuses the WebAssembly rendering stack rather than the desktop one. Vanilla SFML 3 renders
through the fixed-function pipeline, and the Switch's mesa/nouveau driver exposes a core
profile only, so the desktop path cannot work there. The Switch therefore runs on
[VRSFML](https://github.com/vittorioromeo/VRSFML) over SDL 3, the same as the web build, with
a Switch video, audio and joystick backend added to SDL.

**State:** it builds, boots and plays. In Ryujinx menus, file select, level loading, movement,
lighting, shaders and controller input all work; on a real console it runs too, **provided it
is launched in title takeover mode** — see [Run it on hardware](#run-it-on-hardware), because
launching it the usual way fails in a thoroughly misleading way. **Audio is silent.**

The design decisions, the platform traps behind them and the running task list live in
[`switch_port_status.md`](../switch_port_status.md). This page is only about getting a build
and running it.


## Prerequisites

|What|Why|
|-|-|
|[Docker Desktop](https://www.docker.com/products/docker-desktop/)|The whole toolchain runs in the official `devkitpro/devkita64` image (~2.8 GB pulled). It is downloaded automatically on the first build.|
|A network connection, once|CMake fetches SDL 3, VRSFML and Lua at pinned revisions on the first configure. Later builds are offline.|
|~4 GB of disk|Image, fetched dependencies, object files and a 104 MB staged asset tree.|

**Do not install devkitPro natively on Windows.** It plants a global MSYS2 tree at
`C:\devkitPro`, registers system-wide `DEVKITPRO` / `DEVKITA64` environment variables, and its
CMake toolchain files assume POSIX paths. The Docker image exists precisely so none of that
is needed — the same arrangement the web build uses with `emscripten/emsdk`.

Nothing else is required to build. In particular the SDL and VRSFML working copies described
at the bottom of this page are **not** needed: the changes they hold are committed as patches
under `patches/`, and the build applies them to its own fetched copies.


## Build

From the repository root:

```bat
build_switch.bat
```

That runs `docker/build_switch.sh` inside the container, which configures with devkitPro's
`Switch.cmake` toolchain file and builds. The first run pulls the image, clones the
dependencies, applies the three Switch patches and stages `data/` into romfs, so expect it to
take a while; later runs are incremental and quick.

The result is:

```
build_switch_engine/deceptus.nro     ~117 MB
```

It is self-contained: `data/` is embedded in the `.nro` as a romfs image, so there is no
folder to ship alongside it. `main()` mounts romfs and changes into it before anything reads
a config or a texture. Saves cannot go there — romfs is read-only — and land in
`sdmc:/switch/deceptus` instead.

`build_switch.bat` builds the game into `build_switch_engine` when called without arguments. It
also takes a source directory and a build directory, which is how it builds the small diagnostic
projects described below.


## Continuous integration

`.github/workflows/switch.yml` builds this on every push and pull request against `master`,
and can be run by hand from the Actions tab on any branch. The job runs **inside**
`devkitpro/devkita64` rather than shelling out to docker from the runner, which is the same
arrangement as `build_switch.bat` with one less layer. It builds the smoke test first as a
toolchain canary, then the engine, then runs the validation suite below, and uploads
`deceptus.nro` as an artifact.

It does not upload the `.elf`. That is what you would need to symbolise a crash address from
a console fatal screen, but it is 214 MB — rebuild locally at the same commit if it ever
comes to that.

## Verify the build

```bat
uv run --with pytest pytest lab/switch_smoke/test_switch_build.py -v
```

24 structural tests over the produced `.nro`. They exist because the interesting failures here
are **silent**: SDL quietly substitutes dummy drivers when a backend is missing, so a broken
port still looks like a clean build. The tests check that the Switch video and joystick
backends really got linked, that romfs is really embedded (measured as bytes appended past the
NRO image), that it is nested as `data/` rather than dumped at the romfs root, and that the
staged tree still matches source `data/` file by file, so stale incremental staging cannot
ship old assets unnoticed.


## Run it in Ryujinx

Ryujinx emulates *nvservices*, which is the layer the homebrew mesa/nouveau GL stack talks to,
so the whole rendering path works there. It has been by far the most valuable test available
for this port — every bug listed in the status document was found by running it.

### One-time setup

1. Install Ryujinx. This port was developed against **1.3.2**, unpacked at
   `D:\games\ryujinx-1.3.2-win_x64\publish`. The scripts below default to that path.
2. Copy `prod.keys` (dumped from your own console) into `%APPDATA%\Ryujinx\system\`.
3. Set `"update_checker_type": "Off"` in `%APPDATA%\Ryujinx\Config.json`. Without it a GitHub
   404 dialog blocks start-up, because the project was removed from GitHub.

### Launch it by hand

```bat
Ryujinx.exe D:\deceptus\deceptus_engine\build_switch_engine\deceptus.nro
```

Ryujinx maps the **keyboard to Player 1** as a Pro Controller, so it is playable without a
gamepad. Mind the layout: Nintendo puts A where a standard pad puts B, and SDL's button names
are positional, so the button the game treats as *confirm* is SDL's `A` = Nintendo's `B` =
the **X** key.

|Game action|SDL button|Nintendo button|Keyboard|
|-|-|-|-|
|confirm / jump|A|B|<kbd>X</kbd>|
|cancel|B|A|<kbd>Z</kbd>|
|move|left stick|left stick|<kbd>W</kbd><kbd>A</kbd><kbd>S</kbd><kbd>D</kbd>|
|menu navigation|d-pad|d-pad|arrow keys|
|pause|Start|Plus|<kbd>+</kbd>|

### Launch and screenshot it from a script

```bat
powershell -File lab/switch_smoke/run_ryujinx.ps1 ^
  -NroPath D:\deceptus\deceptus_engine\build_switch_engine\deceptus.nro ^
  -OutputPath out.png -SettleSeconds 60
```

Launches the emulator, waits, captures the window, prints the tail of the guest log and kills
it. Good for "does it still boot".

### Play it from a script

```bat
uv run --with pywin32 --with pillow python lab/switch_smoke/drive_ryujinx.py
```

Drives the whole path — main menu, file select, load the catacombs save, walk, jump — and
captures each step into `lab/switch_smoke/out/`. This is the counterpart to
`lab/map_render/drive_desktop.py` and is the quickest way to confirm a change did not break
anything downstream of the menu.

Two details it depends on, both learned the hard way: it sends real `keybd_event` presses
rather than posting window messages, because the emulator reads key state rather than its
message queue, and it captures with `PrintWindow` and `PW_RENDERFULLCONTENT`, because a plain
screen grab of a hardware-accelerated surface comes back blank or cropped.

### Reading the engine's own log

The engine's log reaches the host through `svcOutputDebugString` and appears in Ryujinx's
guest log. `run_ryujinx.ps1` prints only the last 40 lines; the whole thing is at
`%TEMP%\ryujinx_stdout.txt`:

```powershell
Get-Content "$env:TEMP\ryujinx_stdout.txt" | Select-String "OutputDebugString" |
  ForEach-Object { ($_ -split "OutputDebugString: ")[-1].TrimEnd() }
```

Note that this works for `stderr` only. **`std::cout` does not reach the host**: its filebuf
caches the original `FILE*`, so the usual `stdout = stderr;` trick moves `printf` but not the
engine's own logging. The start-up traces use `fprintf(stderr, …)` for that reason.


## Run it on hardware

Copy `deceptus.nro` to the SD card under `/switch/` and launch it from the homebrew menu.

**Launch it in title takeover mode.** This is not optional for this game, and it is the
difference between it running and it crashing on the same binary. Hold <kbd>R</kbd> while
starting any *game* from the HOME menu; that opens the homebrew menu with the whole
application memory pool, and anything launched from there inherits it. Opening the homebrew
menu from the Album instead runs it as a **library applet**, inside that applet's much smaller
pool — the game loads over a hundred megabytes of assets and does not fit.

The failure when it does not fit is deeply unhelpful, which is why it is worth stating plainly:
an image fails to decode with `outofmem` somewhere in start-up, the caller dereferences the
null result, and the console shows a fatal error — `2168-0002`, a data abort — with a register
dump rather than a message. If you see that, check the applet type before anything else.

The build logs both numbers at start-up for exactly this reason:

```
[i] switch: applet type 0, memory 3281 MB used of 3285 MB
```

Applet type `0` is `AppletType_Application`, which is what title takeover mode looks like.
Anything else means applet mode, and the line after it says so.

### Reading the log on hardware

There is no terminal on a console, and `stderr` only reaches `svcOutputDebugString`, which
needs a debugger or an emulator attached. So the Switch build writes its log to the SD card:

```
sdmc:/switch/deceptus/logs/<date>.log
```

VRSFML's own errors are redirected into that same log rather than being written to `stderr`,
so an SFML failure is visible in it too. This is the only artefact a hardware run leaves
behind — read it before guessing.

Saves live next to it, under `sdmc:/switch/deceptus/settings`.

### Still open on hardware

What the driver really reports. `lab/switch_smoke` prints `GL_VERSION`, `GL_RENDERER`, the
profile bits and the result of compiling a `#version 430 core` shader, so that question can be
answered in one run — it has only ever been answered under emulation, where the answer is
`4.3 (Core Profile) Mesa 20.1.0-rc3` on `nouveau`.


## What does not work yet

|Area|State|
|-|-|
|Audio|**Silent.** VRSFML brings its own miniaudio, and miniaudio has no Switch backend, so it resolves to its null backend. Fixing it means writing a custom miniaudio backend over libnx `audout`. Note that SDL's Switch audio backend, which does exist, is *not* the answer: VRSFML builds SDL with `SDL_AUDIO OFF` and never routes audio through it.|
|Hardware|Runs, in title takeover mode. How well it plays there — frame rate, room transitions, long sessions — has not been measured yet.|
|Failed asset loads|A texture that fails to load returns null and the caller dereferences it. That should be a logged error rather than a crash, but which callers need hardening is not yet known — the log now names the asset, so the next hardware run says.|
|Debug scaffolding|Start-up traces are still compiled in. Harmless, but they are listed for removal in the status document.|


## Diagnostic builds

Two tiny projects build in seconds and run in seconds, against a ~117 MB engine build. When a
question can be phrased as "does this work on this driver at all", they are much faster than
instrumenting the engine — the context-sharing bug that blocked this port for a long time was
isolated by the second one in a single run.

```bat
build_switch.bat lab/switch_smoke     build_switch
build_switch.bat lab/switch_fbo_probe build_switch_fbo_probe
```

- **`lab/switch_smoke`** — EGL bring-up, a core-profile GL context, a clear colour and a
  shader compile. Proves the toolchain and driver are sound.
- **`lab/switch_fbo_probe`** — dumps the GL driver strings, then walks a matrix of framebuffer
  configurations printing `glCheckFramebufferStatus` for each, including the cross-context
  case.


## Working on the port itself

Only needed to *change* the SDL or VRSFML side. Building and running the game needs none of
this.

The Switch support in those two projects is carried as patches in this repository:

|Patch|What it holds|
|-|-|
|`patches/switch-sdl3-backend.patch`|The SDL 3 Switch platform: video, joystick and audio backends, plus the CMake platform detection|
|`patches/switch-vrsfml-backend.patch`|VRSFML platform gates — every change is a `#if` chain that had no Switch case|
|`patches/switch-lua-c89-numbers.patch`|Makes Lua's own `LUA_C89_NUMBERS` knob settable, which `luaconf.h` otherwise overwrites|

They are edited in working copies **outside** the repository, at the exact revisions the
patches were generated against:

```bat
powershell -File lab/switch_smoke/setup_working_copies.ps1
```

That clones `D:\deceptus\sdl3_switch` and `D:\deceptus\vrsfml_switch`, puts each on a
`switch-backend` branch and applies its patch. The trees are scratch space — about a gigabyte
of it — and deleting them costs nothing, because this script brings them back.

### The iteration loop

The engine builds against its **own** fetched copies under `build_switch_engine/_deps/`, so
editing a working copy alone changes nothing. Both halves of the round trip are scripted:

```bat
powershell -File lab/switch_smoke/sync_switch_patches.ps1
build_switch.bat
```

`sync_switch_patches.ps1` regenerates the patches from the working copies *and* mirrors the
changed files into `_deps/`. Both are required: CMake refuses to configure when a patch
neither applies nor reverse-applies, so a mirrored edit without a regenerated patch fails the
build before anything compiles.

### The CRLF trap

This is the one thing in the whole setup that will waste an afternoon.

The patches are LF. The trees under `_deps/` are cloned inside the Linux container and are LF.
The working copies sit on a Windows host with `core.autocrlf=true`, so everything git has
checked out there is **CRLF**. Copy a file verbatim from a working copy into `_deps/` and the
patch stops applying *inside the container* — while `git apply --check` still passes on the
host, which makes it look fine.

`sync_switch_patches.ps1` converts to LF on the way in, and `setup_working_copies.ps1` clones
with `core.autocrlf=false`, so following the scripts avoids it. If a fetched tree does need
resetting, **delete it and let CMake re-clone it in the container** rather than running
`git checkout` or `git clean` on it from Windows. `git checkout .` would not be enough anyway:
the patches add new files, which are untracked and would survive, and the patch would then
fail as "already exists".
