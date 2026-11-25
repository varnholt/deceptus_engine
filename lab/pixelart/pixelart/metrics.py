"""Pixel-art style numbers for an RGBA image (opaque pixels only).

colours/tile   distinct colours per 24x24 tile (median over tiles that are at least half opaque)
flat pairs     share of neighbouring opaque pixel pairs that are exactly the same colour
near pairs     share of pairs that differ by 1-12 (sum of abs RGB): smooth-gradient steps
dark share     share of pixels with max channel <= 30 (outlines, joints, cracks)
palette        distinct colours over the whole image
"""
import sys
import numpy as np
from PIL import Image


def metrics(path, box=None):
    im = Image.open(path).convert("RGBA")
    if box:
        im = im.crop(box)
    a = np.array(im).astype(int)
    solid = a[..., 3] >= 128
    rgb = a[..., :3]
    out = {}
    per_tile = []
    for ty in range(0, a.shape[0] - 23, 24):
        for tx in range(0, a.shape[1] - 23, 24):
            m = solid[ty:ty + 24, tx:tx + 24]
            if m.mean() < 0.5:
                continue
            px = rgb[ty:ty + 24, tx:tx + 24][m]
            per_tile.append(len({tuple(p) for p in px}))
    out["colours/tile"] = int(np.median(per_tile)) if per_tile else 0
    flat = near = pairs = 0
    for dy, dx in ((0, 1), (1, 0)):
        a0 = rgb[: rgb.shape[0] - dy, : rgb.shape[1] - dx]
        a1 = rgb[dy:, dx:]
        m = solid[: rgb.shape[0] - dy, : rgb.shape[1] - dx] & solid[dy:, dx:]
        d = np.abs(a0 - a1).sum(-1)[m]
        flat += (d == 0).sum()
        near += ((d > 0) & (d <= 12)).sum()
        pairs += m.sum()
    out["flat pairs"] = round(flat / max(pairs, 1), 3)
    out["near pairs"] = round(near / max(pairs, 1), 3)
    out["dark share"] = round((rgb[solid].max(-1) <= 30).mean(), 3)
    out["palette"] = len({tuple(p) for p in rgb[solid]})
    return out


if __name__ == "__main__":
    for p in sys.argv[1:]:
        print(p, metrics(p))
