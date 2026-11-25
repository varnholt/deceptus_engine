"""The graveyard mausoleum: portal wall with an arched doorway, columns, pediment, ruined side
walls, a dark passage and the dais under dstar's owl shrine mound. Level pixels, origin x 5300.

Runs inside Blender after blender_kit/core.py and blender_kit/graves.py (run_passes.py does that).
"""
import bpy, math, random


# ---------------------------------------------------------------- layout (level pixels)
CX = X(5535)            # opening centre, over the shrine mound
HW = 50 * PX            # opening half width
SPRING = Z(1380)        # where the arch starts
RING = 16 * PX          # voussoir depth
BLOCK_X0, BLOCK_X1 = X(5395), X(5675)
BLOCK_TOP = Z(1296)
CORNICE_TOP = Z(1284)
PEDIMENT_TOP = Z(1240)
COLS = [5426, 5452]      # left pair, mirrored about the opening centre; the inner pedestals clear dstar's shrine mound (x 5472-5592)
COL_R = 7 * PX
CAP_Z = SPRING - 14 * PX
PEDESTAL_TOP = Z(1532)
CAM = dict(x0=5280, x1=5952, y0=1200, y1=1560)  # 28 x 15 tiles


def mirror_px(px):
    return 2 * 5535 - px


def inside_arch(x, z, extra=0.0):
    r = HW + extra
    if abs(x - CX) > r:
        return False
    if z <= SPRING:
        return True
    return (x - CX) ** 2 + (z - SPRING) ** 2 <= r * r


def block_clear_of_arch(xa, xb, za, zb, extra):
    for x in (xa, (xa + xb) / 2, xb):
        for z in (za, (za + zb) / 2, zb):
            if inside_arch(x, z, extra):
                return False
    return True


def ring_fill(b, side, y0, y1):
    """Wedge stones that close the wall against the outside of the voussoir ring."""
    ro = HW + RING + PX
    for i in range(10):
        a0 = math.radians(i * 9.0)
        a1 = math.radians((i + 1) * 9.0)
        near = [(CX + side * ro * math.cos(a), SPRING + ro * math.sin(a)) for a in (a0, a1)]
        far = [(CX + side * (ro + 14 * PX) * math.cos(a), SPRING + (ro + 14 * PX) * math.sin(a)) for a in (a1, a0)]
        outline = near + far
        if side > 0:
            outline = outline[::-1]
        b.prism(outline, y0, y1)


def clip_rect_to_doorway(xa, xb, za, zb, r, side, step=0.5 * PX):
    """Rectangle minus the doorway (a strip below the spring, a disc above), as one outline.

    Walks the rectangle boundary in small steps and pushes every point inside the doorway onto
    its edge, which is exact as long as the rectangle is not cut in two. Returns None when
    nothing usable is left.
    """
    def push(x, z):
        dx = x - CX
        if abs(dx) >= r:
            return x, z
        if z <= SPRING:
            return CX + side * r, z
        d = math.hypot(dx, z - SPRING)
        if d >= r:
            return x, z
        if d < 1e-6:
            return CX + side * r, SPRING
        return CX + dx * r / d, SPRING + (z - SPRING) * r / d

    corners = [(xa, za), (xb, za), (xb, zb), (xa, zb)]
    pts = []
    for i in range(4):
        (x0, z0), (x1, z1) = corners[i], corners[(i + 1) % 4]
        n = max(1, int(math.hypot(x1 - x0, z1 - z0) / step))
        for k in range(n):
            pts.append(push(x0 + (x1 - x0) * k / n, z0 + (z1 - z0) * k / n))
    # drop repeats so the face has no zero-length edges
    clean = []
    for p in pts:
        if not clean or math.hypot(p[0] - clean[-1][0], p[1] - clean[-1][1]) > 0.05 * PX:
            clean.append(p)
    if len(clean) > 2 and math.hypot(clean[0][0] - clean[-1][0], clean[0][1] - clean[-1][1]) <= 0.05 * PX:
        clean.pop()
    if len(clean) < 3:
        return None
    area = 0.5 * sum(clean[i][0] * clean[(i + 1) % len(clean)][1] - clean[(i + 1) % len(clean)][0] * clean[i][1] for i in range(len(clean)))
    if abs(area) < 9 * PX * PX:
        return None
    return clean


def arc_outline(ri, ro, a0, a1, steps):
    pts = [(CX + ri * math.cos(a0 + (a1 - a0) * k / steps), SPRING + ri * math.sin(a0 + (a1 - a0) * k / steps)) for k in range(steps + 1)]
    pts += [(CX + ro * math.cos(a0 + (a1 - a0) * k / steps), SPRING + ro * math.sin(a0 + (a1 - a0) * k / steps)) for k in range(steps, -1, -1)]
    return pts[::-1]


def build_scene(seed=7):
    clear_scene()
    scn = bpy.context.scene
    stone = stone_variants("stone", (0.045, 0.056, 0.062), moss=0.55)
    stone_dark = stone_variants("stone_dark", (0.032, 0.040, 0.046), moss=0.35)
    trim = stone_variants("trim", (0.058, 0.068, 0.074), moss=0.4, tint_var=0.03)

    # portal wall, cut around the arch and its voussoir ring
    wall = Builder("portal_wall", stone, seed)
    r_cut = HW + RING

    def keep(xm, zm, xa, xb, za, zb):
        if block_clear_of_arch(xa, xb, za, zb, r_cut + 0.5 * PX):
            return True
        # each half on its own, so a stone over the doorway's centre line is pushed towards its own side
        for lo, hi, side in ((xa, min(xb, CX), -1), (max(xa, CX), xb, 1)):
            if hi - lo < PX:
                continue
            stone_pts = clip_rect_to_doorway(lo + PX, hi, za + PX, zb, r_cut + PX, side)
            seam_hi = hi + PX if (side > 0 or hi < CX) else hi
            seam_pts = clip_rect_to_doorway(lo, seam_hi, za, zb + PX, r_cut, side)
            if stone_pts and seam_pts:
                wall.prism(stone_pts, -0.35, 0.35)
                wall.prism(seam_pts, -0.15, 0.45, material_index=1)
        return False
    ashlar(wall, BLOCK_X0, BLOCK_X1, Z(1564), BLOCK_TOP, keep=keep)
    wall.finish()

    # quoins: long and short blocks alternating up both corners, standing a little proud of the wall
    quoins = Builder("quoins", trim, seed + 21)
    z, k = Z(1540), 0
    while z < BLOCK_TOP - 6 * PX:
        h = 14 * PX
        long_ = 22 * PX if k % 2 == 0 else 13 * PX
        quoins.box(BLOCK_X0, BLOCK_X0 + long_, z, min(z + h, BLOCK_TOP), -0.45, 0.35, jitter=PX * 0.4, seam=PX)
        quoins.box(BLOCK_X1 - long_, BLOCK_X1, z, min(z + h, BLOCK_TOP), -0.45, 0.35, jitter=PX * 0.4, seam=PX)
        z += h
        k += 1
    quoins.finish()
    # base course: a row of deep plinth stones the whole front stands on
    base = Builder("base_course", trim, seed + 22)
    x = BLOCK_X0 - 3 * PX
    while x < BLOCK_X1 + 3 * PX - 1e-6:
        w = min(base.rng.uniform(26, 40) * PX, BLOCK_X1 + 3 * PX - x)
        base.box(x, x + w, Z(1566), Z(1540), -0.55, 0.4, jitter=PX * 0.5, seam=PX)
        x += w
    base.finish()

    # voussoirs with a raised keystone, a thin inner archivolt set back, dark joints behind both
    vous = Builder("voussoirs", trim, seed + 1)
    n = 13
    for i in range(n):
        a0 = math.pi * i / n + math.radians(0.9)
        a1 = math.pi * (i + 1) / n - math.radians(0.9)
        ro = HW + RING + (6 * PX if i == n // 2 else 0)
        vous.prism(arc_outline(HW, ro, a0, a1, 4), -0.48, 0.3)
    for i in range(n):
        a0 = math.pi * i / n + math.radians(1.0)
        a1 = math.pi * (i + 1) / n - math.radians(1.0)
        vous.prism(arc_outline(HW - 5 * PX, HW - PX, a0, a1, 3), -0.2, 0.5)
    vous.finish()
    joints = Builder("arch_joints", seam_material(), seed)
    joints.prism(arc_outline(HW - 6 * PX, HW + RING + 7 * PX, 0.0, math.pi, 40), -0.15, 0.4)
    joints.finish(bevel=0)

    # jambs below the spring
    jamb = Builder("jambs", trim, seed + 2)
    for side in (-1, 1):
        xa = CX + side * HW
        xb = CX + side * (HW + RING)
        z = Z(1564)
        while z < SPRING - 1e-6:
            h = min(12 * PX, SPRING - z)
            jamb.box(min(xa, xb) + PX, max(xa, xb) - PX, z + PX, z + h, -0.4, 0.3, jitter=PX * 0.5, seam=PX)
            z += h
    jamb.finish()

    # columns: pedestal, base ring, shaft, necking, capital, impost
    cols = Builder("columns", trim, seed + 3)
    for px in COLS + [mirror_px(p) for p in COLS]:
        cx = X(px)
        cols.box(cx - 10 * PX, cx + 10 * PX, Z(1564), PEDESTAL_TOP, -1.05, -0.2)
        cols.box(cx - 9 * PX, cx + 9 * PX, PEDESTAL_TOP, PEDESTAL_TOP + 3 * PX, -1.0, -0.25)
        cols.cylinder(cx, PEDESTAL_TOP + 3 * PX, PEDESTAL_TOP + 6 * PX, COL_R + 2 * PX, cy=-0.62)
        cols.cylinder(cx, PEDESTAL_TOP + 6 * PX, CAP_Z, COL_R, cy=-0.62)
        # flutes: shallow grooves down the visible half of the shaft
        for dx_px in (-4.5, -1.5, 1.5, 4.5):
            y_face = -0.62 - math.sqrt(COL_R ** 2 - (dx_px * PX) ** 2)
            cols.box(cx + (dx_px - 0.5) * PX, cx + (dx_px + 0.5) * PX, PEDESTAL_TOP + 8 * PX, CAP_Z - 4 * PX,
                     y_face - 0.012, y_face + 0.03, material_index=1)
        cols.cylinder(cx, CAP_Z - 3 * PX, CAP_Z - 2 * PX, COL_R + 1 * PX, cy=-0.62)
        cols.box(cx - 9 * PX, cx + 9 * PX, CAP_Z, CAP_Z + 5 * PX, -1.0, -0.25)
        cols.box(cx - 10 * PX, cx + 10 * PX, CAP_Z + 5 * PX, CAP_Z + 9 * PX, -1.05, -0.2)
    for side in (-1, 1):
        xs = [X(p) if side < 0 else X(mirror_px(p)) for p in COLS]
        cols.box(min(xs) - 11 * PX, max(xs) + 11 * PX, CAP_Z + 9 * PX, SPRING, -1.1, -0.15)
    cols.finish()

    # cornice, pediment with raking cornice, cross finial
    top = Builder("top", trim, seed + 4)
    top.box(BLOCK_X0 - 6 * PX, BLOCK_X1 + 6 * PX, BLOCK_TOP, BLOCK_TOP + 4 * PX, -0.7, 0.4)
    top.box(BLOCK_X0 - 9 * PX, BLOCK_X1 + 9 * PX, BLOCK_TOP + 4 * PX, CORNICE_TOP, -0.85, 0.45)
    mid = (BLOCK_X0 + BLOCK_X1) / 2
    top.prism([(BLOCK_X0 - 2 * PX, CORNICE_TOP), (BLOCK_X1 + 2 * PX, CORNICE_TOP), (mid, PEDIMENT_TOP)], -0.45, 0.4)
    half_base = (BLOCK_X1 - BLOCK_X0) / 2

    def inside_tympanum(xm, zm, xa, xb, za, zb):
        hw = half_base * (PEDIMENT_TOP - zb) / (PEDIMENT_TOP - CORNICE_TOP)
        xa, xb = max(xa, mid - hw), min(xb, mid + hw)
        return (xa, xb) if xb - xa > 4 * PX else False
    tympanum = Builder("tympanum", stone, seed + 23)
    ashlar(tympanum, BLOCK_X0, BLOCK_X1, CORNICE_TOP, PEDIMENT_TOP - 3 * PX, course=11 * PX, min_w=22, max_w=40,
           keep=inside_tympanum, depth=(-0.6, -0.45))
    tympanum.finish()
    left_rake = [(BLOCK_X0 - 9 * PX, CORNICE_TOP), (BLOCK_X0 - 4 * PX, CORNICE_TOP), (mid, PEDIMENT_TOP - 4 * PX), (mid, PEDIMENT_TOP + 4 * PX)]
    right_rake = [(2 * mid - x, z) for x, z in left_rake][::-1]
    top.prism(left_rake, -0.8, 0.4)
    top.prism(right_rake, -0.8, 0.4)
    top.box(mid - 3 * PX, mid + 3 * PX, PEDIMENT_TOP, PEDIMENT_TOP + 30 * PX, -0.5, -0.1)
    top.box(mid - 11 * PX, mid + 11 * PX, PEDIMENT_TOP + 18 * PX, PEDIMENT_TOP + 24 * PX, -0.5, -0.1)
    top.box(mid - 6 * PX, mid + 6 * PX, PEDIMENT_TOP, PEDIMENT_TOP + 4 * PX, -0.6, 0.0)
    # dentils under the cornice
    x = BLOCK_X0 + 2 * PX
    while x < BLOCK_X1 - 4 * PX:
        top.box(x, x + 3 * PX, BLOCK_TOP - 4 * PX, BLOCK_TOP, -0.55, 0.0, seam=0)
        x += 7 * PX
    # a winged skull carved in the tympanum, the old memento mori over a tomb door
    winged_skull(top, mid, CORNICE_TOP + (PEDIMENT_TOP - CORNICE_TOP) * 0.42, -0.62, seed + 9)
    for ax, base_z in ((BLOCK_X0 - 6 * PX, CORNICE_TOP), (BLOCK_X1 + 6 * PX, CORNICE_TOP)):
        top.box(ax - 4 * PX, ax + 4 * PX, base_z, base_z + 4 * PX, -0.8, 0.3)
        top.prism([(ax - 3 * PX, base_z + 4 * PX), (ax + 3 * PX, base_z + 4 * PX), (ax, base_z + 10 * PX)], -0.75, 0.25)
    top.finish()

    # ruined side walls: courses broken off along a falling profile, loose stones near the edge
    def left_profile(x):
        t = (x - X(5312)) / (BLOCK_X0 - X(5312))
        return Z(1420) + t * (Z(1372) - Z(1420)) + 0.4 * math.sin(x * 2.3) + 0.3 * math.sin(x * 5.3 + 2.0)

    def right_profile(x):
        t = max(0.0, (x - BLOCK_X1) / (X(5904) - BLOCK_X1))
        return Z(1352) + (t ** 0.8) * (Z(1512) - Z(1352)) + 0.45 * math.sin(x * 1.7) + 0.3 * math.sin(x * 4.1 + 1.0)

    for name, x0, x1, prof, sd in (("ruin_left", X(5312), BLOCK_X0, left_profile, 11), ("ruin_right", BLOCK_X1, X(5904), right_profile, 12)):
        ruin = Builder(name, stone_dark, sd)
        lr = random.Random(sd)

        carried = {}   # course bottom z -> spans of the stones kept in the course ending there

        def keep_ruin(xm, zm, xa, xb, za, zb, prof=prof, lr=lr, carried=carried):
            # a ragged top: each stone's own height limit wanders around the falling profile
            limit = prof(xm) - lr.uniform(0.0, 0.6)
            supported = za <= Z(1564) + 1e-6 or sum(
                max(0.0, min(xb, b1) - max(xa, b0)) for b0, b1 in carried.get(round(za, 4), [])) >= 0.6 * (xb - xa)
            if zb > limit or not supported:
                return False
            carried.setdefault(round(zb, 4), []).append((xa, xb))
            return True
        ashlar(ruin, x0, x1, Z(1564), Z(1340), keep=keep_ruin, depth=(-0.15, 0.45), tilt=0.035, jitter=PX * 2)
        ruin.finish()

    rng = random.Random(seed + 5)
    rub = Builder("rubble", stone_dark, seed + 6)
    for i in range(14):
        x = X(rng.uniform(5690, 5900))
        w = rng.uniform(6, 16) * PX
        h = rng.uniform(4, 9) * PX
        rub.box(x - w / 2, x + w / 2, Z(1562), Z(1554) + h, -0.9, -0.5, tilt=rng.uniform(-0.4, 0.4))
    rub.finish()

    # interior: dark passage with steps down, lit faintly from deep inside
    inner = Builder("interior", emissive_material("passage", (0.008, 0.013, 0.040), 1.0), seed + 7)
    inner.box(CX - HW - 0.5, CX + HW + 0.5, Z(1560), SPRING + HW + 0.6, 3.0, 3.4)
    inner.finish(bevel=0)
    # the passage recedes: arched rings set further back and a little narrower each time,
    # caught by a cold glow rising from the stairs below
    tunnel = Builder("tunnel", stone_dark, seed + 8)
    half_circle = [math.pi * t / 30 for t in range(31)]
    for k in range(3):
        r_in = HW - (4 + 5 * k) * PX
        r_out = r_in + 5 * PX
        y0 = 0.5 + 0.7 * k
        spring_k = SPRING - (3 + 3 * k) * PX
        pts = [(CX + r_in * math.cos(a), spring_k + r_in * math.sin(a)) for a in half_circle]
        pts += [(CX + r_out * math.cos(a), spring_k + r_out * math.sin(a)) for a in reversed(half_circle)]
        tunnel.prism(pts[::-1], y0, y0 + 0.6)
        for side in (-1, 1):
            xa, xb = sorted((CX + side * r_in, CX + side * r_out))
            tunnel.box(xa, xb, Z(1560), spring_k, y0, y0 + 0.6)
    tunnel.finish(bevel=PX)
    # the owl shrine's grass mound (level tiles, x 5472-5592, top y 1505) rests on this stone dais,
    # so nothing in the doorway hangs in the air
    dais = Builder("dais", trim, seed + 9)
    dais.box(X(5474), X(5590), Z(1566), Z(1548), -0.95, 0.35, seam=PX)
    dais.box(X(5480), X(5584), Z(1548), Z(1516), -0.85, 0.35, seam=PX)
    dais.box(X(5476), X(5588), Z(1516), Z(1508), -0.95, 0.35, seam=PX)
    for xa, xb, za, zb in ((5492, 5572, 1542, 1540), (5492, 5572, 1524, 1522), (5492, 5494, 1542, 1522), (5570, 5572, 1542, 1522)):
        dais.box(X(xa), X(xb), Z(za), Z(zb), -0.87, -0.8, material_index=1)
    dais.finish()

    deep = bpy.data.lights.new("stair_glow", "POINT")
    deep.energy = 6.0
    deep.color = (0.30, 0.42, 1.0)
    deep.shadow_soft_size = 0.4
    dob = bpy.data.objects.new("stair_glow", deep)
    dob.location = (CX, 1.4, Z(1500))
    scn.collection.objects.link(dob)

    grow_moss_and_ivy(scn, seed)

    setup_stage(scn, CAM)
    return scn




def winged_skull(b, cx, cz, y_face, seed, scale=1.6):
    """Relief: a skull with spread wings on a smooth panel, raised well off the face so its own
    shadow separates it. Each wing is a fan of broad feathers, one stone each, so the converter
    gives every feather its lit upper edge."""
    PX = globals()["PX"] * scale
    lift = 0.14
    panel = [(cx - 30 * PX, cz - 5 * PX), (cx + 30 * PX, cz - 5 * PX), (cx + 30 * PX, cz + 5 * PX), (cx - 30 * PX, cz + 5 * PX)]
    b.prism(panel[::-1], y_face - 0.02, y_face + 0.03)
    # cranium and jaw
    head = [(cx + 6.5 * PX * math.cos(a), cz + 1.5 * PX + 6 * PX * math.sin(a)) for a in [math.pi * t / 12 for t in range(13)]]
    head += [(cx - 6.5 * PX, cz - 1 * PX), (cx - 4 * PX, cz - 4 * PX), (cx + 4 * PX, cz - 4 * PX), (cx + 6.5 * PX, cz - 1 * PX)]
    b.prism(head[::-1], y_face - lift, y_face)
    b.box(cx - 3.5 * PX, cx + 3.5 * PX, cz - 7 * PX, cz - 4 * PX, y_face - lift * 0.8, y_face)
    # sockets, nose and the gaps between the teeth are cut deep, in joint colour
    for ex in (-2.6, 2.6):
        b.box(cx + (ex - 1.6) * PX, cx + (ex + 1.6) * PX, cz - 0.5 * PX, cz + 2.2 * PX, y_face - lift - 0.01, y_face - lift + 0.03, material_index=1)
    b.box(cx - 0.7 * PX, cx + 0.7 * PX, cz - 3.2 * PX, cz - 1.4 * PX, y_face - lift - 0.01, y_face - lift + 0.03, material_index=1)
    for tx in (-2, 0, 2):
        b.box(cx + (tx - 0.35) * PX, cx + (tx + 0.35) * PX, cz - 6.5 * PX, cz - 4.5 * PX, y_face - lift * 0.8 - 0.01, y_face - lift * 0.8 + 0.03, material_index=1)
    # wings: five broad feathers per side fanning from behind the skull, longest at the top
    for side in (-1, 1):
        for k, (length, angle) in enumerate(((22, 8), (20, -4), (17, -16), (13, -28), (9, -40))):
            a = math.radians(angle)
            ux, uz = side * math.cos(a), math.sin(a)          # along the feather
            nx, nz = -uz * side, ux * side                    # across it
            root_x, root_z = cx + side * 5 * PX, cz + (2.5 - k * 1.2) * PX
            w = 2.0 * PX
            pts = [(root_x + nx * w * 0.6, root_z + nz * w * 0.6),
                   (root_x + ux * length * 0.6 * PX + nx * w, root_z + uz * length * 0.6 * PX + nz * w),
                   (root_x + ux * length * PX, root_z + uz * length * PX),
                   (root_x + ux * length * 0.6 * PX - nx * w, root_z + uz * length * 0.6 * PX - nz * w),
                   (root_x - nx * w * 0.6, root_z - nz * w * 0.6)]
            if side > 0:
                pts = pts[::-1]
            b.prism(pts, y_face - lift * (0.75 - 0.08 * k), y_face)

def ruin_ledges(scn, x0_px, x1_px, step_px=4):
    """Short ledges along the broken tops of a ruin, for moss to sit on."""
    bpy.context.view_layer.update()
    deps = bpy.context.evaluated_depsgraph_get()
    ledges = []
    x = x0_px
    while x < x1_px:
        z = surface_top(scn, deps, X(x + step_px / 2))
        if z is not None and z > Z(1550):
            y = surface_front(scn, deps, X(x + step_px / 2), z - 2 * PX)
            if y is not None:
                ledges.append((x, x + step_px, GROUND_PY - z / PX, y))
        x += step_px
    return ledges


def grow_moss_and_ivy(scn, seed):
    """Moss cushions on the cornice, imposts, pedestals and the broken ruin tops; ivy pouring
    down from the cornice corners, one impost and both ruins."""
    impost_left = (min(COLS) - 11, max(COLS) + 11)
    impost_right = (mirror_px(max(COLS)) - 11, mirror_px(min(COLS)) + 11)
    ledges = [(5386, 5684, 1284, -0.85),
              (*impost_left, 1380, -1.1), (*impost_right, 1380, -1.1)]
    ledges += [(px - 10, px + 10, 1532, -1.05) for px in COLS + [mirror_px(p) for p in COLS]]
    ledges += ruin_ledges(scn, 5312, 5395) + ruin_ledges(scn, 5675, 5904)
    moss_cushions("moss", ledges, seed + 200, cover=0.8)
    for k, (x, top_py, width, length) in enumerate(((5400, 1288, 40, 110), (5674, 1288, 26, 60), (5640, 1384, 30, 80),
                                                    (5350, 1430, 44, 70), (5730, 1390, 50, 100))):
        ivy_curtain(scn, f"ivy_{k}", seed + 300 + k, x, top_py, width, length)


def mausoleum_wear(seed=77):
    """The same wear the graves carry, on the mausoleum: cracks in the stones, chips at the foot of the walls."""
    scn = bpy.context.scene
    bpy.context.view_layer.update()
    deps = bpy.context.evaluated_depsgraph_get()
    rng = random.Random(seed)
    b = Builder("mausoleum_wear", stone_variants("chips", (0.032, 0.040, 0.046)), seed)

    def front(x_px, py):
        return surface_front(scn, deps, X(x_px), Z(py))

    # cracks: wall stones, voussoirs, ruins, dais
    for _ in range(46):
        x, py = rng.uniform(5300, 5900), rng.uniform(1290, 1545)
        y = front(x, py)
        if y is None or y > 0.2:
            continue
        crack(b, rng, X(x), Z(py), rng.uniform(5, 18) * PX, y - 0.004, down=rng.random() < 0.7)
    # rain stains: dark streaks running down the wall from under the cornice, the imposts and the
    # keystone, wide where the water drips off and thinning out below
    stains = Builder("stains", material("stain", (0.020, 0.024, 0.030), rough=0.95, tint_var=0.02), seed + 1)
    sources = [(rng.uniform(5400, 5670), 1300) for _ in range(9)]
    sources += [(rng.uniform(5418, 5460), 1384), (rng.uniform(5610, 5652), 1384), (5535, 1312)]
    for sx, sy in sources:
        y = front(sx, sy + 3)
        if y is None:
            continue
        length = rng.uniform(10, 34)
        width = rng.uniform(1.0, 2.6)
        steps = 3
        for k in range(steps):
            w = width * (1.0 - 0.3 * k)
            b0 = sy + length * k / steps
            stains.box(X(sx) - w / 2 * PX, X(sx) + w / 2 * PX, Z(b0 + length / steps), Z(b0), y - 0.006, y + 0.02)
    stains.finish(bevel=0)
    # fallen chips at the foot of the walls
    for _ in range(10):
        x = X(rng.uniform(5320, 5890))
        w = rng.uniform(2, 5) * PX
        b.box(x - w, x + w, Z(1560), Z(1560) + rng.uniform(2, 4) * PX, -1.1, -0.9, tilt=rng.uniform(-0.6, 0.6))
    ob = b.finish(bevel=0)
    return ob


def build_mausoleum(seed=7):
    scn = build_scene(seed)
    mausoleum_wear()
    return scn
