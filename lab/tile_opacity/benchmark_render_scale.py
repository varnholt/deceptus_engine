"""Measures what the level render target resolution costs, on the desktop build.

The level render targets are created at the window resolution, not at the view resolution, so at
1280 x 720 every full screen pass rasterises four times the fragments the 640 x 360 pixel art needs.
This runs the catacombs at each render scale with vsync off and samples the frame rate out of the
window title, which the game updates once a second.

    uv run --with pywin32 --with pillow python benchmark_render_scale.py [scales...]

A scale of 0 keeps the old behaviour of matching the window, 1 renders at the view resolution.
"""

import json
import shutil
import statistics
import subprocess
import sys
import time
from pathlib import Path

import win32gui

REPO_ROOT = Path(r"D:/deceptus/deceptus_engine")
sys.path.insert(0, str(REPO_ROOT / "lab" / "map_render"))

import drive_desktop as desktop  # noqa: E402

CATACOMBS_LEVEL = "data/level-catacombs/level.json"
GAME_CONFIGURATION_PATH = desktop.SETTINGS_DIRECTORY / "game.json"
GAME_CONFIGURATION_BACKUP_PATH = desktop.SETTINGS_DIRECTORY / "game.json.benchmark_backup"
OUTPUT_DIRECTORY = Path(__file__).resolve().parent / "out"

SETTLE_SECONDS = 6.0
SAMPLE_SECONDS = 25.0

VK_F10 = 0x79
PROFILE_SECTIONS = "--sections" in sys.argv


def back_up_settings() -> None:
    if GAME_CONFIGURATION_PATH.exists() and not GAME_CONFIGURATION_BACKUP_PATH.exists():
        shutil.copy2(GAME_CONFIGURATION_PATH, GAME_CONFIGURATION_BACKUP_PATH)
        print(f"backed up {GAME_CONFIGURATION_PATH.name}")


def restore_settings() -> None:
    if GAME_CONFIGURATION_BACKUP_PATH.exists():
        shutil.copy2(GAME_CONFIGURATION_BACKUP_PATH, GAME_CONFIGURATION_PATH)
        GAME_CONFIGURATION_BACKUP_PATH.unlink()
        print("restored game.json")


def configure(render_scale: int) -> None:
    configuration = json.loads(GAME_CONFIGURATION_PATH.read_text())
    section = configuration["GameConfiguration"]
    section["vsync"] = False
    section["fullscreen"] = False
    section["render_scale"] = render_scale
    GAME_CONFIGURATION_PATH.write_text(json.dumps(configuration, indent=4))


def install_save() -> None:
    if desktop.SAVE_STATE_PATH.exists() and not desktop.SAVE_STATE_BACKUP_PATH.exists():
        shutil.copy2(desktop.SAVE_STATE_PATH, desktop.SAVE_STATE_BACKUP_PATH)

    slots = json.loads(desktop.SAVE_STATE_PATH.read_text()) if desktop.SAVE_STATE_PATH.exists() else [{}, {}, {}]
    slots[0] = {
        "levelindex": 0,
        "checkpoints": {CATACOMBS_LEVEL: 3},
        # null, not {} - Level::loadSaveState indexes a const nlohmann json
        "levelstate": None,
        "playerinfo": slots[0].get("playerinfo", {}) if slots else {},
    }
    slots[0]["playerinfo"]["name"] = "catacombs"
    desktop.SAVE_STATE_PATH.write_text(json.dumps(slots, indent=4))


def read_frame_rate() -> int | None:
    """Pulls the frame rate out of the window title, which reads 'deceptus - 61fps [Release]'."""
    handle = desktop.find_window()
    if not handle:
        return None

    title = win32gui.GetWindowText(handle)
    marker = "fps"
    if marker not in title:
        return None

    digits = title.split("-", 1)[-1].split(marker, 1)[0].strip()
    return int(digits) if digits.isdigit() else None


def wait_for_level(stdout_path: Path, timeout_s: float) -> bool:
    """Waits until the log says the level finished loading, rather than assuming it did."""
    deadline = time.time() + timeout_s
    while time.time() < deadline:
        if stdout_path.exists():
            text = stdout_path.read_text(encoding="utf-8", errors="replace")
            if "level loading finished" in text:
                return True
        time.sleep(1.0)

    return False


def run_once(render_scale: int) -> dict:
    configure(render_scale)
    install_save()

    executable = REPO_ROOT / "build_rel" / "deceptus.exe"
    run_index = len(list(OUTPUT_DIRECTORY.glob(f"stdout_scale_{render_scale}_*.txt")))
    stdout_path = OUTPUT_DIRECTORY / f"stdout_scale_{render_scale}_{run_index}.txt"

    samples: list[int] = []
    loaded = False
    with stdout_path.open("w", encoding="utf-8", errors="replace") as log_file:
        process = subprocess.Popen([str(executable)], cwd=str(REPO_ROOT), stdout=log_file, stderr=subprocess.STDOUT)
        try:
            print(f"  waiting for the main menu")
            time.sleep(14)

            # a run that never gets past the menu still produces a perfectly plausible frame rate,
            # which is how a menu screen once got read as a level measurement. so the confirm
            # presses are retried and the level load is verified from the log before any sampling
            handle = None
            for attempt in range(6):
                handle = desktop.focus_window()
                if not handle:
                    print("  no game window - did it die during start up?")
                    return {"render_scale": render_scale, "samples": [], "loaded": False}

                desktop.send_key(handle, desktop.VK_RETURN, settle_s=2.0)
                desktop.send_key(handle, desktop.VK_RETURN, settle_s=2.0)

                print(f"  waiting for the level to load (attempt {attempt + 1})")
                if wait_for_level(stdout_path, timeout_s=25.0):
                    break

                print("  still in the menu, retrying")
            else:
                print("  FAILED to load the level - not sampling")
                return {"render_scale": render_scale, "samples": [], "loaded": False}

            # F10 opens the profiling window, which is the only thing that makes the engine write
            # its per section submit costs to the log. it is off by default on desktop, so a run
            # that wants sections has to ask for it
            if PROFILE_SECTIONS:
                desktop.send_key(handle, VK_F10, settle_s=1.0)

            # the first seconds after a load still carry streaming and shader compilation
            time.sleep(SETTLE_SECONDS)

            print(f"  sampling for {SAMPLE_SECONDS:.0f}s")
            deadline = time.time() + SAMPLE_SECONDS
            while time.time() < deadline:
                frame_rate = read_frame_rate()
                if frame_rate:
                    samples.append(frame_rate)
                time.sleep(1.0)

            desktop.grab_window(handle, OUTPUT_DIRECTORY / f"scale_{render_scale}.png")
            loaded = True
        finally:
            if process.poll() is None:
                process.kill()
                process.wait(timeout=10)
            subprocess.run(["taskkill", "/F", "/IM", "deceptus.exe"], capture_output=True)

    return {"render_scale": render_scale, "samples": samples, "loaded": loaded}


def main() -> int:
    scales = [int(argument) for argument in sys.argv[1:] if argument.isdigit()] or [0, 1]

    executable = REPO_ROOT / "build_rel" / "deceptus.exe"
    if not executable.exists():
        print(f"desktop build not found at {executable}")
        return 1

    OUTPUT_DIRECTORY.mkdir(exist_ok=True)
    back_up_settings()

    results = []
    try:
        for render_scale in scales:
            print(f"\n=== render scale {render_scale} ===")
            results.append(run_once(render_scale))
    finally:
        restore_settings()
        desktop.restore_save_state()

    print("\n--- per run ---")
    grouped = {}
    for result in results:
        samples = result["samples"]
        if not result.get("loaded") or not samples:
            print(f"  scale {result['render_scale']}: NO RESULT - the level never loaded")
            continue

        # a run whose own samples are spread wide was competing with something else on the machine,
        # and pooling it silently would hide that rather than show it
        spread = (max(samples) - min(samples)) / statistics.median(samples) * 100
        print(
            f"  scale {result['render_scale']}: median {statistics.median(samples):6.1f} fps"
            f"  min {min(samples):4d}  max {max(samples):4d}  spread {spread:5.1f}%"
        )
        grouped.setdefault(result["render_scale"], []).extend(samples)

    print("\n--- pooled by scale ---")
    baseline = None
    for render_scale in sorted(grouped):
        samples = grouped[render_scale]
        median = statistics.median(samples)
        if baseline is None:
            baseline = median

        target = "window resolution" if render_scale == 0 else f"{render_scale}x the view"
        print(
            f"  scale {render_scale} ({target:18s}): median {median:6.1f} fps"
            f"  min {min(samples):4d}  max {max(samples):4d}  n={len(samples)}"
            f"  ({median / baseline:4.2f}x)"
        )

    return 0


if __name__ == "__main__":
    sys.exit(main())
