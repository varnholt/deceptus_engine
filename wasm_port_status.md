# WASM Port Status — branch `wasm-vrsfml`

> Pick-up document for the next session. All context needed to continue is here.
> **Rule: every session must update this file before ending.**

---

## Goal

Build `deceptus_engine` targeting WebAssembly via Emscripten using
**VRSFML** (vittorio romeo's SFML rewrite) instead of stock SFML 2.
The result should run the game in a browser.

## Porting principles (owner's requirements)

1. **Full functionality** — NEVER remove desktop functionality. Every `#ifdef __EMSCRIPTEN__` guard must have a `#ifndef __EMSCRIPTEN__` (or `#else`) counterpart that preserves the original code unchanged. Adding a WASM path means adding a parallel alternative, not replacing or deleting the existing code. Even if WASM can't run a full deferred-lighting pass, the desktop path must keep its complete pipeline in the `#ifndef __EMSCRIPTEN__` branch.
2. **Minimal code changes** — touch only what the error requires. No refactoring, no cleanup beyond the fix.
3. **SFML2-compatible where possible** — prefer fixes that also compile unchanged on the `master` branch (SFML 2). Where a VRSFML API is fundamentally different (e.g. `applyView` removed, `Utf8String`) document why master compatibility is not possible for that particular change.

## Build command (Docker)

```
docker run --rm -v "D:/deceptus/deceptus_engine:/workspace" -w /workspace emscripten/emsdk bash -c "cd build_wasm && make -j4 2>&1 | tail -80"
```

---

## Key VRSFML API differences from SFML 2

| SFML 2 | VRSFML |
|--------|--------|
| `sf::Sprite::setPosition()` etc. | `position`, `origin`, `scale`, `rotation`, `textureRect`, `color` are **public members** |
| `textureRect = sf::IntRect(...)` | `textureRect = sf::FloatRect{...}` (FloatRect, float literals required) |
| `sf::Text text(font)` | `sf::Text text(font, data)` — 2-arg constructor |
| `text.setString(str)` | `text.setString(str.c_str())` — takes `Utf8String`; pass `const char*` for `std::string` |
| `sf::Font f` (default ctor) | No default ctor — `sf::Font::openFromFile(path).value()` |
| `sf::Texture t` (default ctor) | No default ctor — `std::make_shared<sf::Texture>(std::move(*sf::Texture::create(size)))` |
| `sf::Image` (default ctor) | No default ctor — use `std::optional<sf::Image>` for members |
| `sf::RenderTexture` (default ctor) | No default ctor — `auto rt = *sf::RenderTexture::create(size)` |
| `sf::Shader` (default ctor) | No default ctor — use `sf::Shader::loadFromFile(...)` factory |
| `sf::CircleShape` (default ctor) | `sf::CircleShape{sf::CircleShape::Data{.radius=r}}` |
| `sf::RectangleShape` (default ctor) | `sf::RectangleShape{sf::RectangleShape::Data{.size = {w, h}}}` |
| `window.setView(v)` / `getView()` | **Removed** — pass `states.view = sf::View::fromRect(...)` via `RenderStates` |
| `window.close()` / `isOpen()` | **Removed** |
| `pushGLStates()` / `popGLStates()` | **Removed** |
| `sf::VideoMode::getDesktopMode()` | `sf::VideoModeUtils::getDesktopMode()` |
| `sf::View(FloatRect)` | `sf::View::fromRect(sf::FloatRect{...})` |
| `shader.setUniform(string, ...)` | Takes `UniformLocation` — get via `shader.getUniformLocation("name")` |
| `sf::RenderStates(blend)` | `sf::RenderStates{.blendMode = sf::BlendAlpha}` aggregate |
| `sf::StencilMode{...}` | Designated initializers **required** |
| `draw(ptr, count, type, states)` | `draw(std::span<const sf::Vertex>{ptr, count}, type, states)` |
| `sprite.move(delta)` | `sprite.position += delta` |
| `sprite.getColor()` | `sprite.color` (direct member) |
| `rect.findIntersection(other)` | `sf::findIntersection(rect, other)` (free function) |
| `sf::base::Optional::has_value()` | `.hasValue()` or `operator bool()` |
| `sf::degrees(existingAngle)` | Assign `sf::Angle` directly — `sf::degrees(float)` creates from float |
| `sf::VertexArray` | **Not in VRSFML** — use compat shim in `src/wasm/SFML/Graphics.hpp` |
| `sf::Time::Zero` | `sf::Time{}` (value-initialized zero) |
| `sftr()` return type | Non-WASM: `sf::String`; WASM: `sf::Utf8String` — use `.c_str()` for manual `setString` calls with `std::string` |
| `std::string::fromUtf8(...)` | Bogus — was `sf::String::fromUtf8`. In WASM just use `str.c_str()` |
| `sf::AudioContext::create()` | WASM: stub in `src/wasm/SFML/Audio.hpp` |
| `sf::Shader::UniformLocation` members | Use `sf::base::Optional<sf::Shader::UniformLocation>` not `std::optional` |

---

## Shim files (`src/wasm/`)

| File | Purpose |
|------|---------|
| `SFML/Graphics.hpp` | Umbrella include + `VertexArray` compat (with `resize`) + `Drawable` compat |
| `SFML/System.hpp` | `Vec2`/`Vec3`/`Rect` aliases, `Vector3f` alias, `IntRect` compat struct |
| `SFML/Audio.hpp` | Umbrella include of real VRSFML audio headers (miniaudio backend; **no stubs**) |
| `SFML/Window.hpp` | Window compat |
| `migrate_sfml2_to_vrsfml.py` | Migration helper script |

Note: `SFML/System/String.hpp` is **not** present in the shim tree.
`sfmlstring.h` guards its include with `#ifndef __EMSCRIPTEN__`.

---

## What has been migrated (sessions 1–6, committed)

**Sessions 1–4 (~120 files):**
`ingamemenu/*`, `level.cpp/h`, `levelscript.cpp`, `luanode`, `rainoverlay`,
`stenciltilemap`, `room`, `tilemap`, `tilemapfactory`, `bouncer`, `bubblecube`,
`buttonrect`, `checkpoint`, `collapsingplatform`, `conveyorbelt`, `damagerect`,
`deathblock`, `destructibleblockingrect`, `dialogue`, `door`,
`lightsystem.h/cpp`, `fadetransitioneffect.cpp`, `spawneffect.cpp`,
`game.cpp`, `waterbubbles.cpp`, `layerdata.cpp`, `debugdraw.cpp`,
`cameras`, `logui`, `profilingui`, `audio`, `config`, animation files,
`eventserializer`, `lazytexture`, `meshtools`, `ambientocclusion`,
`infolayer`, `bitmapfont`, `controlleroverlay`, `parallaxlayer`, `rainoverlay`.

**Session 5:**
`dust.h/cpp`, `extra.cpp`, `fireflies.cpp`, `sfmlstring.h` (EMSCRIPTEN guard),
`imagelayer.cpp`, `infooverlay.cpp`, `interactionhelp.h/cpp`,
`gateway.h/cpp`, `laser.cpp`.

**Session 7 (this session — not yet committed):**
- `thirdparty/imgui/imgui-SFML.cpp` — fixed `RenderDrawLists` `#ifndef __EMSCRIPTEN__` structural bug: closing `}` was inside the `#ifndef` block, leaving the function unclosed and causing "function definition is not allowed here" for all subsequent functions; moved `#endif` before the function's closing `}`
- `src/menus/menuscreenvideo.cpp` — `sf::VideoMode::getDesktopMode()` → `sf::VideoModeUtils::getDesktopMode()` under `#ifdef __EMSCRIPTEN__` with matching includes
- `src/opengl/glslprogram.cpp` — `GL_DOUBLE` case guarded with `#ifndef __EMSCRIPTEN__` (WebGL has no double type)
- `src/opengl/render3d/menubackgroundscene.cpp` — `position`/`scale` public member access on Camera3D/TexturedObject → `setPosition()`/`setScale()`/`getPosition()` accessors
- `src/opengl/render3d/texturedobject.cpp` — `sf::Image()` default ctor → `sf::Image::loadFromFile()` factory under `#ifdef __EMSCRIPTEN__`
- `src/game/level/level.cpp` — fixed unclosed `#ifndef __EMSCRIPTEN__` in `createViews()` that excluded all function definitions from `updateViews()` through `zoomReset()` (lines 753–879); added `#endif` before `createViews()` closing brace; fixed stray `#endif` inside `drawLightMap()`; guarded `_render_targets.lighting->setView()` with `#ifndef __EMSCRIPTEN__`; used `sf::View::fromRect()` in `updateViews()` under `#ifdef __EMSCRIPTEN__`
- `src/game/debug/logui.h` — wrapped `LogUi` class declaration in `#ifndef __EMSCRIPTEN__` (implementations already guarded in .cpp)
- `src/game/game.h` — guarded `#include "game/debug/logui.h"` and `std::unique_ptr<LogUi> _log_ui` member in `#ifndef __EMSCRIPTEN__`
- `src/game/game.cpp` — guarded all four `_log_ui` usage sites with `#ifndef __EMSCRIPTEN__`

**Build result: `[100%] Built target deceptus` — zero errors.**

**Session 6 (previous session):**
- `src/game/items/itemlantern.cpp` — `sf::Time::Zero` → `sf::Time{}` (7 sites), `sf::degrees(Angle)` → direct assignment, shader/dust block guarded with `#ifndef __EMSCRIPTEN__`
- `src/game/items/itemlantern.h` — `std::optional<sf::Shader::UniformLocation>` → `sf::base::Optional<sf::Shader::UniformLocation>` (10 members)
- `src/game/mechanisms/interactionhelp.cpp` — removed spurious `.c_str()` on `sftr()` result (now `sf::Utf8String`, no `.c_str()` needed)
- `src/menus/menu.cpp` — `sf::View(FloatRect)` → `sf::View::fromRect`, `window.setView` → `states.view =`
- `src/menus/menuscreen.cpp` — `std::make_shared<sf::Texture>(size)` → factory
- `src/framework/tools/sfmlstring.h` — WASM branch now returns `sf::Utf8String` (safe for UTF-8/CJK); non-WASM unchanged
- `src/menus/menuscreenmain.cpp` — `sf::Text` 2-arg ctor, `sf::RenderTexture` factory, `fromUtf8` → `.c_str()`, build string via `tr()` + `.c_str()`
- `src/menus/menuscreenachievements.cpp` — `sf::Text` 2-arg ctor (bulk sed)
- `src/menus/menuscreenaudio.cpp` — `sf::Text` 2-arg ctor (bulk sed)
- `src/menus/menuscreencontrols.h` — `sf::RectangleShape _cursor_highlight` → Data init
- `src/menus/menuscreencontrols.cpp` — `sf::Text` 2-arg ctor, `fromUtf8` → `.c_str()`, `binding_name.c_str()`
- `src/menus/menuscreenfileselect.cpp` — `sf::Text` 2-arg ctor, `_name.c_str()`
- `src/menus/menuscreengame.cpp` — `sf::Text` 2-arg ctor (bulk sed)
- `src/menus/menuscreenoptions.cpp` — `sf::Text` 2-arg ctor, lambda `const std::string&` → `const auto&`
- `src/menus/menuscreenpause.cpp` — `sf::Text` 2-arg ctor (bulk sed)
- `src/menus/menuscreennameselect.cpp` — `sf::Text` 2-arg ctor, `_name.c_str()`
- `src/menus/menuscreenvideo.cpp` — `sf::Text` 2-arg ctor, `sf::VideoMode::getDesktopMode()` → `sf::VideoModeUtils::getDesktopMode()`, `display_mode_strings` → `sf::Utf8String[]`, `std::format(...).c_str()`
- `src/menus/menuscreencredits.cpp` — `sf::Text` 2-arg ctor (bulk sed)
- `src/wasm/SFML/Audio.hpp` — added `AudioContext` stub

---

**Session 8 (committed):**
- `src/game/game.cpp` — unguarded the render-texture→window compositing block; guarded `_window->popGLStates()`; `sf::RenderStates{.blendMode = sf::BlendAlpha}` for WASM
- `src/framework/tools/logthread.cpp` — all methods stubbed for WASM: no thread, no file I/O
- `build_wasm/_deps/sfml-src/include/SFML/Graphics/DefaultShader.hpp` — removed `layout(location = N)` from uniform declarations (WebGL 2 only allows it on `in`/`out`)
- `CMakeLists.txt` — added `-sASSERTIONS=1`, `-sEXPORTED_RUNTIME_METHODS`, `-sASYNCIFY=1`

**Session 10 (this session — not yet committed):**
- `lab/wasm_browser_test/` — new Selenium/pytest smoke test suite; auto-starts HTTP server, captures browser console errors, filters favicon 404 noise, separate tests for SEVERE/GL/memory errors; run with `lab/wasm_browser_test/run.bat`
- `src/game/game.cpp` — `initializeWindow()`: added `#ifdef __EMSCRIPTEN__` branch to create `_window_render_texture` using VRSFML factory (`sf::RenderTexture::create()`); moved `_render_targets.create()` and log line outside the `#ifndef __EMSCRIPTEN__` guard (both work in WASM)
- `src/game/controller/gamecontrollerdetection.cpp` — `start()` and `stop()` guarded with `#ifndef __EMSCRIPTEN__`; SDL controller hotplug thread cannot run in single-threaded WASM (events arrive via main loop anyway)
- `CMakeLists.txt` — added `-sNO_DISABLE_EXCEPTION_CATCHING` temporarily for debugging; **removed after identifying root cause**
- `src/game/game.h` — `_test_scene` and `_menu_background` members wrapped in `#ifndef __EMSCRIPTEN__`
- `src/game/game.cpp` — all 7 usage sites of `_test_scene`/`_menu_background` guarded: construction in `initialize()`, VSync callback, draw loop, update loop, `changeResolution()`; `toggleFullScreen()` body was already fully guarded

## Current status (session 13 end)

**Build: PASSING — `[100%] Built target deceptus`**
**Selenium test: PASSING — all 4 tests pass (no SEVERE errors, no GL errors, no memory errors, canvas not black)**
**Canvas: RENDERING — 90,076 non-black pixels (logo + text at correct 2x scale)**
**FPS: 60fps in browser (headless Selenium shows ~25fps — expected, no real display)**

**Runtime errors fixed (browser console, in order across all sessions):**
1. `thread constructor failed` (error 138) — `LogThread` `std::thread` stubbed
2. `layout(location) : invalid layout qualifier` — patched `DefaultShader.hpp` uniforms
3. `'HEAPU32' was not exported` — added `EXPORTED_RUNTIME_METHODS`
4. `emscripten_sleep without async support` — added `-sASYNCIFY=1`
5. `memory access out of bounds` — `_window_render_texture` was null in WASM (creation was inside `#ifndef __EMSCRIPTEN__`); fixed with VRSFML factory branch
6. `thread constructor failed` (second source) — `GameControllerDetection::start()` spawned SDL thread; guarded with `#ifndef __EMSCRIPTEN__`
7. `shader compilation failed: layout syntax error` — `data/shaders/texture.vs/fs` uses GLSL 4.30 with `layout(binding=N)` samplers (desktop only); `MenuBackgroundScene` and `ForestScene` guarded with `#ifndef __EMSCRIPTEN__`

**Session 10 continued — black screen fix:**
- `src/game/game.cpp` — `draw()`: VRSFML `sf::Sprite` does **not** store a texture pointer; texture is passed at draw time via `sf::RenderStates::texture` (same as original code). But `textureRect` defaults to `{0,0,0,0}` → zero-size quad → nothing drawn. Added `#ifdef __EMSCRIPTEN__` block to set `window_texture_sprite.textureRect` to the full render-texture size before the compositing blit.

**VRSFML Sprite API note (add to API differences table):**
| SFML 2 | VRSFML |
|--------|--------|
| `sf::Sprite` stores a `const sf::Texture*` set via constructor or `setTexture()` | No texture stored — pass via `sf::RenderStates::texture` at draw time; must set `textureRect` manually (no auto-sync from texture size) |

**What runs in WASM at session 10 end:**
- Full initialization (localization, window, render textures, render targets, player, menus)
- Main menu and game loop running; compositing blit to window working
- Audio stubs silent (expected — audio is stubbed for WASM)
- 3D menu background absent (desktop-only GLSL 4.30 shaders — visual only)

**Session 12 — black canvas root cause found and fixed:**

Session 8 removed `layout(location = N)` from GLSL uniform declarations in `DefaultShader.hpp` to comply with GLSL ES 3.00 (WebGL 2 does not allow explicit uniform locations for non-`in`/`out` variables). This was correct. However, VRSFML's `setupDrawMVP()` and `setupDrawTexture()` still uploaded to hardcoded locations 0, 1, and 3:
- `glUniform3fv(0u, ...)` for `sf_u_mvpRow0`
- `glUniform3fv(1u, ...)` for `sf_u_mvpRow1`
- `glUniform2f(3u, ...)` for `sf_u_invTextureSize`

Without explicit layout qualifiers the WebGL driver assigns locations arbitrarily. The MVP uniforms stayed at their default (0,0,0), so `gl_Position = vec4(0,0,0,1)` for all vertices → degenerate triangles → nothing visible.

**Fix (4 VRSFML files in `build_wasm/_deps/sfml-src/`):**
- `include/SFML/Graphics/Priv/ShaderBase.hpp` — changed 3 `bool m_hasBuiltInUniform*` members to `int m_builtInUniformLocation*` that store the actual GL location (or -1 if absent)
- `src/SFML/Graphics/Shader.cpp` — constructor now stores `glGetUniformLocation(...)` value directly instead of `!= -1`
- `include/SFML/Graphics/RenderTarget.hpp` — `setupDrawMVP` and `setupDrawTexture` parameters changed from `bool upload*` to `int location*`
- `src/SFML/Graphics/RenderTarget.cpp` — `setupDrawMVP` and `setupDrawTexture` pass stored locations to `glUniform*fv`, guarded with `!= -1`

**Result:** `[100%] Built target deceptus`, all 4 Selenium tests pass, canvas has 24,342 non-black pixels.

**Session 11 — canvas still black, investigation:**
The Selenium screenshot test (4th test) passed because the browser showed a white page (no canvas element found), not because game content was visible. The canvas is still blank/black.

Root cause narrowed: **`glDrawArrays()` via VRSFML's auto-batch system produces no visible output on the WebGL canvas**, while `glClear()` works correctly (colored background was confirmed in earlier diagnostic). This rules out framebuffer binding as the cause.

Diagnostics performed:
- `_window->setActive(true)` added before compositing draw — calls `glBindFramebuffer(GL_FRAMEBUFFER, m_defaultFrameBuffer)` — no change.
- Green fill of `_window_render_texture` before `display()` — still only 669 white pixels in screenshot, confirming `_window->draw()` produces no output regardless of texture content.
- Direct `sf::RectangleShape` (640×360, default WHITE fill) drawn directly to `_window` — also invisible. This confirms the issue is NOT in the compositing path but in VRSFML's `glDrawArrays()`-based batch flush itself.

**Current state of `src/game/game.cpp`:** clean — only `_window->setActive(true)` added in the `#ifdef __EMSCRIPTEN__` compositing block. No diagnostic code remaining.

**Root cause candidates:**
1. VRSFML's VAO/VBO setup doesn't survive WebGL context constraints (VAO must be bound when `glDrawArrays()` is called; VRSFML calls `VertexBuffer::unbind()` in `resetGLStatesImpl()` which may zero the VAO binding)
2. MVP uniforms (`sf_u_mvpRow0`, `sf_u_mvpRow1`) not uploaded before `flush()` — all vertices land at clip-space origin → degenerate triangle
3. Default shader fails to compile silently (no GL errors reported by tests, but compilation could fail without console output)
4. `WindowContext::setActiveThreadLocalGlContext()` early-returns due to same GL context ID in Emscripten → `RenderTarget::setActive()` short-circuits → `resetGLStates()` never called → VBO/VAO never initialized

**Session 13 — view/viewport scaling bug — FIXED:**

Root cause: the original desktop code (SFML 3) used `window.setView(view)` which persists globally on the render target. The WASM port changed `menu.cpp` to `states.view = sf::View::fromRect(...)` (per-draw-call, VRSFML API), but sub-draw calls in several files dropped `states` and used `computeView()` (1:1 pixel scale on the 1280×720 render texture).

**Three fixes applied:**

1. **`src/menus/menuscreenmain.cpp`** — `draw_all_text` lambda: added `const sf::RenderStates& drawStates` parameter and forwarded to all `target.draw()` calls. Both call sites (`draw_all_text(temp_texture, states)` and `draw_all_text(window, states)`) updated. Fade-in composite sprite draw changed to use `sf::RenderStates{.blendMode = sf::BlendAlpha}` without the 640×360 view (the temp texture is already rendered at 2x; compositing must be 1:1).

2. **`src/menus/menuscreencredits.cpp`** — `window.draw(*_text_code)` and `window.draw(*_text_artwork)` → both now pass `states`.

3. **`src/framework/image/layer.cpp`** — `Layer::draw` had `/*states*/` (parameter dropped) and built a fresh `sf::RenderStates{.texture=..., .blendMode=...}` with no view. Changed to use the incoming `states`, set `.texture` and `.blendMode` on it, and forward to `target.draw(*_sprite, states)`.

**Result:** Selenium canvas non-black pixels jumped from 24,587 → 90,076. Menu logo and text both render at 2x scale, filling the full 1280×720 canvas correctly.

**Session 13 — 60fps fix — FIXED:**

Game ran at 30fps. Root cause: `Window::display()` in VRSFML calls `sfml_yield_to_raf()` when vsync is enabled — a second `requestAnimationFrame` yield from inside the already-RAF-driven `emscripten_set_main_loop_arg` callback. This doubled the frame interval to ~33ms → 30fps. `setFramerateLimit(60)` added its own `ThisThread::sleepFor(~16ms)` on top.

**Fix:** Wrapped `_window->setVerticalSyncEnabled(...)` and `_window->setFramerateLimit(60)` in `#ifndef __EMSCRIPTEN__` at both call sites in `src/game/game.cpp` (lines ~237 and ~1062). Under WASM, frame timing is driven by `emscripten_set_main_loop_arg` with `fps=0` (requestAnimationFrame) — no additional throttling needed.

**Result:** Game runs at 60fps in browser. All 4 Selenium tests pass.

**CMakeLists.txt Emscripten link flags (current):**
```cmake
target_link_options(deceptus PRIVATE
    -sUSE_WEBGL2=1
    -sMIN_WEBGL_VERSION=2
    -sMAX_WEBGL_VERSION=2
    -sFULL_ES3=1
    -sALLOW_MEMORY_GROWTH=1
    -sINITIAL_MEMORY=67108864
    -sEXIT_RUNTIME=0
    -sASYNCIFY=1
    -sASSERTIONS=1
    "-sEXPORTED_RUNTIME_METHODS=['HEAPU32','HEAPU8','HEAP32','HEAPF32','HEAPU16']"
    --preload-file ${CMAKE_SOURCE_DIR}/data@data
)
```

**Rebuild command (if CMakeLists.txt changes again):**
```
docker run --rm -v "D:/deceptus/deceptus_engine:/workspace" -w /workspace emscripten/emsdk bash -c "cd build_wasm && cmake .. -DCMAKE_TOOLCHAIN_FILE=/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake 2>&1 | tail -3 && make -j4 2>&1 | grep -E 'error:|Built target deceptus' | head -20"
```

**Note:** `-sASSERTIONS=1` adds overhead — remove it once the game runs successfully.

**Session 14 — level load crash — FIXED:**

Two root causes identified from browser crash log:

**Cause 1 — Threads:** WASM is single-threaded; `std::jthread`/`std::thread` construction throws error 138 → `Aborted`.
- `src/game/io/lazytexture.cpp` (`loadTexture()`): Wrapped `std::jthread` in `#ifndef __EMSCRIPTEN__`. Under WASM, loads synchronously: `sf::Image::loadFromFile()` + `sf::Texture::loadFromImage()` inline, then `_loading.clear()`.
- `src/game/level/level.cpp` (file watcher setup ~line 491 and destructor join ~line 228): Both wrapped in `#ifndef __EMSCRIPTEN__`. File watching is a desktop-only dev feature.

**Cause 2 — Shaders:** VRSFML prepends `#version 300 es\n\nprecision highp float;\n\n#line 1\n` to all shaders. All game shaders used GLSL 1.x syntax (`varying`, `gl_TexCoord[0]`, `texture2D()`, `gl_FragColor`, fixed-function MVP built-ins) which is illegal in GLSL ES 3.00.

**All shader files updated to GLSL ES 3.00:**
- Vertex shaders (`fog.vert`, `ring.vert`, `waterfall.vert`): Rewritten as VRSFML passthrough using `sf_a_position`/`sf_a_color`/`sf_a_texCoord` attributes and `sf_u_mvpRow0`/`sf_u_mvpRow1`/`sf_u_invTextureSize` uniforms.
- `stencil_write.vert`: Rewritten as VRSFML passthrough, outputs `interpolated_uv = sf_a_texCoord * sf_u_invTextureSize`.
- `death.vert`: Rewritten — `gl_Vertex.y -= time*128` → displace `sf_a_position.y` before MVP transform.
- Fragment shaders (`fog.frag`, `ring.frag`, `waterfall.frag`, `stencil_write.frag`, `flash.frag`, `water.frag`, `blur.frag`, `brightness.frag`, `death.frag`, `light.frag`, `light_noise.frag`, `void_standalone.frag`): Removed `#version` directives, replaced `varying`→`in`, `texture2D()`→`texture()`, `gl_TexCoord[0].xy`→`sf_v_texCoord`, `gl_Color`→`sf_v_color`, `gl_FragColor`→`sf_fragColor`, added `layout(location=0) out vec4 sf_fragColor;`.
- Uniform `texture` renamed to `u_texture` in `flash.frag`, `blur.frag`, `brightness.frag`, `light_noise.frag` to avoid shadowing the `texture()` built-in function in GLSL ES 3.00.

**C++ updates for uniform rename (`"texture"` → `"u_texture"`):**
- `src/game/level/luanode.cpp:138`
- `src/game/mechanisms/destructibleblockingrect.cpp:107`
- `src/game/shaders/blurshader.cpp:26`
- `src/game/shaders/gammashader.cpp:24`
- `src/game/effects/lightsystem.cpp:450`

**VRSFML attribute/uniform reference:**
```glsl
// Vertex shader — declare these to get VRSFML's auto-set MVP:
uniform vec3 sf_u_mvpRow0;   // VRSFML sets via glGetUniformLocation("sf_u_mvpRow0")
uniform vec3 sf_u_mvpRow1;
uniform vec2 sf_u_invTextureSize;
layout(location = 0) in vec2 sf_a_position;  // hardcoded locations
layout(location = 1) in vec4 sf_a_color;
layout(location = 2) in vec2 sf_a_texCoord;  // NOT normalized — multiply by sf_u_invTextureSize
out vec4 sf_v_color;
out vec2 sf_v_texCoord;
// MVP transform pattern:
vec3 pos = vec3(sf_a_position, 1.0);
gl_Position = vec4(dot(sf_u_mvpRow0, pos), dot(sf_u_mvpRow1, pos), 0.0, 1.0);

// Fragment shader (fragment-only: default vertex provides sf_v_texCoord, sf_v_color):
in vec4 sf_v_color;
in vec2 sf_v_texCoord;   // already normalized 0..1
layout(location = 0) out vec4 sf_fragColor;
```

---

**Session 15 — black canvas in level (deferred pipeline was `#ifndef __EMSCRIPTEN__` guarded) — FIXED:**

**Root cause:** The entire deferred rendering pipeline in `Level::draw()` — atmosphere distortion, glow, light map, deferred shading, gamma — was inside `#ifndef __EMSCRIPTEN__`. In WASM only the background layer draws to `level_background`, but nothing ever composited into `window`. Canvas stayed black after level load.

**Fix: `src/game/level/level.cpp` — full deferred pipeline now runs on both platforms:**

Platform differences handled with targeted `#ifdef`/`#ifndef` guards:
- **Atmosphere distortion** (`#ifndef __EMSCRIPTEN__`): Desktop uses `_atmosphere_shader`; WASM blits `level_background` directly to `level` (and `normal_tmp` to `normal`) via `sf::RenderStates{.texture = ...}` with explicit `textureRect`.
- **Glow** (`#ifndef __EMSCRIPTEN__`): `drawGlowSprite()` desktop-only (requires atmosphere shader).
- **`drawLightMap()`**: `_light_system->draw(*lighting, *lighting2, states)` uses `sf::RenderStates{.view = *_level_view}` in WASM; `{}` on desktop.
- **`drawParallaxMaps()`**: Desktop uses `target.setView(parallax->_view)` + `target.draw(*tile_map)`. WASM uses `target.draw(*tile_map, sf::RenderStates{.view = parallax->_view})`. View restore guard removed for WASM (no `setView`).
- **`drawLayers()` tilemap draws**: WASM passes `sf::RenderStates{.view = *_level_view}`; desktop passes `{}`.
- **Debug draws / screenshots** (`#ifndef __EMSCRIPTEN__`): `_light_system->drawDebug`, `drawDebugInformation`, all `takeScreenshot` calls.
- **Final gamma blit**: `level_texture_sprite.textureRect` explicitly set in `#ifdef __EMSCRIPTEN__` (VRSFML requires manual textureRect).
- **`sf::Vector2f{Vector2u}` fix**: VRSFML has no implicit Vec2u→Vec2f conversion. Fixed by using explicit `{static_cast<float>(size.x), static_cast<float>(size.y)}` in two places.

**Build result: `[100%] Built target deceptus` — zero errors.**

**Session 16 — 1/4-screen scale + player/sprites invisible — FIXED:**

**Fix 1 — 1/4-screen scale** (`src/game/level/level.cpp`): On desktop, the final `window->draw(level_texture_sprite)` call used `level_view` set on the window render texture via `setView()`, so `view_to_texture_scale` mapped correctly. In WASM without `setView()`, the deferred texture (1280×720) was drawn at scale 0.5 → 640×360 in the top-left quarter. Fix: guard the `scale = {view_to_texture_scale, ...}` assignment with `#ifndef __EMSCRIPTEN__`. WASM defaults to scale `{1.0, 1.0}` so the full-size deferred texture fills the window render texture.

**Fix 2 — view propagation for all game objects** (`src/game/mechanisms/gamemechanism.h/.cpp`, `level.h/.cpp`, `player.h/.cpp`, `imagelayer.h`, `luanode.h`):
- Added new virtual `GameMechanism::draw(target, normal, const sf::RenderStates& states)` with default implementation that calls the 2-arg version. Existing mechanisms compile unchanged via the fallback; the view is silently ignored for mechanisms not yet updated.
- `drawMechanismsAtZ()` now accepts and passes `const sf::RenderStates& states = {}` to mechanism draws.
- `drawLayers()`, `drawPostLightingLayers()`, `drawOverlayLayers()` each define a local `level_view_states`:
  - Desktop (`#ifndef __EMSCRIPTEN__`): `sf::RenderStates{}` (view applied via persistent `setView()` already)
  - WASM (`#else`): `sf::RenderStates{.view = *_level_view}`
- All draw callsites (tilemaps, mechanisms, enemies, player, image layers) receive this states.
- `Player::draw()` updated to accept `const sf::RenderStates& states = {}` and forward to all `Animation::draw()` calls (wallslide, current_cycle, aux_cycle). `Animation::draw()` already accepted states and propagates `.view` through `target.draw(_vertices, ..., states)`.
- `LuaNode` and `ImageLayer`: added `using GameMechanism::draw;` to un-hide the inherited 3-arg virtual (C++ name-hiding fix).
- `drawPlayer()` in level.cpp updated to accept and pass states.

**Build: `[100%] Built target deceptus` — zero errors.**

---

**Session 17 — LuaNode draw 3-arg + sprite texture — FIXED:**

**Fix 1 — LuaNode 3-arg draw override** (`src/game/level/luanode.h/.cpp`):
- Added `void draw(sf::RenderTarget&, sf::RenderTarget&, const sf::RenderStates& states) override;` to luanode.h.
- Redirected `draw(target, normal)` to `draw(target, normal, {})` so the 3-arg contains the actual logic.
- Sprite draws now use `sf::RenderStates sprite_states = states; sprite_states.texture = _texture.get();` so the level view is forwarded and texture is set.
- Flash shader case: `sprite_states.shader = &(*_flash_shader)` added on top.

**Fix 2 — LuaNode sprite texture (white rectangle bug)**:
- Root cause: `addSprite()` in VRSFML branch creates `sf::Sprite()` with no texture (VRSFML sprites don't store textures). `_texture` was loaded but never passed to draw calls.
- Fix: `sprite_states.texture = _texture.get()` in the sprite draw loop. On desktop, SFML2 ignores `states.texture` (sprite's own texture wins), so this is safe cross-platform.

**Known remaining draw issues:**
- Mechanisms that override `draw(target, normal)` but not the 3-arg version still draw without view (wrong position in WASM). Fix: add `using GameMechanism::draw;` to each such mechanism header, then update its draw implementation.
- `PlayerStencil::draw()`, `AnimationPlayer::draw()`, `_level_script.draw()`, `Gun::drawProjectileHitAnimations()` — these draws in `Level::draw()` also lack view.

---

**Session 18 — WASM audio migration — CODE COMPLETE, build result pending:**

**Goal:** Enable real audio in WASM using VRSFML's miniaudio WebAudio backend.

**Context:** Desktop (`build_deb`) and WASM (`build_wasm`) both use `GIT_TAG master` VRSFML but were fetched at different times. The WASM fetch is NEWER and has breaking Audio API changes. The migration targets the NEW API (in `build_wasm/_deps/sfml-src/include/SFML/Audio/`).

**CMakeLists.txt already changed (committed? check):**
- Removed `set(SFML_BUILD_AUDIO OFF ...)` from Emscripten block → audio module now builds for WASM.
- Added `target_link_libraries(deceptus PRIVATE SFML::Audio)` unconditionally (desktop already had it via the `if(NOT EMSCRIPTEN)` block — that block now only has `libglew_static`).
- Added `-sAUDIO_WORKLET=1 -sWASM_WORKERS=1` to Emscripten link options (required for miniaudio WebAudio backend).

**src/wasm/SFML/Audio.hpp already changed:**
- Old: WASM stubs for SoundBuffer/Sound/Music/Listener/AudioContext.
- New: includes individual VRSFML audio headers directly (no umbrella `SFML/Audio.hpp` in the WASM VRSFML version):
```cpp
#include <SFML/Audio/AudioContext.hpp>
#include <SFML/Audio/InputSoundFile.hpp>
#include <SFML/Audio/Listener.hpp>
#include <SFML/Audio/Music.hpp>
#include <SFML/Audio/MusicReader.hpp>
#include <SFML/Audio/OutputSoundFile.hpp>
#include <SFML/Audio/PlaybackDevice.hpp>
#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Audio/SoundBufferRecorder.hpp>
#include <SFML/Audio/SoundFileFactory.hpp>
#include <SFML/Audio/SoundFileReader.hpp>
#include <SFML/Audio/SoundFileWriter.hpp>
#include <SFML/Audio/SoundRecorder.hpp>
#include <SFML/Audio/SoundStream.hpp>
```

**New VRSFML Audio API (compared to old SFML2/older VRSFML):**

| Old | New |
|-----|-----|
| `sf::SoundBuffer buf; buf.loadFromFile(std::string)` | `auto buf = sf::SoundBuffer::loadFromFile(sf::Path{path})` → returns `sf::base::Optional<sf::SoundBuffer>` |
| `sf::Sound sound(buffer)` | `sf::Sound sound(playbackDevice, buffer)` — needs `sf::PlaybackDevice&` |
| `sound.getStatus() == sf::Sound::Status::Stopped` | `!sound.isPlaying()` (from `MiniaudioSoundSource`) |
| `sound.setVolume(0..100)` | `sound.setVolume(0..1)` — scale changed! |
| `sound.position = sf::Vector3f{x,y,z}` | `sound.setPosition(sf::Vec3f{x,y,z})` |
| `sf::Music music; music.openFromFile(path)` | `auto reader = sf::MusicReader::openFromFile(path).value(); sf::Music music(device, reader);` |
| `music.getStatus() == sf::SoundStream::Status::Stopped` | `!music.isPlaying()` |
| `music.setLoop(bool)` | `music.setLooping(bool)` |
| `sf::Listener::setPosition(Vec3f)` | `sf::Listener` is now a struct; use `playbackDevice.applyListener(listener)` |
| `sf::Vector3f` / `sf::Vector2f` | Still valid (aliased in `src/wasm/SFML/System.hpp`) |
| `sf::Path` from `std::string` | `sf::Path{some_std_string}` — wraps `std::filesystem::path`, accepts string-like |

**Key constraints:**
- `sf::PlaybackDevice` is **non-copyable and non-movable** → store as `std::unique_ptr<sf::PlaybackDevice>` in `Audio`.
- `sf::Sound` is **non-copyable and non-movable** → `std::unique_ptr<sf::Sound>` still fine (pointer is movable).
- `sf::SoundBuffer` IS copyable+movable → store by value in `unordered_map<string, SoundBuffer>`.
- `sf::MusicReader` is movable (not copyable) → store as value or `unique_ptr`.
- `sf::Music` is non-copyable and non-movable → store as `unique_ptr<Music>`.
- `PlaybackDevice` must outlive all `Sound` and `Music` objects bound to it.
- `AudioContext` must exist before `PlaybackDevice` is created.

**`Audio` class changes needed (`src/game/audio/audio.h`):**
1. Add `std::unique_ptr<sf::PlaybackDevice> _playback_device;` member.
2. Change `_sound_buffers` from `unordered_map<string, shared_ptr<sf::SoundBuffer>>` to `unordered_map<string, sf::SoundBuffer>` (value type).
3. Change `PlayInfo::_pos` from `optional<sf::Vector3f>` to `optional<sf::Vec3f>` (or keep Vector3f — it's aliased).
4. Change `loadFile` return type from `shared_ptr<sf::SoundBuffer>` to `sf::base::Optional<sf::SoundBuffer>`.
5. The `SoundThread::_sound` stays as `unique_ptr<sf::Sound>` — just the constructor changes.

**`Audio::Audio()` changes (`src/game/audio/audio.cpp`):**
```cpp
Audio::Audio()
{
   auto handle = sf::AudioContext::getDefaultPlaybackDeviceHandle();
   if (handle.hasValue())
   {
      _playback_device = std::make_unique<sf::PlaybackDevice>(*handle);
   }
   initializeSamples();
}
```

**`Audio::loadFile()` changes:**
```cpp
sf::base::Optional<sf::SoundBuffer> Audio::loadFile(const std::string& filename)
{
   const std::string full_path = sfx_path + filename;
   if (!std::filesystem::exists(full_path))
   {
      Log::Error() << "audio file does not exist: " << filename;
      return sf::base::nullOpt;
   }
   auto buffer = sf::SoundBuffer::loadFromFile(sf::Path{full_path});
   if (!buffer.hasValue())
   {
      Log::Error() << "unable to load file: " << filename;
      return sf::base::nullOpt;
   }
   return buffer;
}
```

**`Audio::addSample()` changes:**
```cpp
void Audio::addSample(const std::string& sample)
{
   std::lock_guard<std::mutex> lock(_mutex);
   if (_sound_buffers.find(sample) != _sound_buffers.end())
   {
      return;
   }
   auto buffer = loadFile(sample);
   if (buffer.hasValue())
   {
      _sound_buffers.emplace(sample, std::move(*buffer));
   }
}
```

**`Audio::playSample()` key changes:**
- Status check: `thread._sound == nullptr || !thread._sound->isPlaying()`
- Sound construction: `thread_it->_sound = std::make_unique<sf::Sound>(*_playback_device, *it->second)`
  - But `it->second` is now `sf::SoundBuffer` (not `shared_ptr`) → `*it->second` → just `it->second`
  - So: `std::make_unique<sf::Sound>(*_playback_device, it->second)`
- Note: `sf::Sound` stores a reference to the buffer → buffer must stay alive. Since we store by value in `_sound_buffers`, as long as `addSample` never erases entries, this is safe.
- Position: `sf::Vec3f{pos.x, pos.y, pos.z}` passed via `AudioSettings` or `setPosition()`.
- IMPORTANT: no `thread_it->_sound->setBuffer(...)` on reuse — create fresh `Sound` each time (or reuse same sound if buffer matches, but simplest: always create new).

**`SoundThread::setVolume()` change:**
```cpp
void Audio::SoundThread::setVolume(float volume)
{
   const auto master = GameConfiguration::getInstance()._audio_volume_master * 0.01f;
   const auto sfx = GameConfiguration::getInstance()._audio_volume_sfx * 0.01f;
   _sound->setVolume(master * sfx * volume);   // ← remove * 100.0f — VRSFML uses 0..1
}
```

**`SoundThread::setPosition()` change:**
```cpp
void Audio::SoundThread::setPosition(const sf::Vector2f& pos)
{
   _sound->setPosition(sf::Vec3f{pos.x, pos.y, 0.0f});  // setPosition() not .position member
}
```

**`musicplayer.cpp` changes (`src/game/audio/musicplayer.cpp`):**
- `sf::Music` member: change from value to `std::unique_ptr<sf::Music>`.
- `sf::MusicReader` member: add `std::unique_ptr<sf::MusicReader>` (one per track, loaded at init time or on open).
- `openFromFile` call: `auto reader = sf::MusicReader::openFromFile(sf::Path{path}); _music_reader = std::make_unique<sf::MusicReader>(std::move(*reader)); _music = std::make_unique<sf::Music>(*playbackDevice, *_music_reader);`
- Status: `_music->getStatus() == sf::SoundStream::Status::Stopped` → `!_music->isPlaying()`
- `setLoop(bool)` → `setLooping(bool)`
- `MusicPlayer` needs a reference to `PlaybackDevice` — either inject it or get from `Audio::getInstance()`.

**Session 18 — what was actually done:**

All five files migrated and clang-formatted:
- `src/wasm/SFML/Audio.hpp` — added `MusicReader.hpp` include
- `src/game/audio/audio.h` — added `<SFML/System.hpp>`, `<string>`, `<unordered_map>`; `_sound_buffers` value type changed to `sf::SoundBuffer`; `loadFile` return type → `sf::base::Optional<sf::SoundBuffer>`; added `std::unique_ptr<sf::PlaybackDevice> _playback_device`
- `src/game/audio/audio.cpp` — ctor creates PlaybackDevice; `loadFile` uses factory; `addSample` stores by value; `playSample` always creates new Sound; `isPlaying()` replaces `getStatus()`; volume 0..1; `setPosition()` method
- `src/game/audio/musicplayer.h` — added `<SFML/System.hpp>`; `std::array<sf::Music,2>` → `unique_ptr` array; added `_music_readers` array + `_playback_device`; `current()`/`next()` → `currentMusic()`/`nextMusic()` returning `sf::Music*`
- `src/game/audio/musicplayer.cpp` — full rewrite; ctor creates PlaybackDevice + tries empty.ogg slots; `beginTransition` creates MusicReader+Music; `isPlaying()`; volume 0..1

**Build result: `[100%] Built target deceptus` — zero errors (one non-fatal warning: `-pthread + ALLOW_MEMORY_GROWTH` — expected and harmless).**

**Session 19 — playeraudio fix + pthread linker fix:**
- `src/game/player/playeraudio.cpp` — `sf::Listener::setPosition(...)` (old static API) → `Audio::getInstance().updateListenerPosition(pos)` (new struct-based API through `_playback_device->applyListener()`)
- `CMakeLists.txt` — added `-pthread` compile option and `-pthread -sPTHREAD_POOL_SIZE=4` link options for Emscripten; VRSFML audio (miniaudio) uses pthreads which requires all objects compiled with `-matomics -mbulk-memory` (implied by `-pthread`), and the linker needs `-sUSE_PTHREADS=1` (implied by `-pthread` at link time)
- `thirdparty/lua/CMakeLists.txt` — added `if(EMSCRIPTEN) target_compile_options(lua PRIVATE -pthread) endif()` so Lua objects also have atomics/bulk-memory features (linker rejected `ldo.c.o` otherwise)

**CMakeLists.txt Emscripten link flags (current):**
```cmake
target_compile_options(deceptus PRIVATE -pthread)
target_link_options(deceptus PRIVATE
    -pthread
    -sPTHREAD_POOL_SIZE=4
    -sUSE_WEBGL2=1
    -sMIN_WEBGL_VERSION=2
    -sMAX_WEBGL_VERSION=2
    -sFULL_ES3=1
    -sALLOW_MEMORY_GROWTH=1
    -sINITIAL_MEMORY=67108864
    -sEXIT_RUNTIME=0
    -sASYNCIFY=1
    -sASSERTIONS=1
    -sAUDIO_WORKLET=1
    -sWASM_WORKERS=1
    "-sEXPORTED_RUNTIME_METHODS=['HEAPU32','HEAPU8','HEAP32','HEAPF32','HEAPU16']"
    --preload-file ${CMAKE_SOURCE_DIR}/data@data
)
```

**Session 19 continued — server + browser fixes:**
- `lab/wasm_browser_test/serve.py` — new helper script; serves `build_wasm/` on port **9080** with `Cross-Origin-Opener-Policy: same-origin` + `Cross-Origin-Embedder-Policy: require-corp` headers required for SharedArrayBuffer (pthreads in WASM needs cross-origin isolation)
- `server_wasm.bat` — now calls `serve.py` instead of plain `python -m http.server` (Docker occupies port 8080; switched to 9080)
- `lab/wasm_browser_test/test_wasm_browser.py` — updated fixture to use same COOP/COEP server on port 9080
- **Open** `http://localhost:9080/deceptus.html` (not 8080)

**Session 19 continued — sprite rendering fixes:**
- `src/game/items/itemlantern.cpp:42` — helmet sprite was invisible: VRSFML sprites carry no texture; added `sf::RenderStates{.texture = _player_texture.get()}` to the `target.draw()` call
- `src/game/layers/ambientocclusion.cpp` — investigated but **no fix applied**; `std::filesystem::exists()` is used unguarded in many other WASM-active files and works fine, so that is NOT the cause of AO tiles not rendering; root cause still unknown

---

**Session 20 — AO diagnostic logging added:**

AO confirmed not rendering in WASM (works on desktop). Added diagnostic `Log::Info()` calls to `src/game/layers/ambientocclusion.cpp`:
- After UV file load loop: `"AO load complete: N sprites across M chunk rows"` — confirms whether sprites are populated
- At start of `draw()`: `"AO draw: player_chunk=(x,y) sprite_map_rows=N"` — confirms draw is called and shows player position
- Per-chunk draw: `"AO drawing N sprites at chunk (x,y)"` — confirms which sprites are rendered

**To diagnose:** Open http://localhost:9080/deceptus.html, enter the level, open browser DevTools → Console. Look for the three AO log messages. If `sprite_map_rows=0`, loading failed. If no per-chunk draw messages appear but `sprite_map_rows>0`, the player is out of range of all tiles.

**Build: `[100%] Built target deceptus` — zero errors.**

**Session 20 continued — startup crash fixed (`sf::GraphicsContext` + `sf::AudioContext` `.value()` guards):**

Both `sf::GraphicsContext::create()` and `sf::AudioContext::create()` return empty optionals in WASM under certain conditions (before user interaction, AUDIO_WORKLET not ready, or WebGL init order). The unconditional `.value()` calls in `main.cpp` panicked with `[[SFML OPTIONAL FAILURE]]: not engaged!`.

**Fix (`src/main.cpp`):**
```cpp
#ifndef __EMSCRIPTEN__
   auto graphics_context = sf::GraphicsContext::create().value();
   auto audio_context    = sf::AudioContext::create().value();
#else
   auto graphics_context = sf::GraphicsContext::create();
   auto audio_context    = sf::AudioContext::create();
#endif
```
Desktop keeps `.value()` (crash on failure is correct). WASM stores both as optionals — if either fails, the optional is empty but execution continues. The `Audio` class already guards `_playback_device` with `hasValue()`.

**Note:** If `graphics_context` is empty in WASM the renderer will likely be broken. Watch for GL errors in the next test run; if `GraphicsContext::create()` routinely returns null in WASM there may be a VRSFML WASM initialization order issue requiring deeper investigation.

**Build: `[100%] Built target deceptus` — zero errors.**

**Next session TODO:**
1. Test that the game starts in browser (no more "not engaged!" crash).
2. Analyze AO browser console output (look for "AO load complete" / "AO draw" lines) and fix root cause; remove diagnostic logs after fix.
3. Verify audio plays in browser (SFX + music) — requires COOP/COEP server (port 9080).

---

## Quick-reference fix patterns

```cpp
// IntRect → FloatRect for textureRect
_sprite->textureRect = sf::FloatRect{
   {static_cast<float>(col * W), static_cast<float>(row * H)},
   {static_cast<float>(W), static_cast<float>(H)}
};

// View — VRSFML uses per-draw-call view via RenderStates (setView removed)
states.view = sf::View::fromRect(sf::FloatRect{{x, y}, {w, h}});
// then pass states to all draw() calls

// findIntersection — free function
sf::findIntersection(rect_a, rect_b)   // returns sf::base::Optional<sf::FloatRect>
// test with .hasValue() or operator bool(), NOT .has_value()

// RectangleShape construction
sf::RectangleShape{sf::RectangleShape::Data{.size = {w, h}}}

// Texture creation
std::make_shared<sf::Texture>(std::move(*sf::Texture::create(size)))

// RenderTexture creation
auto rt = *sf::RenderTexture::create(size);

// sf::Text construction
std::make_unique<sf::Text>(font, sf::Text::Data{})

// setString with std::string
text.setString(some_std_string.c_str());

// sftr() returns sf::Utf8String in WASM — pass directly to setString
text.setString(sftr("key"));   // no .c_str() needed

// sf::Time zero
sf::Time{}   // replaces sf::Time::Zero

// sf::Angle — assign directly, don't wrap in sf::degrees()
sprite.rotation = existing_angle;   // NOT sf::degrees(existing_angle)
sf::Angle ang = sf::degrees(90.0f); // sf::degrees(float) → Angle is fine

// Shader UniformLocation members — use sf::base::Optional not std::optional
sf::base::Optional<sf::Shader::UniformLocation> _ul_time;
_ul_time = shader.getUniformLocation("u_time");  // direct assignment works
if (_ul_time.hasValue()) { shader.setUniform(*_ul_time, value); }
```
