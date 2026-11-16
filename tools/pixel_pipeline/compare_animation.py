"""Finishes rendered animation frames and stacks them under his frames of the same animation.

python compare_animation.py <render glob> <his animation name, e.g. run> <frame count> <output png> [--scale 3]

His frames are on top, the finished renders below, each frame the same 72 x 48 box, so poses can be
compared column by column. Finishing is finish_frames.finish with his palette minus the prop colors
(the book's cream must not appear on the character), feet on the bottom row and his outline.
"""

import argparse
import glob
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import numpy
from PIL import Image

import downscale
import finish_frames
import match_metrics


def character_palette():
    props = set(tuple(color) for color in downscale.PROP_COLORS)
    return numpy.array([color for color in downscale.his_palette() if tuple(int(value) for value in color) not in props])


def strip(frames):
    return numpy.concatenate([numpy.pad(frame, ((0, 0), (0, 2), (0, 0))) for frame in frames], axis=1)


def compare(render_glob, animation, count, output, scale):
    palette = character_palette()
    mine = [finish_frames.finish(path, [], palette, True) for path in sorted(glob.glob(render_glob))]
    his_frames = match_metrics.his_frames()
    his = [numpy.asarray(his_frames[f"{animation}_{index:02d}"]).astype(numpy.uint8) for index in range(count)]
    top, bottom = strip(his), strip(mine)
    width = max(top.shape[1], bottom.shape[1])
    top = numpy.pad(top, ((0, 0), (0, width - top.shape[1]), (0, 0)))
    bottom = numpy.pad(bottom, ((0, 0), (0, width - bottom.shape[1]), (0, 0)))
    both = numpy.concatenate([top, numpy.zeros((4, width, 4), numpy.uint8), bottom], axis=0)
    image = Image.fromarray(both)
    background = Image.new("RGBA", image.size, (90, 90, 96, 255))
    background.alpha_composite(image)
    background.resize((image.width * scale, image.height * scale), Image.NEAREST).save(output)
    directory = os.path.dirname(sorted(glob.glob(render_glob))[0])
    for index, frame in enumerate(mine):
        Image.fromarray(frame).save(os.path.join(directory, f"finished_{index:02d}.png"))
    return len(mine)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("render_glob")
    parser.add_argument("animation")
    parser.add_argument("count", type=int)
    parser.add_argument("output")
    parser.add_argument("--scale", type=int, default=3)
    arguments = parser.parse_args()
    print(compare(arguments.render_glob, arguments.animation, arguments.count, arguments.output, arguments.scale))
