"""Generates placeholder map marker layers and injects them into data/game/map.psd.

The art is deliberately plain: it exists so the artist has correctly named, correctly sized
layers to paint over, not because these glyphs are any good.

Layer naming is marker_<kind>_<zoom>, zoom 1 is the most zoomed in detail level and 4 the most
zoomed out, matching the existing zoom_level_* layers.
"""

import sys
from pathlib import Path

from PIL import Image
from psd_tools import PSDImage
from psd_tools.api.layers import PixelLayer
from psd_tools.constants import Compression

ROOT_ARGUMENTS = [argument for argument in sys.argv[1:] if not argument.startswith("--")]
REPO_ROOT = Path(ROOT_ARGUMENTS[0]) if ROOT_ARGUMENTS else Path(__file__).resolve().parents[2]
PSD_PATH = REPO_ROOT / "data" / "game" / "map.psd"

# one size per zoom level, biggest when zoomed in
SIZES = {1: 9, 2: 7, 3: 5, 4: 3}

COLORS = {
    "player": (255, 255, 255, 255),
    "checkpoint": (86, 240, 128, 255),
    "portal": (86, 200, 255, 255),
    "door": (255, 214, 92, 255),
}

# where the layers are parked inside the canvas so the artist can find them. they are hidden at
# runtime and stamped at the marker positions instead, so this only matters for editing.
PARK_ORIGIN = (8, 296)
PARK_SPACING = 12


def draw_pixels(size: int, kind: str) -> set[tuple[int, int]]:
    """Returns the set of lit pixels for one icon."""
    last = size - 1
    center = size // 2
    pixels: set[tuple[int, int]] = set()

    if kind == "player":
        # diamond
        for y in range(size):
            span = center - abs(y - center)
            for x in range(center - span, center + span + 1):
                pixels.add((x, y))
        return pixels

    if kind == "door":
        # filled block with a lighter core, reads as a small gate
        for y in range(1, last):
            for x in range(1, last):
                if x in (1, last - 1) or y in (1, last - 1):
                    pixels.add((x, y))
        if size <= 3:
            pixels = {(x, y) for y in range(size) for x in range(size) if x in (0, last) or y in (0, last)}
        return pixels

    # checkpoint and portal are both rings, told apart by their centre mark and their colour
    if size <= 3:
        pixels = {(x, y) for y in range(size) for x in range(size)}
        if kind == "portal":
            pixels.discard((center, center))
        return pixels

    for y in range(size):
        for x in range(size):
            distance = max(abs(x - center), abs(y - center))
            # a squared-off ring keeps the shape readable at these sizes
            if distance == center or (distance == center - 1 and (x in (0, last) or y in (0, last))):
                pixels.add((x, y))

    # trim the corners so the ring looks round rather than square
    for corner in ((0, 0), (0, last), (last, 0), (last, last)):
        pixels.discard(corner)

    if kind == "checkpoint":
        for x in range(center - 1, center + 2):
            pixels.add((x, center))
    else:
        pixels.add((center, center))
        pixels.add((center, center - 1))
        pixels.add((center, center + 1))

    return pixels


def make_icon(kind: str, size: int) -> Image.Image:
    image = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    color = COLORS[kind]
    for x, y in draw_pixels(size, kind):
        if 0 <= x < size and 0 <= y < size:
            image.putpixel((x, y), color)
    return image


def build_sheet() -> Image.Image:
    """Preview sheet, 4 kinds across, 4 zoom levels down."""
    cell = 12
    sheet = Image.new("RGBA", (cell * len(COLORS), cell * len(SIZES)), (16, 18, 34, 255))
    for column, kind in enumerate(COLORS):
        for row, zoom in enumerate(SIZES):
            icon = make_icon(kind, SIZES[zoom])
            offset = ((cell - icon.width) // 2, (cell - icon.height) // 2)
            sheet.alpha_composite(icon, (column * cell + offset[0], row * cell + offset[1]))
    return sheet.resize((sheet.width * 8, sheet.height * 8), Image.NEAREST)


def inject() -> None:
    psd = PSDImage.open(str(PSD_PATH))
    existing = {layer.name for layer in psd.descendants()}

    added = 0
    for column, kind in enumerate(COLORS):
        for row, zoom in enumerate(SIZES):
            name = f"marker_{kind}_{zoom}"
            if name in existing:
                print(f"skipping {name}, already present")
                continue

            icon = make_icon(kind, SIZES[zoom])
            left = PARK_ORIGIN[0] + column * PARK_SPACING
            top = PARK_ORIGIN[1] + row * PARK_SPACING
            layer = PixelLayer.frompil(icon, psd, name, top=top, left=left, compression=Compression.RLE)
            psd.append(layer)
            added += 1

    psd.save(str(PSD_PATH))
    print(f"added {added} marker layers to {PSD_PATH}")


if __name__ == "__main__":
    build_sheet().save(Path(__file__).parent / "marker_sheet.png")
    print("preview written to marker_sheet.png")
    if "--inject" in sys.argv:
        inject()
