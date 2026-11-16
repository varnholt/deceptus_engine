"""Turns one of his sprite frames into an input picture for image-to-3D generators.

python ai_reference.py <frame name, e.g. idle_00> <output png> [--soften pixels]

The frame is enlarged 8x with Scale2x (rounds the diagonal steps without inventing colors), softened
a little so the generator sees shapes instead of pixel blocks, and centred on a transparent square
canvas with a margin, the way image-to-3D models expect their input.
"""

import argparse

import numpy
from PIL import Image, ImageFilter

import match_metrics


def scale2x(image):
    """Scale2x (EPX): doubles a pixel image and rounds diagonal steps without inventing colors."""
    height, width = image.shape[:2]
    padded = numpy.pad(image, ((1, 1), (1, 1), (0, 0)), mode="edge")
    center = padded[1:-1, 1:-1]
    above, below = padded[:-2, 1:-1], padded[2:, 1:-1]
    left, right = padded[1:-1, :-2], padded[1:-1, 2:]

    def same(first, second):
        return (first == second).all(axis=-1)

    result = numpy.repeat(numpy.repeat(center, 2, axis=0), 2, axis=1)
    no_cross = ~same(above, below) & ~same(left, right)
    corners = (
        (0, 0, same(above, left) & no_cross, above),
        (0, 1, same(above, right) & no_cross, above),
        (1, 0, same(below, left) & no_cross, below),
        (1, 1, same(below, right) & no_cross, below),
    )
    for row_offset, column_offset, condition, source in corners:
        target = result[row_offset::2, column_offset::2]
        target[condition] = source[condition]
    return result


def reference(frame_name, soften):
    frame = numpy.asarray(match_metrics.his_frames()[frame_name]).astype(numpy.uint8)
    rows, columns = numpy.where(frame[..., 3] > 0)
    frame = frame[rows.min():rows.max() + 1, columns.min():columns.max() + 1]
    enlarged = frame
    for _ in range(3):
        enlarged = scale2x(enlarged)
    image = Image.fromarray(numpy.asarray(enlarged).astype(numpy.uint8), "RGBA")
    if soften > 0:
        # soften color and edges together, then make the edge hard again so the outline stays clean
        softened = image.filter(ImageFilter.GaussianBlur(soften))
        alpha = softened.getchannel("A").point(lambda value: 255 if value > 127 else 0)
        softened.putalpha(alpha)
        image = softened
    side = int(max(image.size) * 1.25)
    canvas = Image.new("RGBA", (side, side), (0, 0, 0, 0))
    canvas.paste(image, ((side - image.width) // 2, (side - image.height) // 2), image)
    return canvas.resize((1024, 1024), Image.LANCZOS)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("frame_name")
    parser.add_argument("output")
    parser.add_argument("--soften", type=float, default=2.0)
    arguments = parser.parse_args()
    reference(arguments.frame_name, arguments.soften).save(arguments.output)
    print(arguments.output)
