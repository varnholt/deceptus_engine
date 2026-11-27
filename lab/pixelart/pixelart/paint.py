"""Draws a piece from its render passes the way a pixel artist would, instead of reducing the render.

Light and colour come from separate passes, so neither drags the other's noise into the art:
- light (key light alone on white matt surfaces) gives each pixel one of a few light bands: lit,
  half, shade, deep (turned away or in cast shadow), like the steps of an artist's ramp;
- albedo (each material's colour without light) gives each stone's tint and where moss grows;
- ids give the shapes (one id per stone) and the joints (id 0);
- depth gives the relief edges, drawn as lines;
- classes give the material (wall stone, trim, glass, roof, foliage, metal) and whether a part
  stands back (side rooms), which picks the ramp and its steps.

Steps, in order:
1. bands from the light, relative to the brightest lit stone of the piece;
2. clusters: patches of one band within one stone smaller than MIN_CLUSTER pixels take the band
   around them, so there is no speckle by construction;
3. tones: each band is a fixed step on the material's ramp, a stone a step lighter or darker by its
   albedo against its material's, side rooms a step lower;
4. drawn edges: a lit line along each stone's top where it meets a joint (stopping short and broken
   now and then, wear.py), a lit rim where a raised part faces the light, a dark cast line where it
   ends towards the lower right; hand marks (wear.py);
5. joints a dark step under the stones beside them; moss green where the albedo is; a dark outline
   along the silhouette;
6. curve smoothing (antialias.py) and the grade that sits the piece into its level (grade.py).

usage: python -m pixelart.paint <passes dir> <piece> <out.png>
"""
import sys
from pathlib import Path

import numpy as np
from PIL import Image
from scipy import ndimage

from .antialias import soften_notches
from .grade import grade, lum
from .stylize import GREENS, PASSAGE, RAMP, clump_moss
from .wear import hand_marks, highlight_breaks

# light bands, as shares of the light on the brightest lit stones: deep | shade | half | lit
BAND_EDGES = (0.12, 0.40, 0.72)
DEEP, SHADE, HALF, LIT = 0, 1, 2, 3
BRIGHT_PERCENTILE = 98
MIN_CLUSTER = 6
# material classes as blender_kit/core.py writes them
SEAM, GLASS, ROOF, FOLIAGE, METAL, TRIM, STONE = 1, 2, 3, 4, 5, 6, 7
# ramp steps per band (deep, shade, half, lit) for each painted material
STEPS = {STONE: (1, 2, 3, 4), TRIM: (1, 2, 3, 5), ROOF: (1, 1, 2, 3), METAL: (0, 1, 1, 2)}
RECEDE_STEPS = 1        # side rooms stand back: one ramp step darker
JOINT_STEPS = 2         # joints sit this many steps under the stones beside them
CAST_STEPS = 2          # the line of shadow a raised part throws, this many steps under the stone
TINT_OCTAVE = 0.6       # a stone this many octaves lighter or darker in albedo than its material is one step off
DEPTH_PER_PX = 65535 / (20.0 * 24)   # 16-bit depth units per level pixel (core.DEPTH_RANGE 20 units, 24 px per unit)
RELIEF_STEP_PX = 0.8    # a depth step of this many pixels or more is a relief edge worth a line
MOSS_GREEN = 1.15       # albedo this much greener than red and blue is moss
MOSS_UP = 0.35          # moss only on faces turned up at least this much (normal's up component)
OUTLINE_STEP = 0        # ramp step of the silhouette line
NO_SIDE = -1
TOWARDS_LIGHT = ((-1, 0), (0, -1))   # the moon lights from the upper left: the up and left neighbours
ORTHOGONAL = ((-1, 0), (1, 0), (0, -1), (0, 1))
AROUND = ORTHOGONAL + ((-1, -1), (-1, 1), (1, -1), (1, 1))


def load(directory, piece, kind):
    return np.asarray(Image.open(Path(directory) / f"{piece}_{kind}.png").convert("RGBA"))


def packed(rgba):
    rgb = rgba[..., :3].astype(np.int64)
    return (rgb[..., 0] << 16) | (rgb[..., 1] << 8) | rgb[..., 2]


def shifted(a, dy, dx, fill):
    """a[y + dy, x + dx] for every pixel, `fill` beyond the border."""
    pad = np.pad(a, ((1, 1), (1, 1)) + ((0, 0),) * (a.ndim - 2), constant_values=fill)
    h, w = a.shape[:2]
    return pad[1 + dy:1 + dy + h, 1 + dx:1 + dx + w]


def light_bands(light, surface):
    """DEEP, SHADE, HALF or LIT from the key light relative to the brightest surfaces."""
    top = max(np.percentile(light[surface], BRIGHT_PERCENTILE), 1e-6)
    return np.digitize(light / top, BAND_EDGES).astype(np.int32)


def merge_small_clusters(band, key, surface):
    """Patches of one band inside one stone smaller than MIN_CLUSTER take the most common band of
    the pixels around them in the same stone; repeated until none is left (or nothing changes)."""
    band = band.copy()
    for _ in range(8):
        small = np.zeros(band.shape, dtype=bool)
        for b in np.unique(band[surface]):
            labels, n = ndimage.label(surface & (band == b))
            if n == 0:
                continue
            sizes = np.bincount(labels.ravel())
            small |= (labels > 0) & (sizes[labels] < MIN_CLUSTER)
        if not small.any():
            break
        votes = np.zeros(band.shape + (4,), dtype=np.int32)
        others = np.where(surface & ~small, band, NO_SIDE)
        for dy, dx in AROUND:
            nb = shifted(others, dy, dx, NO_SIDE)
            same_stone = shifted(key, dy, dx, -1) == key
            for b in range(4):
                votes[..., b] += (nb == b) & same_stone
        decided = small & (votes.max(-1) > 0)
        if not decided.any():
            break
        band[decided] = votes.argmax(-1)[decided]
    return band


def band_steps(band, cls, recede):
    """Ramp step per pixel from its light band and material; side rooms a step lower."""
    step = np.zeros(band.shape, dtype=np.int32)
    for c, steps in STEPS.items():
        here = cls == c
        step[here] = np.asarray(steps)[band[here]]
    return step - np.where(recede, RECEDE_STEPS, 0)


def stone_tints(albedo_lum, key, cls, surface):
    """A step up or down for each stone whose albedo sits clearly above or below its material's."""
    tint = np.zeros(key.shape, dtype=np.int32)
    for c in STEPS:
        here = surface & (cls == c)
        if not here.any():
            continue
        _, inv = np.unique(key[here], return_inverse=True)
        stone_lum = np.bincount(inv, weights=albedo_lum[here]) / np.bincount(inv)
        octaves = np.log2(np.maximum(stone_lum, 1e-3) / max(np.median(stone_lum), 1e-3))
        tint[here] = np.clip(np.round(octaves / TINT_OCTAVE), -1, 1).astype(np.int32)[inv]
    return tint


def relief_lines(depth, surface):
    """(rim, cast): rim = pixels of a raised part whose neighbour towards the light lies clearly
    behind them; cast = pixels lying clearly behind their neighbour towards the light, where a
    raised part throws its line of shadow."""
    near = depth.astype(np.float64)
    rim = np.zeros(depth.shape, dtype=bool)
    cast = np.zeros(depth.shape, dtype=bool)
    step = RELIEF_STEP_PX * DEPTH_PER_PX
    for dy, dx in TOWARDS_LIGHT:
        towards = shifted(near, dy, dx, 0.0)
        inside = shifted(surface, dy, dx, False)
        rim |= surface & inside & (near - towards >= step)
        cast |= surface & inside & (towards - near >= step)
    return rim, cast


def joint_steps(step, seam, surface):
    """Joints a few steps under the lowest stone beside them."""
    body = np.where(surface & ~seam, step, 99)
    near = np.min([shifted(body, dy, dx, 99) for dy, dx in ORTHOGONAL], axis=0)
    return np.where(near < 99, near - JOINT_STEPS, 0)


def top_lines(key, seam, surface, band, seed):
    """Pixels on a stone's top row under a joint, in light: the artist's lit top line, stopping short
    of the ends and broken now and then (wear.highlight_breaks)."""
    under_joint = surface & shifted(seam, -1, 0, False)
    return under_joint & (band >= HALF) & ~highlight_breaks(key, surface, seed)


def silhouette(solid):
    """Solid pixels with an empty pixel beside them."""
    return solid & ~np.logical_and.reduce([shifted(solid, dy, dx, False) for dy, dx in ORTHOGONAL])


def paint(directory, piece, out_path, palette=None, gain=1.0, contrast=1.0, saturation=1.0, night=0.0,
          seed=7, antialias=0.6):
    pal = palette or {}
    ramp = np.asarray(pal.get("stone", RAMP))
    greens = np.asarray(pal.get("foliage", GREENS))
    passage = np.asarray(pal.get("passage", PASSAGE))
    light = load(directory, piece, "light")[..., 0].astype(np.float64)
    albedo = load(directory, piece, "albedo")[..., :3].astype(np.float64)
    ids = load(directory, piece, "ids")
    classes = load(directory, piece, "classes")
    depth = np.asarray(Image.open(Path(directory) / f"{piece}_depth.png"))
    solid = ids[..., 3] >= 128
    key = packed(ids)
    cls = np.where(solid, np.round(classes[..., 0] / 255.0 * 16).astype(np.int32), 0)
    recede = classes[..., 1] >= 128
    seam = solid & ((key == 0) | (cls == SEAM))
    surface = solid & ~seam & np.isin(cls, list(STEPS))
    green = (albedo[..., 1] > MOSS_GREEN * albedo[..., 0]) & (albedo[..., 1] > MOSS_GREEN * albedo[..., 2])
    # moss grows where water stands: on faces turned up (ledges, sills), in clumps, never as speckle;
    # green grime on upright walls stays stone
    up = load(directory, piece, "normals")[..., 1] / 127.5 - 1.0 > MOSS_UP
    moss = clump_moss(surface & green & up, surface)

    band = merge_small_clusters(light_bands(light, surface), key, surface)
    step = band_steps(band, cls, recede) + stone_tints(lum(albedo), key, cls, surface)
    rim, cast = relief_lines(depth, surface)
    step += np.where(rim & (band >= SHADE), 1, 0) + np.where(top_lines(key, seam, surface, band, seed), 1, 0)
    step -= np.where(cast, CAST_STEPS, 0)
    beside_joint = shifted(seam, -1, 0, False) | shifted(seam, 1, 0, False)
    step += hand_marks(key, surface, surface & ~(rim | cast | beside_joint), seed)
    step = np.where(seam, joint_steps(step, seam, surface), step)
    step = np.clip(step, 0, len(ramp) - 1)

    out = np.zeros(ids.shape, dtype=np.uint8)
    painted = surface | seam
    out[..., :3][painted] = ramp[step[painted]]
    leaf = (solid & (cls == FOLIAGE)) | moss
    out[..., :3][leaf] = greens[np.clip(band[leaf] + 1, 0, len(greens) - 1)]
    glass = solid & (cls == GLASS)
    out[..., :3][glass] = passage[np.clip(band[glass], 0, len(passage) - 1)]
    edge = silhouette(solid)
    out[..., :3][edge & ~leaf] = ramp[OUTLINE_STEP]
    out[..., 3][solid] = 255
    soften_notches(out, seam | edge, antialias)
    grade(out, gain=gain, contrast=contrast, saturation=saturation, night=night)
    Image.fromarray(out).save(out_path)


if __name__ == "__main__":
    paint(sys.argv[1], sys.argv[2], sys.argv[3])
