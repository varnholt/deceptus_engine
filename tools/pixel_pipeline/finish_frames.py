"""Turns 8x renders (from blender/render_set.py) into finished sprite frames.

python finish_frames.py <render directory> <output directory> [--ground] [--eye-offsets x0,y0,x1,y1]

Per frame: downscale.py's majority vote in his palette (stray pixels are kept: he uses single
pixel highlights on purpose, cleaning them lowered the idle match from 96% to 91%), his eyes (1x2 pixels each) stamped where the model's eye anchors are while the
face looks at the camera, optionally moved so the lowest pixel stands on the bottom row, and the
outline drawn last.
"""

import argparse
import glob
import json
import os

import numpy
from PIL import Image

import downscale

EYE = (51, 20, 35)


def remove_strays(sprite):
    """A pixel whose color appears in none of its 4 neighbours takes the most common neighbour color."""
    cleaned = sprite.copy()
    height, width = sprite.shape[:2]
    for y in range(height):
        for x in range(width):
            if sprite[y, x, 3] == 0:
                continue
            neighbours = [tuple(sprite[ny, nx]) for ny, nx in ((y - 1, x), (y + 1, x), (y, x - 1), (y, x + 1))
                          if 0 <= ny < height and 0 <= nx < width]
            opaque = [color for color in neighbours if color[3] > 0]
            if len(opaque) < 3 or tuple(sprite[y, x]) in opaque or tuple(sprite[y, x, :3]) == EYE:
                continue
            cleaned[y, x] = max(set(opaque), key=opaque.count)
    return cleaned


def stamp_eyes(sprite, features, offsets):
    for index, (position, facing) in enumerate(zip(features["eyes"], features["eye_facing"])):
        if facing < 0.15:
            continue
        x = int(round(position[0] + offsets[index][0] - 0.5))
        y = int(round(position[1] + offsets[index][1] - 0.5))
        for row in (y, y + 1):
            if 0 <= x < sprite.shape[1] and 0 <= row < sprite.shape[0] and sprite[row, x, 3] > 0:
                sprite[row, x, :3] = EYE
    return sprite


HEAD_COLORS = [(140, 40, 92), (155, 44, 105), (188, 54, 116), (214, 67, 118), (102, 30, 80), (171, 50, 109), (212, 66, 117), (194, 59, 115),
                (190, 107, 69), (219, 150, 107), (51, 20, 35), (64, 26, 71), (83, 38, 82), (95, 34, 78)]
def without_outer_outline(head):
    """His head without its outer outline: add_outline draws that ring once for the whole figure."""
    head = head.copy()
    opaque = head[..., 3] > 0
    padded = numpy.pad(~opaque, 1, constant_values=True)
    on_edge = padded[:-2, 1:-1] | padded[2:, 1:-1] | padded[1:-1, :-2] | padded[1:-1, 2:]
    head[opaque & on_edge & (head[..., :3] == 0).all(axis=-1)] = 0
    return head


def load_head_library(calibration_directory, sources):
    """His heads cut from his frames (head_library.py), each with its offset to the model's tracked head.

    sources: {name: (animation, frame index)}; calibration_directory holds <name>.json, the features of
    the model posed to match that frame.
    """
    import head_library
    import match_metrics
    frames = match_metrics.his_frames()
    library = {}
    for name in sources:
        head, (left, top) = head_library.head_from_frame(frames[name])
        tracked = json.load(open(os.path.join(calibration_directory, f"{name}.json")))["head"]
        library[name] = (without_outer_outline(head), (left - tracked[0], top - tracked[1]))
    return library


def stamp_head(sprite, features, head, offset):
    """His drawn head placed where the model's head is; rendered hair or skin poking out around it is removed."""
    if features.get("head_facing", 0.0) < 0.3:
        return sprite, False
    left = int(round(features["head"][0] + offset[0]))
    top = int(round(features["head"][1] + offset[1]))
    stamped = sprite.copy()
    height, width = head.shape[:2]
    covered = numpy.zeros(sprite.shape[:2], dtype=bool)
    for y in range(height):
        for x in range(width):
            target_x, target_y = left + x, top + y
            if head[y, x, 3] > 0 and 0 <= target_x < sprite.shape[1] and 0 <= target_y < sprite.shape[0]:
                stamped[target_y, target_x] = head[y, x]
                covered[target_y, target_x] = True
    # a three pixel ring around his head, above its chin row: rendered hair or skin there is the model's head poking out
    region = covered.copy()
    for _ in range(3):
        grown = region.copy()
        grown[1:] |= region[:-1]
        grown[:-1] |= region[1:]
        grown[:, 1:] |= region[:, :-1]
        grown[:, :-1] |= region[:, 1:]
        region = grown
    region &= ~covered
    region[top + height - 1:] = False
    head_colored = numpy.zeros_like(covered)
    for color in HEAD_COLORS:
        head_colored |= (sprite[..., :3] == color).all(axis=-1)
    poking_out = region & head_colored & ~covered
    # removed pixels peel from the outside in: a pixel next to empty space becomes empty, one enclosed by
    # the figure takes the most common color around it (otherwise the outline pass would draw a hole)
    for _ in range(4):
        transparent = stamped[..., 3] == 0
        padded = numpy.pad(transparent, 1, constant_values=True)
        next_to_empty = padded[:-2, 1:-1] | padded[2:, 1:-1] | padded[1:-1, :-2] | padded[1:-1, 2:]
        peel = poking_out & next_to_empty
        stamped[peel] = 0
        poking_out &= ~peel
    for y, x in zip(*numpy.where(poking_out)):
        neighbours = [tuple(stamped[ny, nx]) for ny, nx in ((y - 1, x), (y + 1, x), (y, x - 1), (y, x + 1))
                      if 0 <= ny < stamped.shape[0] and 0 <= nx < stamped.shape[1] and stamped[ny, nx, 3] > 0 and not poking_out[ny, nx]]
        if neighbours:
            stamped[y, x] = max(set(neighbours), key=neighbours.count)
    return stamped, True


def ground(sprite):
    rows = numpy.where(sprite[..., 3].max(axis=1) > 0)[0]
    if len(rows) == 0:
        return sprite
    # his boots touch the last row, there is no outline below them
    shift = sprite.shape[0] - 1 - rows.max()
    return numpy.roll(sprite, shift, axis=0) if shift >= 0 else sprite


def finish(render_path, offsets, palette, use_ground, head=None):
    render = numpy.array(Image.open(render_path).convert("RGBA"))
    sprite = downscale.downscale(render, 8, palette)
    feature_path = render_path[:-4] + ".json"
    features = json.load(open(feature_path)) if os.path.exists(feature_path) else {}
    head_stamped = False
    if head is not None and "head" in features:
        sprite, head_stamped = stamp_head(sprite, features, *head)
    if features and offsets and not head_stamped:
        sprite = stamp_eyes(sprite, features, offsets)
    if use_ground:
        sprite = ground(sprite)
    return downscale.add_outline(sprite)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("render_directory")
    parser.add_argument("output_directory")
    parser.add_argument("--ground", action="store_true")
    parser.add_argument("--eye-offsets", default="")
    parser.add_argument("--head", default="", help="which of his heads to use: idle_00, run_00 or run_06")
    parser.add_argument("--calibration", default="", help="directory with the features of the model posed to match his frames")
    arguments = parser.parse_args()
    head = None
    if arguments.head:
        head = load_head_library(arguments.calibration, [arguments.head])[arguments.head]
    values = [float(value) for value in arguments.eye_offsets.split(",")] if arguments.eye_offsets else []
    offsets = [(values[index], values[index + 1]) for index in range(0, len(values), 2)]
    palette = downscale.his_palette()
    os.makedirs(arguments.output_directory, exist_ok=True)
    for render_path in sorted(glob.glob(os.path.join(arguments.render_directory, "*.png"))):
        Image.fromarray(finish(render_path, offsets, palette, arguments.ground, head)).save(os.path.join(arguments.output_directory, os.path.basename(render_path)))
