"""His idle frame as a projection texture: the outer outline replaced by the neighbouring color.

python projection_texture.py <output.png> [<animation> <frame index>]

The downscale draws the outer outline again, so it must not be painted onto the model (it would
smear along the edges in other poses). Black lines inside the figure stay, they are part of his drawing.
The figure is also grown by a few pixels with its edge colors, so a model edge that lands just
outside his silhouette still picks up a color of his instead of nothing.
"""

import json
import os
import sys

import numpy
from PIL import Image

PIPELINE_DIRECTORY = os.path.dirname(os.path.abspath(__file__))
SPRITE_DIRECTORY = os.path.join(PIPELINE_DIRECTORY, "..", "..", "data", "sprites")


def his_frame(animation_name="player_idle_r", frame_index=0):
    animations = json.load(open(os.path.join(SPRITE_DIRECTORY, "animations.json")))
    offset_x, offset_y = animations[animation_name]["frame_offset"]
    offset_x += frame_index * 72
    sheet = numpy.array(Image.open(os.path.join(SPRITE_DIRECTORY, "player.png")).convert("RGBA"))
    return sheet[offset_y:offset_y + 48, offset_x:offset_x + 72].copy()


def grow(image, mask, steps):
    """Fills pixels outside the mask with the most common neighbouring color inside it, ring by ring."""
    image = image.copy()
    mask = mask.copy()
    height, width = mask.shape
    for _ in range(steps):
        new_mask = mask.copy()
        for y in range(height):
            for x in range(width):
                if mask[y, x]:
                    continue
                neighbours = [image[ny, nx, :3].tobytes() for ny, nx in ((y - 1, x), (y + 1, x), (y, x - 1), (y, x + 1))
                              if 0 <= ny < height and 0 <= nx < width and mask[ny, nx]]
                if neighbours:
                    most_common = max(set(neighbours), key=neighbours.count)
                    image[y, x, :3] = numpy.frombuffer(most_common, dtype=numpy.uint8)
                    image[y, x, 3] = 255
                    new_mask[y, x] = True
        mask = new_mask
    return image


if __name__ == "__main__":
    frame = his_frame(sys.argv[2], int(sys.argv[3])) if len(sys.argv) > 3 else his_frame()
    opaque = frame[..., 3] > 0
    black = (frame[..., :3] == 0).all(axis=-1) & opaque
    outside = numpy.pad(~opaque, 1, constant_values=True)
    touches_outside = outside[:-2, 1:-1] | outside[2:, 1:-1] | outside[1:-1, :-2] | outside[1:-1, 2:]
    outer_outline = black & touches_outside
    interior = opaque & ~outer_outline
    texture = grow(frame, interior, 6)
    Image.fromarray(texture).save(sys.argv[1])
