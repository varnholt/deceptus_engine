"""Hand-made wear for stone pieces: what makes dstar's blocks look drawn rather than generated.

Generated stones all get the same treatment: a lit line along the whole top edge, the same cut
corner, the same soft mottling. An artist varies it stone by stone:
- the lit top line stops short of the stone's ends, by a different amount on every stone, and is
  sometimes broken once;
- a few stones carry one deliberate mark: a short crack stroke, or a pit (a dark pixel with a lit
  one at its lower right);
- a few stones have a chipped top edge: a notch in the joint's dark.

Everything is decided per stone from a seed, so the same piece always wears the same way, and
expressed in shade steps on the stylizer's ramp, so it adds no colours of its own.
"""
import numpy as np
from scipy import ndimage

TRIM_MAX = 3          # the lit top line stops up to this many pixels short of each end
GAP_CHANCE = 0.25     # share of lit top lines broken once in the middle
MARK_CHANCE = 0.3     # share of stones with one mark
CHIP_CHANCE = 0.15    # share of stones with a chipped top edge
MIN_STONE_PX = 60     # smaller pieces are carving, not blocks: no wear
CRACK_STEPS = 2       # how much darker a crack stroke is
CHIP_STEPS = 3        # a chip goes down to the joint's dark


def same_stone_run(key, stone, dy, dx, limit):
    """For every pixel, how many pixels of the same stone follow in direction (dy, dx), up to limit."""
    run = np.zeros(key.shape, dtype=np.int32)
    alive = stone.copy()
    shifted = key
    for _ in range(limit):
        shifted = np.roll(np.roll(shifted, -dy, 0), -dx, 1)
        alive = alive & (shifted == key)
        run += alive
    return run


def stone_index(key, stone):
    """(index image with -1 off stone, pixel count per stone)."""
    index = np.full(key.shape, -1, dtype=np.int64)
    _, inv = np.unique(key[stone], return_inverse=True)
    index[stone] = inv
    return index, np.bincount(inv)


def highlight_breaks(key, stone, seed):
    """Top-edge pixels to leave unlit: each stone's ends trimmed by its own amount, and on some stones
    a one-pixel gap somewhere along the line."""
    rng = np.random.default_rng(seed)
    index, counts = stone_index(key, stone)
    n = len(counts)
    trim_left = rng.integers(0, TRIM_MAX + 1, n)
    trim_right = rng.integers(0, TRIM_MAX + 1, n)
    gap_at = np.where(rng.random(n) < GAP_CHANCE, rng.random(n), -1.0)
    to_left = same_stone_run(key, stone, 0, -1, 64)
    to_right = same_stone_run(key, stone, 0, 1, 64)
    safe = np.maximum(index, 0)
    broken = stone & ((to_left < trim_left[safe]) | (to_right < trim_right[safe]))
    width = to_left + to_right + 1
    gap = stone & (gap_at[safe] >= 0) & (to_left == np.floor(gap_at[safe] * width).astype(int))
    return broken | gap


def hand_marks(key, stone, interior, seed):
    """Shade-step offsets for a few deliberate marks: crack strokes, pits and chipped top edges."""
    rng = np.random.default_rng(seed + 1)
    index, counts = stone_index(key, stone)
    h, w = key.shape
    delta = np.zeros((h, w), dtype=np.int32)
    kp = np.pad(key, 1, constant_values=-1)
    top_row = stone & (kp[:-2, 1:-1] != key)
    for s, box in enumerate(ndimage.find_objects(index + 1)):
        if (box is None) or (counts[s] < MIN_STONE_PX):
            continue
        own = index[box] == s
        mark = rng.random() < MARK_CHANCE
        chip = rng.random() < CHIP_CHANCE
        if mark:
            ys, xs = np.nonzero(own & interior[box])
            if len(ys):
                add_mark(delta[box], own & interior[box], ys, xs, rng)
        if chip:
            ty, tx = np.nonzero(own & top_row[box])
            if len(ty) > 4:
                k = rng.integers(1, len(ty) - 1)
                delta[box][ty[k], tx[k]] -= CHIP_STEPS
    return delta


def add_mark(delta, inside, ys, xs, rng):
    """One mark at a random inside pixel of a stone (all arrays cut to the stone's box): a crack
    stroke two to four pixels long, or a pit, dark with a lit lower right lip."""
    h, w = inside.shape
    k = rng.integers(len(ys))
    y, x = ys[k], xs[k]
    if rng.random() < 0.5:
        step = 1 if rng.random() < 0.5 else -1
        for i in range(rng.integers(2, 5)):
            yy, xx = y + i, x + i * step
            if (yy < h) and (0 <= xx < w) and inside[yy, xx]:
                delta[yy, xx] -= CRACK_STEPS
        return
    delta[y, x] -= CRACK_STEPS
    if (y + 1 < h) and (x + 1 < w) and inside[y + 1, x + 1]:
        delta[y + 1, x + 1] += 1
