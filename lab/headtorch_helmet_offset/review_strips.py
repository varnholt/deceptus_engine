"""Crops every captured strip down to the head, so all cycles can be reviewed side by side.

The player is found by its head torch lamp, which is the only strongly yellow green thing on screen.
"""

import argparse
from pathlib import Path

import numpy as np
from PIL import Image

OUTPUT_DIRECTORY = Path(__file__).resolve().parent / "out"

FRAME_WIDTH = 420 * 3 + 4
CROP_HALF_WIDTH = 70
CROP_ABOVE = 46
CROP_BELOW = 74
ZOOM = 3


def find_lamp(frame):
    """Finds the head torch lamp. The level has its own yellow green markers, so candidates are
    restricted to the middle of the crop and then to the blob nearest its centre."""
    pixels = np.asarray(frame).astype(int)
    mask = (
        (pixels[:, :, 1] > 150)
        & (pixels[:, :, 0] > 120)
        & (pixels[:, :, 2] < 120)
        & (pixels[:, :, 1] > pixels[:, :, 2] + 60)
    )

    height, width = mask.shape
    search = np.zeros_like(mask)
    search[int(height * 0.15) : int(height * 0.85), int(width * 0.20) : int(width * 0.80)] = True
    mask &= search

    rows, columns = np.nonzero(mask)
    if len(columns) == 0:
        return None

    centre_x, centre_y = width // 2, height // 2
    distances = (columns - centre_x) ** 2 + (rows - centre_y) ** 2
    seed_index = int(np.argmin(distances))
    seed_x, seed_y = columns[seed_index], rows[seed_index]

    near = (np.abs(columns - seed_x) < 20) & (np.abs(rows - seed_y) < 20)
    return int(np.median(columns[near])), int(np.median(rows[near]))


def head_row(strip_path):
    strip = Image.open(strip_path).convert("RGB")
    tiles = []
    for frame_index in range((strip.width + FRAME_WIDTH - 1) // FRAME_WIDTH):
        frame = strip.crop((frame_index * FRAME_WIDTH, 0, min(strip.width, frame_index * FRAME_WIDTH + FRAME_WIDTH - 4), strip.height))
        lamp = find_lamp(frame)
        if lamp is None:
            continue
        lamp_x, lamp_y = lamp
        tile = frame.crop(
            (
                max(0, lamp_x - CROP_HALF_WIDTH),
                max(0, lamp_y - CROP_ABOVE),
                lamp_x + CROP_HALF_WIDTH,
                lamp_y + CROP_BELOW,
            )
        )
        tiles.append(tile.resize((tile.width * ZOOM // 3, tile.height * ZOOM // 3), Image.NEAREST))

    if not tiles:
        return None

    row = Image.new("RGB", (sum(tile.width + 4 for tile in tiles), max(tile.height for tile in tiles)), (10, 10, 14))
    offset_x = 0
    for tile in tiles:
        row.paste(tile, (offset_x, 0))
        offset_x += tile.width + 4
    return row


def main():
    parser = argparse.ArgumentParser(description="crop captured strips down to the head")
    parser.add_argument("--out", required=True, help="output image path")
    parser.add_argument("--filter", default="", help="only include strips whose name contains this")
    arguments = parser.parse_args()

    rows = []
    for strip_path in sorted(OUTPUT_DIRECTORY.glob("*.png")):
        if arguments.filter and arguments.filter not in strip_path.stem:
            continue
        row = head_row(strip_path)
        if row is None:
            print(f"{strip_path.stem}: no lamp found in any frame")
            continue
        rows.append((strip_path.stem, row))

    if not rows:
        return

    total_width = max(row.width for _, row in rows)
    total_height = sum(row.height + 8 for _, row in rows)
    sheet = Image.new("RGB", (total_width, total_height), (10, 10, 14))
    offset_y = 0
    for name, row in rows:
        sheet.paste(row, (0, offset_y))
        offset_y += row.height + 8
        print(f"{offset_y - row.height - 8:5d} {name}")

    sheet.save(arguments.out)
    print(f"wrote {arguments.out} ({sheet.width}x{sheet.height})")


if __name__ == "__main__":
    main()
