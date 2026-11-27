"""Reduces render passes made at a multiple of level size back to one pixel per level pixel.

A pass rendered at level size takes each pixel from one sample of the scene: scan texture and
edges come out as speckle and half-tones that the stylizer then carries into the art. Rendered
larger and reduced, every level pixel is decided by the whole area it covers:
- beauty and normals: the mean of the covered samples, weighted by coverage;
- ids: the stone covering most of the pixel; joints by coverage, so a one-pixel joint stays one
  pixel wide wherever it falls on the larger grid (see joint_pixels);
- light, turn, albedo and ao (paint stage): the mean, like beauty; classes: the class covering most of it;
  depth (16-bit grey): the nearest sample, so silhouettes keep the object in front;
- alpha: solid where samples cover at least half of the pixel;
- albedo also as albedo_mode: the most common colour of each block (colours compared at MODE_LEVELS
  levels per channel), so a small painted feature (an iris, a stripe) keeps its own colour instead of
  blending with its surroundings into a colour that is in none of them.

usage: python -m pixelart.reduce <in_dir> <out_dir> <piece> <scale>
"""
import shutil
import sys
from pathlib import Path

import numpy as np
from PIL import Image

PASSES = ("beauty", "ids", "normals")
PAINT_PASSES = ("light", "turn", "classes", "albedo", "depth", "ao")   # only pieces with paint_passes render these
OPAQUE = 128
MODE_LEVELS = 32
NO_STONE = -1


def blocks(rgba, scale):
    """The image as (rows, scale, columns, scale, 4) blocks, cropped to whole blocks."""
    h, w = rgba.shape[0] // scale, rgba.shape[1] // scale
    return rgba[:h * scale, :w * scale].reshape(h, scale, w, scale, 4)


def mean_pass(rgba, scale):
    """Colour as the coverage-weighted mean of each block, alpha as its coverage."""
    b = blocks(rgba.astype(np.float64), scale)
    alpha = b[..., 3:4] / 255.0
    colour = (b[..., :3] * alpha).sum(axis=(1, 3)) / np.maximum(alpha.sum(axis=(1, 3)), 1e-6)
    out = np.concatenate([colour, alpha.mean(axis=(1, 3)) * 255.0], axis=-1)
    return np.clip(np.round(out), 0, 255).astype(np.uint8)


def joint_pixels(cover):
    """Joint where it covers more than half of a pixel, or exactly half when the pixel above and the
    pixel to the left are not joint already: a joint split evenly over two pixels stays one wide."""
    up = np.pad(cover, ((1, 0), (0, 0)))[:-1]
    left = np.pad(cover, ((0, 0), (1, 0)))[:, :-1]
    return (cover > 0.5) | ((cover == 0.5) & (up < 0.5) & (left < 0.5))


def majority(values):
    """The most frequent non-negative value along the last axis; NO_STONE where there is none."""
    counts = (values[..., :, None] == values[..., None, :]).sum(-1)
    counts = np.where(values >= 0, counts, -1)
    best = np.take_along_axis(values, counts.argmax(-1)[..., None], -1)[..., 0]
    return np.where(counts.max(-1) > 0, best, NO_STONE)


def majority_pass(rgba, scale):
    """Each pixel takes the colour covering most of it (flat-colour passes such as the classes)."""
    b = blocks(rgba.astype(np.int64), scale)
    covered = b[..., 3] >= OPAQUE
    key = (b[..., 0] << 16) | (b[..., 1] << 8) | b[..., 2]
    h, w = key.shape[0], key.shape[2]
    values = np.where(covered, key, NO_STONE).transpose(0, 2, 1, 3).reshape(h, w, scale * scale)
    known = np.maximum(majority(values), 0)
    out = np.zeros((h, w, 4), dtype=np.uint8)
    out[..., 0], out[..., 1], out[..., 2] = (known >> 16) & 255, (known >> 8) & 255, known & 255
    out[..., 3] = np.where(covered.mean(axis=(1, 3)) >= 0.5, 255, 0)
    return out


def mode_pass(rgba, scale):
    """Each pixel takes the most common colour among its covered samples (colours compared after
    quantizing to MODE_LEVELS levels; the mean of the winning samples is kept)."""
    b = blocks(rgba.astype(np.int64), scale)
    h, w = b.shape[0], b.shape[2]
    samples = b.transpose(0, 2, 1, 3, 4).reshape(h, w, scale * scale, 4)
    q = samples[..., :3] * MODE_LEVELS // 256
    code = np.where(samples[..., 3] >= OPAQUE, (q[..., 0] * MODE_LEVELS + q[..., 1]) * MODE_LEVELS + q[..., 2], NO_STONE)
    winner = majority(code)
    chosen = (code == winner[..., None]) & (winner[..., None] >= 0)
    colour = (samples[..., :3] * chosen[..., None]).sum(2) / np.maximum(chosen.sum(2), 1)[..., None]
    out = np.zeros((h, w, 4), dtype=np.uint8)
    out[..., :3] = np.round(colour)
    out[..., 3] = np.where((samples[..., 3] >= OPAQUE).mean(2) >= 0.5, 255, 0)
    return out


def depth_pass(grey16, scale):
    """16-bit depth: the nearest sample of each pixel, so a silhouette keeps its front object."""
    h, w = grey16.shape[0] // scale, grey16.shape[1] // scale
    b = grey16[:h * scale, :w * scale].reshape(h, scale, w, scale)
    return b.max(axis=(1, 3)).astype(np.uint16)   # near = high values


def ids_pass(rgba, scale):
    """Stone ids by majority, joints by coverage (see joint_pixels)."""
    b = blocks(rgba.astype(np.int64), scale)
    key = (b[..., 0] << 16) | (b[..., 1] << 8) | b[..., 2]
    covered = b[..., 3] >= OPAQUE
    seam = covered & (key == 0)
    h, w = key.shape[0], key.shape[2]
    stones = np.where(covered & ~seam, key, NO_STONE).transpose(0, 2, 1, 3).reshape(h, w, scale * scale)
    out_key = majority(stones)
    out_key[joint_pixels(seam.mean(axis=(1, 3)))] = 0
    out = np.zeros((h, w, 4), dtype=np.uint8)
    known = np.maximum(out_key, 0)
    out[..., 0], out[..., 1], out[..., 2] = (known >> 16) & 255, (known >> 8) & 255, known & 255
    out[..., 3] = np.where(covered.mean(axis=(1, 3)) >= 0.5, 255, 0)
    return out


def reduce_passes(src, dst, piece, scale):
    """Writes the piece's passes from src (rendered at scale) to dst at level size."""
    src, dst = Path(src), Path(dst)
    dst.mkdir(parents=True, exist_ok=True)
    reducers = {"ids": ids_pass, "classes": majority_pass}
    for kind in PASSES + PAINT_PASSES:
        path = src / f"{piece}_{kind}.png"
        if (kind in PAINT_PASSES) and not path.exists():
            continue
        if kind == "depth":
            Image.fromarray(depth_pass(np.asarray(Image.open(path)), scale)).save(dst / f"{piece}_{kind}.png")
            continue
        rgba = np.asarray(Image.open(path).convert("RGBA"))
        Image.fromarray(reducers.get(kind, mean_pass)(rgba, scale)).save(dst / f"{piece}_{kind}.png")
        if kind == "albedo":
            Image.fromarray(mode_pass(rgba, scale)).save(dst / f"{piece}_albedo_mode.png")
    lettering = src / f"{piece}_inscriptions.json"   # already in level pixels
    if lettering.exists():
        shutil.copy(lettering, dst / lettering.name)


if __name__ == "__main__":
    a = sys.argv
    reduce_passes(a[1], a[2], a[3], int(a[4]))
