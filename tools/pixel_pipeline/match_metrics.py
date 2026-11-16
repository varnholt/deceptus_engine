"""How close finished frames are to dstar's: exact pixels overall, inside his head, and silhouette overlap.

python match_metrics.py <finished directory>

Expects idle_00.png and run_00.png .. run_11.png (the rig posed to match his frames). Exact counts a
pixel only when both frames have exactly the same color there; the share is taken over every pixel
where either frame has the figure.
"""

import json
import os
import sys

import numpy
from PIL import Image

PIPELINE_DIRECTORY = os.path.dirname(os.path.abspath(__file__))
SPRITE_DIRECTORY = os.path.join(PIPELINE_DIRECTORY, "..", "..", "data", "sprites")
HEAD_COLORS = [(140, 40, 92), (155, 44, 105), (188, 54, 116), (214, 67, 118), (102, 30, 80), (171, 50, 109), (212, 66, 117), (194, 59, 115),
               (190, 107, 69), (219, 150, 107), (51, 20, 35), (64, 26, 71)]


def his_frames():
    animations = json.load(open(os.path.join(SPRITE_DIRECTORY, "animations.json")))
    sheet = numpy.array(Image.open(os.path.join(SPRITE_DIRECTORY, "player.png")).convert("RGBA")).astype(numpy.int64)
    frames = {}
    for animation_name, prefix, count in (("player_idle_r", "idle", 1), ("player_run_r", "run", 12)):
        offset_x, offset_y = animations[animation_name]["frame_offset"]
        for index in range(count):
            frames[f"{prefix}_{index:02d}"] = sheet[offset_y:offset_y + 48, offset_x + index * 72:offset_x + (index + 1) * 72]
    return frames


def metrics(mine, his):
    mine_on, his_on = mine[..., 3] > 0, his[..., 3] > 0
    union = mine_on | his_on
    same = mine_on & his_on & (numpy.abs(mine[..., :3] - his[..., :3]).sum(axis=-1) == 0)
    head = numpy.zeros_like(his_on)
    for color in HEAD_COLORS:
        head |= (his[..., :3] == color).all(axis=-1) & his_on
    return {"exact": same.sum() / union.sum(), "head": (same & head).sum() / max(head.sum(), 1), "silhouette": (mine_on & his_on).sum() / union.sum()}


if __name__ == "__main__":
    directory = sys.argv[1]
    results = {}
    for name, his in his_frames().items():
        path = os.path.join(directory, f"{name}.png")
        if os.path.exists(path):
            results[name] = metrics(numpy.array(Image.open(path).convert("RGBA")).astype(numpy.int64), his)
    run = [value for name, value in results.items() if name.startswith("run")]
    for key in ("exact", "head", "silhouette"):
        print(f"{key:10s} idle {results['idle_00'][key]:.3f}   run mean {numpy.mean([value[key] for value in run]):.3f}   run min {min(value[key] for value in run):.3f}")
