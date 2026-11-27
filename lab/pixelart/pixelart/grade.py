"""Grading after the conversion, to sit a finished piece into its level: brightness, contrast,
saturation and a pull towards the night blue of the scenery.

It maps every colour of the piece to one new colour, so flat areas stay flat, edges stay where
they are and the pixel structure is untouched; only the palette moves.
"""
import numpy as np

NIGHT = np.array([28.0, 36.0, 92.0])   # the blue of the graveyard's night scenery
LUMA = np.array([0.299, 0.587, 0.114])


def lum(rgb):
    """Perceived brightness of RGB values (last axis)."""
    return rgb[..., 0] * LUMA[0] + rgb[..., 1] * LUMA[1] + rgb[..., 2] * LUMA[2]


def grade(img, gain=1.0, contrast=1.0, saturation=1.0, night=0.0):
    """img: RGBA uint8, changed in place. gain scales brightness, contrast spreads tones around the
    piece's mid tone, saturation scales colourfulness, night (0-1) pulls colours towards NIGHT."""
    if (gain, contrast, saturation, night) == (1.0, 1.0, 1.0, 0.0):
        return
    opaque = img[..., 3] > 0
    colours, index = np.unique(img[..., :3][opaque].reshape(-1, 3), axis=0, return_inverse=True)
    c = colours.astype(float)
    luma = c @ LUMA
    mid = np.median(luma)
    c = luma[:, None] + (c - luma[:, None]) * saturation
    c = mid + (c - mid) * contrast
    c = c * gain
    # night blue at each colour's own brightness, so the pull changes hue, not lightness
    tinted = NIGHT[None, :] * ((c @ LUMA) / (NIGHT @ LUMA))[:, None]
    c = c * (1.0 - night) + tinted * night
    img[..., :3][opaque] = np.clip(np.round(c), 0, 255).astype(np.uint8)[index.ravel()]
