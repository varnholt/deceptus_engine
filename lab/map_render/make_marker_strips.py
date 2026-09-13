"""Writes one marker spriteset layer per zoom step into data/game/map.psd.

Each layer is a horizontal strip of square cells, one cell per marker kind, in the order the
engine expects. Cell size is the layer height, so the code derives it instead of hardcoding it.
"""
import sys
from pathlib import Path
from PIL import Image
from psd_tools import PSDImage
from psd_tools.api.layers import PixelLayer
from psd_tools.constants import Compression

sys.path.insert(0, str(Path(__file__).parent))
from make_markers import COLORS, draw_pixels  # noqa: E402

REPO_ROOT = Path(__file__).resolve().parents[2]
PSD_PATH = REPO_ROOT / "data" / "game" / "map.psd"

SIZES = {1: 9, 2: 7, 3: 5, 4: 3}
KIND_ORDER = ["player", "checkpoint", "portal", "door"]
PARK_ORIGIN = (8, 300)
PARK_SPACING = 11


def make_strip(size: int) -> Image.Image:
    strip = Image.new("RGBA", (size * len(KIND_ORDER), size), (0, 0, 0, 0))
    for index, kind in enumerate(KIND_ORDER):
        for x, y in draw_pixels(size, kind):
            if 0 <= x < size and 0 <= y < size:
                strip.putpixel((index * size + x, y), COLORS[kind])
    return strip


def main() -> None:
    psd = PSDImage.open(str(PSD_PATH))
    existing = {layer.name for layer in psd.descendants()}
    assert not any(name.startswith("marker") for name in existing), "psd still has marker layers"

    for row, (zoom, size) in enumerate(SIZES.items()):
        name = f"markers_{zoom}"
        strip = make_strip(size)
        layer = PixelLayer.frompil(
            strip, psd, name,
            top=PARK_ORIGIN[1] + row * PARK_SPACING,
            left=PARK_ORIGIN[0],
            compression=Compression.RLE,
        )
        psd.append(layer)
        print(f"{name}: {strip.width}x{strip.height} ({len(KIND_ORDER)} cells of {size}px)")

    psd.save(str(PSD_PATH))
    Image.new("RGBA", (1, 1))
    print("saved", PSD_PATH)


if __name__ == "__main__":
    main()
