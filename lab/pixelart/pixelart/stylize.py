"""Turns a Blender render into pixel art in dstar's language, using the render only as information.

Inputs are three passes of the same frame: beauty (lit colour), ids (one flat colour per stone,
joints black) and normals (green-up tangent space). Every stone gets a flat tone from dstar's
shrine ramp, at most one step lighter or darker for broad patches, a bright line on up and left
facing bevels and a dark one on down and right facing ones. Joints, cracks and silhouette edges
become his outline colour. Foliage takes his grass greens, the doorway his crypt purples.

usage: python -m pixelart.stylize <beauty> <ids> <normals> <out> [shift]
  shift moves every stone tone by whole ramp steps (negative = darker, for pieces further back)
"""
import sys
import numpy as np
from PIL import Image

from .grade import grade
from .tidy import tidy

# dstar's owl shrine, stone ramp from outline to highlight
RAMP = np.array([(5, 10, 3), (28, 33, 26), (44, 49, 42), (52, 57, 50), (73, 78, 71), (96, 101, 94), (131, 136, 129), (182, 187, 180)])
OUTLINE = 0
# shares of stone bodies per base tone (levels 2..5), taken from the shrine's tone counts
BASE_LEVELS = [2, 3, 4, 5]
# a stone whose mean brightness is below this share of the median stone counts as deep in shadow
DEEP_SHADOW = 0.45
BASE_SHARES = [0.35, 0.25, 0.30, 0.10]   # tuned (style/tune.py) against the shrine's tone counts
GREENS = np.array([(7, 24, 14), (12, 37, 23), (18, 50, 31), (23, 63, 33), (35, 82, 45), (55, 110, 62)])
PASSAGE = np.array([(12, 0, 24), (13, 1, 25), (17, 3, 34), (20, 6, 43), (25, 15, 51), (31, 23, 65)])


def lum(rgb):
    return rgb[..., 0] * 0.299 + rgb[..., 1] * 0.587 + rgb[..., 2] * 0.114


def mortar(level, seam, stone, ramp):
    """Joint colours the way dstar paints them: a dark tone of the stone, two steps under its
    neighbours, darkest only where joints cross; the stone corners facing away from the light
    are rounded into the joint."""
    lv = np.where(stone, level, 99)
    p = np.pad(lv, 1, constant_values=99)
    nb = np.stack([p[:-2, 1:-1], p[2:, 1:-1], p[1:-1, :-2], p[1:-1, 2:]], -1)
    near = nb.min(-1)
    m = np.where(near < 99, np.clip(near - 2, 1, 3), 1)
    sp = np.pad(seam, 1)
    seam_nb = sp[:-2, 1:-1].astype(int) + sp[2:, 1:-1] + sp[1:-1, :-2] + sp[1:-1, 2:]
    m = np.where(seam_nb >= 3, m - 1, m)
    # rounded corners: a stone pixel with joint below and right (or below and left) joins the joint
    below, right, left = sp[2:, 1:-1], sp[1:-1, 2:], sp[1:-1, :-2]
    corner = stone & below & (right | left)
    level[corner] = np.clip(level[corner] - 2, 1, 3)
    return ramp[np.clip(m, 0, len(ramp) - 1)[seam]]


def _neighbours8(mask):
    p = np.pad(mask, 1).astype(np.int32)
    h, w = mask.shape
    return sum(p[1 + dy:1 + dy + h, 1 + dx:1 + dx + w] for dy in (-1, 0, 1) for dx in (-1, 0, 1) if dy or dx)


def clump_moss(mossy, stone, min_neighbours=3):
    """dstar paints moss as clumps, never as speckle: drop moss pixels with few moss neighbours,
    fill pin holes inside clumps."""
    keep = mossy & (_neighbours8(mossy) >= min_neighbours)
    keep = keep & (_neighbours8(keep) >= 2)
    holes = stone & ~keep & (_neighbours8(keep) >= 6)
    return keep | holes


def shade_clumps(out, moss, greens):
    """Clumps get his ivy shading: a dark green underside where the clump hangs over stone,
    a lit top row where light falls on it."""
    p = np.pad(moss, 1)
    below_open = moss & ~p[2:, 1:-1]
    above_open = moss & ~p[:-2, 1:-1] & ~below_open
    out[..., :3][below_open] = greens[0]
    out[..., :3][above_open] = greens[min(len(greens) - 1, 4)]


def shade_feet(level, solid, stone, rows):
    """Contact shade: the lowest stone rows of every column, where a piece meets the ground, one step darker."""
    rows_from_bottom = np.cumsum(solid[::-1], axis=0)[::-1]
    resting = rows_from_bottom <= rows
    level[resting & stone] = np.maximum(level[resting & stone] - 1, 1)


def absorb_growth_specks(out, leaf, stone):
    """Single grass and moss pixels scattered over a stone read as noise, not growth: they take the
    colour of the stone around them. Leaves with company (clumps, bushes) stay."""
    lonely = leaf & (_neighbours8(leaf) <= 1) & (_neighbours8(stone) >= 5)
    if not lonely.any():
        return
    col = out[..., :3].astype(np.int64)
    packed = (col[..., 0] << 16) | (col[..., 1] << 8) | col[..., 2]
    pp = np.pad(np.where(stone, packed, -1), 1, constant_values=-1)
    for y, x in zip(*np.nonzero(lonely)):
        around = pp[y:y + 3, x:x + 3].ravel()
        around = around[around >= 0]
        vals, counts = np.unique(around, return_counts=True)
        v = vals[counts.argmax()]
        out[y, x, :3] = ((v >> 16) & 255, (v >> 8) & 255, v & 255)
    leaf[lonely] = False
    stone[lonely] = True


def _run_length(key, stone, dy, dx, limit):
    """For every pixel, how many pixels of the same stone follow in direction (dy, dx), up to limit."""
    run = np.zeros(key.shape, dtype=np.int32)
    alive = stone.copy()
    shifted = key
    for _ in range(limit):
        shifted = np.roll(np.roll(shifted, -dy, 0), -dx, 1)
        alive = alive & (shifted == key)
        run += alive
    return run


def pillow(key, stone, width):
    """dstar's pillowed stones as a ramp offset: a lit band along the top and left edge `width`
    pixels deep, fading into the body, and the body falling off into shade towards the bottom and
    right edge."""
    up = _run_length(key, stone, -1, 0, width)
    left = _run_length(key, stone, 0, -1, width)
    down = _run_length(key, stone, 1, 0, width)
    right = _run_length(key, stone, 0, 1, width)
    near_top = np.maximum(width - up, 0) / width
    near_left = np.maximum(width - left, 0) / width
    near_bottom = np.maximum(width - down, 0) / width
    near_right = np.maximum(width - right, 0) / width
    return 0.9 * np.maximum(near_top, 0.6 * near_left) - 0.8 * np.maximum(near_bottom, 0.6 * near_right)


def ramp_colour(ramp, position, sub_steps):
    """Colours between the ramp's shades: position 2.5 is halfway from shade 2 to shade 3,
    snapped to sub_steps in-between shades so the piece keeps a small, deliberate palette."""
    position = np.clip(np.round(np.asarray(position, dtype=float) * sub_steps) / sub_steps, 0, len(ramp) - 1)
    lo = np.floor(position).astype(int)
    hi = np.minimum(lo + 1, len(ramp) - 1)
    t = (position - lo)[..., None]
    return np.round(ramp[lo] * (1 - t) + ramp[hi] * t).astype(np.int32)


def pixelize(beauty_p, ids_p, normals_p, out_p, shift=0, base_shares=None, palette=None, k_scale=0.5, crack_k=2.2, grain=0.0, max_step=1, micro=4, micro_k=1.0, moss_clumps=False, foot_rows=0, outline_level=OUTLINE, soft=0.0, sub_steps=3, bevel=0, speck=0.0, form=0.0, value_tones=False, gain=1.0, contrast=1.0, saturation=1.0, night=0.0):
    shares = base_shares or BASE_SHARES
    pal = palette or {}
    ramp = np.array(pal.get("stone", RAMP))
    greens = np.array(pal.get("foliage", GREENS))
    passage_cols = np.array(pal.get("passage", PASSAGE))
    beauty = np.array(Image.open(beauty_p).convert("RGBA")).astype(np.float32)
    ids = np.array(Image.open(ids_p).convert("RGBA"))
    nrm = np.array(Image.open(normals_p).convert("RGBA")).astype(np.float32)
    h, w = beauty.shape[:2]
    solid = beauty[..., 3] >= 128
    rgb = beauty[..., :3]
    L = lum(rgb)
    # broad patches only: average brightness over a small window inside the same stone
    Ls = L.copy()
    key = (ids[..., 0].astype(np.int64) << 16) | (ids[..., 1].astype(np.int64) << 8) | ids[..., 2]

    seam = solid & ((key == 0) | (rgb.max(-1) <= 12))
    # classify whole stones (id regions) by their mean colour, never single pixels
    body = solid & ~seam
    u, inv0 = np.unique(key[body], return_inverse=True)
    cnt0 = np.bincount(inv0)
    mr = np.bincount(inv0, weights=rgb[..., 0][body]) / cnt0
    mg = np.bincount(inv0, weights=rgb[..., 1][body]) / cnt0
    mb = np.bincount(inv0, weights=rgb[..., 2][body]) / cnt0
    # foliage is clearly green against blue too; moss-tinted stone (green over red only) stays stone
    is_leaf = (mg > mr * 1.18) & (mg > mb * 1.10)
    is_passage = ~is_leaf & (mb > mr * 1.45) & (mb > mg * 1.3) & (cnt0 > 400)
    cls = np.zeros((h, w), dtype=np.int8)
    cls[body] = np.where(is_leaf[inv0], 1, np.where(is_passage[inv0], 2, 3))
    leaf, passage, stone = cls == 1, cls == 2, cls == 3

    level = np.full((h, w), -1, dtype=np.int32)

    # stone bodies: one base tone per stone, by rank of its mean brightness
    keys = key[stone]
    uniq, inv = np.unique(keys, return_inverse=True)
    sums = np.bincount(inv, weights=L[stone])
    counts = np.bincount(inv)
    means = sums / counts
    sq = np.bincount(inv, weights=L[stone] ** 2)
    std = np.sqrt(np.maximum(sq / counts - means ** 2, 0))
    # rank stones by brightness, weighted by their area, so the tone shares are shares of pixels
    srt = np.argsort(means)
    cum = np.cumsum(counts[srt]) - counts[srt] / 2
    order = np.empty(len(means)); order[srt] = cum / counts.sum()
    edges = np.cumsum(shares)
    base = np.array([BASE_LEVELS[min(np.searchsorted(edges, q, side="right"), 3)] for q in order]) + shift
    if value_tones:
        # tones follow the render's own brightness (on a log scale, as the eye sees it) instead of
        # fixed shares, so a bright stone stays bright and a dark one dark
        logs = np.log(np.maximum(means, 1.0))
        lo, hi = np.percentile(logs, 5), np.percentile(logs, 95)
        position = 1.5 + 4.0 * np.clip((logs - lo) / max(hi - lo, 1e-6), 0, 1)
        base = np.round(position).astype(int) + shift

    # 5x5 mean of brightness restricted to the pixel's own stone
    acc = np.zeros((h, w)); num = np.zeros((h, w))
    for dy in range(-1, 2):
        for dx in range(-1, 2):
            sk = np.roll(np.roll(key, dy, 0), dx, 1)
            sl = np.roll(np.roll(L, dy, 0), dx, 1)
            same = (sk == key)
            acc += np.where(same, sl, 0); num += same
    Ls = acc / np.maximum(num, 1)
    pix_mean = means[inv]
    pix_std = std[inv]
    delta = (1 - grain) * Ls[stone] + grain * L[stone] - pix_mean
    raw_delta = L[stone] - pix_mean
    k = np.maximum(pix_std * k_scale, 3.5)
    lv = base[inv] + np.clip(np.round(delta / k), -max_step, max_step).astype(np.int32)
    # the part of a stone a cast shadow falls on, and stones lying deep in shadow altogether, get no
    # edge light: dstar paints deep shadow as one flat dark area, not as a grid of lit edges
    deep_shadow = means < DEEP_SHADOW * np.median(means)
    in_shadow = (lv < base[inv]) | deep_shadow[inv]
    # cracks: clearly darker than the stone around them
    crack = raw_delta < -np.maximum(pix_std * crack_k, 12.0)

    # bevels from the normal pass (light from the upper left)
    nx = nrm[..., 0][stone] / 127.5 - 1
    ny = nrm[..., 1][stone] / 127.5 - 1
    # one step only: tipped tops and bevels must not stack into white specks
    up_face = ny > 0.45
    # with pillowed stones the bevel band does this job; lighting the rounded rim as well doubles the edge
    rim_light = 0 if bevel else 1
    lv = lv + rim_light * np.where(up_face | ((nx < -0.45) & (ny > -0.2)), 1, 0)
    lv = lv - rim_light * (np.where(ny < -0.45, 1, 0) + np.where(nx > 0.45, 1, 0))
    # dstar's convention on every stone: lit top row and left column, shaded bottom row and right column
    # edges are joints (id 0), silhouettes or other real stones; carvings (small regions) are not edges
    area = np.zeros((h, w), dtype=np.int64)
    ub, ib = np.unique(key[solid], return_inverse=True)
    area[solid] = np.bincount(ib)[ib]
    carving = solid & (key != 0) & (area < 60)
    # engraved strokes are joint-dark in the render; raised relief keeps its own stone tone and edges
    lsum = np.zeros((h, w))
    lsum[solid] = (np.bincount(ib, weights=L[solid]) / np.bincount(ib))[ib]
    engraved = carving & (lsum < 20)
    # thin strips (incised borders, mouldings one or two pixels wide) are line work, not stones:
    # lighting stair-stepped edges along them pixel by pixel would only make dotted lines,
    # so they neither get edge light themselves nor count as an edge of the stone they lie on
    kp4 = np.pad(key, 1, constant_values=-1)
    core = (kp4[:-2, 1:-1] == key) & (kp4[2:, 1:-1] == key) & (kp4[1:-1, :-2] == key) & (kp4[1:-1, 2:] == key)
    core_share = np.bincount(inv, weights=core[stone].astype(float)) / counts
    thin = np.zeros((h, w), dtype=bool)
    thin[stone] = core_share[inv] < 0.25
    kk = np.where(engraved | thin, -2, key)
    kp = np.pad(kk, 1, constant_values=-1)

    def edge(nb):
        return ((nb != key) & (nb != -2))[stone]
    top_edge = edge(kp[:-2, 1:-1])
    left_edge = edge(kp[1:-1, :-2])
    bottom_edge = edge(kp[2:, 1:-1])
    right_edge = edge(kp[1:-1, 2:])
    # darkening (shift) applies to bodies only: lit top edges keep their full brightness, as in dstar's background pieces
    # small carved parts (frame strips, emblems, letters) get one step, only real stones the full top light
    big = (counts[inv] >= 60) & (core_share[inv] >= 0.25)
    # the lit top row sits two steps over the stone's unshaded tone, never stacked on the bevel light;
    # small carved parts get no edge light of their own, their tone already separates them
    top_lv = base[inv] - shift + 2
    lv = np.where(top_edge & big & ~in_shadow, np.maximum(lv - np.where(up_face, 1, 0), top_lv), lv)
    lv = lv + np.where(left_edge & ~top_edge & big & ~in_shadow, 1, 0)
    lv = lv - np.where(bottom_edge, 1, 0) - np.where(right_edge & ~bottom_edge & ~top_edge, 1, 0)
    # the brightest tone is kept for long, unbroken ledges (see below); bodies stop one step under it
    lv = np.clip(lv, 1, len(ramp) - 2)
    lv[crack] = OUTLINE
    lv[engraved[stone]] = OUTLINE
    level[stone] = lv
    if foot_rows:
        shade_feet(level, solid, stone, foot_rows)
    seam_rgb = mortar(level, seam, stone, ramp)
    # moss and lichen growing on a stone stay green, they are not part of its tone
    mossy = stone & (rgb[..., 1] > rgb[..., 0] * 1.22) & (rgb[..., 1] > rgb[..., 2] * 1.02) & (rgb.max(-1) > 14)
    if moss_clumps:
        mossy = clump_moss(mossy, stone)

    out = np.zeros((h, w, 4), dtype=np.uint8)
    # dstar paints each tone with near-duplicate shades: add a small step from the fine surface detail
    shade = ramp[level[stone]].astype(np.int32)
    fine = np.clip(np.round(raw_delta / np.maximum(pix_std * micro_k, 1.5)), -1, 1).astype(np.int32)
    keep_pure = (level[stone] == OUTLINE)
    if soft:
        # the inside of a stone keeps the render's own light and shade (falloff, shade towards the
        # joints) in in-between shades of the ramp; edges, joints and carvings stay crisp
        any_edge = top_edge | bottom_edge | left_edge | right_edge
        frac = np.clip(delta / (2.0 * k), -soft, soft)
        if bevel:
            frac = frac + pillow(key, stone, bevel)[stone] * np.where(big & ~in_shadow, 1.0, 0.0)
        if form:
            # round forms (column shafts, mouldings) shade across their curve: lit where the surface
            # turns to the light at the upper left, darker where it turns away
            frac = frac + form * np.clip(-nx + 0.5 * ny, -1.0, 1.0)
        if speck:
            frac = frac + speck * np.clip(np.round(raw_delta / np.maximum(pix_std, 1.5)), -1, 1)
        frac = np.where(any_edge | keep_pure, 0.0, frac)
        shade = ramp_colour(ramp, level[stone] + frac, sub_steps)
    shade = shade + np.where(keep_pure, 0, fine * micro)[:, None]
    out[..., :3][stone] = np.clip(shade, 0, 255)
    out[..., :3][seam] = seam_rgb

    # foliage and the passage: quantise brightness onto dstar's own colours
    leaf = leaf | mossy
    stone = stone & ~mossy
    if leaf.any():
        t = (L[leaf] - L[leaf].min()) / max(np.ptp(L[leaf]), 1)
        if soft:
            out[..., :3][leaf] = np.clip(ramp_colour(greens, t * (len(greens) - 1), sub_steps), 0, 255)
        else:
            q = np.clip((t * len(greens)).astype(int), 0, len(greens) - 1)
            out[..., :3][leaf] = greens[q]
        if moss_clumps:
            shade_clumps(out, mossy, greens)
            absorb_growth_specks(out, leaf, stone)
    if passage.any():
        q = np.clip(((L[passage] - L[passage].min()) / max(np.ptp(L[passage]), 1) * len(passage_cols)).astype(int), 0, len(passage_cols) - 1)
        out[..., :3][passage] = passage_cols[q]
    out[..., 3][solid] = 255

    # silhouette: stone pixels touching empty space take the outline
    pad = np.pad(solid, 1)
    edge = solid & ~(pad[:-2, 1:-1] & pad[2:, 1:-1] & pad[1:-1, :-2] & pad[1:-1, 2:])
    out[..., :3][edge & stone] = ramp[outline_level]
    out[..., :3][edge & leaf] = greens[0]

    # clean up: a lone pixel whose four neighbours mostly agree takes their colour
    for _ in range(2):
        col = out[..., :3].astype(np.int64)
        c = (col[..., 0] << 16) | (col[..., 1] << 8) | col[..., 2]
        cp = np.pad(c, 1, constant_values=-1)
        nb = np.stack([cp[:-2, 1:-1], cp[2:, 1:-1], cp[1:-1, :-2], cp[1:-1, 2:]], -1)
        same = (nb == c[..., None]).sum(-1)
        lone = solid & (stone | leaf) & (same == 0) & ~edge
        ys, xs = np.nonzero(lone)
        for y, x in zip(ys, xs):
            vals, cnt = np.unique(nb[y, x][nb[y, x] >= 0], return_counts=True)
            if len(cnt) and cnt.max() >= 3:
                v = vals[cnt.argmax()]
                out[y, x, :3] = ((v >> 16) & 255, (v >> 8) & 255, v & 255)
    report = tidy(out)
    grade(out, gain=gain, contrast=contrast, saturation=saturation, night=night)
    Image.fromarray(out).save(out_p)
    return report


if __name__ == "__main__":
    a = sys.argv
    pixelize(a[1], a[2], a[3], a[4], int(a[5]) if len(a) > 5 else 0)
