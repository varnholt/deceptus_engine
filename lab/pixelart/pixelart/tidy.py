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
import numpy as np
from scipy import ndimage

MAX_HOLE_PX = 4           # gaps between meshes are a few pixels; anything larger may be a real opening (a cross ring, a fence)
MAX_ISLAND_PX = 3         # dark groups up to this size are specks; joints and cracks are longer
DARK_LUMINANCE = 32       # joint and outline colours are below this
SLIVER_MAX_LENGTH = 4     # shorter one-pixel dark runs are slivers, longer ones are drawn lines
SPECK_CONTRAST = 40       # a speck is at least this much brighter than the median of its neighbours

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


def _surrounding_colour(packed, mask, region):
    """Most common colour on the one-pixel ring around region, among pixels in mask."""
    ring = ndimage.binary_dilation(region, EIGHT) & ~region & mask
    vals, counts = np.unique(packed[ring], return_counts=True)
    return vals[counts.argmax()] if len(vals) else None


def fill_holes(img):
    opaque = img[..., 3] > 0
    holes, n = ndimage.label(~opaque)
    fixed = 0
    h, w = opaque.shape
    for label, sl in enumerate(ndimage.find_objects(holes), start=1):
        region = holes[sl] == label
        if region.sum() > MAX_HOLE_PX:
            continue
        if sl[0].start == 0 or sl[1].start == 0 or sl[0].stop == h or sl[1].stop == w:
            continue
        full = np.zeros_like(opaque)
        full[sl] = region
        ring = ndimage.binary_dilation(full, EIGHT) & ~full
        if not opaque[ring].all():
            continue
        lum = _luminance(img[..., :3][ring].astype(float))
        img[..., :3][full] = img[..., :3][ring][lum.argmin()]
        img[..., 3][full] = 255
        fixed += 1
    return fixed


def remove_dark_islands(img):
    opaque = img[..., 3] > 0
    dark = _dark_stone(img)
    islands, _ = ndimage.label(dark, EIGHT)
    packed = _packed(img[..., :3])
    fixed = 0
    for label, sl in enumerate(ndimage.find_objects(islands), start=1):
        region = np.zeros_like(dark)
        region[sl] = islands[sl] == label
        if region.sum() > MAX_ISLAND_PX:
            continue
        ring = ndimage.binary_dilation(region, EIGHT) & ~region
        # an island at the silhouette is part of the outline
        if not opaque[ring].all():
            continue
        colour = _surrounding_colour(packed, opaque & ~dark, region)
        if colour is not None:
            img[..., :3][region] = _unpack(np.array([colour]))[0]
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
            region = np.zeros_like(dark)
            region[sl] = runs[sl] == label
            length = max(sl[0].stop - sl[0].start, sl[1].stop - sl[1].start)
            # a sliver is alone: no dark pixel touches it from the side
            touching = ndimage.binary_dilation(region, EIGHT) & ~region & dark
            if length > SLIVER_MAX_LENGTH or touching.any() or region.sum() == 1:
                continue
            ring = ndimage.binary_dilation(region, EIGHT) & ~region
            if not opaque[ring].all():
                continue
            colour = _surrounding_colour(packed, opaque & ~dark, region)
            if colour is not None:
                img[..., :3][region] = _unpack(np.array([colour]))[0]
                dark &= ~region
                fixed += 1
    return fixed


def remove_bright_specks(img):
    opaque = img[..., 3] > 0
    lum = _luminance(img[..., :3].astype(float))
    h, w = lum.shape
    p = np.pad(np.where(opaque, lum, np.nan), 1, constant_values=np.nan)
    neighbours = np.stack([p[1 + dy:1 + dy + h, 1 + dx:1 + dx + w] for dy in (-1, 0, 1) for dx in (-1, 0, 1) if dy or dx], -1)
    with np.errstate(all="ignore"):
        surround = np.nanmedian(neighbours, -1)
    bright = opaque & (lum - surround > SPECK_CONTRAST)
    # part of a highlight line: a bright neighbour on both sides along a row or a column
    bp = np.pad(bright, 1)
    in_line = (bp[1:-1, :-2] & bp[1:-1, 2:]) | (bp[:-2, 1:-1] & bp[2:, 1:-1])
    specks = bright & ~in_line
    packed = _packed(img[..., :3])
    pp = np.pad(np.where(opaque & ~bright, packed, -1), 1, constant_values=-1)
    fixed = 0
    for y, x in zip(*np.nonzero(specks)):
        around = pp[y:y + 3, x:x + 3].ravel()
        around = around[around >= 0]
        if len(around):
            vals, counts = np.unique(around, return_counts=True)
            img[y, x, :3] = _unpack(np.array([vals[counts.argmax()]]))[0]
            fixed += 1
    return fixed


def replace_lone_pixels(img):
    opaque = img[..., 3] > 0
    packed = _packed(img[..., :3])
    p = np.pad(np.where(opaque, packed, -1), 1, constant_values=-1)
    h, w = packed.shape
    neighbours = np.stack([p[1 + dy:1 + dy + h, 1 + dx:1 + dx + w] for dy in (-1, 0, 1) for dx in (-1, 0, 1) if dy or dx], -1)
    shares = (neighbours == packed[..., None]).sum(-1)
    candidates = opaque & (shares == 0) & (neighbours >= 0).all(-1)
    fixed = 0
    for y, x in zip(*np.nonzero(candidates)):
        vals, counts = np.unique(neighbours[y, x], return_counts=True)
        if counts.max() >= 5:
            img[y, x, :3] = _unpack(np.array([vals[counts.argmax()]]))[0]
            fixed += 1
    return fixed


def tidy(img):
    """Cleans img (RGBA uint8, changed in place); returns how many artifacts of each kind it fixed."""
    return {
        "holes": fill_holes(img),
        "dark islands": remove_dark_islands(img),
        "slivers": remove_slivers(img),
        "bright specks": remove_bright_specks(img),
        "lone pixels": replace_lone_pixels(img),
    }
