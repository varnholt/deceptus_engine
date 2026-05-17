
# Building the Deceptus Engine in Docker

The Docker setup compiles the engine inside an Arch Linux container and produces
a self-contained tarball (binary + all runtime `.so` files) that can be copied
directly onto another machine such as a Steam Deck.

---

## Quick start (automated)

The Python script handles image build, compilation, bundling, and tarball creation
in one step. Run it from the `docker/` directory using [uv](https://github.com/astral-sh/uv):

```bash
uv run python build_and_package.py
```

Output: `../build_output/deceptus_<timestamp>.tgz`

---

## Manual steps

### 1. Build the Docker image

**Linux / macOS**
```bash
docker build -t deceptus_engine .
```

**Windows**
```cmd
docker build -t deceptus_engine .
```

---

### 2. Run the container

**Linux / macOS**
```bash
docker run -it \
  -v "$(pwd)/build_output:/home/builder/output" \
  deceptus_engine
```

**Windows**
```cmd
docker run -it ^
  -v "%CD%\build_output:/home/builder/output" ^
  deceptus_engine
```

---

### 3. Bundle binary and dependencies

Inside the container, run from the build directory:

```bash
cd /home/builder/deceptus_engine/build
bash /path/to/bundle_deploy.sh [OPTIONS]
```

#### bundle_deploy.sh options

| Option | Default | Description |
|---|---|---|
| *(none)* | — | Bundle binary + libs, exclude render libs (recommended for Steam Deck) |
| `--no-strip-render-libs` | off | Also bundle GPU/render libs (libGL, libEGL, libvulkan, libOpenGL, libGLX, libGLdispatch, libdrm, libnvidia). Use only when the target system has no working Mesa or driver stack. |
| `--help` | — | Print usage and exit. |

**Default (Steam Deck and any system with Mesa):**
```bash
bash bundle_deploy.sh
```

**Include render libs (bare/embedded targets):**
```bash
bash bundle_deploy.sh --no-strip-render-libs
```

Output layout:
```
deploy/
  deceptus       — release binary
  lib/           — all bundled .so files (flat, symlinks resolved)
  run.sh         — launcher; sets LD_LIBRARY_PATH and execs the binary
```

---

### 4. Create the tarball

```bash
tar czf /home/builder/output/deceptus_build.tgz -C deploy .
```

---

## Deploying to Steam Deck

1. Copy the `.tgz` to the Steam Deck.
2. Extract it alongside the game data directory:
   ```bash
   mkdir -p ~/deceptus
   tar xzf deceptus_build.tgz -C ~/deceptus
   cp -r /path/to/data ~/deceptus/
   ```
3. Launch:
   ```bash
   ~/deceptus/run.sh
   ```

The `run.sh` wrapper sets `LD_LIBRARY_PATH` to the bundled `lib/` directory.
GPU/render libs are intentionally excluded from the bundle so the Steam Deck's
own Mesa driver stack is used.

---

## Dependency notes

| Dependency | Source | How bundled |
|---|---|---|
| SFML 3 | FetchContent (built from source) | Picked up by `ldd` via RPATH |
| SDL3 | FetchContent (built from source) | Picked up by `ldd` + explicit glob of `_deps/sdl3-build/libSDL3.so*` |
| Lua 5.4 | FetchContent (built as static lib) | Statically linked, no `.so` needed |
| GLEW | FetchContent | Picked up by `ldd` |
| System X11/audio/font libs | Arch Linux packages | Picked up by `ldd` |
| GL/EGL/Vulkan/DRM | Host system (stripped by default) | Not bundled — provided by target's GPU driver stack |
