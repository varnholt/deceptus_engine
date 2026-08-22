"""Checks the frame is still correct when the window clear is skipped.

Game::draw only skips clearing the window when the composited window texture covers it exactly,
which needs a window that is a whole multiple of the 640 x 360 view - the console's 1280 x 720 is,
a maximised desktop window generally is not. So this run forces 1280 x 720 windowed, confirms from
the log that the blit covers the window, and captures a frame of the level and of the menu.

    uv run --with pywin32 --with pillow python verify_window_clear_skip.py
"""

import json
import shutil
import subprocess
import sys
import time
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "lab" / "map_render"))

import drive_desktop as desktop  # noqa: E402

CATACOMBS_LEVEL = "data/level-catacombs/level.json"
GAME_CONFIGURATION_PATH = desktop.SETTINGS_DIRECTORY / "game.json"
GAME_CONFIGURATION_BACKUP_PATH = desktop.SETTINGS_DIRECTORY / "game.json.window_clear_backup"
OUTPUT_DIRECTORY = Path(__file__).resolve().parent / "out"


def configure() -> None:
    if GAME_CONFIGURATION_PATH.exists() and not GAME_CONFIGURATION_BACKUP_PATH.exists():
        shutil.copy2(GAME_CONFIGURATION_PATH, GAME_CONFIGURATION_BACKUP_PATH)

    configuration = json.loads(GAME_CONFIGURATION_PATH.read_text())
    section = configuration["GameConfiguration"]
    section["vsync"] = True
    section["fullscreen"] = False
    section["windowed_width"] = 1280
    section["windowed_height"] = 720
    GAME_CONFIGURATION_PATH.write_text(json.dumps(configuration, indent=4))


def restore() -> None:
    if GAME_CONFIGURATION_BACKUP_PATH.exists():
        shutil.copy2(GAME_CONFIGURATION_BACKUP_PATH, GAME_CONFIGURATION_PATH)
        GAME_CONFIGURATION_BACKUP_PATH.unlink()
        print("restored game.json")


def install_save() -> None:
    if desktop.SAVE_STATE_PATH.exists() and not desktop.SAVE_STATE_BACKUP_PATH.exists():
        shutil.copy2(desktop.SAVE_STATE_PATH, desktop.SAVE_STATE_BACKUP_PATH)

    slots = json.loads(desktop.SAVE_STATE_PATH.read_text()) if desktop.SAVE_STATE_PATH.exists() else [{}, {}, {}]
    slots[0] = {
        "levelindex": 0,
        "checkpoints": {CATACOMBS_LEVEL: 3},
        "levelstate": None,
        "playerinfo": slots[0].get("playerinfo", {}) if slots else {},
    }
    slots[0]["playerinfo"]["name"] = "catacombs"
    desktop.SAVE_STATE_PATH.write_text(json.dumps(slots, indent=4))


def main() -> int:
    OUTPUT_DIRECTORY.mkdir(exist_ok=True)
    configure()
    install_save()

    stdout_path = OUTPUT_DIRECTORY / "stdout_window_clear_skip.txt"
    executable = REPO_ROOT / "build_rel" / "deceptus.exe"

    with stdout_path.open("w", encoding="utf-8", errors="replace") as log_file:
        process = subprocess.Popen([str(executable)], cwd=str(REPO_ROOT), stdout=log_file, stderr=subprocess.STDOUT)
        try:
            time.sleep(14)
            handle = desktop.focus_window()
            if not handle:
                print("no game window")
                return 1

            desktop.grab_window(handle, OUTPUT_DIRECTORY / "window_clear_skip_menu.png")

            for _ in range(6):
                desktop.send_key(handle, desktop.VK_RETURN, settle_s=2.0)
                desktop.send_key(handle, desktop.VK_RETURN, settle_s=2.0)
                text = stdout_path.read_text(encoding="utf-8", errors="replace")
                if "level loading finished" in text:
                    break

            time.sleep(8)
            desktop.grab_window(handle, OUTPUT_DIRECTORY / "window_clear_skip_level.png")
        finally:
            if process.poll() is None:
                process.kill()
                process.wait(timeout=10)
            subprocess.run(["taskkill", "/F", "/IM", "deceptus.exe"], capture_output=True)
            restore()
            desktop.restore_save_state()

    text = stdout_path.read_text(encoding="utf-8", errors="replace")
    for line in text.splitlines():
        if "video mode" in line or "window render texture" in line or "render target profile" in line:
            print(line.strip())

    return 0


if __name__ == "__main__":
    sys.exit(main())
