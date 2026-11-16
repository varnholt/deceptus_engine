"""High resolution renders (8x) down to sprite pixels in dstar's palette, with his black outline.

python downscale.py <factor> <output directory> <render.png>...

Every sprite pixel is the 8x8 block of the render it covers: opaque when most of the block is
covered, colored with the block's most common palette color (each render pixel snapped to his
palette first, so edges never blend into in-between colors). Then a one pixel black outline is
drawn around the silhouette, like his.
"""

import json
import os
import sys

import numpy
from PIL import Image

PIPELINE_DIRECTORY = os.path.dirname(os.path.abspath(__file__))
SPRITE_DIRECTORY = os.path.join(PIPELINE_DIRECTORY, "..", "..", "data", "sprites")
# props the player holds (the book's pages) need colors his own frames do not have
PROP_COLORS = [(247, 236, 213), (214, 198, 176)]


def his_palette():
    """Every color of his idle and run frames, black included (he draws black lines inside the figure too)."""
    animations = json.load(open(os.path.join(SPRITE_DIRECTORY, "animations.json")))
    sheet = numpy.array(Image.open(os.path.join(SPRITE_DIRECTORY, "player.png")).convert("RGBA"))
    colors = set()
    for animation_name in ("player_idle_r", "player_run_r"):
        animation = animations[animation_name]
        offset_x, offset_y = animation["frame_offset"]
        width, height = animation["frame_size"]
        frames = sheet[offset_y:offset_y + height, offset_x:offset_x + width * animation["sprite_count"]]
        opaque = frames[frames[..., 3] > 0][:, :3]
        colors.update(tuple(int(value) for value in color) for color in opaque)
    colors.update(PROP_COLORS)
    return numpy.array(sorted(colors), dtype=numpy.float64)


def snap(pixels, palette):
    """Index of the nearest palette color for every pixel (redmean distance)."""
    flat = pixels.reshape(-1, 3).astype(numpy.float64)
    mean_red = (flat[:, None, 0] + palette[None, :, 0]) * 0.5
    difference = flat[:, None, :] - palette[None, :, :]
    distance = (2.0 + mean_red / 256.0) * difference[..., 0] ** 2 + 4.0 * difference[..., 1] ** 2 + (2.0 + (255.0 - mean_red) / 256.0) * difference[..., 2] ** 2
    return distance.argmin(axis=1).reshape(pixels.shape[:2])


def downscale(render, factor, palette):
    height, width = render.shape[0] // factor, render.shape[1] // factor
    indices = snap(render[..., :3], palette)
    covered = render[..., 3] > 127
    sprite = numpy.zeros((height, width, 4), dtype=numpy.uint8)
    for y in range(height):
        for x in range(width):
            block_covered = covered[y * factor:(y + 1) * factor, x * factor:(x + 1) * factor]
            if block_covered.mean() < 0.5:
                continue
            block_indices = indices[y * factor:(y + 1) * factor, x * factor:(x + 1) * factor][block_covered]
            sprite[y, x, :3] = palette[numpy.bincount(block_indices).argmax()]
            sprite[y, x, 3] = 255
    return sprite


def add_outline(sprite):
    opaque = sprite[..., 3] > 0
    ring = numpy.zeros_like(opaque)
    ring[1:, :] |= opaque[:-1, :]
    ring[:-1, :] |= opaque[1:, :]
    ring[:, 1:] |= opaque[:, :-1]
    ring[:, :-1] |= opaque[:, 1:]
    ring &= ~opaque
    outlined = sprite.copy()
    outlined[ring] = (0, 0, 0, 255)
    return outlined


if __name__ == "__main__":
    factor = int(sys.argv[1])
    output_directory = sys.argv[2]
    os.makedirs(output_directory, exist_ok=True)
    palette = his_palette()
    for render_path in sys.argv[3:]:
        render = numpy.array(Image.open(render_path).convert("RGBA"))
        sprite = add_outline(downscale(render, factor, palette))
        Image.fromarray(sprite).save(os.path.join(output_directory, os.path.basename(render_path)))
