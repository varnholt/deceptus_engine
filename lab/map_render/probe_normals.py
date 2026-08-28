"""Loads the graveyard and dumps the render targets so the normal pass can be inspected.

    uv run --with pywin32 --with pillow python probe_normals.py [build_dir]

The in-game S key writes every render target into the working directory; texture_map_normal_NN.png
is the one the deferred lighting shader reads its surface normals from.
"""

import json
import shutil
import sys
import time
from pathlib import Path

import drive_desktop
from drive_desktop import (
    REPO_ROOT,
    SAVE_STATE_BACKUP_PATH,
    SAVE_STATE_PATH,
    VK_RETURN,
    VK_S,
    find_window,
    restore_save_state,
    send_key,
)

GRAVEYARD_LEVEL_INDEX = 2
DESTINATION = Path(__file__).resolve().parent / "out" / "normals"


def install_graveyard_save_state() -> None:
    if SAVE_STATE_PATH.exists() and not SAVE_STATE_BACKUP_PATH.exists():
        shutil.copy2(SAVE_STATE_PATH, SAVE_STATE_BACKUP_PATH)

    slots = json.loads(SAVE_STATE_PATH.read_text()) if SAVE_STATE_PATH.exists() else [{}, {}, {}]
    slots[0] = {
        "levelindex": GRAVEYARD_LEVEL_INDEX,
        "checkpoints": {},
        "levelstate": None,
        "playerinfo": slots[0].get("playerinfo", {}) if slots else {},
    }
    SAVE_STATE_PATH.write_text(json.dumps(slots, indent=4))
    print(f"pointed slot 0 at level index {GRAVEYARD_LEVEL_INDEX}")


def collect_dumps() -> None:
    DESTINATION.mkdir(parents=True, exist_ok=True)
    # the game is still flushing the files when the key handler returns
    time.sleep(2.5)
    for dump in sorted(REPO_ROOT.glob("texture_*.png")):
        shutil.move(str(dump), str(DESTINATION / dump.name))
        print(f"collected {dump.name}")


def main() -> int:
    build_directory = sys.argv[1] if len(sys.argv) > 1 else "build_rel"
    executable = REPO_ROOT / build_directory / "deceptus.exe"
    if not executable.exists():
        print(f"{executable} not found")
        return 1

    for stale in REPO_ROOT.glob("texture_*.png"):
        stale.unlink()

    install_graveyard_save_state()

    import subprocess

    log_path = Path(__file__).resolve().parent / "out" / "normals_game.log"
    log_path.parent.mkdir(parents=True, exist_ok=True)
    log_file = log_path.open("w", encoding="utf-8", errors="replace")
    process = subprocess.Popen([str(executable)], cwd=str(REPO_ROOT), stdout=log_file, stderr=subprocess.STDOUT)
    print(f"started {executable} (pid {process.pid})")

    try:
        handle = None
        for _ in range(60):
            time.sleep(1.0)
            handle = find_window()
            if handle:
                break
        if not handle:
            print("game window not found")
            return 1

        time.sleep(3.0)
        send_key(handle, VK_RETURN)
        time.sleep(1.5)
        send_key(handle, VK_RETURN)
        time.sleep(12.0)

        drive_desktop.grab_window(handle, DESTINATION.parent / "normals_window.png")

        send_key(handle, VK_S, hold_s=0.15, settle_s=1.0)
        collect_dumps()
    finally:
        process.terminate()
        time.sleep(1.0)
        restore_save_state()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
