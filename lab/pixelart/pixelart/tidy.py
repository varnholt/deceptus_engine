"""The last step of every stylized piece: tidies the artifacts a render leaves behind, the way an
artist cleans a sprite by hand, and reports what it changed.

- holes: small transparent pockets enclosed by the piece (gaps between meshes) take the colour
  of the darkest pixel around them, which is joint colour
- dark islands: tiny groups of very dark pixels inside a surface that are not part of a joint or
  crack network read as dirt on the screen; they take the surrounding colour
- slivers: dark runs one pixel wide and only a few pixels long (half-rendered seams, stains)
- bright specks: a pixel far brighter than its surroundings that is not part of a highlight
  line (the ends of lit edges, where they run into a joint); dstar stops his highlight lines a
  pixel short of the corner
- lone pixels: a pixel whose colour none of its eight neighbours share, while most of them
  agree on another colour, takes that colour
"""
import warnings

import numpy as np
from scipy import ndimage

MAX_HOLE_PX = 4           # gaps between meshes are a few pixels; anything larger may be a real opening (a cross ring, a fence)
MAX_ISLAND_PX = 3         # dark groups up to this size are specks; joints and cracks are longer
DARK_LUMINANCE = 32       # joint and outline colours are below this
SLIVER_MAX_LENGTH = 4     # shorter one-pixel dark runs are slivers, longer ones are drawn lines
SPECK_CONTRAST = 28       # a speck is at least this much brighter than the median of its neighbours (one tone step or more)

EIGHT = np.ones((3, 3), dtype=bool)


def _luminance(rgb):
    return rgb[..., 0] * 0.299 + rgb[..., 1] * 0.587 + rgb[..., 2] * 0.114


def _dark_stone(img):
    """Joint and outline dark: dark and not green, so shaded leaves and moss are never taken for dirt."""
    rgb = img[..., :3].astype(float)
    green = (rgb[..., 1] > rgb[..., 0] * 1.15) & (rgb[..., 1] > rgb[..., 2] * 1.05)
    return (img[..., 3] > 0) & (_luminance(rgb) < DARK_LUMINANCE) & ~green


def _packed(rgb):
    c = rgb.astype(np.int64)
    return (c[..., 0] << 16) | (c[..., 1] << 8) | c[..., 2]


def _unpack(v):
    return np.stack([(v >> 16) & 255, (v >> 8) & 255, v & 255], -1).astype(np.uint8)


def _window(sl, shape):
    """The bounding box sl grown by one pixel on every side, clipped to the image."""
    return (slice(max(sl[0].start - 1, 0), min(sl[0].stop + 1, shape[0])),
            slice(max(sl[1].start - 1, 0), min(sl[1].stop + 1, shape[1])))


def neighbour_mode(neighbours):
    """Most common value along the last axis for every row at once, ignoring entries below 0
    (no neighbour there): returns (value, count). Ties go to the first neighbour in order."""
    valid = neighbours >= 0
    agree = (neighbours[..., :, None] == neighbours[..., None, :]) & valid[..., :, None] & valid[..., None, :]
    counts = agree.sum(-1)
    best = counts.argmax(-1)
    value = np.take_along_axis(neighbours, best[..., None], -1)[..., 0]
    return value, np.take_along_axis(counts, best[..., None], -1)[..., 0]


def _surrounding_colour(packed, mask, region):
    """Most common colour on the one-pixel ring around region, among pixels in mask."""
    ring = ndimage.binary_dilation(region, EIGHT) & ~region & mask
    vals, counts = np.unique(packed[ring], return_counts=True)
    return vals[counts.argmax()] if len(vals) else None


def fill_holes(img):
    opaque = img[..., 3] > 0
    holes, _ = ndimage.label(~opaque)
    fixed = 0
    for label, sl in enumerate(ndimage.find_objects(holes), start=1):
        if (holes[sl] == label).sum() > MAX_HOLE_PX:
            continue
        win = _window(sl, opaque.shape)
        region = holes[win] == label
        # a hole touching the image border is open space, not a gap between meshes
        if region.shape != (sl[0].stop - sl[0].start + 2, sl[1].stop - sl[1].start + 2):
            continue
        ring = ndimage.binary_dilation(region, EIGHT) & ~region
        if not opaque[win][ring].all():
            continue
        colours = img[win][..., :3][ring]
        img[win][..., :3][region] = colours[_luminance(colours.astype(float)).argmin()]
        img[win][..., 3][region] = 255
        fixed += 1
    return fixed


def remove_dark_islands(img):
    opaque = img[..., 3] > 0
    dark = _dark_stone(img)
    islands, _ = ndimage.label(dark, EIGHT)
    packed = _packed(img[..., :3])
    fixed = 0
    for label, sl in enumerate(ndimage.find_objects(islands), start=1):
        if (islands[sl] == label).sum() > MAX_ISLAND_PX:
            continue
        win = _window(sl, dark.shape)
        region = islands[win] == label
        ring = ndimage.binary_dilation(region, EIGHT) & ~region
        # an island at the silhouette (or the image border) is part of the outline
        if not opaque[win][ring].all() or region.shape != (sl[0].stop - sl[0].start + 2, sl[1].stop - sl[1].start + 2):
            continue
        colour = _surrounding_colour(packed[win], opaque[win] & ~dark[win], region)
        if colour is not None:
            img[win][..., :3][region] = _unpack(np.array([colour]))[0]
            fixed += 1
    return fixed


def remove_slivers(img):
    opaque = img[..., 3] > 0
    dark = _dark_stone(img)
    packed = _packed(img[..., :3])
    fixed = 0
    for axis_structure in (np.array([[0, 1, 0], [0, 1, 0], [0, 1, 0]], bool), np.array([[0, 0, 0], [1, 1, 1], [0, 0, 0]], bool)):
        runs, _ = ndimage.label(dark, axis_structure)
        for label, sl in enumerate(ndimage.find_objects(runs), start=1):
            length = max(sl[0].stop - sl[0].start, sl[1].stop - sl[1].start)
            if length > SLIVER_MAX_LENGTH or (runs[sl] == label).sum() == 1:
                continue
            win = _window(sl, dark.shape)
            region = runs[win] == label
            if region.shape != (sl[0].stop - sl[0].start + 2, sl[1].stop - sl[1].start + 2):
                continue   # at the image border
            ring = ndimage.binary_dilation(region, EIGHT) & ~region
            # a sliver is alone: no dark pixel touches it from the side
            if (ring & dark[win]).any() or not opaque[win][ring].all():
                continue
            colour = _surrounding_colour(packed[win], opaque[win] & ~dark[win], region)
            if colour is not None:
                img[win][..., :3][region] = _unpack(np.array([colour]))[0]
                dark[win][region] = False
                fixed += 1
    return fixed


def remove_bright_specks(img):
    opaque = img[..., 3] > 0
    lum = _luminance(img[..., :3].astype(float))
    h, w = lum.shape
    p = np.pad(np.where(opaque, lum, np.nan), 1, constant_values=np.nan)
    neighbours = np.stack([p[1 + dy:1 + dy + h, 1 + dx:1 + dx + w] for dy in (-1, 0, 1) for dx in (-1, 0, 1) if dy or dx], -1)
    with warnings.catch_warnings():
        warnings.simplefilter("ignore", RuntimeWarning)   # pixels with no opaque neighbour have no median
        surround = np.nanmedian(neighbours, -1)
    bright = opaque & (lum - surround > SPECK_CONTRAST)
    # part of a highlight line: a bright neighbour on both sides along a row or a column
    bp = np.pad(bright, 1)
    in_line = (bp[1:-1, :-2] & bp[1:-1, 2:]) | (bp[:-2, 1:-1] & bp[2:, 1:-1])
    specks = bright & ~in_line
    packed = _packed(img[..., :3])
    pp = np.pad(np.where(opaque & ~bright, packed, -1), 1, constant_values=-1)
    h, w = packed.shape
    around = np.stack([pp[1 + dy:1 + dy + h, 1 + dx:1 + dx + w] for dy in (-1, 0, 1) for dx in (-1, 0, 1) if dy or dx], -1)
    value, count = neighbour_mode(around[specks])
    take = count > 0
    ys, xs = np.nonzero(specks)
    img[ys[take], xs[take], :3] = _unpack(value[take])
    return int(take.sum())


def replace_lone_pixels(img):
    opaque = img[..., 3] > 0
    packed = _packed(img[..., :3])
    p = np.pad(np.where(opaque, packed, -1), 1, constant_values=-1)
    h, w = packed.shape
    neighbours = np.stack([p[1 + dy:1 + dy + h, 1 + dx:1 + dx + w] for dy in (-1, 0, 1) for dx in (-1, 0, 1) if dy or dx], -1)
    shares = (neighbours == packed[..., None]).sum(-1)
    candidates = opaque & (shares == 0) & (neighbours >= 0).all(-1)
    value, count = neighbour_mode(neighbours[candidates])
    take = count >= 5
    ys, xs = np.nonzero(candidates)
    img[ys[take], xs[take], :3] = _unpack(value[take])
    return int(take.sum())


def tidy(img):
    """Cleans img (RGBA uint8, changed in place); returns how many artifacts of each kind it fixed."""
    return {
        "holes": fill_holes(img),
        "dark islands": remove_dark_islands(img),
        "slivers": remove_slivers(img),
        "bright specks": remove_bright_specks(img),
        "lone pixels": replace_lone_pixels(img),
    }
