"""Rebuilds the contact sheet from the full frames the sweep already saved.

    uv run --with pillow python montage.py [columns]

Kept separate from the sweep so the framing can be changed without driving the game again.

The cells are whole frames at half size. The capture is 1280x720 for a 640x360 game, so half is the
native resolution and the pixel art lands unscaled. Cropping in on the middle of the window loses
the ropes whose room clamps the camera: near a room edge the player is not centred at all.
"""

import re
import sys
from pathlib import Path

from PIL import Image, ImageDraw

OUTPUT_DIRECTORY = Path(__file__).resolve().parent / "out"
CATACOMBS_TMX_PATH = Path(__file__).resolve().parents[2] / "data" / "level-catacombs" / "catacombs.tmx"

CELL_SCALE = 0.5
LABEL_HEIGHT_PX = 16
GUTTER_PX = 5


def read_rope_labels():
    """Maps each rope id to its lamp variant and length, for the cell captions."""
    text = CATACOMBS_TMX_PATH.read_text(encoding="utf-8")
    start = text.index('name="ropes_with_light"')
    end = text.index("</objectgroup>", start)

    labels = {}
    current = None
    for line in text[start:end].split("\n"):
        object_match = re.search(r'<object id="(\d+)"', line)
        if object_match:
            current = int(object_match.group(1))
            labels[current] = {"lamp_sprite": 1, "length_px": 0}
            continue
        if current is None:
            continue
        lamp_match = re.search(r'name="lamp_sprite" type="int" value="(\d+)"', line)
        if lamp_match:
            labels[current]["lamp_sprite"] = int(lamp_match.group(1))
        polyline_match = re.search(r'<polyline points="0,0 0,([-\d.]+)"', line)
        if polyline_match:
            labels[current]["length_px"] = int(abs(float(polyline_match.group(1))))
    return labels


def build():
    columns = int(sys.argv[1]) if len(sys.argv) > 1 else 6
    labels = read_rope_labels()

    cells = []
    for path in sorted(OUTPUT_DIRECTORY.glob("rope_*_full.png")):
        rope_id = int(path.stem.split("_")[1])
        image = Image.open(path).convert("RGB")
        crop = image.resize((int(image.width * CELL_SCALE), int(image.height * CELL_SCALE)), Image.NEAREST)
        info = labels.get(rope_id, {"lamp_sprite": 0, "length_px": 0})
        cells.append((f"id {rope_id}   lamp {info['lamp_sprite']}   {info['length_px']}px", crop))

    if not cells:
        print("no frames in out/")
        return 1

    cell_width = max(image.width for _, image in cells)
    cell_height = max(image.height for _, image in cells)
    rows = (len(cells) + columns - 1) // columns

    sheet = Image.new(
        "RGB",
        (
            columns * (cell_width + GUTTER_PX) + GUTTER_PX,
            rows * (cell_height + LABEL_HEIGHT_PX + GUTTER_PX) + GUTTER_PX,
        ),
        (12, 12, 16),
    )
    draw = ImageDraw.Draw(sheet)

    for index, (label, image) in enumerate(cells):
        x = GUTTER_PX + (index % columns) * (cell_width + GUTTER_PX)
        y = GUTTER_PX + (index // columns) * (cell_height + LABEL_HEIGHT_PX + GUTTER_PX)
        sheet.paste(image, (x, y))
        draw.text((x + 2, y + cell_height + 2), label, fill=(210, 210, 220))

    path = OUTPUT_DIRECTORY / "catacombs_ropes_contact_sheet.png"
    sheet.save(path)
    print(f"saved {path} ({len(cells)} cells, {sheet.width}x{sheet.height})")
    return 0


if __name__ == "__main__":
    sys.exit(build())
