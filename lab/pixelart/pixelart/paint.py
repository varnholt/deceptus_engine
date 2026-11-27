"""Draws a piece from its render passes the way a pixel artist would, instead of reducing the render.

Light and colour come from separate passes, so neither drags the other's noise into the art:
- light (key light alone on white matt surfaces) gives each pixel one of a few light bands: lit,
  half, shade, deep (turned away or in cast shadow), like the steps of an artist's ramp;
- albedo (each material's colour without light) gives each stone's tint and where moss grows;
- ids give the shapes (one id per stone) and the joints (id 0);
- depth gives the relief edges, drawn as lines; ambient occlusion the contact shadows;
- classes give the material (wall stone, trim, glass, roof, foliage, metal) and whether a part
  stands back (side rooms), which picks the ramp and its steps.

Steps, in order:
1. bands from the light, relative to the brightest lit stone of the piece;
2. clusters: patches of one band within one stone smaller than MIN_CLUSTER pixels take the band
   around them, so there is no speckle by construction, and a band on only a small share of a stone
   takes the stone's main band: one tone per stone, as dstar paints his blocks;
3. tones: each band is a fixed step on the material's ramp, a stone a step lighter or darker by its
   albedo against its material's, side rooms a step lower;
4. drawn edges: a lit line along each stone's top where it meets a joint (stopping short and broken
   now and then, wear.py), dstar's block edges (sides lit near the top, a glint at the top corners,
   bottom corners cut into the joint), a lit rim where a raised part faces the light, a dark cast
   line where it ends towards the lower right; hand marks (wear.py);
5. joints a dark step under the stones beside them; moss green where the albedo is; a dark outline
   along the silhouette;
6. inside the stones, in in-between shades: each stone's place within its light band (the light
   falling off across a face, stone by stone), a slight roundness (lighter top rows, darker bottom
   rows), patches from its own albedo texture, never single pixels, and contact shadow where the
   ambient occlusion pass shows an enclosed corner;
7. curve smoothing (antialias.py) and the grade that sits the piece into its level (grade.py);
8. a normal map for the game's lights beside the art (<art>_normals.png), drawn to match the stones.

usage: python -m pixelart.paint <passes dir> <piece> <out.png>
"""
import sys
from pathlib import Path

import numpy as np
from PIL import Image
from scipy import ndimage

from .antialias import soften_notches
from .grade import grade, lum
from .stylize import GREENS, PASSAGE, RAMP, clump_moss, ramp_colour
from .wear import hand_marks, highlight_breaks, same_stone_run

# light bands, as shares of the light on the brightest lit stones: deep | shade | half | lit
BAND_EDGES = (0.12, 0.40, 0.72)
DEEP, SHADE, HALF, LIT = 0, 1, 2, 3
BRIGHT_PERCENTILE = 98
MIN_CLUSTER = 6
STONE_MINORITY = 0.33   # a band on less than this share of a stone is taken into the stone's main band
MARK_STEPS = 1          # hand marks one ramp step darker
# material classes as blender_kit/core.py writes them
SEAM, GLASS, ROOF, FOLIAGE, METAL, TRIM, STONE = 1, 2, 3, 4, 5, 6, 7
# ramp steps per band (deep, shade, half, lit) for each painted material
STEPS = {STONE: (1, 2, 3, 4), TRIM: (1, 2, 3, 5), ROOF: (1, 1, 2, 3), METAL: (0, 1, 1, 2)}
RECEDE_STEPS = 1        # side rooms stand back: one ramp step darker
JOINT_STEPS = 2         # joints sit this many steps under the stones beside them
SIDE_LIGHT_ROWS = 4     # dstar lights a block's sides only this far down from its top
CAST_STEPS = 2          # the line of shadow a raised part throws, this many steps under the stone
TINT_OCTAVE = 0.6       # a stone this many octaves lighter or darker in albedo than its material is one step off
DEPTH_PER_PX = 65535 / (20.0 * 24)   # 16-bit depth units per level pixel (core.DEPTH_RANGE 20 units, 24 px per unit)
RELIEF_STEP_PX = 0.8    # a depth step of this many pixels or more is a relief edge worth a line
MOSS_GREEN = 1.15       # albedo this much greener than red and blue is moss
MOSS_UP = 0.35          # moss only on faces turned up at least this much (normal's up component)
OUTLINE_STEP = 0        # ramp step of the silhouette line
BODY_FLOOR = 1          # the darkest ramp step a stone's face may take
SUB_STEPS = 3           # in-between shades per ramp step, for gradients and texture inside the stones
GRADIENT = 1.0          # how far a stone's place inside its light band moves its tone (in ramp steps)
PILLOW = 1.0 / 3        # stones a sub-step lighter along their top, a sub-step darker along their bottom
PILLOW_ROWS = 2         # rows from a stone's top or bottom that the pillow shading reaches
TEXTURE = 1.0 / 3       # albedo detail lifts or lowers a patch of a stone by this much
TEXTURE_CONTRAST = 0.12 # albedo this share above or below its stone's mean is a patch
MIN_PATCH = 4           # texture patches smaller than this many pixels are dropped (no grain)
PILLOW_TILT = 0.45      # how far a stone's edge rows tilt its normal (up at the top, out at the sides) for the game's lights
AO_OPEN = 0.92          # ambient occlusion above this is open surface, no contact shadow
AO_CLOSED = 0.55        # at or below this a corner counts as fully enclosed (the physics are faint; dstar is not)
AO_STRENGTH = 1.5       # ramp steps a fully enclosed corner goes down
# colour families (models): pixels grouped by albedo colour, each family drawn on its own ramp with
# the family's colour on BASE_STEP (the half-lit band, where most of a model's surface sits), darker
# steps at SHADE_VALUES of it leaning towards a cool SHADOW_HUE, lighter steps mixed towards LIGHT_HUE
BASE_STEP = 3
SHADE_VALUES = (0.18, 0.38, 0.66)
LIGHT_MIX = (0.25, 0.45, 0.62, 0.78)
SHADOW_HUE = np.array([52.0, 36.0, 110.0])
LIGHT_HUE = np.array([255.0, 240.0, 215.0])
MIN_FAMILY_LUM = 40.0   # near-black materials still get a readable ramp
FAMILY_ROUNDS = 12
FAMILY_GRID = 8         # albedo levels per channel when looking for the families (speed, not look)
MIN_FAMILY_PATCH = 2    # a lone pixel of one family inside another takes its neighbours' family
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


def one_tone_per_stone(band, key, surface):
    """A band covering less than STONE_MINORITY of a stone takes the stone's main band: light fall-off
    and penumbra must not paint a lighter patch onto one stone; a cast shadow crossing a stone over a
    large share of it stays."""
    band = band.copy()
    stones, inv = np.unique(key[surface], return_inverse=True)
    counts = np.zeros((len(stones), 4))
    np.add.at(counts, (inv, band[surface]), 1)
    share = counts / counts.sum(1, keepdims=True)
    main = counts.argmax(1)
    minor = share[inv, band[surface]] < STONE_MINORITY
    values = band[surface]
    values[minor] = main[inv][minor]
    band[surface] = values
    return band


def band_steps(band, cls, recede):
    """Ramp step per pixel from its light band and material; side rooms a step lower."""
    step = np.zeros(band.shape, dtype=np.int32)
    for c, steps in STEPS.items():
        here = cls == c
        step[here] = np.asarray(steps)[band[here]]
    return step - np.where(recede, RECEDE_STEPS, 0)


def stone_tints(albedo_lum, key, cls, recede, surface):
    """A step up or down for each stone whose albedo sits clearly above or below that of its material
    in its own part of the building: the side rooms are darker by design, which RECEDE_STEPS already
    shows, so their stones are measured against each other, not against the chapel's."""
    tint = np.zeros(key.shape, dtype=np.int32)
    for c, back in ((c, back) for c in STEPS for back in (False, True)):
        here = surface & (cls == c) & (recede == back)
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


def block_edges(key, seam, surface, band):
    """dstar's wall blocks: (lit, cut). lit = the sides beside a vertical joint in the block's top
    rows, and a glint where the top line meets the side; cut = the bottom corners cut into the joint,
    two pixels at the bottom left and one at the bottom right, so a dark wedge opens where a
    vertical joint meets the bed joint below. Only in light (half and lit bands)."""
    lit_here = surface & (band >= HALF)
    beside_joint = surface & (shifted(seam, 0, -1, False) | shifted(seam, 0, 1, False))
    under_joint = surface & shifted(seam, -1, 0, False)
    rows_above = same_stone_run(key, surface, -1, 0, SIDE_LIGHT_ROWS)
    lit = lit_here & ((beside_joint & (rows_above < SIDE_LIGHT_ROWS)) | (under_joint & beside_joint))
    rows_below = same_stone_run(key, surface, 1, 0, 2)
    to_left = same_stone_run(key, surface, 0, -1, 2)
    to_right = same_stone_run(key, surface, 0, 1, 2)
    bottom_row = surface & (rows_below == 0) & shifted(seam, 1, 0, False)
    above_it = surface & (rows_below == 1) & shifted(shifted(seam, 1, 0, False), 1, 0, False)
    cut = lit_here & ((bottom_row & ((to_left < 2) | (to_right < 1))) | (above_it & (to_left < 1)))
    return lit, cut


def light_gradient(light, band, key, surface):
    """Each stone's place inside its light band, -0.5 (darkest end) to +0.5 (lightest), from the mean
    light on its main band: neighbouring stones shift a little as the light falls off across a face,
    while each stone stays one tone."""
    top = max(np.percentile(light[surface], BRIGHT_PERCENTILE), 1e-6)
    edges = np.r_[0.0, BAND_EDGES, 1.0]
    rel = light / top
    _, inv = np.unique(key[surface], return_inverse=True)
    stone_rel = (np.bincount(inv, weights=rel[surface]) / np.bincount(inv))[inv]
    b = band[surface]
    lo, hi = edges[b], edges[b + 1]
    place = np.zeros(key.shape)
    place[surface] = np.clip((stone_rel - lo) / np.maximum(hi - lo, 1e-6) - 0.5, -0.5, 0.5)
    return place


def pillow(key, surface):
    """+1 on a stone's top rows fading to 0, -1 on its bottom rows: the slight roundness dstar gives
    his blocks."""
    up = same_stone_run(key, surface, -1, 0, PILLOW_ROWS)
    down = same_stone_run(key, surface, 1, 0, PILLOW_ROWS)
    near_top = np.maximum(PILLOW_ROWS - up, 0) / PILLOW_ROWS
    near_bottom = np.maximum(PILLOW_ROWS - down, 0) / PILLOW_ROWS
    return np.where(surface, near_top - near_bottom, 0.0)


def albedo_texture(albedo_lum, key, surface):
    """+1 / -1 where the stone's own colour (scan texture, grime) is clearly lighter or darker than its
    mean, kept only in patches of MIN_PATCH pixels or more, so texture reads as patches, not grain."""
    _, inv = np.unique(key[surface], return_inverse=True)
    mean = np.zeros(key.shape)
    mean[surface] = (np.bincount(inv, weights=albedo_lum[surface]) / np.bincount(inv))[inv]
    rel = np.where(surface, albedo_lum / np.maximum(mean, 1e-3) - 1.0, 0.0)
    texture = np.zeros(key.shape)
    for sign in (1, -1):
        patch = surface & (sign * rel > TEXTURE_CONTRAST)
        labels, n = ndimage.label(patch)
        if n:
            sizes = np.bincount(labels.ravel())
            texture[(labels > 0) & (sizes[labels] >= MIN_PATCH)] = sign
    return texture


def contact_shadow(directory, piece, surface):
    """0 on open surface down to 1 in enclosed corners (under ledges, where buttress meets wall, in
    reveals), from the ambient occlusion pass; zero when the piece has none."""
    path = Path(directory) / f"{piece}_ao.png"
    if not path.exists():
        return np.zeros(surface.shape)
    ao = load(directory, piece, "ao")[..., 0] / 255.0
    return np.where(surface, np.clip((AO_OPEN - ao) / (AO_OPEN - AO_CLOSED), 0.0, 1.0), 0.0)


def drawn_normals(directory, piece, key, surface, solid):
    """The normal map for the game's moving lights, drawn to match the stones: the large form from
    the normals pass, each stone rounded (its top rows tilted up, bottom rows down, side columns
    out), so a candle lights the stone tops the way they are drawn and no texture flickers.
    Encoding as the normals pass: R = x, G = up, B = towards the camera."""
    rgba = load(directory, piece, "normals").astype(np.float64)
    n = rgba[..., :3] / 127.5 - 1.0
    up = same_stone_run(key, surface, -1, 0, PILLOW_ROWS)
    down = same_stone_run(key, surface, 1, 0, PILLOW_ROWS)
    left = same_stone_run(key, surface, 0, -1, PILLOW_ROWS)
    right = same_stone_run(key, surface, 0, 1, PILLOW_ROWS)
    near = lambda run: np.maximum(PILLOW_ROWS - run, 0) / PILLOW_ROWS
    n[..., 1] += np.where(surface, PILLOW_TILT * (near(up) - near(down)), 0.0)
    n[..., 0] += np.where(surface, PILLOW_TILT * (near(right) - near(left)), 0.0)
    n /= np.maximum(np.linalg.norm(n, axis=-1, keepdims=True), 1e-6)
    out = np.zeros(rgba.shape, dtype=np.uint8)
    out[..., :3] = np.clip(np.round((n + 1.0) * 127.5), 0, 255)
    out[..., 3] = np.where(solid, 255, 0)
    return out


def colour_families(albedo, surface, count):
    """(family per pixel, -1 off the surface; the families' colours): the albedo's colours grouped
    into `count` colours (k-means over the pixels, seeded from the most common colour and then the
    farthest ones), each pixel in the family nearest its own colour, so painted detail (eyes, a
    print, a stripe) keeps its colour; lone pixels take their neighbours' family (no speckle)."""
    colour = albedo[surface]
    grid = np.round(colour / 255.0 * (FAMILY_GRID - 1)).astype(np.int64)
    cells, inv, weight = np.unique(grid, axis=0, return_inverse=True, return_counts=True)
    cell_colour = np.stack([np.bincount(inv.ravel(), weights=colour[:, c]) for c in range(3)], 1) / weight[:, None]
    count = max(1, min(count, len(cells)))
    centres = [cell_colour[weight.argmax()]]
    for _ in range(count - 1):
        far = np.min([((cell_colour - c) ** 2).sum(1) for c in centres], axis=0) * weight
        centres.append(cell_colour[far.argmax()])
    centres = np.array(centres)
    for _ in range(FAMILY_ROUNDS):
        member = np.argmin(((cell_colour[:, None] - centres[None]) ** 2).sum(-1), axis=1)
        for f in range(count):
            mine = member == f
            if mine.any():
                centres[f] = (cell_colour[mine] * weight[mine, None]).sum(0) / weight[mine].sum()
    family = np.full(surface.shape, -1, dtype=np.int32)
    family[surface] = np.argmin(((colour[:, None] - centres[None]) ** 2).sum(-1), axis=1)
    return despeckle(family, surface), centres


def despeckle(family, surface):
    """Patches of one family smaller than MIN_FAMILY_PATCH take the most common family around them."""
    small = np.zeros(family.shape, dtype=bool)
    for f in np.unique(family[surface]):
        labels, n = ndimage.label(family == f)
        if n:
            small |= (labels > 0) & (np.bincount(labels.ravel())[labels] < MIN_FAMILY_PATCH)
    families = int(family.max()) + 1
    votes = np.zeros(family.shape + (families,), dtype=np.int32)
    for dy, dx in AROUND:
        nb = shifted(np.where(small, -1, family), dy, dx, -1)
        for f in range(families):
            votes[..., f] += nb == f
    decided = small & (votes.max(-1) > 0)
    family = family.copy()
    family[decided] = votes.argmax(-1)[decided]
    return family


def family_ramp(colour, hue_shift):
    """An artist's ramp for one material colour: the colour itself at BASE_STEP; darker steps at
    SHADE_VALUES of its brightness, leaning towards SHADOW_HUE by hue_shift (the hue moves, the
    value stays); lighter steps mixed towards the warm LIGHT_HUE."""
    base = np.asarray(colour, dtype=np.float64)
    base = base * max(1.0, MIN_FAMILY_LUM / max(lum(base[None])[0], 1e-3))
    ramp = []
    for i, share in enumerate(SHADE_VALUES):
        c = base * share
        t = hue_shift * (BASE_STEP - i) / BASE_STEP
        ramp.append((1 - t) * c + t * SHADOW_HUE * lum(c[None])[0] / lum(SHADOW_HUE[None])[0])
    ramp.append(base)
    ramp += [(1 - t) * base + t * LIGHT_HUE for t in LIGHT_MIX]
    return np.clip(np.round(ramp), 0, 255)


def silhouette(solid):
    """Solid pixels with an empty pixel beside them."""
    return solid & ~np.logical_and.reduce([shifted(solid, dy, dx, False) for dy, dx in ORTHOGONAL])


def paint(directory, piece, out_path, palette=None, gain=1.0, contrast=1.0, saturation=1.0, night=0.0,
          seed=7, antialias=0.6, colours=0, hue_shift=0.25, wall_marks=True, grow_moss=True):
    """Writes the art and its drawn normal map. colours = 0 draws every stone on the palette's stone
    ramp (a building in one material); colours > 0 groups the stones into that many colour families
    from their albedo and draws each on its own ramp (family_ramp), as for a model in many materials.
    wall_marks: dstar's block edges, lit top lines and hand marks; grow_moss: green up-facing albedo as moss."""
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
    moss = clump_moss(surface & green & up, surface) if grow_moss else np.zeros(surface.shape, dtype=bool)

    band = one_tone_per_stone(merge_small_clusters(light_bands(light, surface), key, surface), key, surface)
    family, family_colours = colour_families(albedo, surface, colours) if colours else (None, None)
    # in colour families the family carries each stone's colour; on one ramp the stone's own tint does
    step = band_steps(band, cls, recede) + (0 if colours else stone_tints(lum(albedo), key, cls, recede, surface))
    rim, cast = relief_lines(depth, surface)
    step += np.where(rim & (band >= SHADE), 1, 0)
    step -= np.where(cast, CAST_STEPS, 0)
    cut = np.zeros(surface.shape, dtype=bool)
    if wall_marks:
        step += np.where(top_lines(key, seam, surface, band, seed), 1, 0)
        beside_joint = shifted(seam, -1, 0, False) | shifted(seam, 1, 0, False)
        # a mark is one step darker in a ramp of few steps; two would turn it into a black scratch
        step += np.maximum(hand_marks(key, surface, surface & ~(rim | cast | beside_joint), seed), -MARK_STEPS)
        side_lit, cut = block_edges(key, seam, surface, band)
        step += np.where(side_lit, 1, 0)
    step = np.where(seam, joint_steps(step, seam, surface), step)
    step = np.where(cut, joint_steps(step, seam | cut, surface & ~cut), step)
    step = np.clip(step, 0, len(ramp) - 1)
    # inside the stones, in in-between shades: the light's fall-off stone by stone, the stones'
    # roundness and the texture of their own colour; joints and cuts stay on whole steps
    body = surface & ~cut
    shading = (GRADIENT * light_gradient(light, band, key, surface) + PILLOW * pillow(key, surface)
               + TEXTURE * albedo_texture(lum(albedo), key, surface)
               - AO_STRENGTH * contact_shadow(directory, piece, surface))
    position = np.clip(step + np.where(body, shading, 0.0), 0, len(ramp) - 1)
    # stone in the deepest shadow stays stone: only joints, cuts and lines may reach the line colour,
    # or shadowed stones read as holes
    position = np.where(body & ~cast, np.maximum(position, BODY_FLOOR), position)

    out = np.zeros(ids.shape, dtype=np.uint8)
    painted = surface | seam
    out[..., :3][painted] = np.clip(ramp_colour(ramp, position[painted], SUB_STEPS), 0, 255)
    ramps = [family_ramp(c, hue_shift) for c in family_colours] if colours else []
    for f, own in enumerate(ramps):
        here = family == f
        out[..., :3][here] = np.clip(ramp_colour(own, position[here], SUB_STEPS), 0, 255)
    leaf = (solid & (cls == FOLIAGE)) | moss
    out[..., :3][leaf] = greens[np.clip(band[leaf] + 1, 0, len(greens) - 1)]
    glass = solid & (cls == GLASS)
    out[..., :3][glass] = passage[np.clip(band[glass], 0, len(passage) - 1)]
    edge = silhouette(solid)
    out[..., :3][edge & ~leaf] = ramp[OUTLINE_STEP]
    for f, own in enumerate(ramps):
        # the outline in each family's own darkest shade (selective outline), not one black for all
        out[..., :3][edge & ~leaf & (family == f)] = own[OUTLINE_STEP]
    out[..., 3][solid] = 255
    soften_notches(out, seam | edge, antialias)
    grade(out, gain=gain, contrast=contrast, saturation=saturation, night=night)
    Image.fromarray(out).save(out_path)
    Image.fromarray(drawn_normals(directory, piece, key, surface, solid)).save(normals_path(out_path))


def normals_path(art_path):
    """Where the drawn normal map of an art file goes: <name>_normals.png beside it."""
    art_path = Path(art_path)
    return art_path.with_name(art_path.stem + "_normals.png")


if __name__ == "__main__":
    paint(sys.argv[1], sys.argv[2], sys.argv[3])
