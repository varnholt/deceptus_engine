"""Prices and pictures dropping the normal pass for the catacombs background tileset.

The background normal map is nearly flat - 69% of its opaque pixels are exactly (128,128,255) and
the mean deviation from flat is 12/255 - yet bg0 + bg2 + bg4 rasterise it every frame, which measured
1.08x of the frame's fill. Skipping it is the biggest single cut found so far, and it is a real
visual change, so it needs pictures rather than an argument.

No engine change is involved. TileMap::load discovers a normal map purely by filename, so renaming
catacombs-background-diffuse_normals.png out of the way IS the experiment.

    uv run --with pywin32 --with pillow --with numpy python compare_background_normals.py

Three passes, in this order:

    with_normals    the tree as it is
    floor           the same thing again, unchanged
    without_normals the normal map renamed away

"floor" is not redundant. Water, candles, fireflies and the idle animation move on their own, so two
captures of the *same* build differ by a few percent of pixels no matter what the renderer does.
Without that number the third pass is unreadable - a change is only real when it comes in above the
floor. Previous sessions measured the floor at the catacombs checkpoints as ~2.4% of the frame in the
worst case, and up to 100% of a single 64 px cell at checkpoint 5.
"""

import json
import shutil
import subprocess
import sys
import time
from pathlib import Path

import numpy as np
from PIL import Image

REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "lab" / "map_render"))

import drive_desktop as desktop  # noqa: E402

OUTPUT_DIRECTORY = Path(__file__).resolve().parent / "out" / "background_normals"
CATACOMBS_LEVEL = "data/level-catacombs/level.json"

NORMAL_MAP_PATH = REPO_ROOT / "data" / "level-catacombs" / "tilesets" / "catacombs-background-diffuse_normals.png"
NORMAL_MAP_PARKED_PATH = NORMAL_MAP_PATH.with_suffix(".png.parked")

# the checkpoints rather than hand picked tile coordinates: the noise floor at these six spots is
# already characterised from earlier sessions, and a checkpoint is reproducible in a way that
# walking to a coordinate is not
CHECKPOINTS = [1, 2, 3, 4, 5]

# a 64 px cell, because a frame wide mean hides a lot. a whole 384 px tile block going missing barely
# moves the mean, and that is exactly the failure this is looking for
CELL_PX = 64
DIFFERENCE_THRESHOLD = 16


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
    slots[0]["playerinfo"]["name"] = "bgnormals"
    desktop.SAVE_STATE_PATH.write_text(json.dumps(slots, indent=4))


def capture_pass(label: str) -> bool:
    install_save()

    stdout_path = OUTPUT_DIRECTORY / f"stdout_{label}.txt"
    with stdout_path.open("w", encoding="utf-8", errors="replace") as log_file:
        process = subprocess.Popen(
            [str(REPO_ROOT / "build_rel" / "deceptus.exe")], cwd=str(REPO_ROOT), stdout=log_file, stderr=subprocess.STDOUT
        )
        try:
            time.sleep(14)
            handle = desktop.focus_window()
            if not handle:
                print("  no game window")
                return False

            desktop.send_key(handle, desktop.VK_RETURN, settle_s=2.0)
            desktop.send_key(handle, desktop.VK_RETURN, settle_s=2.0)

            deadline = time.time() + 30
            while time.time() < deadline:
                if "level loading finished" in stdout_path.read_text(encoding="utf-8", errors="replace"):
                    break
                time.sleep(1.0)
            else:
                print("  level never loaded")
                return False

            time.sleep(4)
            for checkpoint in CHECKPOINTS:
                handle = desktop.focus_window()
                desktop.run_console_command(handle, f"tpc {checkpoint}")
                # a checkpoint with a fade transition needs a moment before the frame is the frame
                time.sleep(3.0)
                desktop.grab_window(handle, OUTPUT_DIRECTORY / f"cp{checkpoint}_{label}.png")
                print(f"  captured checkpoint {checkpoint}")
        finally:
            if process.poll() is None:
                process.kill()
                process.wait(timeout=10)
            subprocess.run(["taskkill", "/F", "/IM", "deceptus.exe"], capture_output=True)

    return True


def compare(label_a: str, label_b: str) -> None:
    print(f"\n{label_a} vs {label_b}")
    print(f"  {'spot':6s} {'frame %':>8s} {'worst cell %':>13s} {'mean |d|':>9s} {'max |d|':>8s}")
    for checkpoint in CHECKPOINTS:
        path_a = OUTPUT_DIRECTORY / f"cp{checkpoint}_{label_a}.png"
        path_b = OUTPUT_DIRECTORY / f"cp{checkpoint}_{label_b}.png"
        if not (path_a.exists() and path_b.exists()):
            print(f"  cp{checkpoint}   missing capture")
            continue

        image_a = np.asarray(Image.open(path_a).convert("RGB")).astype(np.int16)
        image_b = np.asarray(Image.open(path_b).convert("RGB")).astype(np.int16)
        if image_a.shape != image_b.shape:
            print(f"  cp{checkpoint}   size mismatch {image_a.shape} vs {image_b.shape}")
            continue

        difference = np.abs(image_a - image_b).max(axis=2)
        changed = difference > DIFFERENCE_THRESHOLD
        frame_percent = changed.mean() * 100.0

        height, width = changed.shape
        worst_cell = 0.0
        for top in range(0, height - CELL_PX + 1, CELL_PX):
            for left in range(0, width - CELL_PX + 1, CELL_PX):
                worst_cell = max(worst_cell, changed[top : top + CELL_PX, left : left + CELL_PX].mean() * 100.0)

        print(f"  cp{checkpoint}   {frame_percent:7.2f}  {worst_cell:12.1f}  {difference.mean():8.2f}  {difference.max():7d}")

        # a side by side with the difference amplified, so the change can be looked at rather than
        # only read as a number
        amplified = np.clip(np.abs(image_a - image_b) * 6, 0, 255).astype(np.uint8)
        strip = np.concatenate([image_a.astype(np.uint8), image_b.astype(np.uint8), amplified], axis=1)
        Image.fromarray(strip).save(OUTPUT_DIRECTORY / f"strip_cp{checkpoint}_{label_a}_vs_{label_b}.png")


def main() -> int:
    executable = REPO_ROOT / "build_rel" / "deceptus.exe"
    if not executable.exists():
        print(f"desktop build not found at {executable}")
        return 1

    if not NORMAL_MAP_PATH.exists():
        print(f"background normal map not found at {NORMAL_MAP_PATH}")
        return 1

    OUTPUT_DIRECTORY.mkdir(parents=True, exist_ok=True)

    try:
        print("pass 1/3: with_normals")
        capture_pass("with_normals")

        print("pass 2/3: floor (same tree, again)")
        capture_pass("floor")

        print("pass 3/3: without_normals")
        NORMAL_MAP_PATH.rename(NORMAL_MAP_PARKED_PATH)
        try:
            capture_pass("without_normals")
        finally:
            NORMAL_MAP_PARKED_PATH.rename(NORMAL_MAP_PATH)
            print("restored the normal map")
    finally:
        desktop.restore_save_state()

    compare("with_normals", "floor")
    compare("with_normals", "without_normals")

    print("\nread it this way: the second table is only a real change where it is well above the")
    print("first one, at the same checkpoint. side by side strips are in")
    print(f"  {OUTPUT_DIRECTORY}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
