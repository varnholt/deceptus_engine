"""A small gothic chapel for the graveyard, modelled after dstar's reference (a painted miniature
chapel): a narrow, tall gable front with a steep gable; a deep pointed portal of moulded orders
with dog-tooth ornament under a crocketed gablet that holds a cusped rose; a tall traceried window
in the gable; square corner buttresses with blind niches and crocketed pinnacles; a side of two
bays, each window under its own crocketed gablet between pinnacled buttresses; a roof of single
tiles with spiked cresting; a celtic cross on the apex. Walls are smooth dressed stone with faint
joints, never brick courses.

Built in its own frame (u along the front, v back along the side, w up, in model units of one
level pixel), then scaled, turned so the front faces the player at the left and the side runs
back to the right, and tipped to show the roof. Level pixels, origin x 5300; the level's
collision ceiling runs at y 1128-1152.

Runs inside Blender after blender_kit/core.py, blender_kit/graves.py and mausoleum.py.
"""
import bpy, bmesh, math, random
from mathutils import Matrix, Vector

FRONT_W = 300          # front wall width between the corner buttress centres
SIDE_L = 250           # side wall length
EAVES_H = 430          # wall height
GABLE_RISE = 400       # a steep gable, rising out of view
BUTTRESS = 80          # corner buttress section
PORTAL_HALF = 32       # door opening half width
PORTAL_SPRING = 104
ORDER = 6              # width of each archivolt order
ORDERS = 4
PORTAL_APEX = PORTAL_SPRING + math.sqrt((2 * PORTAL_HALF + ORDERS * ORDER) ** 2 - PORTAL_HALF ** 2)
FIRST_SETBACK = PORTAL_APEX + 14   # the buttresses step back just above the portal
SCALE = 1.2            # model units to level pixels: the portal is six player heights; walls and gable rise past the camera room's top
TURN_DEG = 0.0         # the front faces the player, like the rest of the scenery
TIP_DEG = 8.0
PORTAL_SCREEN_X = 5628  # the portal's centre on screen; dstar's owl shrine stands on the ground before the door
FOOT_SCREEN_Y = 1574    # the plinth's lowest point, just under the grass line
JOINT = 1.0             # the joint between two stones: at least one game pixel, so the courses read
CHAPEL_CAM = dict(x0=5208, x1=5952, y0=1056, y1=1560)   # 31 x 21 tiles


# ---------------------------------------------------------------- building in the chapel's frame
class Part:
    """A Builder in the chapel's frame: u along the front, v into the building, w up."""

    def __init__(self, name, mat, seed):
        self.b = Builder(name, mat, seed)
        self.rng = self.b.rng

    def block(self, u0, u1, w0, w1, v0, v1, material_index=0):
        self.b.box(u0 * PX, u1 * PX, w0 * PX, w1 * PX, v0 * PX, v1 * PX, material_index=material_index)

    def slab(self, outline, v0, v1, material_index=0):
        """A convex u-w outline extruded from v0 to v1."""
        self.b.prism([(u * PX, w * PX) for u, w in outline], v0 * PX, v1 * PX, material_index=material_index)

    def side_slab(self, outline, u0, u1, material_index=0):
        """A convex v-w outline extruded along u from u0 to u1."""
        bm = self.b.bm
        near = [bm.verts.new((u0 * PX, v * PX, w * PX)) for v, w in outline]
        far = [bm.verts.new((u1 * PX, v * PX, w * PX)) for v, w in outline]
        faces = [bm.faces.new(near), bm.faces.new(far[::-1])]
        for i in range(len(outline)):
            j = (i + 1) % len(outline)
            faces.append(bm.faces.new((near[i], far[i], far[j], near[j])))
        self.b._tag(faces)
        for f in faces:
            f.material_index = material_index

    def cone(self, u, v, w0, w1, r0, r1=0.0, segments=8):
        res = bmesh.ops.create_cone(self.b.bm, cap_ends=True, segments=segments, radius1=r0 * PX,
                                    radius2=max(r1, 0.001) * PX, depth=(w1 - w0) * PX)
        bmesh.ops.translate(self.b.bm, verts=res["verts"], vec=(u * PX, v * PX, (w0 + w1) / 2 * PX))
        self.b._tag({f for vv in res["verts"] for f in vv.link_faces})

    def finish(self, bevel=0.5):
        return self.b.finish(bevel=PX * bevel)

    def start_stone(self):
        """Everything built until end_stone is one stone for the converter: one id, one tone."""
        value = self.rng.random()
        layer = self.b.rand_layer

        def tag(faces):
            for f in faces:
                f[layer] = value
        self.b._tag = tag

    def end_stone(self):
        del self.b._tag   # back to the Builder's own: a new value per piece

    def glass(self, outline, v0, v1):
        GLASS[0].slab(outline, v0, v1)


GLASS = []   # the one Part every window pane goes into, set up by build_chapel_model


class SideView:
    """Front-plane drawing calls redirected onto the side wall: u becomes v, the face at u = face
    looking along +u, depths measured outwards from it."""

    def __init__(self, part, face):
        self.p, self.face, self.rng = part, face, part.rng

    def slab(self, outline, v0, v1, material_index=0):
        self.p.side_slab(outline, self.face - v1, self.face - v0, material_index)

    def block(self, u0, u1, w0, w1, v0, v1, material_index=0):
        self.p.side_slab([(u0, w0), (u1, w0), (u1, w1), (u0, w1)][::-1], self.face - v1, self.face - v0, material_index)

    def cone(self, u, v, w0, w1, r0, r1=0.0, segments=8):
        self.p.cone(self.face - v, u, w0, w1, r0, r1, segments)

    def glass(self, outline, v0, v1):
        GLASS[0].side_slab(outline, self.face - v1, self.face - v0)


def pointed_arc(cu, half, radius, spring, steps=10):
    """Two-centred pointed arch from the left spring over the apex to the right spring."""
    # the left arc is centred at cu + half and runs from the spring (angle pi) to the apex over
    # cu, where cos(angle) = -half / radius
    sweep = math.acos(half / radius)
    left = [(cu + half + radius * math.cos(math.pi - sweep * t / steps),
             spring + radius * math.sin(math.pi - sweep * t / steps)) for t in range(steps + 1)]
    return left + [(2 * cu - u, w) for u, w in reversed(left)][1:]


def arch_wedges(cu, half, r_in, r_out, spring, steps=12, per_stone=1):
    """The ring between two concentric pointed arches as convex pieces."""
    outer = pointed_arc(cu, half, r_out, spring, steps)
    inner = pointed_arc(cu, half, r_in, spring, steps)
    for i in range(0, len(outer) - 1, per_stone):
        j = min(i + per_stone, len(outer) - 1)
        yield outer[i:j + 1] + inner[i:j + 1][::-1]


def opening(cu, half, spring, sill, steps=10):
    """A pointed opening (radius = width) as one convex outline."""
    return pointed_arc(cu, half, 2 * half, spring, steps) + [(cu + half, sill), (cu - half, sill)]


def ring(cu, cw, r_in, r_out, segments=20):
    for t in range(segments):
        a0, a1 = 2 * math.pi * t / segments, 2 * math.pi * (t + 1) / segments
        yield [(cu + r_out * math.cos(a0), cw + r_out * math.sin(a0)), (cu + r_out * math.cos(a1), cw + r_out * math.sin(a1)),
               (cu + r_in * math.cos(a1), cw + r_in * math.sin(a1)), (cu + r_in * math.cos(a0), cw + r_in * math.sin(a0))]


def inside_frame(u, w, cu, half, spring, sill, grow=2.0):
    """Inside an opening grown by `grow` along its own arcs (same centres as the frame), so a wall
    cut to it hides its stepped edge under the frame instead of showing it beyond."""
    if (w < sill) or (abs(u - cu) > half + grow):
        return False
    if w <= spring:
        return True
    r = 2 * half + grow
    return ((u - (cu + half)) ** 2 + (w - spring) ** 2 <= r * r) and ((u - (cu - half)) ** 2 + (w - spring) ** 2 <= r * r)


def inside_opening(u, w, cu, half, spring, sill):
    if (w < sill) or (abs(u - cu) > half):
        return False
    if w <= spring:
        return True
    r = 2 * half
    return ((u - (cu + half)) ** 2 + (w - spring) ** 2 <= r * r) and ((u - (cu - half)) ** 2 + (w - spring) ** 2 <= r * r)


# ---------------------------------------------------------------- ornament
def crockets(p, a, b, spacing, v0, v1, size=2.4):
    """Leaf knobs climbing from a to b: a rounded bud on a short stalk, curling up."""
    n = max(1, int(math.dist(a, b) / spacing))
    v = (v0 + v1) / 2
    for k in range(n):
        t = (k + 0.6) / n
        u, w = a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t
        s = size * p.rng.uniform(0.9, 1.1)
        p.cone(u, v, w - s * 0.2, w + s * 0.9, s * 0.9, s * 0.75, segments=8)
        p.cone(u, v, w + s * 0.9, w + s * 1.6, s * 0.75, s * 0.15, segments=8)


def gablet(p, cu, half, base, apex, v_front, v_back, finial=True):
    """A small steep gable: moulded rakes with crockets and a fleur finial."""
    for sgn in (-1, 1):
        rake = [(cu + sgn * half, base), (cu + sgn * (half - 4.5), base), (cu, apex - 5), (cu, apex)]
        p.slab(rake if sgn < 0 else rake[::-1], v_front, v_back)
        crockets(p, (cu + sgn * (half - 1), base + 5), (cu + sgn * 1.5, apex - 1), 6.5, v_front - 1.2, v_front + 2.5)
    if finial:
        p.cone(cu, (v_front + v_back) / 2, apex - 1, apex + 4, 2.2, 1.0, segments=6)
        for du in (-2.4, 2.4):
            p.cone(cu + du, (v_front + v_back) / 2, apex + 2, apex + 5, 1.6, 0.3, segments=5)
        p.cone(cu, (v_front + v_back) / 2, apex + 3, apex + 9, 1.6, 0.2, segments=6)


def pinnacle(p, cu, cv, base, width, height, broken=False):
    """Square shaft, a crocketed gablet on each face, a crocketed spire with a finial; a broken
    one has lost the top of its spire."""
    h = width / 2
    shaft_top = base + height * 0.34
    p.block(cu - h, cu + h, base, shaft_top, cv - h, cv + h)
    # a blind lancet on the shaft's front and side
    p.slab(opening(cu, h * 0.45, shaft_top - h * 2.2, base + 3, 6), cv - h - 0.1, cv - h + 0.8, material_index=1)
    p.side_slab(opening(cv, h * 0.45, shaft_top - h * 2.2, base + 3, 6), cu + h - 0.8, cu + h + 0.1, material_index=1)
    gablet(p, cu, h + 1.5, shaft_top, shaft_top + width * 1.05, cv - h - 1.5, cv - h + 2.0, finial=False)
    spire_top = base + height
    if broken:
        stump = shaft_top + (spire_top - shaft_top) * 0.45
        p.cone(cu, cv, shaft_top, stump, width * 0.44, width * 0.25, segments=4)
        p.slab([(cu - width * 0.25, stump), (cu + width * 0.1, stump), (cu - width * 0.05, stump + 3)], cv - width * 0.3, cv + width * 0.3)
        return
    p.cone(cu, cv, shaft_top, spire_top, width * 0.44, 0.3, segments=4)
    n = int((spire_top - shaft_top - width) / 5)
    for k in range(1, n + 1):
        w = shaft_top + width * 0.9 + (spire_top - shaft_top - width) * k / (n + 1)
        r = width * 0.44 * (1 - (w - shaft_top) / (spire_top - shaft_top)) + 0.6
        for du, dv in ((-1, 0), (1, 0), (0, -1)):
            p.cone(cu + du * r, cv + dv * r, w - 1.5, w + 2.5, 1.9, 0.3, segments=5)
    p.cone(cu, cv, spire_top - 1, spire_top + 3, 2.0, 0.8, segments=6)
    p.cone(cu, cv, spire_top + 2, spire_top + 8, 1.5, 0.2, segments=6)


def dogtooth(p, cu, half, radius, spring, v_face, count=18):
    """Small four-leaved pyramids set along an arch order, the classic gothic ornament."""
    pts = pointed_arc(cu, half, radius, spring, count)
    for u, w in pts[1:-1]:
        p.cone(u, v_face - 0.6, w - 1.4, w + 1.4, 1.6, 0.2, segments=4)


def cusped_head(p, cu, half, spring, v0, v1):
    """Trefoil cusps inside a lancet head: three small rings meeting under the arch."""
    r = min(half * 0.52, 7.0)   # cusps are small points, never big rings
    for du, dw in ((-half * 0.45, 0.0), (half * 0.45, 0.0), (0.0, half * 0.62)):
        for seg in ring(cu + du, spring + dw, r, r + 0.9, 20):
            p.slab(seg, v0, v1)


def traceried_window(p, cu, sill, half, spring, lights=None, blind=False):
    """A deep pointed window: two stepped reveal orders, a hood mould with label stops, a sloped
    sill, mullions, cusped lights and a quatrefoil roundel in the head over dark glass."""
    reveal = 6.0
    if blind:
        p.slab(opening(cu, half, spring, sill), reveal - 1.5, reveal + 1.0)   # blind tracery: stone behind, no glass
    else:
        p.glass(opening(cu, half, spring, sill), reveal, reveal + 1.0)
    for k, (grow, v_face) in enumerate(((0.0, 2.0), (4.5, -0.8))):
        for wedge in arch_wedges(cu, half, 2 * half + grow, 2 * half + grow + 4.5, spring, 24, 2):
            p.slab(wedge, v_face, reveal)
        for sgn in (-1, 1):
            u0 = cu + sgn * (half + grow)
            p.slab([(u0, sill), (u0 + sgn * 3.0, sill), (u0 + sgn * 3.0, spring), (u0, spring)][:: -sgn], v_face, reveal)
    for wedge in arch_wedges(cu, half, 2 * half + 9.0, 2 * half + 12.0, spring, 28, 2):
        p.slab(wedge, -2.4, 1.0)
    for sgn in (-1, 1):
        stop_u = cu + sgn * (half + 10.5)
        p.block(stop_u - 1.6, stop_u + 1.6, spring - 3, spring + 1, -2.6, 1.0)
    p.slab([(cu - half - 7, sill - 4), (cu + half + 7, sill - 4), (cu + half + 7, sill), (cu - half - 7, sill + 0.5)][::-1], -3.0, reveal)
    jamb_stones(p, cu, half + 6.0, sill, spring)
    if lights is None:
        lights = max(2, round(half / 14))   # lights stay window-sized in wide openings
    light_half = (2 * half - (lights - 1) * 2.4) / lights / 2
    light_spring = spring + half * 0.18
    for k in range(lights):
        lc = cu - half + light_half + k * (2 * light_half + 2.4)
        if k < lights - 1:
            mu = lc + light_half
            p.slab([(mu, sill), (mu + 2.4, sill), (mu + 2.4, light_spring + light_half * 1.7), (mu, light_spring + light_half * 1.7)][::-1], 2.6, reveal)
        for wedge in arch_wedges(lc, light_half, 2 * light_half, 2 * light_half + 2.2, light_spring, 12, 2):
            p.slab(wedge, 2.6, reveal)
        cusped_head(p, lc, light_half, light_spring, 3.0, reveal - 0.5)
    rc_w = light_spring + light_half * 2.0 + half * 0.36
    rr = min(half * 0.36, 16.0)
    for seg in ring(cu, rc_w, rr, rr + 1.8, 28):
        p.slab(seg, 2.6, reveal)
    for a in (0, math.pi / 2, math.pi, 3 * math.pi / 2):
        for seg in ring(cu + rr * 0.42 * math.cos(a), rc_w + rr * 0.42 * math.sin(a), rr * 0.36, rr * 0.36 + 1.0, 20):
            p.slab(seg, 3.0, reveal - 0.5)


def jamb_stones(p, cu, inner_half, sill, spring, course=9.0, long_=11.0, short=6.0):
    """Dressed stones framing an opening's sides in long-and-short alternation, a little proud of
    the wall, the way real chapels frame their windows and doors."""
    w, k = sill, 0
    while w < spring - 1:
        h = min(course, spring - w)
        width = long_ if k % 2 == 0 else short
        for sgn in (-1, 1):
            u0 = cu + sgn * inner_half
            u1 = u0 + sgn * width
            p.slab([(min(u0, u1), w + JOINT), (max(u0, u1), w + JOINT), (max(u0, u1), w + h), (min(u0, u1), w + h)][::-1], -0.8, 3.0)
        w += h
        k += 1


def rose_window(p, cu, cw, r, v_face):
    """Moulded ring with dog-tooth, eight cusped petals around a hub, dark glass."""
    p.glass([(cu + r * math.cos(2 * math.pi * t / 24), cw + r * math.sin(2 * math.pi * t / 24)) for t in range(24)][::-1],
            v_face + 4.0, v_face + 5.0)
    for seg in ring(cu, cw, r, r + 3.2, 24):
        p.slab(seg, v_face - 1.4, v_face + 4.0)
    for seg in ring(cu, cw, r + 3.2, r + 4.6, 24):
        p.slab(seg, v_face - 0.4, v_face + 4.0)
    for k in range(8):
        a = 2 * math.pi * k / 8
        for seg in ring(cu + r * 0.56 * math.cos(a), cw + r * 0.56 * math.sin(a), r * 0.24, r * 0.24 + 1.0, 20):
            p.slab(seg, v_face + 0.6, v_face + 3.6)
    for seg in ring(cu, cw, r * 0.16, r * 0.16 + 1.4, 20):
        p.slab(seg, v_face + 0.2, v_face + 3.6)


def blind_niche(p, cu, w0, w1, half, v_face, depth=2.5):
    """A pointed niche sunk into a buttress face, with a crocketed hood."""
    spring = w1 - 2 * half * 0.87
    p.slab(opening(cu, half, spring, w0), v_face + depth, v_face + depth + 0.8, material_index=1)
    for wedge in arch_wedges(cu, half, 2 * half, 2 * half + 1.6, spring, 8, 1):
        p.slab(wedge, v_face - 0.6, v_face + depth)
    for sgn in (-1, 1):
        u0 = cu + sgn * half
        p.slab([(u0, w0), (u0 + sgn * 1.6, w0), (u0 + sgn * 1.6, spring), (u0, spring)][:: -sgn], v_face - 0.6, v_face + depth)


def celtic_cross(p, cu, cv, base, height):
    stem, arm = 3.2, height * 0.32
    arm_w = base + height * 0.66
    p.block(cu - 7, cu + 7, base, base + 5, cv - 6, cv + 6)
    p.block(cu - stem, cu + stem, base + 5, base + height, cv - 2.8, cv + 2.8)
    p.block(cu - arm, cu + arm, arm_w - stem, arm_w + stem, cv - 2.8, cv + 2.8)
    for seg in ring(cu, arm_w, arm * 0.56, arm * 0.56 + 2.4, 20):
        p.slab(seg, cv - 2.2, cv + 2.2)


# ---------------------------------------------------------------- dressed stone walls
def dressed_wall(p, outline, holes, v_face, thickness, course=9.0, min_w=20.0, max_w=36.0, quoins=True, decay=0.004):
    """A wall of coursed stone as on real gothic chapels: shallow courses of one height, stones two
    to four times as long as tall in running bond, thin crisp joints, and at the wall's vertical
    edges quoins in long-and-short alternation standing a little proud. Cut along the outline
    and around the holes (functions (u, w) -> inside)."""
    us = [u for u, _ in outline]
    ws = [w for _, w in outline]
    u_min, u_max, w_min, w_max = min(us), max(us), min(ws), max(ws)

    def inside_outline(u, w):
        n, hit = len(outline), False
        for i in range(n):
            (u1, w1), (u2, w2) = outline[i], outline[(i + 1) % n]
            if (w1 > w) != (w2 > w) and u < u1 + (w - w1) * (u2 - u1) / (w2 - w1):
                hit = not hit
        return hit
    w, row = w_min, 0
    while w < w_max - 0.5:
        h = min(course, w_max - w)
        u = u_min
        first = True
        while u < u_max - 0.5:
            bw = p.rng.uniform(min_w, max_w)
            if quoins and first:
                bw = 26.0 if row % 2 else 15.0
            if quoins and (u_max - (u + bw) < 26.0):
                bw = u_max - u - ((15.0 if row % 2 else 26.0) if (u_max - u) > 40 else 0.0)
                bw = max(bw, 1.0)
            is_quoin = quoins and (first or (u + bw >= u_max - 0.5))
            first = False
            # decay: now and then a stone has fallen out, leaving the dark core behind it
            fallen = (not is_quoin) and (row > 2) and (p.rng.random() < decay)
            ua, ub = max(u, u_min), min(u + bw, u_max)
            # a block is cut into thin vertical strips so it can follow the outline and the holes;
            # the strips stay one stone for the converter
            part = getattr(p, "p", p)
            part.start_stone()
            strip = 2.0
            s = ua
            while s < ub - 1e-6:
                e = min(s + strip, ub)
                um = (s + e) / 2
                lo, hi = w, w + h
                rows = [lo + (hi - lo) * t / 8 for t in range(9)]
                filled = [inside_outline(um, rw) and not any(hole(um, rw) for hole in holes) for rw in rows]
                start = None
                for t in range(9):
                    if filled[t] and start is None:
                        start = t
                    if (not filled[t] or t == 8) and start is not None:
                        end = t if not filled[t] else 8
                        if end > start:
                            joint_l = JOINT if abs(s - ua) < 1e-6 else 0.0
                            joint_b = JOINT if start == 0 else 0.0
                            proud = -0.6 if is_quoin else 0.0
                            if not fallen:
                                p.block(s + joint_l, e, rows[start] + joint_b, rows[end], v_face + proud, v_face + thickness)
                            p.block(s, e, rows[start], rows[end], v_face + 0.4, v_face + 0.9, material_index=1)
                        start = None
                s = e
            part.end_stone()
            u += bw
        w += h
        row += 1


# ---------------------------------------------------------------- weathered stone
GRIME_DISTANCE = 0.12       # ambient-occlusion reach for grime in crevices and under ledges
CORNER_DISTANCE = 1.4       # a broader, softer occlusion: contact shadow where buttresses meet the walls
DEPTH_NEAR, DEPTH_FAR = -3.4, 0.6   # depth along the tipped axis of the buttress fronts and of the window reveals
DEPTH_DARKEST = 0.7         # brightness at DEPTH_FAR relative to the buttress fronts
EDGE_WEAR = 0.35            # how much lighter the worn arrises are
STREAKS = 0.0               # rain streaks turn into stripe noise at game size


def weather_material(m):
    """Layers what real chapel stone shows over the kit's stone colour: grime collecting in
    crevices and under every ledge (ambient occlusion), arrises worn lighter (curvature), and
    rain streaks running down (noise stretched vertically)."""
    nt = m.node_tree
    bsdf = next(n for n in nt.nodes if n.type == "BSDF_PRINCIPLED")
    link = next(l for l in nt.links if (l.to_node == bsdf) and (l.to_socket.name == "Base Color"))
    colour = link.from_socket
    nt.links.remove(link)

    ao = nt.nodes.new("ShaderNodeAmbientOcclusion")
    ao.inputs["Distance"].default_value = GRIME_DISTANCE
    grime = nt.nodes.new("ShaderNodeMapRange")
    grime.inputs["To Min"].default_value = 0.42
    grime.inputs["To Max"].default_value = 1.0
    nt.links.new(ao.outputs["AO"], grime.inputs["Value"])

    geo = nt.nodes.new("ShaderNodeNewGeometry")
    wear = nt.nodes.new("ShaderNodeMapRange")
    wear.inputs["From Min"].default_value = 0.52
    wear.inputs["From Max"].default_value = 0.58
    wear.inputs["To Min"].default_value = 1.0
    wear.inputs["To Max"].default_value = 1.0 + EDGE_WEAR
    nt.links.new(geo.outputs["Pointiness"], wear.inputs["Value"])

    tc = nt.nodes.new("ShaderNodeTexCoord")
    stretch = nt.nodes.new("ShaderNodeMapping")
    stretch.inputs["Scale"].default_value = (9.0, 9.0, 0.35)
    nt.links.new(tc.outputs["Object"], stretch.inputs["Vector"])
    streak_noise = nt.nodes.new("ShaderNodeTexNoise")
    streak_noise.inputs["Scale"].default_value = 3.0
    streak_noise.inputs["Detail"].default_value = 2.0
    nt.links.new(stretch.outputs["Vector"], streak_noise.inputs["Vector"])
    streak = nt.nodes.new("ShaderNodeMapRange")
    streak.inputs["From Min"].default_value = 0.55
    streak.inputs["From Max"].default_value = 0.75
    streak.inputs["To Min"].default_value = 1.0
    streak.inputs["To Max"].default_value = 1.0 - STREAKS
    nt.links.new(streak_noise.outputs["Fac"], streak.inputs["Value"])

    corner_ao = nt.nodes.new("ShaderNodeAmbientOcclusion")
    corner_ao.inputs["Distance"].default_value = CORNER_DISTANCE
    corner = nt.nodes.new("ShaderNodeMapRange")
    corner.inputs["To Min"].default_value = 0.4
    corner.inputs["To Max"].default_value = 1.0
    nt.links.new(corner_ao.outputs["AO"], corner.inputs["Value"])
    both = nt.nodes.new("ShaderNodeMath"); both.operation = "MULTIPLY"
    nt.links.new(grime.outputs[0], both.inputs[0])
    nt.links.new(corner.outputs[0], both.inputs[1])
    factor = nt.nodes.new("ShaderNodeMath"); factor.operation = "MULTIPLY"
    nt.links.new(both.outputs[0], factor.inputs[0])
    nt.links.new(wear.outputs[0], factor.inputs[1])
    factor2 = nt.nodes.new("ShaderNodeMath"); factor2.operation = "MULTIPLY"
    nt.links.new(factor.outputs[0], factor2.inputs[0])
    nt.links.new(streak.outputs[0], factor2.inputs[1])
    # depth shading: a surface further back from the front plane is darker, so the buttress stages,
    # walls and reveals read as layers
    along = nt.nodes.new("ShaderNodeVectorMath"); along.operation = "DOT_PRODUCT"   # depth along the chapel's tipped axis, not raw y
    nt.links.new(tc.outputs["Object"], along.inputs[0])
    along.inputs[1].default_value = (0.0, math.cos(math.radians(TIP_DEG)), math.sin(math.radians(TIP_DEG)))
    depth = nt.nodes.new("ShaderNodeMapRange")
    depth.inputs["From Min"].default_value = DEPTH_NEAR
    depth.inputs["From Max"].default_value = DEPTH_FAR
    depth.inputs["To Min"].default_value = 1.0
    depth.inputs["To Max"].default_value = DEPTH_DARKEST
    nt.links.new(along.outputs["Value"], depth.inputs["Value"])
    factor3 = nt.nodes.new("ShaderNodeMath"); factor3.operation = "MULTIPLY"
    nt.links.new(factor2.outputs[0], factor3.inputs[0])
    nt.links.new(depth.outputs[0], factor3.inputs[1])
    factor2 = factor3
    apply = nt.nodes.new("ShaderNodeVectorMath"); apply.operation = "SCALE"
    nt.links.new(colour, apply.inputs[0])
    nt.links.new(factor2.outputs[0], apply.inputs["Scale"])
    nt.links.new(apply.outputs[0], bsdf.inputs["Base Color"])
    return m


def dark_glass():
    """Old dark glass with only a faint, uneven sheen: no light burns inside."""
    m = bpy.data.materials.new("chapel_glass")
    m.use_nodes = True
    nt = m.node_tree
    nt.nodes.clear()
    out = nt.nodes.new("ShaderNodeOutputMaterial")
    emit = nt.nodes.new("ShaderNodeEmission")
    emit.inputs["Color"].default_value = (0.006, 0.009, 0.026, 1)
    tc = nt.nodes.new("ShaderNodeTexCoord")
    noise = nt.nodes.new("ShaderNodeTexNoise")
    noise.inputs["Scale"].default_value = 5.0
    noise.inputs["Detail"].default_value = 2.0
    nt.links.new(tc.outputs["Object"], noise.inputs["Vector"])
    strength = nt.nodes.new("ShaderNodeMapRange")
    strength.inputs["To Min"].default_value = 0.7
    strength.inputs["To Max"].default_value = 1.4
    nt.links.new(noise.outputs["Fac"], strength.inputs["Value"])
    nt.links.new(strength.outputs[0], emit.inputs["Strength"])
    nt.links.new(emit.outputs[0], out.inputs[0])
    return m


def weathered_stone(name, base, **kw):
    return [weather_material(m) for m in stone_variants(name, base, **kw)]


# ---------------------------------------------------------------- the chapel
def build_front(stone, trim, seed):
    W, H, cu = FRONT_W, EAVES_H, FRONT_W / 2
    r_in = 2 * PORTAL_HALF
    r_out = r_in + ORDERS * ORDER
    portal_apex = PORTAL_SPRING + math.sqrt(r_out ** 2 - PORTAL_HALF ** 2)
    g_half, g_apex = r_out - PORTAL_HALF + 9, portal_apex + 64
    rose_c, rose_r = portal_apex + 24, 11.0
    win_half, win_sill = 62, g_apex + 10
    win_spring = win_sill + 150
    lancets = []   # none beside the portal: the front stays calm around the great window and the door
    gable_rose_c, gable_rose_r = win_spring + 2 * win_half * 0.87 + 62, 34.0
    objects = []

    wall = Part("front_wall", stone, seed)
    outline = [(0, 0), (W, 0), (W, H), (cu, H + GABLE_RISE), (0, H)]
    holes = [lambda u, w: inside_opening(u, w, cu, PORTAL_HALF + 1, PORTAL_SPRING, 0),
             lambda u, w: inside_frame(u, w, cu, win_half, win_spring, win_sill),
             lambda u, w: math.hypot(u - cu, w - rose_c) < rose_r,
             lambda u, w: math.hypot(u - cu, w - gable_rose_c) < gable_rose_r]
    holes += [lambda u, w, l=l: inside_frame(u, w, l[0], l[1], l[3], l[2]) for l in lancets]
    dressed_wall(wall, outline, holes, 0.0, 6.0)
    objects.append(wall.finish(bevel=0.3))

    portal = Part("portal", trim, seed + 1)
    for k in range(ORDERS):
        ra, rb = r_in + k * ORDER, r_in + (k + 1) * ORDER
        v_face = -2.0 - 1.6 * k
        roll = k % 2 == 0
        for wedge in arch_wedges(cu, PORTAL_HALF, ra + (1.6 if roll else 0), rb - (1.6 if roll else 0), PORTAL_SPRING, 16, 2):
            portal.slab(wedge, v_face - (1.2 if roll else 0), 10)
        for wedge in arch_wedges(cu, PORTAL_HALF, ra, rb, PORTAL_SPRING, 16, 2):
            portal.slab(wedge, v_face + 1.0, 10)
        if k == 1:
            dogtooth(portal, cu, PORTAL_HALF, (ra + rb) / 2, PORTAL_SPRING, v_face - 0.4)
        for sgn in (-1, 1):
            ua, ub = cu + sgn * (ra - PORTAL_HALF), cu + sgn * (rb - PORTAL_HALF)
            lo, hi = min(ua, ub), max(ua, ub)
            portal.block(lo - 0.5, hi + 0.5, 0, 14, v_face - 1.5, 10)
            portal.cone((lo + hi) / 2, v_face + 1.0, 14, PORTAL_SPRING - 6, ORDER * 0.42 if roll else ORDER * 0.3, ORDER * 0.42 if roll else ORDER * 0.3, segments=10)
            portal.block(lo - 0.8, hi + 0.8, PORTAL_SPRING - 6, PORTAL_SPRING - 3, v_face - 1.6, 10)
            portal.block(lo - 0.4, hi + 0.4, PORTAL_SPRING - 3, PORTAL_SPRING, v_face - 1.0, 10)
    for wedge in arch_wedges(cu, PORTAL_HALF, r_out, r_out + 3.5, PORTAL_SPRING + 0.5, 18, 2):
        portal.slab(wedge, -10.0, 2)
    jamb_stones(portal, cu, r_out - PORTAL_HALF + 3.5, 0, PORTAL_SPRING, long_=14.0, short=8.0)
    objects.append(portal.finish(bevel=0.35))
    door = Part("door", emissive_material("chapel_door", (0.005, 0.008, 0.024), 1.0), seed + 2)
    door.slab(opening(cu, PORTAL_HALF, PORTAL_SPRING, 0, 12), 14, 15)
    objects.append(door.finish(bevel=0))

    gab = Part("gablet", trim, seed + 3)
    gablet(gab, cu, g_half, PORTAL_SPRING + 8, g_apex, -12, -4)
    rose_window(gab, cu, rose_c, rose_r, -8.0)
    objects.append(gab.finish(bevel=0.35))

    win = Part("gable_window", trim, seed + 4)
    traceried_window(win, cu, win_sill, win_half, win_spring, lights=3)
    for lc, lh, ls, lsp in lancets:
        traceried_window(win, lc, ls, lh, lsp)
    objects.append(win.finish(bevel=0.35))

    top = Part("gable_top", trim, seed + 5)
    top.block(-4, W + 4, H - 5, H, 2, SIDE_L + 4)
    for sgn in (-1, 1):
        eave = (cu - sgn * (W / 2 + 6), H - 1)
        rake = [eave, (eave[0] + sgn * 20, H - 1), (cu, H + GABLE_RISE - 10), (cu, H + GABLE_RISE + 10)]
        top.slab(rake if sgn > 0 else rake[::-1], -8, 10)
        crockets(top, (eave[0] + sgn * 14, H + 22), (cu - sgn * 6, H + GABLE_RISE - 4), 20, -12, -2, size=5.5)
    rose_window(top, cu, gable_rose_c, gable_rose_r, -2.0)
    # a moulded string course under the lancets' sills, broken by the portal's gablet
    gablet_reach = g_half + 4
    for w_band in (116,):
        for u0_, u1_ in ((BUTTRESS * 0.5, cu - gablet_reach), (cu + gablet_reach, W - BUTTRESS * 0.5)):
            top.block(u0_, u1_, w_band - 5, w_band, -4.0, 2.0)
            top.block(u0_, u1_, w_band - 2, w_band, -5.0, 2.0)
    jamb_reach = r_out - PORTAL_HALF + 12   # clear of the portal's jamb stones
    plinth_band(top, BUTTRESS * 0.5 + 2, cu - jamb_reach, -2.5)
    plinth_band(top, cu + jamb_reach, W - BUTTRESS * 0.5 - 2, -2.5)
    celtic_cross(top, cu, 2, H + GABLE_RISE + 4, 48)
    objects.append(top.finish(bevel=0.35))
    return objects


def build_side(stone, trim, seed):
    """Side wall at u = FRONT_W: two bays, each window under its own crocketed gablet."""
    W, L, H = FRONT_W, SIDE_L, EAVES_H
    bays = (L * 0.26,)
    win_half, win_sill, win_spring = 19, 48, 128
    wall = Part("side_wall", stone, seed)
    face = SideView(wall, W)
    holes = [lambda v, w, bc=bc: inside_frame(v, w, bc, win_half, win_spring, win_sill) for bc in bays]
    dressed_wall(face, [(0, 0), (L, 0), (L, H), (0, H)], holes, 0.0, 6.0)
    objects = [wall.finish(bevel=0.3)]
    orn = Part("side_windows", trim, seed + 1)
    face = SideView(orn, W)
    for bc in bays:
        traceried_window(face, bc, win_sill, win_half, win_spring)
        top = win_spring + 2 * win_half * 0.87 + 8
        gablet(face, bc, win_half + 12, top - 6, top + 34, -4.0, 2.0)
    objects.append(orn.finish(bevel=0.35))
    rear = Part("rear_wall", stone, seed + 2)
    rear.block(0, W, 0, H, L - 8, L)
    rear.slab([(0, H), (W, H), (W / 2, H + GABLE_RISE)], L - 8, L)
    objects.append(rear.finish())
    return objects


def coursed_pier(p, u0, u1, w0, w1, v0, v1, course=9.0):
    """A buttress stage in the wall's courses, in running bond: two stones a course with the
    vertical joint alternating side, a dark core behind so the joints read."""
    p.block(u0 + 0.6, u1 - 0.6, w0, w1, v0 + 0.6, v1, material_index=1)
    w, k = w0, 0
    while w < w1 - 0.5:
        h = min(course, w1 - w)
        split = u0 + (u1 - u0) * (0.38 if k % 2 == 0 else 0.64)
        p.block(u0, split, w + JOINT, w + h, v0, v1)
        p.block(split + JOINT, u1, w + JOINT, w + h, v0, v1)
        w += h
        k += 1


def quatrefoil_band(p, u0, u1, w0, height, v_face):
    """A frieze of quatrefoils in square panels, as under the gables of Bigelow Chapel."""
    p.slab([(u0, w0), (u1, w0), (u1, w0 + height), (u0, w0 + height)][::-1], v_face + 0.8, v_face + 3.0)
    n = max(1, int((u1 - u0) / height))
    step = (u1 - u0) / n
    for k in range(n):
        cu, cw = u0 + step * (k + 0.5), w0 + height / 2
        r = height * 0.16
        for a in (0, math.pi / 2, math.pi, 3 * math.pi / 2):
            for seg in ring(cu + r * math.cos(a), cw + r * math.sin(a), r * 0.75, r * 0.75 + max(0.9, height * 0.07), 16):
                p.slab(seg, v_face + 0.2, v_face + 1.2)
        for seg in ring(cu, cw, height * 0.42, height * 0.42 + max(0.8, height * 0.07), 12):
            p.slab(seg, v_face + 0.3, v_face + 1.2)


PLINTH_BAND_W0 = 16       # the quatrefoil band along the foot starts just above the grass and candles
PLINTH_BAND_H = 20        # about one player height: the medallions read at game size


def plinth_band(p, u0, u1, v_face):
    """The quatrefoil frieze repeated low along the front, at the height the player looks at."""
    quatrefoil_band(p, u0, u1, PLINTH_BAND_W0, PLINTH_BAND_H, v_face)


class BlindFace:
    """Front-plane calls shifted onto a buttress face standing forward of the wall at v = face."""

    def __init__(self, part, face):
        self.p, self.face, self.rng = part, face, part.rng

    def slab(self, outline, v0, v1, material_index=0):
        self.p.slab(outline, self.face + v0, self.face + v1, material_index)

    def block(self, u0, u1, w0, w1, v0, v1, material_index=0):
        self.p.block(u0, u1, w0, w1, self.face + v0, self.face + v1, material_index)

    def cone(self, u, v, w0, w1, r0, r1=0.0, segments=8):
        self.p.cone(u, self.face + v, w0, w1, r0, r1, segments)

    def glass(self, outline, v0, v1):
        GLASS[0].slab(outline, self.face + v0, self.face + v1)


def build_buttresses(stone, seed):
    """Square buttresses at the front corners and along the side, stepped and pinnacled."""
    but = Part("buttresses", stone, seed)
    stages = [(0, FIRST_SETBACK, 1.0), (FIRST_SETBACK, EAVES_H * 0.76, 0.86), (EAVES_H * 0.76, EAVES_H + 4, 0.74)]
    for bu, bv, faces_front in ((0, 0, True), (FRONT_W, 0, True)):
        for w0, w1, scale in stages:
            s = BUTTRESS * scale / 2
            if faces_front:
                coursed_pier(but, bu - s, bu + s, w0, w1, bv - s * 1.4, bv + s)
                but.slab([(bu - s - 1.2, w1), (bu + s + 1.2, w1), (bu + s + 1.2, w1 + 3), (bu - s - 1.2, w1 + 3)][::-1], bv - s * 1.4 - 1.2, bv + s)
            else:
                but.block(bu - s, bu + s * 1.4, w0, w1, bv - s, bv + s)
                but.side_slab([(bv - s - 1.2, w1), (bv + s + 1.2, w1), (bv + s + 1.2, w1 + 3), (bv - s - 1.2, w1 + 3)][::-1], bu - s, bu + s * 1.4 + 1.2)
        if faces_front:
            s = BUTTRESS / 2
            plinth_band(BlindFace(but, bv - s * 1.4), bu - s + 3, bu + s - 3, -2.5)
        pinnacle(but, bu, bv, EAVES_H + 7, BUTTRESS * 0.7, 250)
    return [but.finish(bevel=0.35)]


def tiled_roof(roof, deck, u0, u1, eaves, rise, v0, v1, spikes, lost=()):
    """A gable roof with its ridge along v: courses of single tiles, each lifted a little
    differently, over a dark deck, and a row of spiked cresting on the ridge."""
    cu, over = (u0 + u1) / 2, 9
    rows = int(math.hypot((u1 - u0) / 2 + over, rise) / 10)
    for sgn in (-1, 1):
        eave_u = cu - sgn * ((u1 - u0) / 2 + over)
        plane = [(eave_u, eaves - 4), (cu, eaves + rise - 2), (cu, eaves + rise - 5), (eave_u, eaves - 7)]
        deck.slab(plane if sgn < 0 else plane[::-1], v0, v1)
        for r in range(rows):
            t0, t1 = r / rows, min((r + 1.5) / rows, 1.0)
            a = (eave_u + (cu - eave_u) * t0, eaves - 3 + (rise + 1) * t0)
            b = (eave_u + (cu - eave_u) * t1, eaves - 3 + (rise + 1) * t1)
            v = v0 + (6.5 if r % 2 else 0)
            while v < v1:
                length = roof.rng.uniform(11, 14)
                lift = roof.rng.uniform(0.9, 1.3)
                tile = [(a[0], a[1] + lift - 1), (b[0], b[1] + lift - 1), (b[0], b[1] + lift + 1.4), (a[0], a[1] + lift + 1.4)]
                roof.slab(tile if sgn < 0 else tile[::-1], v, min(v + length - 0.35, v1))
                v += length
    for k in range(spikes):
        if k in lost:
            continue
        v = v0 + 14 + k * (v1 - v0 - 28) / max(spikes - 1, 1)
        roof.cone(cu, v, eaves + rise - 1, eaves + rise + 3, 3.8, 3.2, segments=6)
        roof.cone(cu, v, eaves + rise + 3, eaves + rise + roof.rng.uniform(18, 22), 3.0, 0.3, segments=6)
    roof.block(cu - 3.5, cu + 3.5, eaves + rise - 5, eaves + rise + 1, v0, v1)


def build_roof(roof_mat, seed):
    roof = Part("roof", roof_mat, seed)
    deck = Part("roof_deck", roof_mat, seed + 1)
    tiled_roof(roof, deck, 0, FRONT_W, EAVES_H, GABLE_RISE, -6, SIDE_L + 6, 11, lost=(6, 7))
    return [roof.finish(bevel=0.3), deck.finish(bevel=0)]


ANNEX = dict(v0=14, v1=SIDE_L, width=160, eaves=270, rise=120)
ANNEX_DARKEN = 0.18   # the side rooms recede: darker stone than the chapel front


def annex_span(side):
    """The u range of the side room on the left (side -1) or right (+1), clear of the corner buttress."""
    if side < 0:
        return -ANNEX["width"] - 30, -30
    return FRONT_W + 30, FRONT_W + 30 + ANNEX["width"]


def build_annex(stone, trim, roof_mat, seed, side):
    """The side room: a lower block on the chapel's left, set back, its gable end facing the
    front like the chapel's, with a traceried window under a gablet, a pinnacled corner and its
    own tiled roof. Darker stone than the chapel, so it stays in the background. side -1 builds
    the room on the left, +1 its mirror image on the right."""
    u0, u1 = annex_span(side)
    v0, v1, H, rise = ANNEX["v0"], ANNEX["v1"], ANNEX["eaves"], ANNEX["rise"]
    cu = (u0 + u1) / 2
    win_half, win_sill, win_spring = 26, 50, 150   # the gablet over the window ends under the frieze
    wall = Part("annex_wall", stone, seed)
    front = [(u0, -30), (u1, -30), (u1, H), (cu, H + rise), (u0, H)]   # down into the ground: never a gap under it

    class AnnexFront:
        """Front-plane calls placed on the annex's front face at v = v0."""
        def __init__(self, part):
            self.p, self.rng = part, part.rng

        def slab(self, outline, a, b, material_index=0):
            self.p.slab(outline, v0 + a, v0 + b, material_index)

        def block(self, ua, ub, wa, wb, a, b, material_index=0):
            self.p.block(ua, ub, wa, wb, v0 + a, v0 + b, material_index)

        def cone(self, u, v, wa, wb, r0, r1=0.0, segments=8):
            self.p.cone(u, v0 + v, wa, wb, r0, r1, segments)

        def glass(self, outline, a, b):
            GLASS[0].slab(outline, v0 + a, v0 + b)
    face = AnnexFront(wall)
    dressed_wall(face, front, [lambda u, w: inside_frame(u, w, cu, win_half, win_spring, win_sill)], 0.0, 6.0)
    objects = [wall.finish(bevel=0.3)]
    orn = Part("annex_ornament", trim, seed + 1)
    face = AnnexFront(orn)
    traceried_window(face, cu, win_sill, win_half, win_spring)
    gablet(face, cu, win_half + 14, win_spring + 30, win_spring + 2 * win_half * 0.87 + 38, -6.0, 0.0)
    orn.block(u0 - 2, u1 + 2, H - 4, H, v0 - 3, v1 + 2)
    quatrefoil_band(face, u0 + 10, u1 - 10, H - 34, 26, -2.5)   # large enough to read at game size
    pier_clear = BUTTRESS * 0.4 + 8   # the band stops at the pinnacled corner pier
    plinth_band(face, u0 + (pier_clear if side < 0 else 6), u1 - (6 if side < 0 else pier_clear), -2.5)
    for sgn in (-1, 1):
        eave = (cu - sgn * ((u1 - u0) / 2 + 5), H - 1)
        rake = [eave, (eave[0] + sgn * 9, H - 1), (cu, H + rise - 4), (cu, H + rise + 5)]
        face.slab(rake if sgn > 0 else rake[::-1], -4, 8)
        crockets(face, (eave[0] + sgn * 7, H + 10), (cu - sgn * 3, H + rise - 2), 16, -7, 0, size=4.0)
    for bu in ((u0 + 4) if side < 0 else (u1 - 4),):   # the pinnacled corner is the outer one
        for w0, w1, scale in ((0, 70, 0.8), (70, H + 3, 0.66)):
            s = BUTTRESS * scale / 2
            coursed_pier(orn, bu - s, bu + s, w0, w1, v0 - s * 1.4, v0 + s)
        pinnacle(orn, bu, v0, H + 6, BUTTRESS * 0.6, 96)
    objects.append(orn.finish(bevel=0.35))
    roof = Part("annex_roof", roof_mat, seed + 2)
    deck = Part("annex_deck", roof_mat, seed + 3)
    tiled_roof(roof, deck, u0, u1, H, rise, v0 - 4, v1 + 4, 5)
    objects += [roof.finish(bevel=0.3), deck.finish(bevel=0)]
    return objects


def build_plinth(trim, seed):
    plinth = Part("plinth", trim, seed)
    for grow, w0, w1 in ((12, -8, -3), (6, -3, 2)):
        plinth.block(-BUTTRESS / 2 - grow, FRONT_W + BUTTRESS / 2 + grow, w0, w1, -BUTTRESS * 0.7 - grow, SIDE_L + grow)
    return [plinth.finish()]


def annex_materials():
    """The chapel's stone, trim and roof, darkened for the side room."""
    d = ANNEX_DARKEN
    return (weathered_stone("annex_stone", (0.058 * d, 0.066 * d, 0.070 * d), moss=0.16, tint_var=0.09),
            weathered_stone("annex_trim", (0.066 * d, 0.074 * d, 0.078 * d), moss=0.03, tint_var=0.04),
            weathered_stone("annex_roof", (0.060 * d, 0.050 * d, 0.042 * d), moss=0.04, tint_var=0.10))


def build_chapel_model(seed=17):
    """Every part in the chapel's own frame; returns the objects."""
    GLASS[:] = [Part("window_glass", dark_glass(), seed + 99)]
    stone = weathered_stone("chapel_stone", (0.058, 0.066, 0.070), moss=0.16, tint_var=0.09)
    trim = weathered_stone("chapel_trim", (0.066, 0.074, 0.078), moss=0.03, tint_var=0.04)
    roof_mat = weathered_stone("chapel_roof", (0.060, 0.050, 0.042), moss=0.04, tint_var=0.10)
    return (build_plinth(trim, seed + 1) + build_front(stone, trim, seed + 10) + build_side(stone, trim, seed + 30)
            + build_buttresses(stone, seed + 40) + build_roof(roof_mat, seed + 50)
            + build_annex(*annex_materials(), seed + 60, -1) + build_annex(*annex_materials(), seed + 70, 1)
            + [GLASS[0].finish(bevel=0)])


def place_chapel(objects):
    """Scale, turn the front to the player's left, tip to show the roof, then move so the portal
    sits behind the shrine and the plinth's lowest point on the grass line."""
    frame = (Matrix.Rotation(math.radians(TIP_DEG), 4, "X") @ Matrix.Rotation(math.radians(-TURN_DEG), 4, "Z")
             @ Matrix.Scale(SCALE, 4))
    for ob in objects:
        ob.data.transform(frame)
    portal = frame @ Vector((FRONT_W / 2 * PX, -10 * PX, 40 * PX))
    plinth = next(ob for ob in objects if ob.name.startswith("plinth"))
    lowest = min(v.co.z for v in plinth.data.vertices)
    move = Matrix.Translation((X(PORTAL_SCREEN_X) - portal.x, 0.0, Z(FOOT_SCREEN_Y) - lowest))
    for ob in objects:
        ob.data.transform(move)
    PLACEMENT[:] = [move @ frame]


PLACEMENT = []   # chapel frame (level-pixel units) to world, set by place_chapel


def to_level(u, v, w):
    """A point of the chapel's frame as (level x px, level y px, world depth)."""
    p = PLACEMENT[0] @ Vector((u * PX, v * PX, w * PX))
    return p.x / PX + ORIGIN_X_PX, GROUND_PY - p.z / PX, p.y


def grow_moss_and_ivy(scn, seed):
    """Moss cushions on the ledges (buttress offsets, the gable window sill, the plinth), ivy
    climbing the right buttress and the side room as on the reference, a little on the left."""
    ledges = []
    s0, s1 = BUTTRESS / 2, BUTTRESS * 0.86 / 2
    for bu in (0, FRONT_W):
        for w, s in ((FIRST_SETBACK, s0), (EAVES_H * 0.76, s1)):
            a, b = to_level(bu - s, -s * 1.4, w + 3), to_level(bu + s, -s * 1.4, w + 3)
            ledges.append((a[0], b[0], a[1], a[2]))
    a, b = to_level(-BUTTRESS / 2 - 6, -BUTTRESS * 0.7 - 6, 2), to_level(FRONT_W + BUTTRESS / 2 + 6, -BUTTRESS * 0.7 - 6, 2)
    ledges.append((a[0], b[0], a[1], a[2]))
    a, b = to_level(FRONT_W / 2 - 70, -3, 228), to_level(FRONT_W / 2 + 70, -3, 228)
    ledges.append((a[0], b[0], a[1], a[2]))
    moss_cushions("chapel_moss", ledges, seed + 200, cover=0.7)
    x_join, py_join, _ = to_level(FRONT_W + BUTTRESS * 0.6, 0, EAVES_H * 0.55)
    ivy_curtain(scn, "chapel_ivy_join", seed + 390, x_join, py_join, 30, 220)
    for k, (u, w, width, length) in enumerate(((FRONT_W + 8, EAVES_H * 0.9, 26, 170), (-ANNEX["width"] + 6, ANNEX["eaves"], 30, 120), (FRONT_W + ANNEX["width"] - 6, ANNEX["eaves"], 30, 120),
                                               (-6, EAVES_H * 0.9, 26, 170))):
        x, py, _ = to_level(u, 0, w)
        ivy_curtain(scn, f"chapel_ivy_{k}", seed + 300 + k, x, py, width, length)


def chapel_wear(scn, seed):
    """Cracks wandering through the stones of the front, and fallen fragments at its foot."""
    bpy.context.view_layer.update()
    deps = bpy.context.evaluated_depsgraph_get()
    rng = random.Random(seed)
    b = Builder("chapel_wear", weathered_stone("chapel_chips", (0.050, 0.058, 0.062)), seed)
    x0, _, _ = to_level(-ANNEX["width"], 0, 0)
    x1, _, _ = to_level(FRONT_W + ANNEX["width"], 0, 0)
    _, top, _ = to_level(0, 0, EAVES_H + GABLE_RISE * 0.6)
    _, low, _ = to_level(0, 0, 14)
    for _ in range(24):
        x, py = rng.uniform(x0, x1), rng.uniform(top, low)
        y = surface_front(scn, deps, X(x), Z(py))
        if (y is None) or (y > 0.5):
            continue
        crack_line(b, rng, X(x), Z(py), rng.uniform(6, 16), y)
    for _ in range(9):
        x = rng.choice((rng.uniform(x0 - 10, x0 + 30), rng.uniform(x1 - 60, x1)))
        w = rng.uniform(2, 6) * PX
        b.box(X(x) - w, X(x) + w, Z(FOOT_SCREEN_Y), Z(FOOT_SCREEN_Y) + rng.uniform(2, 5) * PX, -2.2, -1.6, tilt=rng.uniform(-0.6, 0.6))
    b.finish(bevel=0)


def build_chapel_scene(seed=17):
    clear_scene()
    scn = bpy.context.scene
    place_chapel([o for o in build_chapel_model(seed) if o is not None])
    grow_moss_and_ivy(scn, seed)
    chapel_wear(scn, seed + 500)
    # dstar's shrine stands before the portal on its plinth, facing the player
    boundary = Builder("boundary_wall", weathered_stone("boundary_stone", (0.050, 0.058, 0.062), moss=0.16, tint_var=0.09), seed + 40)
    course = 9 * SCALE * PX
    ashlar(boundary, X(BOUNDARY_X - 8), X(5960), Z(1570), Z(1100), course=course, min_w=9 * SCALE * 2.2, max_w=9 * SCALE * 4.0,
           depth=(-0.3, 0.6), jitter=PX * 0.3, seam=PX)
    boundary.finish()
    setup_stage(scn, CHAPEL_CAM)
    # a weak cold fill from the right, so walls turned away from the moon keep their stonework
    fill = bpy.data.lights.new("chapel_fill", "SUN")
    fill.energy = 2.2
    fill.color = (0.55, 0.62, 1.0)
    fob = bpy.data.objects.new("chapel_fill", fill)
    fob.rotation_euler = Vector((-0.8, 0.6, -0.5)).normalized().to_track_quat("-Z", "Y").to_euler()
    scn.collection.objects.link(fob)
    return scn


def build_chapel(seed=17):
    return build_chapel_scene(seed)
