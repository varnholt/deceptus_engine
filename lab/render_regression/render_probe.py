"""Pins down what the catacombs level looks like, so render changes can be proven visually neutral.

    uv run python render_probe.py capture <tag> [build_dir]
    uv run python render_probe.py compare <tag_a> <tag_b>
    uv run python render_probe.py diff <tag_a> <tag_b>

`capture` drives the desktop build to a fixed set of catacombs checkpoints and grabs the window at
each of them. `compare` diffs two such sets per pixel and prints the fraction that differs.

The comparison cannot be exact and must not pretend to be. Water, fireflies, smoke, candles and the
player's idle animation all move on their own, so two captures of the *same* build never match byte
for byte. The way around that is to measure the noise instead of guessing at it: capture the
unchanged build twice, compare those two runs, and treat the resulting difference as the floor. A
render change is neutral when its difference against the baseline is no worse than that floor.

What this is really guarding against is tile blocks going missing when the culling window is
tightened, and that failure mode is loud - a missing 384 px block is a large contiguous area of
wrong pixels, nothing like the scattered handful that animation produces. Hence `worst_block`,
which reports the single worst 64 px cell rather than only the frame-wide mean: a whole missing
chunk shows up there long before it moves the average.
"""

import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "map_render"))

from drive_desktop import (  # noqa: E402
    REPO_ROOT,
    capture,
    find_window,
    install_clean_save_state,
    restore_save_state,
    run_console_command,
    send_key,
    VK_RETURN,
)

import subprocess  # noqa: E402

from PIL import Image, ImageChops  # noqa: E402

OUTPUT_ROOT = Path(__file__).resolve().parent / "out"

# checkpoints rather than raw coordinates: they are named level content, so they stay valid when the
# level is edited, and `tpc` settles the camera in a defined place
VIEWPOINTS = ["tps", "tpc 1", "tpc 2", "tpc 3", "tpc 4", "tpc 5"]

# a pixel counts as different when any channel moves by more than this, which ignores the dithering
# and gamma noise that a shader reproduces slightly differently from frame to frame
CHANNEL_TOLERANCE = 24

# side of the cell used for the worst-cell statistic, in pixels
BLOCK_PX = 64


def capture_set(tag: str, build_directory: str) -> int:
    executable = REPO_ROOT / build_directory / "deceptus.exe"
    if not executable.exists():
        print(f"{executable} not found")
        return 1

    output_directory = OUTPUT_ROOT / tag
    output_directory.mkdir(parents=True, exist_ok=True)

    install_clean_save_state()
    log_path = output_directory / "game.log"
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
        time.sleep(10.0)

        for index, viewpoint in enumerate(VIEWPOINTS):
            run_console_command(handle, viewpoint)
            # the camera eases towards the new position, so let it arrive before grabbing
            time.sleep(2.5)
            image = capture()
            if image is None:
                print(f"no window while capturing {viewpoint}")
                return 1
            path = output_directory / f"{index:02d}_{viewpoint.replace(' ', '')}.png"
            image.save(path)
            print(f"saved {path.name}  ({image.size[0]}x{image.size[1]})")
    finally:
        process.terminate()
        try:
            process.wait(timeout=10)
        except subprocess.TimeoutExpired:
            process.kill()
        log_file.close()
        restore_save_state()

    return 0


def compare_images(path_a: Path, path_b: Path) -> tuple[float, float]:
    """Returns (fraction of differing pixels overall, worst fraction within any BLOCK_PX cell)."""
    image_a = Image.open(path_a).convert("RGB")
    image_b = Image.open(path_b).convert("RGB")
    if image_a.size != image_b.size:
        raise ValueError(f"size mismatch: {image_a.size} vs {image_b.size}")

    difference = ImageChops.difference(image_a, image_b)
    # collapse the channels to the largest per-pixel deviation, then threshold
    mask = difference.convert("L").point(lambda value: 255 if value > CHANNEL_TOLERANCE else 0)
    width, height = mask.size
    total_different = sum(1 for pixel in mask.getdata() if pixel)

    worst_cell = 0.0
    for top in range(0, height, BLOCK_PX):
        for left in range(0, width, BLOCK_PX):
            cell = mask.crop((left, top, min(left + BLOCK_PX, width), min(top + BLOCK_PX, height)))
            cell_pixels = list(cell.getdata())
            if not cell_pixels:
                continue
            worst_cell = max(worst_cell, sum(1 for pixel in cell_pixels if pixel) / len(cell_pixels))

    return total_different / (width * height), worst_cell


def compare_sets(tag_a: str, tag_b: str) -> int:
    directory_a = OUTPUT_ROOT / tag_a
    directory_b = OUTPUT_ROOT / tag_b
    images_a = sorted(directory_a.glob("*.png"))
    if not images_a:
        print(f"no captures in {directory_a}")
        return 1

    print(f"{'image':<20} {'differing':>10} {'worst_block':>12}")
    worst_overall = 0.0
    worst_block_overall = 0.0
    for image_path in images_a:
        counterpart = directory_b / image_path.name
        if not counterpart.exists():
            print(f"{image_path.name:<20} {'MISSING':>10}")
            return 1
        overall, worst_block = compare_images(image_path, counterpart)
        worst_overall = max(worst_overall, overall)
        worst_block_overall = max(worst_block_overall, worst_block)
        print(f"{image_path.name:<20} {overall * 100:>9.2f}% {worst_block * 100:>11.2f}%")

    print(f"\nworst frame {worst_overall * 100:.2f}% differing, worst {BLOCK_PX}px cell {worst_block_overall * 100:.2f}%")
    return 0


def diff_sets(tag_a: str, tag_b: str) -> int:
    """Writes amplified difference images, because a number alone cannot tell the two cases apart.

    Scattered speckle is animation; a contiguous rectangle is a tile block that stopped being drawn.
    Both can produce the same percentage, so look at the picture before believing the statistic.
    """
    directory_a = OUTPUT_ROOT / tag_a
    directory_b = OUTPUT_ROOT / tag_b
    output_directory = OUTPUT_ROOT / f"diff_{tag_a}_vs_{tag_b}"
    output_directory.mkdir(parents=True, exist_ok=True)

    for image_path in sorted(directory_a.glob("*.png")):
        counterpart = directory_b / image_path.name
        if not counterpart.exists():
            continue
        image_a = Image.open(image_path).convert("RGB")
        image_b = Image.open(counterpart).convert("RGB")
        amplified = ImageChops.difference(image_a, image_b).point(lambda value: min(255, value * 6))
        amplified.save(output_directory / image_path.name)

    print(f"wrote difference images to {output_directory}")
    return 0


def main() -> int:
    if len(sys.argv) < 3:
        print(__doc__)
        return 1
    if sys.argv[1] == "capture":
        return capture_set(sys.argv[2], sys.argv[3] if len(sys.argv) > 3 else "build_rel")
    if sys.argv[1] in ("compare", "diff"):
        if len(sys.argv) < 4:
            print(f"{sys.argv[1]} needs two tags")
            return 1
        if sys.argv[1] == "compare":
            return compare_sets(sys.argv[2], sys.argv[3])
        return diff_sets(sys.argv[2], sys.argv[3])
    print(__doc__)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
