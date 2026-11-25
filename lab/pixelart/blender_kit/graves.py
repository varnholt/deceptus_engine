"""Graveyard props for the pixel-art kit: monuments with depth, mouldings, carving, wear and weeds,
iron fences, sprig bushes, and a layout builder that stands them on a ground line.

Runs inside Blender after core.py. Every monument builds into a Builder in level-pixel units around
a centre x; place() grows, turns, tips and leans the finished piece about its foot.
A layout is a list of (name, material key, function, x, args, kwargs); function None means a fence.
"""
import bpy, bmesh, math, random, zlib
from mathutils import Matrix, Vector

GROUND = 0.0          # set by build_layout from the band's ground line
SEAM = PX             # joint width; place() narrows it so grown monuments keep 1 px joints


def fence(b, x0_px, x1_px, h_px=22, y=-0.55):
    """Wrought iron: posts at both ends, bars with spear tips, two rails. Bars and rails are two
    pixels thick: one-pixel iron converts into dashed lines."""
    x = x0_px
    while x <= x1_px:
        cx = X(x)
        b.box(cx - 1.0 * PX, cx + 1.0 * PX, GROUND, GROUND + h_px * PX, y - 0.03, y + 0.03)
        tip = GROUND + h_px * PX
        b.prism([(cx - 2.0 * PX, tip), (cx + 2.0 * PX, tip), (cx, tip + 4.0 * PX)][::-1], y - 0.03, y + 0.03)
        x += 5
    for zr in (4, h_px - 5):
        b.box(X(x0_px) - PX, X(x1_px) + PX, GROUND + (zr - 1.0) * PX, GROUND + (zr + 1.0) * PX, y - 0.04, y + 0.04)
    for px in (x0_px - 3, x1_px + 3):
        b.box(X(px) - 2.0 * PX, X(px) + 2.0 * PX, GROUND, GROUND + (h_px + 4) * PX, y - 0.06, y + 0.06)
        b.box(X(px) - 3.0 * PX, X(px) + 3.0 * PX, GROUND + (h_px + 4) * PX, GROUND + (h_px + 6) * PX, y - 0.06, y + 0.06)


def bush(b, rng, cx_px, w_px, h_px, depth=(0.35, 1.3)):
    """A ragged bush like dstar's: sprigs fanning up from the root, small pointed leaves along them."""
    sprigs = int(w_px * 1.5)
    for _ in range(sprigs):
        off = rng.gauss(0, w_px * 0.22)
        x = X(cx_px + off * 0.5)
        z = GROUND - 2 * PX
        # sprigs lean outwards the further they start from the middle
        ang = math.radians(90 - off / (w_px * 0.5) * 55 + rng.gauss(0, 12))
        reach = h_px * rng.uniform(0.45, 1.0) * (1.0 - min(abs(off) / (w_px * 0.6), 0.7) * 0.6)
        y = rng.uniform(*depth)
        travelled = 0.0
        while travelled < reach:
            step = rng.uniform(1.8, 3.0)
            ang += math.radians(rng.gauss(0, 9))
            x += math.cos(ang) * step * PX
            z += math.sin(ang) * step * PX
            travelled += step
            core = travelled < reach * 0.55
            for _ in range(2 if core else rng.choice((1, 1, 2))):
                la = ang + math.radians(rng.choice((-1, 1)) * rng.uniform(25, 70))
                ln = rng.uniform(2.2, 3.6) * PX
                lw = rng.uniform(0.8, 1.3) * PX
                bx, bz = x + rng.uniform(-1, 1) * PX, z + rng.uniform(-1, 1) * PX
                tip = (bx + math.cos(la) * ln, bz + math.sin(la) * ln)
                side = (math.cos(la + math.pi / 2) * lw, math.sin(la + math.pi / 2) * lw)
                mid = (bx + math.cos(la) * ln * 0.45, bz + math.sin(la) * ln * 0.45)
                tilt = rng.uniform(-0.35, 0.35)
                verts = [b.bm.verts.new((bx, y, bz)),
                         b.bm.verts.new((mid[0] + side[0], y - tilt * PX * 2, mid[1] + side[1])),
                         b.bm.verts.new((tip[0], y, tip[1])),
                         b.bm.verts.new((mid[0] - side[0], y + tilt * PX * 2, mid[1] - side[1]))]
                f = b.bm.faces.new(verts)
                b._tag([f])
            y += rng.uniform(-0.04, 0.04)


def bushes(scn, seed, x0_px, x1_px, skip=()):
    """Separate bushes along the band with gaps between them; skip lists (x0, x1) spans to leave free."""
    rng = random.Random(seed)
    b = Builder("bushes", material("bush", (0.018, 0.075, 0.032), rough=0.8, tint_var=0.30), seed)
    x = x0_px + rng.uniform(0, 30)
    while x < x1_px:
        w = rng.uniform(30, 80)
        h = rng.uniform(24, 44)
        if not any(a - w / 2 < x < c + w / 2 for a, c in skip):
            bush(b, rng, x, w, h)
        x += w * rng.uniform(0.7, 1.4) + rng.uniform(10, 70)
    return b.finish(bevel=0)




# ------------------------------------------------------------------ outline helpers

def densify(pts, step=0.5 * PX):
    out = []
    for i in range(len(pts)):
        (x0, z0), (x1, z1) = pts[i], pts[(i + 1) % len(pts)]
        n = max(1, int(math.hypot(x1 - x0, z1 - z0) / step))
        out += [(x0 + (x1 - x0) * k / n, z0 + (z1 - z0) * k / n) for k in range(n)]
    return out


def weather(pts, rng, jitter=0.35 * PX, bites=2, bite_depth=(1.5, 3.5), floor_z=None):
    """Roughens a closed outline: a pixel of jitter everywhere, and a few chipped bites.

    Points on the ground line (z <= floor_z) stay put, so the stone still stands on its base.
    """
    pts = densify(pts)
    n = len(pts)
    cx = sum(p[0] for p in pts) / n
    cz = sum(p[1] for p in pts) / n
    out = []
    for x, z in pts:
        if floor_z is not None and z <= floor_z + 1e-4:
            out.append((x, z))
            continue
        dx, dz = x - cx, z - cz
        d = math.hypot(dx, dz) or 1.0
        j = rng.uniform(-jitter, jitter)
        out.append((x + dx / d * j, z + dz / d * j))
    for _ in range(bites):
        i = rng.randrange(n)
        width = rng.randint(4, 10)
        depth = rng.uniform(*bite_depth) * PX
        for k in range(-width, width + 1):
            idx = (i + k) % n
            x, z = out[idx]
            if floor_z is not None and z <= floor_z + 2 * PX:
                continue
            f = (1 - abs(k) / (width + 1)) ** 0.7
            dx, dz = x - cx, z - cz
            d = math.hypot(dx, dz) or 1.0
            out[idx] = (x - dx / d * depth * f, z - dz / d * depth * f)
    return out


def arch_top(x0, x1, z_spring, segs=18):
    cx, r = (x0 + x1) / 2, (x1 - x0) / 2
    return [(cx + r * math.cos(math.pi * k / segs), z_spring + r * math.sin(math.pi * k / segs)) for k in range(segs + 1)]


def slab(b, outline, y0, y1, halo=True):
    """A stone with its dark joint slab one pixel larger behind it."""
    if halo:
        n = len(outline)
        cx = sum(p[0] for p in outline) / n
        cz = sum(p[1] for p in outline) / n
        grown = []
        for x, z in outline:
            dx, dz = x - cx, z - cz
            d = math.hypot(dx, dz) or 1.0
            grown.append((x + dx / d * SEAM, z + dz / d * SEAM))
        b.prism(grown[::-1], y1 - 0.06, y1 + 0.04, material_index=1)
    b.prism(outline[::-1], y0, y1)


def groove(b, x0, x1, z, y, w=0.9):
    b.box(x0, x1, z - w * PX / 2, z + w * PX / 2, y - 0.012, y + 0.02, material_index=1)


INSCRIPTIONS = []   # lettering areas of the monuments built so far, in world space once placed


def lettering(b, rng, cx, half, z_top, lines, y, line_gap=3.2):
    """Records where the epitaph goes instead of modelling it: the lettering is carved into the
    finished pixel art by pixelart.inscribe. Stores the top line's centre on the face and the
    face's right and up directions, which place() then moves along with the monument."""
    INSCRIPTIONS.append(dict(centre=Vector((cx, y, z_top)), right=Vector((cx + PX, y, z_top)),
                             up=Vector((cx, y, z_top + PX)), half_px=half / PX, lines=lines,
                             line_pitch_px=line_gap, seed=rng.randrange(1 << 16)))


def crack(b, rng, x, z, length, y, down=True):
    """A jagged fissure: short dark segments wandering down (or sideways) across a face."""
    ang = math.radians(-90 + rng.uniform(-35, 35)) if down else math.radians(rng.choice((0, 180)) + rng.uniform(-30, 30))
    travelled = 0.0
    while travelled < length:
        step = rng.uniform(1.0, 2.2) * PX
        nx, nz = x + math.cos(ang) * step, z + math.sin(ang) * step
        b.box(min(x, nx) - 0.45 * PX, max(x, nx) + 0.45 * PX, min(z, nz) - 0.45 * PX, max(z, nz) + 0.45 * PX, y - 0.01, y + 0.02, material_index=1)
        x, z = nx, nz
        travelled += step
        ang += math.radians(rng.gauss(0, 30))
        if rng.random() < 0.12:
            crack(b, rng, x, z, (length - travelled) * 0.4, y, down)


def moss_streak(b, rng, x0, x1, z_top, y, depth_px=8):
    """Green growth spilling down from a ledge: little clumps thinning out downwards."""
    x = x0
    while x < x1:
        if rng.random() < 0.6:
            hang = rng.uniform(1, depth_px) * PX
            z = z_top
            while z > z_top - hang:
                w = rng.uniform(0.7, 1.6) * PX
                b.box(x - w, x + w, z - rng.uniform(0.8, 1.6) * PX, z, y - 0.05, y - 0.01, material_index=2)
                z -= rng.uniform(0.8, 1.8) * PX
        x += rng.uniform(0.8, 2.0) * PX


def relief_cross(b, cx, cz, h, y, w=1.3, arm=None, fleury=False):
    """A raised cross on a face; fleury crosses end in small trefoils."""
    arm = arm or h * 0.6
    b.box(cx - w * PX, cx + w * PX, cz - h / 2, cz + h / 2, y - 0.035, y + 0.01)
    az = cz + h * 0.18
    b.box(cx - arm / 2, cx + arm / 2, az - w * PX, az + w * PX, y - 0.035, y + 0.01)
    if fleury:
        for ex, ez in ((cx, cz + h / 2), (cx - arm / 2, az), (cx + arm / 2, az)):
            b.box(ex - 1.8 * PX, ex + 1.8 * PX, ez - 1.0 * PX, ez + 1.0 * PX, y - 0.04, y + 0.01)


def wreath(b, cx, cz, r, y, leaves=14):
    for k in range(leaves):
        a = 2 * math.pi * k / leaves
        x, z = cx + r * math.cos(a), cz + r * math.sin(a)
        b.box(x - 0.9 * PX, x + 0.9 * PX, z - 0.9 * PX, z + 0.9 * PX, y - 0.035, y + 0.01)


def eye_relief(b, cx, cz, r, y):
    pts = [(cx + r * math.cos(a), cz + r * 0.55 * math.sin(a)) for a in [2 * math.pi * k / 20 for k in range(20)]]
    b.prism(pts[::-1], y - 0.03, y + 0.01)
    b.prism([(cx + r * 0.35 * math.cos(a), cz + r * 0.35 * math.sin(a)) for a in [2 * math.pi * k / 12 for k in range(12)]][::-1], y - 0.045, y - 0.02, material_index=1)


def plinth(b, cx, w, steps, y0, y1, rng):
    """Stepped base: each step narrower, each a separate stone with a dark joint."""
    z = GROUND - 2 * PX
    for i, (dw, h) in enumerate(steps):
        half = (w / 2 + dw) * PX
        out = weather([(cx - half, z), (cx + half, z), (cx + half, z + h * PX), (cx - half, z + h * PX)], rng, jitter=0.25 * PX, bites=1, bite_depth=(1, 2), floor_z=z)
        slab(b, out, y0 - 0.05 * (len(steps) - i), y1)
        z += h * PX
    return z


def weeds(b, rng, x0, x1, count, max_h=10):
    """Grass and weed blades in front of a base; uses material slot 2 (the weed green)."""
    for _ in range(count):
        x = X(rng.uniform(x0, x1))
        h = rng.uniform(3, max_h) * PX
        lean = rng.uniform(-0.5, 0.5)
        w = rng.uniform(0.6, 1.1) * PX
        y = rng.uniform(-0.75, -0.55)
        v = [b.bm.verts.new((x - w, y, GROUND - PX)), b.bm.verts.new((x + w, y, GROUND - PX)), b.bm.verts.new((x + lean * h * 0.6, y, GROUND + h))]
        f = b.bm.faces.new(v)
        b._tag([f])
        f.material_index = 2


# ------------------------------------------------------------------ monuments

def headstone2(b, cx_px, w_px, h_px, style="arch", emblem="cross", lines=3, seed=0, broken=False, thick=0.28):
    """A proper headstone: stepped plinth, slab with a raised border, an emblem and lettering."""
    rng = random.Random(seed)
    cx = X(cx_px)
    half = w_px / 2 * PX
    y0, y1 = -thick / 2, thick / 2
    zb = plinth(b, cx, w_px, [(4, 3), (2, 3)], y0 - 0.06, y1 + 0.05, rng)
    zt = zb + h_px * PX
    x0, x1 = cx - half, cx + half
    if style == "arch":
        body = [(x0, zb), (x1, zb)] + arch_top(x0, x1, zt - half)
        inner = [(x0 + 2 * PX, zb + 3 * PX), (x1 - 2 * PX, zb + 3 * PX)] + arch_top(x0 + 2 * PX, x1 - 2 * PX, zt - half)
    elif style == "gothic":
        body = [(x0, zb), (x1, zb), (x1, zt - w_px * 0.55 * PX), (cx + half * 0.35, zt - 1.5 * PX), (cx, zt), (cx - half * 0.35, zt - 1.5 * PX), (x0, zt - w_px * 0.55 * PX)]
        inner = [(x0 + 2 * PX, zb + 3 * PX), (x1 - 2 * PX, zb + 3 * PX), (x1 - 2 * PX, zt - w_px * 0.6 * PX), (cx, zt - 3 * PX), (x0 + 2 * PX, zt - w_px * 0.6 * PX)]
    elif style == "shoulder":
        s = 3.5 * PX
        body = [(x0, zb), (x1, zb), (x1, zt - 6 * PX), (x1 - s, zt - 6 * PX)] + arch_top(x0 + s, x1 - s, zt - half + s, 12)[1:-1] + [(x0 + s, zt - 6 * PX), (x0, zt - 6 * PX)]
        inner = [(x0 + 2 * PX, zb + 3 * PX), (x1 - 2 * PX, zb + 3 * PX), (x1 - 2 * PX, zt - 8 * PX), (x0 + 2 * PX, zt - 8 * PX)]
    else:  # capped tablet
        body = [(x0, zb), (x1, zb), (x1, zt - 4 * PX), (x0, zt - 4 * PX)]
        inner = [(x0 + 2 * PX, zb + 3 * PX), (x1 - 2 * PX, zb + 3 * PX), (x1 - 2 * PX, zt - 7 * PX), (x0 + 2 * PX, zt - 7 * PX)]
    if broken:
        # snapped off at a slant, the top missing
        cut = zb + h_px * PX * rng.uniform(0.5, 0.7)
        body = [(x, min(z, cut + (x - x0) / (x1 - x0) * rng.uniform(-4, 4) * PX)) for x, z in densify(body)]
        inner = [(x, min(z, cut - 3 * PX)) for x, z in densify(inner)]
        zt = cut
    body = weather(body, rng, bites=2, floor_z=zb)
    slab(b, body, y0, y1)
    # raised border: the face panel sits a little back from a frame
    frame_out = densify(inner)
    n = len(frame_out)
    ccx = sum(p[0] for p in frame_out) / n
    ccz = sum(p[1] for p in frame_out) / n
    frame_in = [(ccx + (x - ccx) * 0.86, ccz + (z - ccz) * 0.9) for x, z in frame_out]
    for i in range(n):
        j = (i + 1) % n
        q = [frame_out[i], frame_out[j], frame_in[j], frame_in[i]]
        b.prism(q[::-1], y0 - 0.03, y0 + 0.01)
    if style == "tablet" and not broken:
        # cap moulding overhanging the tablet
        cap = weather([(x0 - 1.5 * PX, zt - 4 * PX), (x1 + 1.5 * PX, zt - 4 * PX), (x1 + 0.5 * PX, zt), (x0 - 0.5 * PX, zt)], rng, bites=1, bite_depth=(1, 2))
        slab(b, cap, y0 - 0.05, y1 + 0.03)
    # emblem in the upper face, lettering below it
    ez = min(zt - half * 0.9, zt - 7 * PX) if style in ("arch", "gothic") else zt - 11 * PX
    if not broken or ez < zt - 4 * PX:
        if emblem == "cross":
            relief_cross(b, cx, ez, 7 * PX, y0, fleury=rng.random() < 0.5)
        elif emblem == "wreath":
            wreath(b, cx, ez, 3.2 * PX, y0)
        elif emblem == "eye":
            eye_relief(b, cx, ez, 3.6 * PX, y0)
    lettering(b, rng, cx, half * 0.7, ez - 6 * PX, lines, y0 - 0.005)
    for _ in range(rng.choice((1, 1, 2))):
        crack(b, rng, cx + rng.uniform(-0.7, 0.7) * half, zt - rng.uniform(1, 5) * PX, rng.uniform(6, 16) * PX, y0 - 0.006)
    moss_streak(b, rng, cx - half * rng.uniform(0.3, 1.0), cx + half * rng.uniform(-0.2, 0.9), zt - 0.5 * PX, y0, depth_px=rng.uniform(3, 9))
    moss_streak(b, rng, cx - half - 2 * PX, cx + half + 2 * PX, zb, y0 - 0.06, depth_px=3)
    weeds(b, rng, cx_px - w_px * 0.7, cx_px + w_px * 0.7, int(w_px * 0.5))


def celtic_cross2(b, cx_px, h_px, seed=0):
    rng = random.Random(seed)
    cx = X(cx_px)
    y0, y1 = -0.16, 0.16
    zb = plinth(b, cx, 10, [(5, 3), (3, 3), (1, 4)], y0 - 0.05, y1 + 0.05, rng)
    top = GROUND + h_px * PX
    head = top - 13 * PX
    sw, aw, at = 2.6 * PX, 14 * PX, 2.4 * PX
    # shaft tapers a little and widens at the head
    shaft = [(cx - sw - 0.8 * PX, zb), (cx + sw + 0.8 * PX, zb), (cx + sw, head - at), (cx + aw, head - at - 0.8 * PX), (cx + aw + 0.6 * PX, head), (cx + aw, head + at + 0.8 * PX),
             (cx + sw, head + at), (cx + sw - 0.4 * PX, top - 1.5 * PX), (cx, top), (cx - sw + 0.4 * PX, top - 1.5 * PX), (cx - sw, head + at), (cx - aw, head + at + 0.8 * PX),
             (cx - aw - 0.6 * PX, head), (cx - aw, head - at - 0.8 * PX), (cx - sw, head - at)]
    slab(b, weather(shaft, rng, jitter=0.2 * PX, bites=1, bite_depth=(1, 2), floor_z=zb), y0, y1)
    r0, r1 = 8.0 * PX, 10.6 * PX
    for k in range(4):
        a0 = math.pi / 2 * k + 0.24
        a1 = math.pi / 2 * (k + 1) - 0.24
        seg = [(cx + r0 * math.cos(a0 + (a1 - a0) * t / 6), head + r0 * math.sin(a0 + (a1 - a0) * t / 6)) for t in range(7)]
        seg += [(cx + r1 * math.cos(a0 + (a1 - a0) * t / 6), head + r1 * math.sin(a0 + (a1 - a0) * t / 6)) for t in range(6, -1, -1)]
        slab(b, seg, y0 + 0.03, y1)
    # knotwork boss and interlace strokes down the shaft
    b.prism([(cx + 2.2 * PX * math.cos(a), head + 2.2 * PX * math.sin(a)) for a in [2 * math.pi * t / 12 for t in range(12)]][::-1], y0 - 0.04, y0 + 0.01)
    # incised border down the shaft, as on the old high crosses
    for side in (-1, 1):
        x = cx + side * (sw - 0.9 * PX)
        b.box(x - 0.4 * PX, x + 0.4 * PX, zb + 3 * PX, head - at - 2 * PX, y0 - 0.012, y0 + 0.02, material_index=1)
    groove(b, cx - sw + 0.9 * PX, cx + sw - 0.9 * PX, zb + 3 * PX, y0 - 0.002, w=0.8)
    crack(b, rng, cx + rng.uniform(-1, 1) * PX, head - at - 2 * PX, rng.uniform(5, 12) * PX, y0 - 0.006)
    moss_streak(b, rng, cx - aw, cx - sw, head + at, y0, 4)
    moss_streak(b, rng, cx - 10 * PX, cx + 10 * PX, zb, y0 - 0.1, 3)
    weeds(b, rng, cx_px - 14, cx_px + 14, 14, max_h=9)


def fleury_cross(b, cx_px, h_px, seed=0):
    """A cross with trefoil ends, set in a socket stone on a rough cairn base.

    Proportions are what keep it a grave cross rather than a sword in the ground: a shaft as thick
    as a hand, arms reaching nearly as far as the head stands above them, and a socket stone at
    the foot."""
    rng = random.Random(seed)
    cx = X(cx_px)
    y0, y1 = -0.13, 0.13
    zb = plinth(b, cx, 14, [(4, 4), (1, 4)], y0 - 0.05, y1 + 0.05, rng)
    socket_top = zb + 7 * PX
    slab(b, weather([(cx - 6 * PX, zb), (cx + 6 * PX, zb), (cx + 5 * PX, socket_top), (cx - 5 * PX, socket_top)], rng, jitter=0.2 * PX, bites=1, floor_z=zb), y0 - 0.04, y1 + 0.04)
    top = GROUND + h_px * PX
    w, aw = 2.6 * PX, 12 * PX
    arm_z = top - 14 * PX
    slab(b, weather([(cx - w, socket_top), (cx + w, socket_top), (cx + w, top - 3 * PX), (cx - w, top - 3 * PX)], rng, jitter=0.15 * PX, bites=0, floor_z=socket_top), y0, y1)
    slab(b, [(cx - aw + 2 * PX, arm_z - w), (cx + aw - 2 * PX, arm_z - w), (cx + aw - 2 * PX, arm_z + w), (cx - aw + 2 * PX, arm_z + w)], y0, y1)
    for ex, ez in ((cx, top - 2 * PX), (cx - aw + 1.5 * PX, arm_z), (cx + aw - 1.5 * PX, arm_z)):
        for ox, oz in ((0, 1.8), (-1.8, 0), (1.8, 0), (0, -0.6)):
            p = [(ex + ox * PX + 1.8 * PX * math.cos(a), ez + oz * PX + 1.8 * PX * math.sin(a)) for a in [2 * math.pi * t / 10 for t in range(10)]]
            slab(b, p, y0, y1)
    weeds(b, rng, cx_px - 12, cx_px + 12, 8, max_h=8)


def chest_tomb(b, cx_px, w_px, h_px, seed=0):
    """A table tomb: plinth, panelled chest with corner posts, an overhanging lid, slightly shifted."""
    rng = random.Random(seed)
    cx = X(cx_px)
    half = w_px / 2 * PX
    y0, y1 = -0.45, 0.45
    zb = plinth(b, cx, w_px, [(3, 3)], y0 - 0.05, y1 + 0.05, rng)
    zt = zb + h_px * PX
    slab(b, weather([(cx - half, zb), (cx + half, zb), (cx + half, zt), (cx - half, zt)], rng, bites=1, floor_z=zb), y0, y1)
    for px in (-half + 2 * PX, half - 2 * PX):
        b.box(cx + px - 2 * PX, cx + px + 2 * PX, zb, zt, y0 - 0.04, y0 + 0.05)
    n_panels = max(2, int(w_px // 14))
    pw = (2 * half - 8 * PX) / n_panels
    for i in range(n_panels):
        x = cx - half + 4 * PX + i * pw
        frame = [(x + PX, zb + 3 * PX), (x + pw - PX, zb + 3 * PX), (x + pw - PX, zt - 3 * PX), (x + PX, zt - 3 * PX)]
        for a, c in zip(frame, frame[1:] + frame[:1]):
            groove(b, min(a[0], c[0]), max(a[0], c[0]) + 0.01, a[1], y0 - 0.002) if a[1] == c[1] else b.box(a[0] - 0.45 * PX, a[0] + 0.45 * PX, min(a[1], c[1]), max(a[1], c[1]), y0 - 0.014, y0 + 0.02, material_index=1)
        if i % 2 == 0:
            relief_cross(b, x + pw / 2, (zb + zt) / 2, 5 * PX, y0, w=1.0)
    shift = rng.uniform(1.5, 3.5) * PX
    lid = weather([(cx - half - 2 * PX + shift, zt), (cx + half + 2 * PX + shift, zt), (cx + half + 1 * PX + shift, zt + 4 * PX), (cx - half - 1 * PX + shift, zt + 4 * PX)], rng, bites=2, bite_depth=(1, 2.5))
    slab(b, lid, y0 - 0.06, y1 + 0.04)
    for _ in range(2):
        crack(b, rng, cx + rng.uniform(-0.8, 0.8) * half, zt - PX, rng.uniform(5, 10) * PX, y0 - 0.006)
    moss_streak(b, rng, cx - half - 2 * PX + shift, cx + half + shift, zt + 4 * PX, y0 - 0.06, 6)
    weeds(b, rng, cx_px - w_px * 0.6, cx_px + w_px * 0.6, int(w_px * 0.9))


def pillar_urn(b, cx_px, h_px, seed=0, broken=False):
    """A square memorial pillar with mouldings, crowned by a draped urn (or snapped off)."""
    rng = random.Random(seed)
    cx = X(cx_px)
    y0, y1 = -0.22, 0.22
    zb = plinth(b, cx, 14, [(4, 3), (2, 3)], y0 - 0.05, y1 + 0.05, rng)
    shaft_top = GROUND + (h_px - (2 if broken else 14)) * PX
    w = 5.5 * PX
    slab(b, weather([(cx - w, zb), (cx + w, zb), (cx + w - 0.8 * PX, shaft_top), (cx - w + 0.8 * PX, shaft_top)], rng, bites=2, floor_z=zb), y0, y1)
    b.box(cx - w - PX, cx + w + PX, zb + 6 * PX, zb + 8 * PX, y0 - 0.04, y1, seam=SEAM)
    lettering(b, rng, cx, w * 0.75, zb + 16 * PX, 3, y0 - 0.005, line_gap=3.0)
    eye_relief(b, cx, zb + 22 * PX, 2.6 * PX, y0) if rng.random() < 0.5 else wreath(b, cx, zb + 22 * PX, 2.4 * PX, y0, 10)
    if broken:
        jag = [(cx - w + 0.8 * PX, shaft_top)] + [(cx - w + 0.8 * PX + t * (2 * w - 1.6 * PX) / 6, shaft_top + rng.uniform(0, 4) * PX) for t in range(1, 6)] + [(cx + w - 0.8 * PX, shaft_top)]
        slab(b, jag + [(cx + w - 0.8 * PX, shaft_top - 0.1 * PX), (cx - w + 0.8 * PX, shaft_top - 0.1 * PX)], y0, y1)
    else:
        slab(b, [(cx - w - 1.5 * PX, shaft_top), (cx + w + 1.5 * PX, shaft_top), (cx + w + 0.5 * PX, shaft_top + 2.5 * PX), (cx - w - 0.5 * PX, shaft_top + 2.5 * PX)], y0 - 0.05, y1 + 0.03)
        zb2 = shaft_top + 2.5 * PX
        prof = [(1.8, 0), (3.6, 1.2), (5.2, 3.5), (5.6, 6), (4.6, 8.5), (2.4, 9.6), (3.0, 10.4), (1.4, 11.5)]
        pts = [(cx + r * PX, zb2 + z * PX) for r, z in prof] + [(cx - r * PX, zb2 + z * PX) for r, z in reversed(prof)]
        slab(b, pts, y0, y1)
        # drapery folds across the urn
        for k in range(3):
            groove(b, cx - (4.4 - k) * PX, cx + (3.6 - k) * PX, zb2 + (4.4 + k * 1.6) * PX, y0 - 0.002, w=0.7)
    crack(b, rng, cx + rng.uniform(-2, 2) * PX, shaft_top - 2 * PX, rng.uniform(8, 16) * PX, y0 - 0.006)
    moss_streak(b, rng, cx - w - PX, cx + w + PX, zb + 8 * PX, y0 - 0.04, 5)
    weeds(b, rng, cx_px - 14, cx_px + 14, 14)


def finish_piece(b, pivot_x_px, turn=22.0, tip=7.0, lean=0.0):
    """Turns a monument to show its side, tips it to show its top, leans it, all about its foot;
    returns the transform it applied."""
    px_ = X(pivot_x_px)
    to = Matrix.Translation((px_, 0, GROUND))
    back = Matrix.Translation((-px_, 0, -GROUND))
    m = to @ Matrix.Rotation(math.radians(lean), 4, "Y") @ Matrix.Rotation(math.radians(-tip), 4, "X") @ Matrix.Rotation(math.radians(turn), 4, "Z") @ back
    bmesh.ops.transform(b.bm, matrix=m, verts=list(b.bm.verts))
    return m


GROW = 1.4   # monuments are modelled at dstar's sizes / GROW, then grown about their foot


def place(name, mat, fn, x_px, *args, turn=22.0, tip=7.0, lean=0.0, seed=0, **kw):
    global SEAM
    SEAM = PX / GROW
    globals()["SEAM"] = SEAM
    b = Builder(name, mat, seed or (zlib.crc32(name.encode()) & 0xFFFF))
    first_inscription = len(INSCRIPTIONS)
    fn(b, x_px, *args, seed=seed or (zlib.crc32(name.encode()) & 0xFFFF), **kw)
    px_ = X(x_px)
    grow = Matrix.Translation((px_, 0, GROUND)) @ Matrix.Diagonal((GROW, GROW, GROW, 1)) @ Matrix.Translation((-px_, 0, -GROUND))
    bmesh.ops.transform(b.bm, matrix=grow, verts=list(b.bm.verts))
    moved = finish_piece(b, x_px, turn=turn, tip=tip, lean=lean) @ grow
    for area in INSCRIPTIONS[first_inscription:]:
        for key in ("centre", "right", "up"):
            area[key] = moved @ area[key]
    return b.finish(bevel=PX * 0.9)


def place_fence(name, mat, x0, x1, h_px, lean=0.0):
    b = Builder(name, mat, zlib.crc32(name.encode()) & 0xFFFF)
    fence(b, x0, x1, h_px=h_px)
    if lean:
        b.lean(X((x0 + x1) / 2), GROUND, lean)
    return b.finish(bevel=0)


def materials():
    return (stone_variants("stone", (0.045, 0.056, 0.062), moss=0.18),
            stone_variants("stone_dark", (0.034, 0.042, 0.048), moss=0.15),
            material("iron", (0.010, 0.010, 0.013), rough=0.45, tint_var=0.0))


def build_layout(layout, cam, ground_py, bush_skip, seed):
    global GROUND, SEAM
    GROUND = Z(ground_py)
    SEAM = PX
    globals()["GROUND"], globals()["SEAM"] = GROUND, SEAM
    clear_scene()
    INSCRIPTIONS.clear()
    scn = bpy.context.scene
    stone, dark, iron = materials()
    mats = {"stone": stone, "dark": dark}
    for name, mat, fn, x, args, kw in layout:
        if fn is None:
            place_fence(name, iron, x, args[0], args[1])
        else:
            place(name, mats[mat], fn, x, *args, **kw)
    bushes(scn, seed, cam["x0"] - 20, cam["x1"] + 20, skip=bush_skip)
    ivy(scn, seed, starts=[], x_range=(cam["x0"], cam["x1"]), moss_floor=GROUND + 4 * PX, name="moss", moss_chance=0.22)
    setup_stage(scn, cam)
    return scn


