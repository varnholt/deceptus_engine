"""
One-command driver for the Deceptus Android (GeckoView) lab.

Runs the whole pipeline end to end: copy the WASM build into the app assets, build the debug APK,
make sure an emulator is running, install, launch, and (optionally) stream the logs.

Builds a signed *release* APK by default (nothing in the native shell needs debugging); pass
--debug for a debug APK. APKs are ABI-split: app-arm64-v8a-<type>.apk for phones,
app-x86_64-<type>.apk for the emulator.

Usage (via uv, no manual venv needed):

    uv run deceptus-android                 # copy assets -> build release apk -> install -> launch
    uv run deceptus-android --debug         # same, but a debug APK
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
APK_OUTPUT_DIR = SCRIPT_DIR / "app" / "build" / "outputs" / "apk"

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


def device_abi() -> str:
    """Primary ABI of the connected device (e.g. 'x86_64' emulator, 'arm64-v8a' phone)."""
    abi = adb("shell", "getprop", "ro.product.cpu.abi", capture=True).strip()
    return abi or "x86_64"


def apk_for(build_type: str, abi: str) -> Path:
    # Matches the ABI-split output names, e.g. app-arm64-v8a-release.apk.
    return APK_OUTPUT_DIR / build_type / f"app-{abi}-{build_type}.apk"


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


def stage_apk(build_type: str) -> None:
    announce(f"Building the {build_type} APK(s) (Gradle, ABI-split)")
    gradlew = gradlew_path()
    if not gradlew.is_file():
        fail(f"gradle wrapper not found at {gradlew}")
    gradle_task = "assembleRelease" if build_type == "release" else "assembleDebug"
    run_command([str(gradlew), gradle_task, "--console=plain"], cwd=SCRIPT_DIR)
    produced = sorted((APK_OUTPUT_DIR / build_type).glob(f"app-*-{build_type}.apk"))
    if not produced:
        fail(f"no {build_type} APKs produced in {APK_OUTPUT_DIR / build_type}")
    for apk in produced:
        print(f"    built {apk.name} ({apk.stat().st_size / (1024 * 1024):.0f} MB)")


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


def stage_install(build_type: str) -> None:
    announce("Installing the APK")
    if not connected_devices():
        fail("no device connected; pass --emulator or start one yourself.")
    abi = device_abi()
    apk = apk_for(build_type, abi)
    if not apk.is_file():
        fail(f"no {build_type} APK for the device ABI '{abi}' at {apk} — build it first "
             f"(is '{abi}' in the abi splits in app/build.gradle?).")
    print(f"    device ABI: {abi} -> {apk.name}")
    warn_if_low_storage()
    adb("install", "-r", str(apk))


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
    parser.add_argument("--debug", action="store_true", help="build/install the debug APK (default: signed release)")
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

    # Nothing in the native shell needs debugging, so ship-style release is the default.
    build_type = "debug" if arguments.debug else "release"

    if arguments.only:
        single_stage = {
            "wasm": stage_wasm,
            "assets": stage_assets,
            "apk": lambda: stage_apk(build_type),
            "emulator": lambda: stage_emulator(arguments.avd),
            "install": lambda: stage_install(build_type),
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
    stage_apk(build_type)
    if want_emulator:
        stage_emulator(arguments.avd)
    stage_install(build_type)
    stage_launch()
    if want_logcat:
        stage_logcat()

    print("\n\033[1mdone.\033[0m the game is running on the device.")


if __name__ == "__main__":
    main()
