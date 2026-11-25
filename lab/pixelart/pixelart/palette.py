"""Reads the palette the stylizer needs straight from a reference image of the target artist.

A stone ramp is 8 shades from outline to highlight; foliage is 6 greens. Both come from the
reference's own opaque pixels (green ones are foliage, the rest stone): distinct colours sorted
by brightness are merged with their nearest neighbour until the wanted count remains, each group
keeping its most used colour, so the ramp only holds colours the artist actually painted.

usage: python -m pixelart.palette <reference.png> [<x0,y0,x1,y1>]
"""
import sys

import numpy as np
from PIL import Image


def _lum(rgb):
    return rgb[..., 0] * 0.299 + rgb[..., 1] * 0.587 + rgb[..., 2] * 0.114


def _bands(colours, n):
    """n shades the artist actually used: distinct colours sorted by brightness, neighbours merged
    until n remain; a merged group keeps its most used colour."""
    vals, counts = np.unique(colours, axis=0, return_counts=True)
    if len(vals) < n:
        raise ValueError(f"only {len(vals)} distinct colours to build {n} shades from")
    order = np.argsort(_lum(vals.astype(float)))
    groups = [[(int(counts[i]), tuple(int(v) for v in vals[i]))] for i in order]
    lum_of = lambda grp: _lum(np.array(max(grp)[1], dtype=float))
    while len(groups) > n:
        # merge the two neighbouring groups closest in brightness
        gaps = [lum_of(groups[k + 1]) - lum_of(groups[k]) for k in range(len(groups) - 1)]
        k = int(np.argmin(gaps))
        groups[k:k + 2] = [groups[k] + groups[k + 1]]
    return [max(grp)[1] for grp in groups]


def extract(path, box=None, stone_levels=8, foliage_levels=6):
    im = Image.open(path).convert("RGBA")
    if box:
        im = im.crop(box)
    a = np.array(im).reshape(-1, 4)
    a = a[a[:, 3] >= 128][:, :3].astype(int)
    r, g, b = a[:, 0], a[:, 1], a[:, 2]
    green = (g > r * 1.18) & (g > b * 1.02) & (a.max(1) > 20)
    stone = ~green
    palette = {"stone": _bands(a[stone], stone_levels)}
    if green.sum() >= foliage_levels:
        palette["foliage"] = _bands(a[green], foliage_levels)
    # the outline is the darkest colour the artist used, not merely the darkest band's favourite
    darkest = a[stone][_lum(a[stone].astype(float)).argmin()]
    palette["stone"][0] = tuple(int(v) for v in darkest)
    return palette


if __name__ == "__main__":
    box = tuple(int(v) for v in sys.argv[2].split(",")) if len(sys.argv) > 2 else None
    import json
    print(json.dumps(extract(sys.argv[1], box)))
