"""Epitaphs carved into finished pixel art, on the faces the render recorded (graves.lettering
writes <piece>_inscriptions.json).

Modelled grooves blur into lines at one pixel per level pixel; drawn glyphs read as writing:
- each glyph 2-3 pixels wide and 3 high, with at least one vertical stem
- glyphs grouped into words, lines centred, the first line (the name) the widest
- worn glyphs are faint or gone, more of them the lower the line
- a carving is a darker cut with a lighter lip on its lower right, where the cut's wall
  catches the moon

Only pixels of the face itself are carved (same normal as the face's middle, not moss or leaves),
and every colour comes from the piece's own stone colours, so the palette stays as it was.
"""
import json

import numpy as np

GLYPH_HEIGHT = 3
WORD_GAP = 2
WEAR = 0.6
CUT_SHADE = {1.0: 0.55, 0.45: 0.78}   # carving depth -> brightness factor of the cut
LIP_SHADE = 1.2
FACE_NORMAL_TOLERANCE = 30            # normal-map distance that still counts as the same face
FACE_SAMPLE_RADIUS = 3


def random_glyph(rng):
    """A letter-like mark: a stroke pattern with at least one vertical stem, so it reads as
    writing rather than as a dot."""
    width = int(rng.integers(2, 4))
    cells = rng.random((GLYPH_HEIGHT, width)) < 0.55
    cells[:, int(rng.integers(0, width))] = True
    return cells


def epitaph(lines, half_width, line_pitch, rng, wear=WEAR):
    """Carving depth per glyph cell {(column, row): depth}: columns from the face's centre line,
    rows down from the top of the first line."""
    cells = {}
    for line in range(lines):
        row_top = line * line_pitch
        span = int(half_width - (0 if line == 0 else 1 + line))
        x = -span + int(rng.integers(0, 2))
        while x < span - 1:
            for _ in range(int(rng.integers(2, 5))):
                glyph = random_glyph(rng)
                if x + glyph.shape[1] > span:
                    break
                fate = rng.random() - 0.08 * line
                if fate > 0.2 * wear:
                    depth = 1.0 if fate > 0.5 * wear else 0.45
                    for gy, gx in zip(*np.nonzero(glyph)):
                        cells[(x + int(gx), row_top + int(gy))] = depth
                x += glyph.shape[1] + 1
            x += WORD_GAP
    return cells


def stone_colours(art):
    """The piece's stone colours (opaque, not green), to pick carving shades from."""
    opaque = art[..., 3] > 0
    rgb = art[..., :3][opaque].astype(int)
    stone = rgb[~is_green(rgb)]
    return np.unique(stone, axis=0)


def is_green(rgb):
    rgb = np.asarray(rgb, dtype=int)
    return (rgb[..., 1] > rgb[..., 0]) & (rgb[..., 1] > rgb[..., 2])


def nearest_colour(colours, target):
    return colours[np.argmin(((colours - target) ** 2).sum(axis=1))]


def face_normal(art, normals, x, y):
    """The face's normal: the median over opaque stone pixels around its middle, so an emblem
    or a crack at that spot does not decide it."""
    h, w = art.shape[:2]
    y0, y1 = max(y - FACE_SAMPLE_RADIUS, 0), min(y + FACE_SAMPLE_RADIUS + 1, h)
    x0, x1 = max(x - FACE_SAMPLE_RADIUS, 0), min(x + FACE_SAMPLE_RADIUS + 1, w)
    patch = normals[y0:y1, x0:x1].reshape(-1, 3)
    opaque = (art[y0:y1, x0:x1, 3] > 0).reshape(-1)
    return np.median(patch[opaque], axis=0) if opaque.any() else None


def on_face(art, normals, reference, x, y):
    h, w = art.shape[:2]
    if not ((0 <= x < w) and (0 <= y < h)) or art[y, x, 3] == 0 or is_green(art[y, x, :3]):
        return False
    return np.abs(normals[y, x].astype(int) - reference).sum() <= FACE_NORMAL_TOLERANCE


def carve_area(art, normals, area, colours):
    """Carve one epitaph into art (RGBA, changed in place) along the area's face directions."""
    centre = np.array(area["centre"])
    right, up = np.array(area["right"]), np.array(area["up"])
    scale = np.linalg.norm(right)
    right, up = right / scale, up / np.linalg.norm(up)
    reference = face_normal(art, normals, int(centre[0]), int(centre[1]))
    if reference is None:
        return 0
    rng = np.random.default_rng(area["seed"])
    pitch = max(GLYPH_HEIGHT + 1, int(round(area["line_pitch_px"] * scale)))
    cells = epitaph(area["lines"], area["half_px"] * scale, pitch, rng)
    cut = {}
    for (column, row), depth in cells.items():
        x, y = np.round(centre + right * (column + 0.5) - up * row).astype(int)
        if on_face(art, normals, reference, x, y):
            cut[(x, y)] = max(depth, cut.get((x, y), 0))
    original = art.copy()
    for (x, y), depth in cut.items():
        art[y, x, :3] = nearest_colour(colours, original[y, x, :3] * CUT_SHADE[depth])
    for (x, y), depth in cut.items():
        lip = (x + 1, y + 1)
        if (lip not in cut) and on_face(original, normals, reference, *lip):
            shade = 1.0 + (LIP_SHADE - 1.0) * depth
            art[lip[1], lip[0], :3] = nearest_colour(colours, original[lip[1], lip[0], :3] * shade)
    return len(cut)


def inscribe(art_path, normals_path, areas_path):
    """Carve every recorded epitaph into the art file; returns the number of carved pixels."""
    from PIL import Image
    art = np.array(Image.open(art_path).convert("RGBA"))
    normals = np.array(Image.open(normals_path).convert("RGB"))
    areas = json.loads(open(areas_path, encoding="utf-8").read())
    colours = stone_colours(art)
    carved = sum(carve_area(art, normals, area, colours) for area in areas)
    Image.fromarray(art).save(art_path)
    return carved
