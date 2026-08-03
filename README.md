# Deceptus Engine

[![Windows](https://github.com/varnholt/deceptus_engine/actions/workflows/windows.yml/badge.svg)](https://github.com/varnholt/deceptus_engine/actions/workflows/windows.yml)
[![Linux](https://github.com/varnholt/deceptus_engine/actions/workflows/linux.yml/badge.svg)](https://github.com/varnholt/deceptus_engine/actions/workflows/linux.yml)
[![macOS](https://github.com/varnholt/deceptus_engine/actions/workflows/macos.yml/badge.svg)](https://github.com/varnholt/deceptus_engine/actions/workflows/macos.yml)
[![WASM](https://github.com/varnholt/deceptus_engine/actions/workflows/wasm.yml/badge.svg)](https://github.com/varnholt/deceptus_engine/actions/workflows/wasm.yml)

A C++23/lua-based platformer game engine<br>
It utilizes Box2D for game physics, SFML for rendering, and SDL for game controller support.

It builds for Windows, Linux and macOS, and it also compiles to WebAssembly, so it runs in the
browser without a plugin.

### [▶ Play it in your browser on itch.io](https://deceptus.itch.io/deceptus)

![](doc/screenshots/screenshot.png)

<video src="doc/screenshots/gameplay.mp4" controls loop muted playsinline width="1280"></video>


# Documentation

The complete documentation lives in [doc/readme.md](doc/readme.md). The most travelled paths:

|Topic|Where|
|-|-|
|Designing a level|[designing_a_level.md](doc/level_design/designing_a_level.md)|
|Mechanisms, all 39 of them|[mechanisms.md](doc/level_design/mechanisms.md)|
|Enemies|[enemies.md](doc/level_design/enemies.md)|
|Visual effects, lighting and weather|[visual_effects.md](doc/level_design/visual_effects.md)|
|Writing your own enemies in Lua|[lua_interface/readme.md](doc/lua_interface/readme.md)|
|Cutscenes|[cutscene.md](data/scripts/cutscene.md)|
|Development hotkeys|[development_hotkeys.md](doc/development_hotkeys.md)|


# Credits

|What|Who|
|-|-|
|Artwork|dstar|
|Code|mueslee (Matthias Varnholt)|


# Get a Build

Every push to `master` is built for all four targets by GitHub Actions. Pick the latest run of
[your platform's workflow](https://github.com/varnholt/deceptus_engine/actions) and grab the
artifact at the bottom of the summary page. The Linux and macOS artifacts ship the shared
libraries next to the binary along with a `run.sh` that points the loader at them.


# How to Build

Only a compiler, CMake and the platform's development headers are needed. SFML 3, SDL 3,
Lua 5.4 and GLEW are downloaded and built by CMake via `FetchContent`; Box2D, ImGui, tinyxml2
and glm are vendored in the source tree.

The engine uses C++23, so the compiler has to be recent. CI builds with gcc 14, MSVC 2022,
Homebrew LLVM and the latest Emscripten. Anything older than gcc 13 or Clang 15 will not do.

## Windows
```bash
cmake -B build -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

## Linux
```bash
sudo apt-get install -y \
    gcc-14 g++-14 cmake ninja-build \
    libglm-dev \
    libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev \
    libxkbcommon-dev \
    mesa-common-dev libgl-dev libglvnd-dev \
    libasound2-dev libpulse-dev \
    libogg-dev libvorbis-dev libflac-dev libopenal-dev \
    libfreetype-dev libpng-dev \
    libudev-dev

cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=gcc-14 -DCMAKE_CXX_COMPILER=g++-14
cmake --build build --parallel
```

## macOS
```bash
brew install llvm glm ninja

cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

## Running

The game reads `data/` relative to the working directory, so run it from the repository root:

```bash
./build/deceptus            # Linux, macOS
build\Release\deceptus.exe  # Windows
```

## Web (WebAssembly)

The web build swaps vanilla SFML for [VRSFML](https://github.com/vittorioromeo/VRSFML) and
renders through WebGL 2. It uses pthreads for audio worklets, which means the page has to be
[cross-origin isolated](https://developer.mozilla.org/en-US/docs/Web/API/Window/crossOriginIsolated) —
opening the generated `.html` from `file://` will not work.

With the Emscripten SDK on `PATH`:
```bash
emcmake cmake -B build_wasm -DCMAKE_BUILD_TYPE=Release
cmake --build build_wasm --parallel
```

On Windows, `build_wasm.bat` does the same inside the official `emscripten/emsdk` Docker image,
so no local SDK is needed.

The link step produces `deceptus.html`, `deceptus.js`, `deceptus.wasm` and `deceptus.data`, the
last of which is the whole `data/` directory preloaded into the virtual file system. To play it
locally, `server_wasm.bat` starts a small server on
[localhost:9080](http://localhost:9080/deceptus.html) that sends the required `COOP`/`COEP`
headers.

`emscripten/` holds the hosting shell for the published build: `itch_index.html` is a
player-facing page that shows only the canvas, and `coi-serviceworker.js` establishes
cross-origin isolation on hosts that do not send the headers themselves, itch.io among them.


# Contribute and Talk to Us!
If you're a musician, graphic artist, level designer or programmer, or just want to hang out and chat, [please join us on Discord!](https://discord.gg/EZpkbGDaWD)


# License

The engine and its assets are released under
[Creative Commons Attribution-NonCommercial 4.0 International](LICENSE.md).
