"""Captures the smoke and dust effects so two builds can be compared where the effects are visible.

A frame taken at a checkpoint proves nothing about mechanisms that sit elsewhere in the level, so
this teleports to the objects themselves before grabbing anything.

    uv run --with pywin32 --with pillow python capture_effects.py <label>
"""

import json
import shutil
import subprocess
import sys
import time
from pathlib import Path

REPO_ROOT = Path(r"D:/deceptus/deceptus_engine")
sys.path.insert(0, str(REPO_ROOT / "lab" / "map_render"))

import drive_desktop as desktop  # noqa: E402

OUTPUT_DIRECTORY = Path(__file__).resolve().parent / "out"
CATACOMBS_LEVEL = "data/level-catacombs/level.json"

# taken from the object groups in catacombs.tmx. the portal room is an image layer at px
# (4896, 2352); two dust objects with a flow field sit exactly on it
LOCATIONS = {
    "smoke": (103, 113),
    "dust_portal": (218, 105),
    "water": (239, 125),
}


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
    slots[0]["playerinfo"]["name"] = "effects"
    desktop.SAVE_STATE_PATH.write_text(json.dumps(slots, indent=4))


def main() -> int:
    label = sys.argv[1] if len(sys.argv) > 1 else "build"
    OUTPUT_DIRECTORY.mkdir(exist_ok=True)
    install_save()

    stdout_path = OUTPUT_DIRECTORY / f"effects_{label}.txt"
    with stdout_path.open("w", encoding="utf-8", errors="replace") as log_file:
        process = subprocess.Popen(
            [str(REPO_ROOT / "build_rel" / "deceptus.exe")], cwd=str(REPO_ROOT), stdout=log_file, stderr=subprocess.STDOUT
        )
        try:
            time.sleep(14)
            handle = desktop.focus_window()
            if not handle:
                print("no game window")
                return 1

            desktop.send_key(handle, desktop.VK_RETURN, settle_s=2.0)
            desktop.send_key(handle, desktop.VK_RETURN, settle_s=2.0)

            deadline = time.time() + 30
            while time.time() < deadline:
                if "level loading finished" in stdout_path.read_text(encoding="utf-8", errors="replace"):
                    break
                time.sleep(1.0)
            else:
                print("level never loaded")
                return 1

            time.sleep(4)
            for name, (tile_x, tile_y) in LOCATIONS.items():
                handle = desktop.focus_window()
                # the console splits on whitespace, so "tpp x,y" arrives as two tokens and is ignored
                desktop.run_console_command(handle, f"tpp {tile_x}, {tile_y}")
                # the effects animate, so settle before grabbing
                time.sleep(3.0)
                desktop.grab_window(handle, OUTPUT_DIRECTORY / f"{name}_{label}.png")
                print(f"captured {name} at tile({tile_x},{tile_y})")
        finally:
            if process.poll() is None:
                process.kill()
                process.wait(timeout=10)
            subprocess.run(["taskkill", "/F", "/IM", "deceptus.exe"], capture_output=True)
            desktop.restore_save_state()

    return 0


if __name__ == "__main__":
    sys.exit(main())
