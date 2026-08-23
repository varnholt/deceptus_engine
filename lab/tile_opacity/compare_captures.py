"""Diffs two captures of the same spot and says where they differ, not just how much.

    uv run --with pillow --with numpy python compare_captures.py a.png b.png [more pairs...]

A frame wide mean hides the failure this is looking for. A missing image layer, a frozen particle
effect or a dropped tile block barely moves the average over 1280 x 720, so the worst 64 px cell is
reported next to it - that is the number that goes to 100% when something structural is gone.

Water, candles, fireflies and the idle animation move on their own, so two captures of the same build
differ regardless. Earlier sessions measured that floor at the catacombs checkpoints as about 2.4% of
the frame, and up to 100% of a single cell at the noisiest spot. Read a result against that, never
against zero.
"""

import sys
from pathlib import Path

import numpy as np
from PIL import Image

CELL_PX = 64
DIFFERENCE_THRESHOLD = 16


def best_aligned_difference(image_a: np.ndarray, image_b: np.ndarray) -> tuple[np.ndarray, tuple[int, int]]:
    """Returns the smallest difference over small whole-pixel shifts, and the shift that produced it.

    The camera does not settle on exactly the same pixel between two runs, and a one or two pixel
    offset lights up every wall edge in the scene. That once read as a 16.84% regression in the water
    room which collapsed to 3.63% after realigning by dy=2. So the shift is searched before anything
    is concluded, and reported so a large one is visible rather than silently corrected away.
    """
    search_px = 3
    best_difference = None
    best_shift = (0, 0)

    for shift_y in range(-search_px, search_px + 1):
        for shift_x in range(-search_px, search_px + 1):
            shifted_b = np.roll(np.roll(image_b, shift_y, axis=0), shift_x, axis=1)
            # the wrapped border is meaningless, so both are cropped by the search radius
            crop_a = image_a[search_px:-search_px, search_px:-search_px]
            crop_b = shifted_b[search_px:-search_px, search_px:-search_px]
            difference = np.abs(crop_a - crop_b).max(axis=2)

            if best_difference is None or difference.mean() < best_difference.mean():
                best_difference = difference
                best_shift = (shift_x, shift_y)

    return best_difference, best_shift


def compare(path_a: Path, path_b: Path) -> None:
    image_a = np.asarray(Image.open(path_a).convert("RGB")).astype(np.int16)
    image_b = np.asarray(Image.open(path_b).convert("RGB")).astype(np.int16)

    if image_a.shape != image_b.shape:
        print(f"{path_a.name} vs {path_b.name}: size mismatch {image_a.shape} vs {image_b.shape}")
        return

    difference, shift = best_aligned_difference(image_a, image_b)
    changed = difference > DIFFERENCE_THRESHOLD

    height, width = changed.shape
    worst_cell = 0.0
    worst_position = (0, 0)
    for top in range(0, height - CELL_PX + 1, CELL_PX):
        for left in range(0, width - CELL_PX + 1, CELL_PX):
            cell = changed[top : top + CELL_PX, left : left + CELL_PX].mean() * 100.0
            if cell > worst_cell:
                worst_cell = cell
                worst_position = (left, top)

    print(
        f"{path_a.stem} vs {path_b.stem}: frame {changed.mean() * 100:6.2f}%  "
        f"worst 64px cell {worst_cell:6.1f}% at {worst_position}  mean |d| {difference.mean():5.2f}  "
        f"max {difference.max():3d}  aligned by {shift}"
    )

    amplified = np.clip(difference[:, :, np.newaxis].repeat(3, axis=2) * 6, 0, 255).astype(np.uint8)
    output_path = path_a.parent / f"diff_{path_a.stem}__{path_b.stem}.png"
    Image.fromarray(amplified).save(output_path)


def main() -> int:
    arguments = [Path(argument) for argument in sys.argv[1:]]
    if len(arguments) < 2 or len(arguments) % 2 != 0:
        print(__doc__)
        return 1

    for index in range(0, len(arguments), 2):
        compare(arguments[index], arguments[index + 1])

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
