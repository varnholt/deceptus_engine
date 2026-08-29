"""Stack several helmet contact sheets into one image for review."""

import argparse
from pathlib import Path

from PIL import Image


def main():
    parser = argparse.ArgumentParser(description="stack contact sheets into one review image")
    parser.add_argument("--sheets", required=True, help="directory holding the per cycle sheets")
    parser.add_argument("--out", required=True, help="output image path")
    parser.add_argument("--scale", type=float, default=1.0, help="scale applied to every sheet")
    parser.add_argument("names", nargs="+", help="cycle names to stack")
    arguments = parser.parse_args()

    sheet_directory = Path(arguments.sheets)
    images = []
    for name in arguments.names:
        sheet_path = sheet_directory / f"{name}.png"
        if not sheet_path.exists():
            print(f"missing {sheet_path}")
            continue
        image = Image.open(sheet_path).convert("RGBA")
        if arguments.scale != 1.0:
            image = image.resize((int(image.width * arguments.scale), int(image.height * arguments.scale)), Image.NEAREST)
        images.append(image)

    if not images:
        return

    total_width = max(image.width for image in images)
    total_height = sum(image.height + 6 for image in images)
    montage = Image.new("RGBA", (total_width, total_height), (10, 10, 14, 255))
    offset_y = 0
    for image in images:
        montage.alpha_composite(image, (0, offset_y))
        offset_y += image.height + 6

    montage.save(arguments.out)
    print(f"wrote {arguments.out} ({montage.width}x{montage.height})")


if __name__ == "__main__":
    main()
