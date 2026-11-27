"""Snaps fake pixel art back onto a real pixel grid.

Images that only look like pixel art (AI output, scaled-up screenshots, renders at the wrong size)
have blocks of roughly N x N pixels with blurred, drifting borders and far too many colours. This
finds the block grid, gives every block one colour and limits the palette:

1. Grid, per axis. Where the colour gradient along a line kinks lies the block structure: on
   both sides of a sharp border, and at every source pixel centre when the image was scaled
   with blur (an even ramp between centres, no edge at the border). Kink strength is measured
   per line, so lines kinking in opposite directions do not cancel, and summed per position.
   - Even grids (any scale, also non-integer): for every candidate block size the positions are
     wound around a circle of that circumference, weighted by squared kink strength; on the true
     size they land at one angle. A size dividing the true one aligns them too; a multiple, or
     the frame spacing of a sprite sheet, spreads them; so the largest size aligning them as
     well as the best one is the block size, refined finely because a small error drifts over
     hundreds of blocks.
   - Uneven grids (AI output: blocks of 4 and 5 px mixed): the kinks do not align over long
     distances; the block size is then the typical spacing of neighbouring sharp edges.
   The phase is the one whose blocks hold the least colour variance. The borders are then
   walked one by one, each pulled onto a sharp edge within reach where there is one, so an
   uneven grid is followed. A grid only counts if its blocks are clearly more uniform than
   those of the same grid shifted by half a block; an image without one (already native pixel
   art) keeps one block per pixel.
2. Palette: the colours from a palette image, or `colours` colours clustered from the image
   (k-means in Oklab, where distance follows perceived difference), each moved onto the nearest
   colour the image really contains so no averaged in-between shade appears.
3. Blocks: every pixel takes its nearest palette colour; a block takes its most common one,
   counted over the block's inside only, because the borders carry the blur. One-pixel lines,
   outlines and dithering survive because a majority, not an average, decides. A block that is
   mostly transparent stays transparent.

usage: python -m pixelart.snap <in.png> <out.png> [--colours N | --palette pal.png]
                               [--cell SIZE] [--preview SCALE]
"""
import argparse
from dataclasses import dataclass

import numpy as np
from PIL import Image

OPAQUE = 128                 # alpha from which a pixel counts as painted
SMALLEST_CELL = 2.0          # a block smaller than this is no block: the image is already native
LARGEST_CELL = 64.0
MIN_BLOCKS = 8               # a block size must fit this often along the axis
CELL_STEP = 0.05             # candidate block sizes are tried at this resolution, for non-integer scales
FINE_STEP = 0.001            # the block size is refined to this, so hundreds of blocks do not drift
EVEN_GRID = 0.6              # kinks aligning this well on one size make an even grid
EQUAL_ALIGNMENT = 0.9        # a larger size aligning this close to the best one is the block size
GRID_UNIFORMITY = 0.75       # a grid's blocks hold at most this share of the variance of the half-shifted grid
UNEVEN_UNIFORMITY = 0.4      # stricter for walked borders: landing on sharp edges lowers their variance even in
                             # native art (a 1:1 render passed at 0.71; real uneven grids stay below 0.25)
PEAK_FLOOR = 0.05           # edges weaker than this share of the strong ones are noise
SHARP_EDGE = 1.5             # an edge is sharp when it is this much stronger than its neighbours
PHASE_STEP = 0.25            # phases tried per block, in source pixels
SNAP_REACH = 0.3             # a border may move this share of a block onto a sharp edge
BORDER_TRIM = 0.2            # share of a block's width on each side ignored when counting colours
KMEANS_ROUNDS = 16
KMEANS_SAMPLE = 60000
SEED = 0


@dataclass
class Grid:
    """Block borders along x and y, from 0 to the image size, and the block size they follow."""
    xs: np.ndarray
    ys: np.ndarray
    cell_w: float
    cell_h: float


def sharp_edges(lines):
    """Border positions (pixel index) of edges clearly stronger than the change next to them,
    and their strengths."""
    profile = np.abs(np.diff(lines, axis=1)).sum(axis=(0, 2))
    left = np.r_[0.0, profile[:-1]]
    right = np.r_[profile[1:], 0.0]
    floor = PEAK_FLOOR * np.percentile(profile, 99)
    sharp = (profile >= left) & (profile > right) & (profile > floor) & (profile >= SHARP_EDGE * (left + right) / 2)
    at = np.flatnonzero(sharp)
    return at + 1, profile[at]   # profile entry i is the border before pixel i + 1


def alignment(positions, weights, sizes):
    """For each size: how well the positions share one phase, 0 scattered .. 1 aligned."""
    turns = np.exp(2j * np.pi * positions[None, :] / sizes[:, None]) @ weights / max(weights.sum(), 1e-9)
    return np.abs(turns)


def even_block_size(lines, largest):
    """(block size, alignment) of the even grid the kinks along the lines follow best."""
    kinks = np.abs(np.diff(lines, n=2, axis=1)).sum(axis=(0, 2))
    positions, weights = np.arange(len(kinks)) + 1.0, kinks ** 2   # kink entry i sits on pixel i + 1
    coarse = np.arange(SMALLEST_CELL, largest + 1e-9, CELL_STEP)
    aligned = alignment(positions, weights, coarse)
    size = coarse[np.flatnonzero(aligned >= aligned.max() * EQUAL_ALIGNMENT)[-1]]
    fine = np.arange(size - CELL_STEP, size + CELL_STEP, FINE_STEP)
    aligned = alignment(positions, weights, fine)
    return fine[aligned.argmax()], aligned.max()


def uneven_block_size(edges, largest):
    """The typical spacing of neighbouring sharp edges: the most common spacing, averaged with
    the spacings one pixel either side of it (an uneven grid mixes neighbouring sizes)."""
    spacing = np.diff(edges)
    spacing = spacing[(spacing >= SMALLEST_CELL) & (spacing <= largest)]
    if len(spacing) < MIN_BLOCKS:
        return None
    common = np.bincount(spacing).argmax()
    return float(spacing[np.abs(spacing - common) <= 1].mean())


def block_size(lines, edges):
    """(block size, whether the grid is even) along the lines; size None when they show no
    block structure."""
    largest = min(LARGEST_CELL, lines.shape[1] / MIN_BLOCKS)
    if largest < SMALLEST_CELL:
        return None, False
    size, aligned = even_block_size(lines, largest)
    if aligned >= EVEN_GRID:
        return size, True
    return uneven_block_size(edges, largest), False


def borders_at(size, phase, length):
    """Evenly spaced borders from 0 to `length`, none closer than half a block to the ends."""
    inner = np.round(np.arange(phase, length, size)).astype(int)
    inner = inner[(inner >= size / 2) & (inner <= length - size / 2)]
    return np.r_[0, np.unique(inner), length]


def running_sums(lines):
    """Running sums of the values and their squares along the lines' second dimension."""
    values = lines.astype(np.float64)
    zero = np.zeros((values.shape[0], 1, values.shape[2]))
    return (np.concatenate([zero, values.cumsum(axis=1)], axis=1),
            np.concatenate([zero, (values ** 2).cumsum(axis=1)], axis=1))


def block_variance(sums, borders):
    """Total colour variance inside the blocks between the borders."""
    running, running_sq = sums
    width = np.diff(borders).astype(np.float64)[None, :, None]
    total = np.diff(running[:, borders], axis=1)
    total_sq = np.diff(running_sq[:, borders], axis=1)
    return float((total_sq - total ** 2 / width).sum())


def best_phase(sums, size, length):
    """The phase whose blocks are most uniform inside."""
    phases = np.arange(0.0, size, PHASE_STEP)
    return min(phases, key=lambda phase: block_variance(sums, borders_at(size, phase, length)))


def walk_borders(size, phase, edges, strengths, length):
    """Borders from 0 to `length`, one block apart, each pulled onto the strongest sharp edge
    within reach; the next one is expected a block after where this one landed."""
    reach = max(1.0, size * SNAP_REACH)
    borders, expected = [0], phase
    while expected < length:
        near = np.abs(edges - expected) <= reach
        landed = float(edges[near][np.argmax(strengths[near])]) if near.any() else expected
        if (landed >= size / 2) and (landed <= length - size / 2) and ((landed - borders[-1]) >= size / 2):
            borders.append(int(round(landed)))
        expected = landed + size
    borders.append(length)
    return np.array(borders)


def half_shifted(borders, size, length):
    """The same borders moved half a block on: the worst grid of that size, as a yardstick."""
    inner = np.clip(borders[1:-1] + int(round(size / 2)), 1, length - 1)
    return np.unique(np.r_[0, inner, length])


def axis_borders(lines, cell=None):
    """(borders, block size) along the lines' second dimension; one block per pixel when the
    lines show no grid."""
    length = lines.shape[1]
    native = (np.arange(length + 1), 1.0)
    edges, strengths = sharp_edges(lines)
    size, is_even = (float(cell), True) if cell else block_size(lines, edges)
    if size is None:
        return native
    sums = running_sums(lines)
    phase = best_phase(sums, size, length)
    # an even grid's size is exact; walking it along edges would only follow stray shading
    borders = borders_at(size, phase, length) if is_even else walk_borders(size, phase, edges, strengths, length)
    uniformity = GRID_UNIFORMITY if is_even else UNEVEN_UNIFORMITY
    is_grid = block_variance(sums, borders) <= uniformity * block_variance(sums, half_shifted(borders, size, length))
    if (cell is None) and not is_grid:
        return native
    return borders, size


def detect_grid(rgba, cell=None):
    """The block grid of a fake pixel-art image; a native image gets one block per pixel."""
    painted = rgba.astype(np.float32)
    painted[..., :3] *= painted[..., 3:4] / 255.0   # transparent pixels count as black, so outlines show
    xs, cell_w = axis_borders(painted, cell)
    ys, cell_h = axis_borders(painted.transpose(1, 0, 2), cell)
    return Grid(xs, ys, cell_w, cell_h)


def to_oklab(rgb):
    """sRGB (0-255) to Oklab, where straight distances match perceived colour differences."""
    c = rgb.astype(np.float64) / 255.0
    lin = np.where(c <= 0.04045, c / 12.92, ((c + 0.055) / 1.055) ** 2.4)
    lms = lin @ np.array([[0.4122214708, 0.2119034982, 0.0883024619],
                          [0.5363325363, 0.6806995451, 0.2817188376],
                          [0.0514459929, 0.1073969566, 0.6299787005]])
    return np.cbrt(lms) @ np.array([[0.2104542553, 1.9779984951, 0.0259040371],
                                    [0.7936177850, -2.4285922050, 0.7827717662],
                                    [-0.0040720468, 0.4505937099, -0.8086757660]])


def nearest(points, centres, chunk=65536):
    """Index of the nearest centre for every point."""
    out = np.empty(len(points), dtype=np.int32)
    for start in range(0, len(points), chunk):
        part = points[start:start + chunk]
        out[start:start + chunk] = ((part[:, None, :] - centres[None, :, :]) ** 2).sum(-1).argmin(1)
    return out


def clustered_palette(colours, count):
    """`count` colours of the image: k-means in Oklab, each centre then moved onto the nearest
    colour that really occurs."""
    distinct = np.unique(colours, axis=0)
    if len(distinct) <= count:
        return distinct
    rng = np.random.default_rng(SEED)
    sample = colours[rng.choice(len(colours), min(len(colours), KMEANS_SAMPLE), replace=False)]
    lab = to_oklab(sample)
    centres = [lab[rng.integers(len(lab))]]
    for _ in range(count - 1):   # k-means++: spread the starting centres
        dist = ((lab[:, None, :] - np.array(centres)[None]) ** 2).sum(-1).min(1)
        centres.append(lab[rng.choice(len(lab), p=dist / dist.sum())])
    centres = np.array(centres)
    for _ in range(KMEANS_ROUNDS):
        owner = nearest(lab, centres)
        for k in range(count):
            members = lab[owner == k]
            if len(members):
                centres[k] = members.mean(0)
    distinct_lab = to_oklab(distinct)
    return np.unique(distinct[nearest(centres, distinct_lab)], axis=0)


def palette_from_image(path):
    """The distinct painted colours of a palette image."""
    rgba = np.asarray(Image.open(path).convert("RGBA"))
    painted = rgba[rgba[..., 3] >= OPAQUE][:, :3]
    colours = np.unique(painted, axis=0)
    if not len(colours):
        raise ValueError(f"{path} holds no painted colours")
    return colours


def block_colours(rgba, grid, palette):
    """One RGBA pixel per block: the most common palette colour inside it, or transparent."""
    h, w = rgba.shape[:2]
    cols, rows = len(grid.xs) - 1, len(grid.ys) - 1
    col_of, inner_x = block_of_pixel(grid.xs, w)
    row_of, inner_y = block_of_pixel(grid.ys, h)
    block = (row_of[:, None] * cols + col_of[None, :]).ravel()
    inner = (inner_y[:, None] & inner_x[None, :]).ravel()
    painted = (rgba[..., 3] >= OPAQUE).ravel()
    counted = inner & painted
    index = nearest(to_oklab(rgba[..., :3].reshape(-1, 3)[counted]), to_oklab(palette))
    # the most common palette colour per block: count (block, colour) pairs, keep each block's largest
    pairs, counts = np.unique(block[counted].astype(np.int64) * len(palette) + index, return_counts=True)
    owner = pairs // len(palette)
    order = np.lexsort((-counts, owner))
    first = order[np.r_[True, owner[order][1:] != owner[order][:-1]]]
    out = np.zeros((rows * cols, 4), dtype=np.uint8)
    out[owner[first], :3] = palette[pairs[first] % len(palette)]
    # a block is painted when most of its inside is
    painted_share = np.bincount(block[counted], minlength=rows * cols) / np.maximum(np.bincount(block[inner], minlength=rows * cols), 1)
    out[:, 3] = np.where(painted_share > 0.5, 255, 0)
    out[painted_share <= 0.5, :3] = 0
    return out.reshape(rows, cols, 4)


def block_of_pixel(borders, length):
    """For every pixel along an axis: its block, and whether it lies inside the block's trimmed
    border, where the colour is counted."""
    pixel = np.arange(length)
    block = np.searchsorted(borders, pixel, side="right") - 1
    start, end = borders[block], borders[block + 1]
    trim = ((end - start) * BORDER_TRIM).astype(int)
    return block, (pixel >= start + trim) & (pixel < end - trim)


def snap(rgba, colours=16, palette=None, cell=None):
    """The snapped image (one pixel per block) and the grid it was read on."""
    grid = detect_grid(rgba, cell)
    if palette is None:
        palette = clustered_palette(rgba[rgba[..., 3] >= OPAQUE][:, :3], colours)
    return block_colours(rgba, grid, palette), grid


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("source")
    ap.add_argument("out")
    ap.add_argument("--colours", type=int, default=16, help="palette size read from the image (default 16)")
    ap.add_argument("--palette", help="image whose painted colours are the palette (overrides --colours)")
    ap.add_argument("--cell", type=float, help="block size in source pixels, when detection guesses wrong")
    ap.add_argument("--preview", type=int, default=0, help="also write <out>_xN.png scaled up N times")
    args = ap.parse_args(argv)
    rgba = np.asarray(Image.open(args.source).convert("RGBA"))
    palette = palette_from_image(args.palette) if args.palette else None
    art, grid = snap(rgba, args.colours, palette, args.cell)
    Image.fromarray(art).save(args.out)
    if args.preview > 1:
        stem = args.out.rsplit(".", 1)[0]
        Image.fromarray(art).resize((art.shape[1] * args.preview, art.shape[0] * args.preview), Image.NEAREST).save(f"{stem}_x{args.preview}.png")
    print(f"{args.source}: blocks {grid.cell_w:.2f} x {grid.cell_h:.2f} px -> {art.shape[1]} x {art.shape[0]}, "
          f"{len(np.unique(art[art[..., 3] > 0][:, :3], axis=0))} colours")


if __name__ == "__main__":
    main()
