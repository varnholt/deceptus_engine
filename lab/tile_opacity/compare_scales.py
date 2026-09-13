"""Compares the two render scale captures on a region that does not animate.

The captures come from different moments, so water, flames and the swinging mace differ between
them no matter what the renderer does. Cropping to static geometry isolates what the resolution
change itself costs.
"""

import sys
from pathlib import Path

import numpy as np
from PIL import Image

OUTPUT_DIRECTORY = Path(__file__).resolve().parent / "out"

# static masonry and the clock, well away from water, flames and the mace
REGIONS = {
    "brick_wall": (150, 60, 470, 260),
    "clock": (360, 320, 480, 470),
    "vignette_edge": (1000, 480, 1280, 700),
}


def main() -> int:
    first_path = OUTPUT_DIRECTORY / "scale_0.png"
    second_path = OUTPUT_DIRECTORY / "scale_1.png"
    first = Image.open(first_path).convert("RGB")
    second = Image.open(second_path).convert("RGB")

    for name, box in REGIONS.items():
        first_crop = np.asarray(first.crop(box)).astype(np.int16)
        second_crop = np.asarray(second.crop(box)).astype(np.int16)
        difference = np.abs(first_crop - second_crop)

        changed = (difference.max(axis=2) > 2).mean() * 100
        print(
            f"{name:14s} mean |d| {difference.mean():6.2f}  max {difference.max():4d}"
            f"  pixels differing by more than 2: {changed:5.1f}%"
        )

        # write a side by side with the difference amplified so it is actually visible
        amplified = np.clip(difference * 6, 0, 255).astype(np.uint8)
        strip = np.concatenate(
            [np.asarray(first.crop(box)), np.asarray(second.crop(box)), amplified], axis=1
        )
        Image.fromarray(strip).resize(
            (strip.shape[1] * 2, strip.shape[0] * 2), Image.NEAREST
        ).save(OUTPUT_DIRECTORY / f"compare_{name}.png")

    return 0


if __name__ == "__main__":
    sys.exit(main())
