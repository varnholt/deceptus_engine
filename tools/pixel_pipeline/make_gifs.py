"""Finishes rendered cycles and turns each into a preview GIF plus a frame strip for review.

python make_gifs.py <cycles directory> <output directory> [--scale 4] [--no-ground ladder,rope]

Every subdirectory of the cycles directory holds one cycle's renders (frame_XX.png from blender/ai_cycles.py).
Frames are finished like compare_animation.py (his palette minus the prop colors, his outline), shown on a
dark backdrop at his 100 ms per frame. Climbing cycles keep the camera's fixed frame instead of putting the
lowest pixel on the bottom row, so hands and feet move against a still body.
"""

import argparse
import glob
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import numpy
from PIL import Image

import compare_animation
import finish_frames

BACKDROP = (40, 36, 52, 255)
FRAME_MILLISECONDS = 100


def on_backdrop(frame, scale):
    image = Image.fromarray(frame)
    background = Image.new("RGBA", image.size, BACKDROP)
    background.alpha_composite(image)
    return background.resize((image.width * scale, image.height * scale), Image.NEAREST).convert("RGB")


def make_cycle(render_directory, output_directory, scale, use_ground):
    palette = compare_animation.character_palette()
    name = os.path.basename(os.path.normpath(render_directory))
    frames = [finish_frames.finish(path, [], palette, use_ground) for path in sorted(glob.glob(os.path.join(render_directory, "frame_*.png")))]
    for index, frame in enumerate(frames):
        Image.fromarray(frame).save(os.path.join(render_directory, f"finished_{index:02d}.png"))
    images = [on_backdrop(frame, scale) for frame in frames]
    gif_path = os.path.join(output_directory, f"{name}.gif")
    images[0].save(gif_path, save_all=True, append_images=images[1:], duration=FRAME_MILLISECONDS, loop=0, disposal=1)
    on_backdrop(compare_animation.strip(frames), scale).save(os.path.join(output_directory, f"{name}_strip.png"))
    return gif_path, len(frames)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("cycles_directory")
    parser.add_argument("output_directory")
    parser.add_argument("--scale", type=int, default=4)
    parser.add_argument("--no-ground", default="ladder,rope")
    parser.add_argument("--only", default="")
    arguments = parser.parse_args()
    os.makedirs(arguments.output_directory, exist_ok=True)
    without_ground = arguments.no_ground.split(",")
    for directory in sorted(glob.glob(os.path.join(arguments.cycles_directory, "*", ""))):
        name = os.path.basename(os.path.normpath(directory))
        if arguments.only and name not in arguments.only.split(","):
            continue
        print(make_cycle(directory, arguments.output_directory, arguments.scale, name not in without_ground))
