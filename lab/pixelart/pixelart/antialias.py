"""Selective anti-aliasing the way a pixel artist does it on curves, inside a piece only.

A curve or diagonal between two clearly different tones comes out as a staircase. The pixel in each
concave notch of the staircase (the other tone on two neighbouring sides and on the diagonal between
them) takes an in-between tone, which rounds the curve without blurring it. Convex corners are left
alone, so every block keeps its square corners; joints, the outer silhouette and anything the caller
protects are never touched, and the in-between tone is always a colour the piece already has.
"""
import numpy as np

from .grade import lum

# the two sides of a notch must differ at least this much in brightness (0-255) to need softening
MIN_CONTRAST = 24.0
# a notch side counts as one tone when its three pixels lie this close in brightness
SIDE_SPREAD = 14.0
# the four notch orientations: two orthogonal neighbours and the diagonal between them
NOTCHES = (((-1, 0), (0, -1), (-1, -1)), ((-1, 0), (0, 1), (-1, 1)),
           ((1, 0), (0, -1), (1, -1)), ((1, 0), (0, 1), (1, 1)))


def _shifted(a, dy, dx, fill):
    """a[y + dy, x + dx] for every pixel, `fill` beyond the border."""
    pad = np.pad(a, ((1, 1), (1, 1)) + ((0, 0),) * (a.ndim - 2), constant_values=fill)
    h, w = a.shape[:2]
    return pad[1 + dy:1 + dy + h, 1 + dx:1 + dx + w]


def nearest_colours(targets, palette):
    """The palette colour closest to each target colour (RGB)."""
    d = ((targets[:, None, :] - palette[None, :, :]) ** 2).sum(-1)
    return palette[d.argmin(1)]


def soften_notches(rgba, protected, strength=0.5):
    """Gives the concave staircase notches of the unprotected opaque pixels an in-between tone:
    `strength` of the way from the pixel's own colour to the colour of the side it notches into.
    Returns the number of pixels changed."""
    if strength <= 0:
        return 0
    rgb = rgba[..., :3].astype(np.float64)
    bright = lum(rgb)
    free = (rgba[..., 3] >= 128) & ~protected
    target = np.zeros_like(rgb)
    found = np.zeros(free.shape, dtype=bool)
    for offsets in NOTCHES:
        sides = [_shifted(bright, dy, dx, 0.0) for dy, dx in offsets]   # border pixels fail `usable`
        colours = [_shifted(rgb, dy, dx, 0.0) for dy, dx in offsets]
        usable = np.ones(free.shape, dtype=bool)
        for dy, dx in offsets:
            usable &= _shifted(free, dy, dx, False)
        side = np.stack(sides, -1)
        one_tone = (side.max(-1) - side.min(-1)) <= SIDE_SPREAD
        side_mean = side.mean(-1)
        contrast = np.abs(side_mean - bright) >= MIN_CONTRAST
        # the pixel's other two neighbours stay on its own side, or it is a lone speck, not a notch
        across = [(-dy, -dx) for dy, dx in offsets[:2]]
        own = np.ones(free.shape, dtype=bool)
        for dy, dx in across:
            own &= np.abs(_shifted(bright, dy, dx, 0.0) - bright) < np.abs(side_mean - bright) / 2
        hit = free & usable & one_tone & contrast & own & ~found
        target[hit] = (sum(colours) / 3.0)[hit]
        found |= hit
    if not found.any():
        return 0
    palette = np.unique(rgba[..., :3][rgba[..., 3] >= 128].reshape(-1, 3), axis=0).astype(np.float64)
    mixed = rgb[found] + strength * (target[found] - rgb[found])
    rgba[..., :3][found] = nearest_colours(mixed, palette).astype(np.uint8)
    return int(found.sum())
