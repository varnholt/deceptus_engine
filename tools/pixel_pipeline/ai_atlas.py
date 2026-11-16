"""Crisp texture atlas for the AI model: his idle sprite and his portrait, unsoftened, side by side.

python ai_atlas.py <output directory>

Writes atlas_natural.png (his portrait colors on the head), atlas_sprite.png (the head in his sprite
palette) and atlas.json (where each picture's figure box sits, in atlas pixels, rows top down). Both
pictures are built exactly like the generator inputs (ai_reference.py and the portrait canvas), minus
the softening, so the projection that fits the generations fits these too. Enlarging uses Scale2x and
nearest neighbour only: every one of his pixels stays a sharp block.
"""

import json
import os
import sys

import numpy
from PIL import Image

import ai_reference

SIDE = 1024
PORTRAIT_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "data", "avatars", "adam.png")

SPRITE_HAIR = [(140, 40, 92), (155, 44, 105), (188, 54, 116), (214, 67, 118), (102, 30, 80), (171, 50, 109)]
SPRITE_SKIN = [(219, 150, 107), (190, 107, 69)]
SPRITE_EYE = (51, 20, 35)
SPRITE_BODY = [(100, 34, 58), (71, 27, 51), (119, 49, 81), (208, 67, 73), (31, 133, 125), (57, 173, 135), (13, 63, 83)]


def nearest(color, palette):
    return min(palette, key=lambda entry: sum((int(first) - second) ** 2 for first, second in zip(color, entry)))


def without_outline(pixels, also_grey=False):
    """His black lines removed: each takes the color of the nearest non-black figure pixel, so the projection
    has no dark seams (the sprite pipeline draws the outline again at the end). also_grey removes the
    portrait's grey drop shadow the same way."""
    pixels = pixels.copy()
    opaque = pixels[..., 3] > 0
    black = opaque & (pixels[..., :3].max(axis=-1) < 30)
    if also_grey:
        channels = pixels[..., :3].astype(int)
        black |= opaque & (channels.max(axis=-1) - channels.min(axis=-1) < 20) & (channels.max(axis=-1) > 90)
    for _ in range(64):
        if not black.any():
            break
        filled = black.copy()
        for shift_y, shift_x in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            neighbour = numpy.roll(numpy.roll(pixels, shift_y, axis=0), shift_x, axis=1)
            neighbour_black = numpy.roll(numpy.roll(black, shift_y, axis=0), shift_x, axis=1)
            neighbour_opaque = numpy.roll(numpy.roll(opaque, shift_y, axis=0), shift_x, axis=1)
            take = black & filled & neighbour_opaque & ~neighbour_black
            pixels[take] = neighbour[take]
            filled &= ~take
        black = filled
    return pixels


def body_picture():
    """ai_reference.reference without softening, enlarged with nearest neighbour instead of Lanczos."""
    frame = numpy.asarray(ai_reference.match_metrics.his_frames()["idle_00"]).astype(numpy.uint8)
    rows, columns = numpy.where(frame[..., 3] > 0)
    frame = frame[rows.min():rows.max() + 1, columns.min():columns.max() + 1]
    enlarged = frame
    for _ in range(3):
        enlarged = ai_reference.scale2x(enlarged)
    image = Image.fromarray(numpy.asarray(enlarged).astype(numpy.uint8))
    side = int(max(image.size) * 1.25)
    canvas = Image.new("RGBA", (side, side), (0, 0, 0, 0))
    canvas.paste(image, ((side - image.width) // 2, (side - image.height) // 2), image)
    pixels = numpy.array(canvas.resize((SIDE, SIDE), Image.NEAREST))
    return without_outline(pixels)


def portrait_picture():
    """The portrait canvas of the generator input (10x, centred), Scale2x rounded instead of blurred."""
    portrait = numpy.array(Image.open(PORTRAIT_PATH).convert("RGBA"))
    enlarged = portrait
    for _ in range(3):
        enlarged = ai_reference.scale2x(enlarged)
    image = Image.fromarray(numpy.asarray(enlarged).astype(numpy.uint8)).resize((portrait.shape[1] * 10, portrait.shape[0] * 10), Image.NEAREST)
    canvas = Image.new("RGBA", (SIDE, SIDE), (0, 0, 0, 0))
    canvas.paste(image, ((SIDE - image.width) // 2, (SIDE - image.height) // 2), image)
    return without_outline(numpy.array(canvas), also_grey=True)


def sprite_head(pixels):
    """The portrait in his sprite palette: hair to his hair shades, skin to his skin tones, eyes and brows dark."""
    mapped = pixels.copy()
    cache = {}
    for y, x in zip(*numpy.where(pixels[..., 3] > 0)):
        color = tuple(int(value) for value in pixels[y, x, :3])
        if color not in cache:
            red, green, blue = color
            if red > green + 60 and blue > green + 25:
                cache[color] = nearest(color, SPRITE_HAIR)
            elif red + green + blue < 200 or blue > red + 30:
                cache[color] = SPRITE_EYE
            elif min(color) > 190:
                cache[color] = SPRITE_SKIN[0]
            elif red > green + 40 and green < 110:
                # collar and jacket below the chin
                cache[color] = nearest(color, SPRITE_BODY)
            else:
                cache[color] = nearest(color, SPRITE_SKIN)
        mapped[y, x, :3] = cache[color]
    return mapped


def bleed(pixels, steps=96):
    """Fills the empty area around the figures with the nearest figure color (alpha stays 0), so projections
    that run off a silhouette edge pick up the edge color instead of black."""
    pixels = pixels.copy()
    known = pixels[..., 3] > 0
    for _ in range(steps):
        if known.all():
            break
        grown = known.copy()
        for shift_y, shift_x in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            neighbour = numpy.roll(numpy.roll(pixels, shift_y, axis=0), shift_x, axis=1)
            neighbour_known = numpy.roll(numpy.roll(known, shift_y, axis=0), shift_x, axis=1)
            take = ~grown & neighbour_known
            pixels[take, :3] = neighbour[take, :3]
            grown |= take
        known = grown
    return pixels


def box(pixels):
    rows, columns = numpy.where(pixels[..., 3] > 0)
    return [int(columns.min()), int(columns.max()), int(rows.min()), int(rows.max())]


def build(output_directory):
    os.makedirs(output_directory, exist_ok=True)
    body = body_picture()
    head = portrait_picture()
    layout = {"size": [2 * SIDE, SIDE], "body": {"offset": 0, "box": box(body)}, "head": {"offset": SIDE, "box": box(head)}}
    for name, head_pixels in (("natural", head), ("sprite", sprite_head(head))):
        atlas = numpy.zeros((SIDE, 2 * SIDE, 4), numpy.uint8)
        atlas[:, :SIDE] = bleed(body)
        atlas[:, SIDE:] = bleed(head_pixels)
        Image.fromarray(atlas).save(os.path.join(output_directory, f"atlas_{name}.png"))
    json.dump(layout, open(os.path.join(output_directory, "atlas.json"), "w"), indent=1)
    return layout


if __name__ == "__main__":
    print(build(sys.argv[1] if len(sys.argv) > 1 else r"D:\ai3d\work\atlas"))
