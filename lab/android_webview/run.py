"""
One-command driver for the Deceptus Android (GeckoView) lab.

Runs the whole pipeline end to end: copy the WASM build into the app assets, build the debug APK,
make sure an emulator is running, install, launch, and (optionally) stream the logs.

Usage (via uv, no manual venv needed):

    uv run deceptus-android                 # copy assets -> build apk -> install -> launch
    uv run deceptus-android --wasm          # also rebuild the WASM first (Docker; slow)
    uv run deceptus-android --emulator      # boot the AVD first if nothing is connected
    uv run deceptus-android --logcat        # after launch, stream the game's console/logs
    uv run deceptus-android --all           # --wasm --emulator --logcat plus the default steps

    uv run deceptus-android --only assets   # run a single stage: wasm|assets|apk|install|launch|logcat

Everything is also available as plain steps if you'd rather run them yourself; see README.md.
"""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
import time
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parent.parent
BUILD_WASM_DIR = REPO_ROOT / "build_wasm"
MOBILE_SHELL_HTML = REPO_ROOT / "lab" / "mobile_controls" / "mobile.html"
ASSET_DIR = SCRIPT_DIR / "app" / "src" / "main" / "assets" / "game"
APK_PATH = SCRIPT_DIR / "app" / "build" / "outputs" / "apk" / "debug" / "app-debug.apk"

WASM_ARTIFACTS = ("deceptus.js", "deceptus.wasm", "deceptus.data")

APPLICATION_ID = "com.deceptus.webview"
LAUNCH_ACTIVITY = f"{APPLICATION_ID}/.MainActivity"

# logcat tags the running game writes to (Gecko's isolated content process + page console).
LOGCAT_TAGS = ("Isolated Web Content", "GeckoConsole", "DeceptusWebView")

IS_WINDOWS = os.name == "nt"


def fail(message: str) -> "NoReturn":  # type: ignore[name-defined]
    print(f"\nerror: {message}", file=sys.stderr)
    raise SystemExit(1)


def announce(step: str) -> None:
    print(f"\n\033[1m==> {step}\033[0m", flush=True)


def run_command(command: list[str], *, cwd: Path | None = None, check: bool = True) -> int:
    printable = " ".join(str(part) for part in command)
    print(f"    $ {printable}", flush=True)
    completed = subprocess.run(command, cwd=str(cwd) if cwd else None)
    if check and completed.returncode != 0:
        fail(f"command failed ({completed.returncode}): {printable}")
    return completed.returncode


def android_sdk_root() -> Path:
    sdk_root = os.environ.get("ANDROID_HOME") or os.environ.get("ANDROID_SDK_ROOT")
    if not sdk_root:
        fail("ANDROID_HOME / ANDROID_SDK_ROOT is not set; point it at your Android SDK.")
    sdk_path = Path(sdk_root)
    if not sdk_path.is_dir():
        fail(f"Android SDK not found at {sdk_path}")
    return sdk_path


def adb_path() -> Path:
    executable = "adb.exe" if IS_WINDOWS else "adb"
    adb = android_sdk_root() / "platform-tools" / executable
    if not adb.is_file():
        fail(f"adb not found at {adb}")
    return adb


def emulator_path() -> Path:
    executable = "emulator.exe" if IS_WINDOWS else "emulator"
    emulator = android_sdk_root() / "emulator" / executable
    if not emulator.is_file():
        fail(f"emulator not found at {emulator}")
    return emulator


def gradlew_path() -> Path:
    return SCRIPT_DIR / ("gradlew.bat" if IS_WINDOWS else "gradlew")


def adb(*args: str, check: bool = True, capture: bool = False) -> str:
    command = [str(adb_path()), *args]
    if capture:
        completed = subprocess.run(command, capture_output=True, text=True)
        if check and completed.returncode != 0:
            fail(f"adb {' '.join(args)} failed: {completed.stderr.strip()}")
        return completed.stdout
    run_command(command, check=check)
    return ""


def connected_devices() -> list[str]:
    output = adb("devices", capture=True)
    devices = []
    for line in output.splitlines()[1:]:
        parts = line.split()
        if len(parts) == 2 and parts[1] == "device":
            devices.append(parts[0])
    return devices


# ----------------------------------------------------------------------------------------------------
# pipeline stages
# ----------------------------------------------------------------------------------------------------


def stage_wasm() -> None:
    announce("Building the WASM target (Docker emscripten/emsdk) — this is slow")
    if shutil.which("docker") is None:
        fail("docker not found on PATH; needed to build the WASM target. Skip with (omit) --wasm.")
    # Mirrors build_wasm.bat at the repo root.
    docker_command = [
        "docker", "run", "--rm",
        "-v", f"{REPO_ROOT}:/workspace",
        "-w", "/workspace",
        "emscripten/emsdk",
        "bash", "-c",
        "rm -rf build_wasm && mkdir -p build_wasm && cd build_wasm && "
        "emcmake cmake .. -DCMAKE_BUILD_TYPE=Release && make -j$(nproc)",
    ]
    run_command(docker_command, cwd=REPO_ROOT)


def stage_assets() -> None:
    announce("Copying WASM build + touch shell into app assets")
    if not BUILD_WASM_DIR.is_dir():
        fail(f"{BUILD_WASM_DIR} not found — run with --wasm, or build_wasm.bat, first.")
    if not MOBILE_SHELL_HTML.is_file():
        fail(f"touch shell not found at {MOBILE_SHELL_HTML}")

    ASSET_DIR.mkdir(parents=True, exist_ok=True)
    for artifact in WASM_ARTIFACTS:
        source = BUILD_WASM_DIR / artifact
        if not source.is_file():
            fail(f"missing {artifact} in {BUILD_WASM_DIR} — is the WASM build complete?")
        print(f"    {source} -> {ASSET_DIR / artifact}")
        shutil.copy2(source, ASSET_DIR / artifact)

    # The mobile touch shell doubles as the entry page; the in-app server supplies the COOP/COEP
    # headers, so no service-worker injection is needed here.
    print(f"    {MOBILE_SHELL_HTML} -> {ASSET_DIR / 'index.html'}")
    shutil.copy2(MOBILE_SHELL_HTML, ASSET_DIR / "index.html")


def stage_apk() -> None:
    announce("Building the debug APK (Gradle)")
    gradlew = gradlew_path()
    if not gradlew.is_file():
        fail(f"gradle wrapper not found at {gradlew}")
    run_command([str(gradlew), "assembleDebug", "--console=plain"], cwd=SCRIPT_DIR)
    if not APK_PATH.is_file():
        fail(f"expected APK not produced at {APK_PATH}")
    size_mb = APK_PATH.stat().st_size / (1024 * 1024)
    print(f"    built {APK_PATH.name} ({size_mb:.0f} MB)")


def stage_emulator(avd_name: str | None) -> None:
    announce("Ensuring an emulator/device is connected")
    if connected_devices():
        print("    device already connected")
        return

    available_avds = subprocess.run(
        [str(emulator_path()), "-list-avds"], capture_output=True, text=True
    ).stdout.split()
    if not available_avds:
        fail("no AVDs found; create one in Android Studio or with avdmanager.")
    target_avd = avd_name or available_avds[0]
    if target_avd not in available_avds:
        fail(f"AVD '{target_avd}' not found; available: {', '.join(available_avds)}")

    print(f"    booting AVD '{target_avd}' (background)…")
    # Detached so it keeps running after this script exits.
    creation_flags = subprocess.DETACHED_PROCESS if IS_WINDOWS else 0  # type: ignore[attr-defined]
    subprocess.Popen(
        [str(emulator_path()), "-avd", target_avd, "-netdelay", "none", "-netspeed", "full"],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        creationflags=creation_flags,
    )
    adb("wait-for-device")
    print("    waiting for boot to complete…")
    for _ in range(120):
        booted = subprocess.run(
            [str(adb_path()), "shell", "getprop", "sys.boot_completed"],
            capture_output=True, text=True,
        ).stdout.strip()
        if booted == "1":
            print("    boot complete")
            return
        time.sleep(3)
    fail("emulator did not finish booting in time")


def warn_if_low_storage() -> None:
    # The GeckoView native libs make the APK ~340 MB; a small AVD data partition fails to install.
    output = subprocess.run(
        [str(adb_path()), "shell", "df", "/data"], capture_output=True, text=True
    ).stdout
    for line in output.splitlines():
        if "/data" in line:
            print(f"    /data: {line.split()[-2]} used")
    print("    (if install fails with 'not enough space', grow disk.dataPartition.size in the "
          "AVD config.ini to 12G and cold-boot with -wipe-data - see README.)")


def stage_install() -> None:
    announce("Installing the APK")
    if not APK_PATH.is_file():
        fail(f"APK not found at {APK_PATH} — build it first (drop --only, or run 'apk').")
    if not connected_devices():
        fail("no device connected; pass --emulator or start one yourself.")
    warn_if_low_storage()
    adb("install", "-r", str(APK_PATH))


def stage_launch() -> None:
    announce("Launching the app")
    adb("shell", "am", "start", "-n", LAUNCH_ACTIVITY)
    print("    tap the screen once (TAP TO PLAY) - that gesture also starts audio.")


def stage_logcat() -> None:
    announce("Streaming logs (Ctrl+C to stop)")
    adb("logcat", "-c")
    tag_filters = []
    for tag in LOGCAT_TAGS:
        tag_filters.extend([f"{tag}:*"])
    try:
        subprocess.run([str(adb_path()), "logcat", "-s", *tag_filters])
    except KeyboardInterrupt:
        pass


# ----------------------------------------------------------------------------------------------------
# entry point
# ----------------------------------------------------------------------------------------------------


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Build and run the Deceptus WASM build as an Android (GeckoView) app.",
    )
    parser.add_argument("--wasm", action="store_true", help="rebuild the WASM target first (Docker, slow)")
    parser.add_argument("--emulator", action="store_true", help="boot the AVD if no device is connected")
    parser.add_argument("--logcat", action="store_true", help="stream the game's logs after launch")
    parser.add_argument("--all", action="store_true", help="shorthand for --wasm --emulator --logcat")
    parser.add_argument("--avd", metavar="NAME", help="AVD to boot (default: first available)")
    parser.add_argument(
        "--only",
        choices=["wasm", "assets", "apk", "emulator", "install", "launch", "logcat"],
        help="run a single stage instead of the full pipeline",
    )
    arguments = parser.parse_args()

    if arguments.only:
        single_stage = {
            "wasm": stage_wasm,
            "assets": stage_assets,
            "apk": stage_apk,
            "emulator": lambda: stage_emulator(arguments.avd),
            "install": stage_install,
            "launch": stage_launch,
            "logcat": stage_logcat,
        }[arguments.only]
        single_stage()
        return

    want_wasm = arguments.wasm or arguments.all
    want_emulator = arguments.emulator or arguments.all
    want_logcat = arguments.logcat or arguments.all

    if want_wasm:
        stage_wasm()
    stage_assets()
    stage_apk()
    if want_emulator:
        stage_emulator(arguments.avd)
    stage_install()
    stage_launch()
    if want_logcat:
        stage_logcat()

    print("\n\033[1mdone.\033[0m the game is running on the device.")


if __name__ == "__main__":
    main()
