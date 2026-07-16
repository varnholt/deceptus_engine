# android_webview

Runs the Deceptus **WASM build** inside an embedded browser view on Android, so the game ships as a
native `.apk` and runs on a phone or the Android emulator.

It uses **GeckoView** (Mozilla's Firefox engine), *not* the stock Android `WebView` — see
[Why GeckoView](#why-geckoview-and-not-webview). The native shell is tiny: one full-screen,
landscape `Activity` that starts an in-app loopback HTTP server and points GeckoView at it. **The
WASM build and the desktop build are completely untouched** — everything here lives in the Android
project.

---

## TL;DR

```bash
# from lab/android_webview/ — one command does everything:
uv run deceptus-android --emulator --logcat
```

That copies the WASM build into the app, builds the APK, boots the emulator (if none is running),
installs, launches, and streams the logs. Prereqs and the individual steps are documented below.

---

## Why GeckoView and not WebView

The WASM build is compiled with `-pthread` + `WASM_WORKERS` + `AUDIO_WORKLET` (and VRSFML itself
hard-requires `--shared-memory` on Emscripten), so it needs `SharedArrayBuffer`. Browsers only
expose `SharedArrayBuffer` when the page is a **secure context** *and* **cross-origin isolated**
(`crossOriginIsolated === true`), which requires `COOP: same-origin` + `COEP: require-corp` headers.

**Android's stock WebView never reports `crossOriginIsolated === true`.** Verified against WebView
149 with the headers delivered four different ways — intercepted `shouldInterceptRequest` responses,
a `coi-serviceworker`, the `--site-per-process` flag, and 4 GB RAM — all ended at
`secureContext=true crossOriginIsolated=false SharedArrayBuffer=false`, and the module hung forever
on the `loading-workers` run dependency. It is a WebView engine limitation, not a header-delivery
problem.

**GeckoView supports cross-origin isolation.** Served over a real HTTP response carrying COOP/COEP,
Gecko runs the page in an *Isolated Web Content* process and `SharedArrayBuffer` works — the pthreads
build runs **unmodified**. (Confirm it in logcat: the game's lines are tagged `Isolated Web Content`.)

Single-threading the WASM to avoid all this was rejected: VRSFML mandates shared memory on
Emscripten with no off-switch, so it would mean patching a third-party fork and likely losing audio.

## How it works

| Piece | Responsibility |
|-------|----------------|
| `AssetHttpServer.java` | NanoHTTPD server on `127.0.0.1:8787`. Streams the bundled build (`deceptus.js` / `.wasm` / `.data` + `index.html`) from app assets, adding `Cross-Origin-Opener-Policy: same-origin`, `Cross-Origin-Embedder-Policy: require-corp`, `Cross-Origin-Resource-Policy: same-origin` and correct MIME types (`application/wasm`, …). Loopback is a secure context, so no TLS/cert is needed. |
| `MainActivity.java` | Starts the server, creates a `GeckoRuntime` (console output + remote debugging on) and `GeckoSession`, loads `http://127.0.0.1:8787/index.html` full-screen and landscape. |
| `index.html` | A verbatim copy of [`../mobile_controls/mobile.html`](../mobile_controls/mobile.html): the on-screen D-pad + A/B/X/Y buttons that dispatch synthetic keyboard events to the canvas. |
| `run.py` | The uv driver that orchestrates every step below. |

## Prerequisites

- **uv** (for the `run.py` driver). The driver itself has no Python dependencies.
- A completed **WASM build** in `../../build_wasm/`. Produce it with `build_wasm.bat` from the repo
  root, or let the driver do it with `--wasm` (needs **Docker**; slow).
- **Android SDK** (this machine: `ANDROID_HOME=D:\android_sdk`), platform **android-36**,
  build-tools **36.0.0**, a **JDK 17+** (Temurin 21 here).
- The **Gradle wrapper** is committed; it downloads Gradle 8.11.1 on first run. The **GeckoView AAR**
  is fetched from `maven.mozilla.org` on first build.
- `local.properties` points Gradle at the SDK (`sdk.dir=D:\android_sdk`); it is git-ignored — edit
  if your SDK lives elsewhere.
- **Emulator storage:** the GeckoView native libs make the debug APK ~340 MB, so the AVD's data
  partition must be big enough. The stock `Medium_Phone_API_36.1` ships with `6G` and fills up.
  Grow it once: set `disk.dataPartition.size=12G` in `~/.android/avd/<avd>.avd/config.ini` and
  cold-boot once with `-wipe-data`. (The emulator's `-partition-size` flag is capped at 2047 MB and
  will *not* work.)

## The one command

```bash
cd lab/android_webview

uv run deceptus-android                 # copy assets -> build APK -> install -> launch
uv run deceptus-android --emulator      # also boot the AVD first if nothing is connected
uv run deceptus-android --logcat        # also stream the game's console/logs after launch
uv run deceptus-android --wasm          # also rebuild the WASM target first (Docker; slow)
uv run deceptus-android --all           # = --wasm --emulator --logcat + the default steps
uv run deceptus-android --avd NAME      # pick which AVD to boot (default: first available)
```

Run a **single stage** instead of the whole pipeline:

```bash
uv run deceptus-android --only assets     # wasm | assets | apk | emulator | install | launch | logcat
```

Re-run (at least) `assets` + `apk` after every WASM rebuild.

## Doing it by hand (no uv)

The driver just wraps these steps; run them yourself if you prefer:

```bash
# 1. copy the freshly-built WASM artifacts + touch shell into the app assets (~117 MB)
bash copy_assets.sh

# 2. build the debug APK (first build downloads the GeckoView AAR)
./gradlew.bat assembleDebug            # -> app/build/outputs/apk/debug/app-debug.apk

# 3. boot the emulator (or plug in a device)
"$ANDROID_HOME/emulator/emulator.exe" -avd Medium_Phone_API_36.1 &

# 4. install + launch
adb install -r app/build/outputs/apk/debug/app-debug.apk
adb shell am start -n com.deceptus.webview/.MainActivity

# 5. tap the screen once (TAP TO PLAY) — this user gesture also resumes the AudioContext
```

To rebuild the WASM target itself (from the repo root): `build_wasm.bat` (Docker emscripten/emsdk).

## ABI / device notes

- `app/build.gradle` sets `ndk { abiFilters "x86_64" }` to keep only the ABI the emulator needs
  (GeckoView bundles native libs for every ABI). **For a physical phone, add `"arm64-v8a"`.**
- `adb screencap` captures GeckoView's GL surface as **black** — a screencap limitation, not a
  render failure. Use the emulator window itself, or `scrcpy`, to see the game.

## Debugging

- Page `console.*` **and** the engine's own `Log::Info` lines go to logcat under the
  `Isolated Web Content` tag (enabled via `GeckoRuntimeSettings.consoleOutput(true)`).
  `uv run deceptus-android --only logcat` filters to the relevant tags.
- Remote debugging is on: `about:debugging` in desktop Firefox → *This Firefox* → connect to the
  device → full DevTools (console, network, WASM) against the live game.

## Troubleshooting

| Symptom | Cause / fix |
|---------|-------------|
| `INSTALL_FAILED_INSUFFICIENT_STORAGE` / "not enough space" | AVD data partition too small — grow `disk.dataPartition.size` to `12G` and cold-boot `-wipe-data` (see Prerequisites). |
| Game hangs on a black screen, logcat shows `crossOriginIsolated=false` | You're on WebView, not GeckoView — this project only works with GeckoView. |
| No sound | Browser autoplay policy — audio starts on the first tap (TAP TO PLAY). |
| Gradle "Could not move temporary workspace…" on first build | Transient Windows transform-cache race (antivirus/file lock). Re-run `assembleDebug`. |
| `adb screencap` is black | Expected for GeckoView's GL surface; capture the emulator window instead. |

## Layout

```
android_webview/
├── run.py                             # uv driver: the one command for the whole pipeline
├── pyproject.toml, uv.lock            # uv project (deceptus-android)
├── copy_assets.sh                     # bash equivalent of the "assets" stage
├── gradlew(.bat), gradle/wrapper/     # committed Gradle wrapper
├── settings.gradle                    # adds maven.mozilla.org for GeckoView
├── build.gradle
└── app/
    ├── build.gradle                   # compileSdk 36, minSdk 26, geckoview + nanohttpd, x86_64
    └── src/main/
        ├── AndroidManifest.xml        # single landscape, fullscreen activity; cleartext to loopback
        ├── assets/game/               # WASM build (git-ignored, filled by the assets stage)
        └── java/com/deceptus/webview/
            ├── MainActivity.java      # GeckoView host + starts the asset server
            └── AssetHttpServer.java   # loopback server with COOP/COEP headers
```
