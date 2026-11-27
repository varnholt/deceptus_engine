"""Blender side of the pixel-art kit: coordinate frame, materials, stone builder, stage and render passes.

Runs inside Blender. Coordinates: 1 Blender unit = 24 px (one tile). X(px) and Z(py) map level pixels
onto the scene; set ORIGIN_X_PX / GROUND_PY (build_scene.py does that from the asset config) before
building. The camera looks along +Y, so Y is depth (negative = towards the viewer).
"""
import bpy, bmesh, math, random
from mathutils import Vector, Matrix

PX = 1.0 / 24.0
# render pixels per level pixel (build_scene.py sets it from the piece's render_scale); above 1 the
# passes are reduced to level size afterwards, so each level pixel comes from many samples, not one
RENDER_SCALE = 1
ORIGIN_X_PX = 5300
GROUND_PY = 1556


def X(px):
    return (px - ORIGIN_X_PX) * PX


def Z(py):
    return (GROUND_PY - py) * PX


def clear_scene():
    # read_factory_settings would also unload the MCP add-on, so empty the file by hand
    for coll in (bpy.data.objects, bpy.data.meshes, bpy.data.materials, bpy.data.lights, bpy.data.cameras, bpy.data.worlds, bpy.data.curves):
        for item in list(coll):
            coll.remove(item)


SCAN_DIR = ""   # set by build_scene.py: the textures folder next to the kit, or the asset's "scan_dir"
SCANS = ["lichen_rock", "rock_pitted_mossy", "worn_rock_natural_01", "concrete_wall_008", "dark_rock_02", "rock_surface"]
SCAN_SPAN = 6.0   # blender units one scan covers: about 2.7 texels of the 384 px scan per game pixel


def scan_image(scan, kind):
    path = f"{SCAN_DIR}/{scan}_{kind}_384.png"
    img = bpy.data.images.get(f"{scan}_{kind}") or bpy.data.images.load(path)
    img.name = f"{scan}_{kind}"
    # read raw: the albedo scans are normalised to a mean of 0.5 and used as a x2 multiplier
    img.colorspace_settings.name = "Non-Color"
    return img


def stone_variants(name, base, **kw):
    """One material per scanned stone surface; the Builder hands each block one of them."""
    return [material(f"{name}_{scan}", base, scan=scan, **kw) for scan in SCANS]


def material(name, base, rough=0.9, moss=0.0, tint_var=0.06, scan=None):
    """Stone material: base colour varied per block, darker in cracks, moss on up-facing faces.

    With a scan, the block shows that scanned surface (albedo variation and relief), sampled at
    an offset taken from the block's random value, so no two stones show the same patch.
    """
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    nt = m.node_tree
    for n in list(nt.nodes):
        nt.nodes.remove(n)
    out = nt.nodes.new("ShaderNodeOutputMaterial")
    bsdf = nt.nodes.new("ShaderNodeBsdfPrincipled")
    bsdf.inputs["Roughness"].default_value = rough
    nt.links.new(bsdf.outputs[0], out.inputs[0])

    # per block random brightness from the block-id attribute
    attr = nt.nodes.new("ShaderNodeAttribute"); attr.attribute_name = "block_rand"
    tint = nt.nodes.new("ShaderNodeMapRange")
    tint.inputs["To Min"].default_value = 1.0 - tint_var
    tint.inputs["To Max"].default_value = 1.0 + tint_var
    nt.links.new(attr.outputs["Fac"], tint.inputs["Value"])
    col = nt.nodes.new("ShaderNodeRGB"); col.outputs[0].default_value = (*base, 1)
    mul = nt.nodes.new("ShaderNodeMix"); mul.data_type = "RGBA"; mul.blend_type = "MULTIPLY"
    mul.inputs["Factor"].default_value = 1.0
    nt.links.new(col.outputs[0], mul.inputs[6])
    comb = nt.nodes.new("ShaderNodeCombineColor")
    for i in range(3):
        nt.links.new(tint.outputs[0], comb.inputs[i])
    nt.links.new(comb.outputs[0], mul.inputs[7])

    # weathering: large soft noise darkens patches
    tc = nt.nodes.new("ShaderNodeTexCoord")
    noise = nt.nodes.new("ShaderNodeTexNoise")
    noise.inputs["Scale"].default_value = 0.9
    noise.inputs["Detail"].default_value = 3.0
    nt.links.new(tc.outputs["Object"], noise.inputs["Vector"])
    ramp = nt.nodes.new("ShaderNodeValToRGB")
    ramp.color_ramp.elements[0].position = 0.35; ramp.color_ramp.elements[0].color = (0.72, 0.74, 0.76, 1)
    ramp.color_ramp.elements[1].position = 0.65; ramp.color_ramp.elements[1].color = (1, 1, 1, 1)
    nt.links.new(noise.outputs["Fac"], ramp.inputs[0])
    weather = nt.nodes.new("ShaderNodeMix"); weather.data_type = "RGBA"; weather.blend_type = "MULTIPLY"
    weather.inputs["Factor"].default_value = 1.0
    nt.links.new(mul.outputs[2], weather.inputs[6])
    nt.links.new(ramp.outputs[0], weather.inputs[7])
    final = weather

    if moss > 0:
        geo = nt.nodes.new("ShaderNodeNewGeometry")
        sep = nt.nodes.new("ShaderNodeSeparateXYZ")
        nt.links.new(geo.outputs["Normal"], sep.inputs[0])
        mnoise = nt.nodes.new("ShaderNodeTexNoise")
        mnoise.inputs["Scale"].default_value = 4.0
        mnoise.inputs["Detail"].default_value = 6.0
        nt.links.new(tc.outputs["Object"], mnoise.inputs["Vector"])
        add = nt.nodes.new("ShaderNodeMath"); add.operation = "ADD"
        nt.links.new(sep.outputs["Z"], add.inputs[0])
        scale = nt.nodes.new("ShaderNodeMath"); scale.operation = "MULTIPLY"
        scale.inputs[1].default_value = moss
        nt.links.new(mnoise.outputs["Fac"], scale.inputs[0])
        nt.links.new(scale.outputs[0], add.inputs[1])
        mramp = nt.nodes.new("ShaderNodeValToRGB")
        mramp.color_ramp.interpolation = "CONSTANT"
        mramp.color_ramp.elements[1].position = 0.95
        nt.links.new(add.outputs[0], mramp.inputs[0])
        mcol = nt.nodes.new("ShaderNodeRGB"); mcol.outputs[0].default_value = (0.010, 0.045, 0.014, 1)
        mix = nt.nodes.new("ShaderNodeMix"); mix.data_type = "RGBA"
        nt.links.new(mramp.outputs[0], mix.inputs["Factor"])
        nt.links.new(weather.outputs[2], mix.inputs[6])
        nt.links.new(mcol.outputs[0], mix.inputs[7])
        final = mix

    # damp: stone near the ground turns darker and greener
    sepz = nt.nodes.new("ShaderNodeSeparateXYZ")
    nt.links.new(tc.outputs["Object"], sepz.inputs[0])
    damp_f = nt.nodes.new("ShaderNodeMapRange")
    damp_f.inputs["From Min"].default_value = 0.0
    damp_f.inputs["From Max"].default_value = 4.0
    damp_f.inputs["To Min"].default_value = 1.0
    damp_f.inputs["To Max"].default_value = 0.0
    nt.links.new(sepz.outputs["Z"], damp_f.inputs["Value"])
    damp = nt.nodes.new("ShaderNodeMix"); damp.data_type = "RGBA"; damp.blend_type = "MULTIPLY"
    nt.links.new(damp_f.outputs[0], damp.inputs["Factor"])
    nt.links.new(final.outputs[2], damp.inputs[6])
    damp.inputs[7].default_value = (0.45, 0.70, 0.55, 1)
    nt.links.new(damp.outputs[2], bsdf.inputs["Base Color"])

    if scan:
        # a different patch of the scan for every block: offset the lookup by the block's random value
        off = nt.nodes.new("ShaderNodeCombineXYZ")
        for i, k in enumerate((37.13, 11.71, 23.37)):
            mul_r = nt.nodes.new("ShaderNodeMath"); mul_r.operation = "MULTIPLY"; mul_r.inputs[1].default_value = k
            nt.links.new(attr.outputs["Fac"], mul_r.inputs[0])
            nt.links.new(mul_r.outputs[0], off.inputs[i])
        shifted = nt.nodes.new("ShaderNodeVectorMath"); shifted.operation = "ADD"
        nt.links.new(tc.outputs["Object"], shifted.inputs[0])
        nt.links.new(off.outputs[0], shifted.inputs[1])
        scaled = nt.nodes.new("ShaderNodeVectorMath"); scaled.operation = "SCALE"
        scaled.inputs["Scale"].default_value = 1.0 / SCAN_SPAN
        nt.links.new(shifted.outputs[0], scaled.inputs[0])
        tex_d = nt.nodes.new("ShaderNodeTexImage"); tex_d.image = scan_image(scan, "diff")
        tex_h = nt.nodes.new("ShaderNodeTexImage"); tex_h.image = scan_image(scan, "disp")
        for t in (tex_d, tex_h):
            t.projection = "BOX"; t.projection_blend = 0.25; t.interpolation = "Cubic"
            nt.links.new(scaled.outputs[0], t.inputs["Vector"])
        # albedo: the scan's own light and dark (normalised to mean 0.5) and a little of its tint
        detail = nt.nodes.new("ShaderNodeMix"); detail.data_type = "RGBA"; detail.blend_type = "MULTIPLY"
        detail.inputs["Factor"].default_value = 1.0
        nt.links.new(damp.outputs[2], detail.inputs[6])
        gain = nt.nodes.new("ShaderNodeMix"); gain.data_type = "RGBA"; gain.blend_type = "MULTIPLY"
        gain.inputs["Factor"].default_value = 1.0
        nt.links.new(tex_d.outputs["Color"], gain.inputs[6])
        gain.inputs[7].default_value = (2.0, 2.0, 2.0, 1)
        nt.links.new(gain.outputs[2], detail.inputs[7])
        nt.links.new(detail.outputs[2], bsdf.inputs["Base Color"])
        bump = nt.nodes.new("ShaderNodeBump")
        bump.inputs["Strength"].default_value = 0.8
        bump.inputs["Distance"].default_value = 0.05
        nt.links.new(tex_h.outputs["Color"], bump.inputs["Height"])
        nt.links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])
        return m

    # surface grain, so faces are not flat fills
    grain = nt.nodes.new("ShaderNodeTexNoise")
    grain.inputs["Scale"].default_value = 7.0
    grain.inputs["Detail"].default_value = 8.0
    grain.inputs["Roughness"].default_value = 0.65
    nt.links.new(tc.outputs["Object"], grain.inputs["Vector"])
    bump = nt.nodes.new("ShaderNodeBump")
    bump.inputs["Strength"].default_value = 0.9
    bump.inputs["Distance"].default_value = 0.03
    nt.links.new(grain.outputs["Fac"], bump.inputs["Height"])
    nt.links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])
    return m


def seam_material():
    """Near-black, unlit: mortar joints and the outline around every stone."""
    m = bpy.data.materials.get("seam")
    if m:
        return m
    m = bpy.data.materials.new("seam")
    m.use_nodes = True
    nt = m.node_tree
    bsdf = next(n for n in nt.nodes if n.type == "BSDF_PRINCIPLED")
    bsdf.inputs["Base Color"].default_value = (0.0025, 0.003, 0.0045, 1)
    bsdf.inputs["Roughness"].default_value = 1.0
    return m


def emissive_material(name, colour, strength):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    nt = m.node_tree
    for n in list(nt.nodes):
        nt.nodes.remove(n)
    out = nt.nodes.new("ShaderNodeOutputMaterial")
    em = nt.nodes.new("ShaderNodeEmission")
    em.inputs["Strength"].default_value = strength
    # vertical gradient: black under the vault, a faint glow rising from the steps below
    tc = nt.nodes.new("ShaderNodeTexCoord")
    sep = nt.nodes.new("ShaderNodeSeparateXYZ")
    nt.links.new(tc.outputs["Object"], sep.inputs[0])
    ramp = nt.nodes.new("ShaderNodeValToRGB")
    ramp.color_ramp.elements[0].position = 0.0
    ramp.color_ramp.elements[0].color = (0, 0, 0, 1)
    ramp.color_ramp.elements[1].position = 1.0
    ramp.color_ramp.elements[1].color = (*colour, 1)
    mr = nt.nodes.new("ShaderNodeMapRange")
    mr.inputs["From Min"].default_value = Z(1330)
    mr.inputs["From Max"].default_value = Z(1470)
    nt.links.new(sep.outputs["Z"], mr.inputs["Value"])
    nt.links.new(mr.outputs[0], ramp.inputs[0])
    nt.links.new(ramp.outputs[0], em.inputs["Color"])
    nt.links.new(em.outputs[0], out.inputs[0])
    return m


class Builder:
    """Collects bevelled boxes and prisms into one mesh, with a random value per block."""

    def __init__(self, name, mat, seed=1):
        self.name = name
        self.mat = mat
        self.bm = bmesh.new()
        self.rand_layer = self.bm.faces.layers.float.new("block_rand")
        self.rng = random.Random(seed)

    def _tag(self, faces):
        r = self.rng.random()
        for f in faces:
            f[self.rand_layer] = r

    def box(self, x0, x1, z0, z1, y0=-0.5, y1=0.5, jitter=0.0, tilt=0.0, seam=0.0, material_index=0):
        """A stone; seam > 0 also puts a dark slab that much larger right behind it (the outline)."""
        res = bmesh.ops.create_cube(self.bm, size=1.0)
        verts = res["verts"]
        sx, sy, sz = x1 - x0, y1 - y0, z1 - z0
        cx, cy, cz = (x0 + x1) / 2, (y0 + y1) / 2 + self.rng.uniform(-jitter, jitter), (z0 + z1) / 2
        m = Matrix.Translation((cx, cy, cz)) @ Matrix.Rotation(tilt, 4, "Y") @ Matrix.Diagonal((sx, sy, sz, 1))
        bmesh.ops.transform(self.bm, matrix=m, verts=verts)
        faces = {f for v in verts for f in v.link_faces}
        self._tag(faces)
        for f in faces:
            f.material_index = material_index
        if seam > 0:
            res = bmesh.ops.create_cube(self.bm, size=1.0)
            back_y0 = cy + sy * 0.25
            m = Matrix.Translation((cx, (back_y0 + y1 + 0.1) / 2, cz)) @ Matrix.Rotation(tilt, 4, "Y") @ Matrix.Diagonal((sx + 2 * seam, y1 + 0.1 - back_y0, sz + 2 * seam, 1))
            bmesh.ops.transform(self.bm, matrix=m, verts=res["verts"])
            for f in {f for v in res["verts"] for f in v.link_faces}:
                f.material_index = 1

    def prism(self, outline, y0, y1, material_index=0):
        """Extrudes a 2D (x, z) polygon between depths y0 and y1."""
        vs_front = [self.bm.verts.new((x, y0, z)) for x, z in outline]
        vs_back = [self.bm.verts.new((x, y1, z)) for x, z in outline]
        faces = [self.bm.faces.new(vs_front[::-1]), self.bm.faces.new(vs_back)]
        n = len(outline)
        for i in range(n):
            j = (i + 1) % n
            faces.append(self.bm.faces.new((vs_front[i], vs_front[j], vs_back[j], vs_back[i])))
        self._tag(faces)
        for f in faces:
            f.material_index = material_index

    def cylinder(self, cx, cz0, cz1, radius, cy=0.0, segments=24):
        res = bmesh.ops.create_cone(self.bm, cap_ends=True, segments=segments, radius1=radius, radius2=radius, depth=cz1 - cz0)
        bmesh.ops.translate(self.bm, verts=res["verts"], vec=(cx, cy, (cz0 + cz1) / 2))
        self._tag({f for v in res["verts"] for f in v.link_faces})

    def lean(self, pivot_x, pivot_z, degrees):
        """Tilts everything built so far about a point on the ground, like a settling gravestone."""
        m = Matrix.Translation((pivot_x, 0, pivot_z)) @ Matrix.Rotation(math.radians(degrees), 4, "Y") @ Matrix.Translation((-pivot_x, 0, -pivot_z))
        bmesh.ops.transform(self.bm, matrix=m, verts=list(self.bm.verts))

    def finish(self, bevel=PX * 0.6):   # a rounded rim wider than a pixel renders as a soft, smeared edge
        me = bpy.data.meshes.new(self.name)
        variants = self.mat if isinstance(self.mat, list) else [self.mat]
        # slots: the variants, the seam, then growth (moss, weeds); each block takes one variant
        for f in self.bm.faces:
            if f.material_index == 1:
                f.material_index = len(variants)
            elif f.material_index == 2:
                f.material_index = len(variants) + 1
            else:
                f.material_index = min(int(f[self.rand_layer] * 7919) % len(variants), len(variants) - 1)
        bmesh.ops.recalc_face_normals(self.bm, faces=self.bm.faces)
        self.bm.to_mesh(me)
        self.bm.free()
        ob = bpy.data.objects.new(self.name, me)
        bpy.context.scene.collection.objects.link(ob)
        for v in variants:
            me.materials.append(v)
        me.materials.append(seam_material())
        me.materials.append(growth_material())
        if bevel:
            mod = ob.modifiers.new("bevel", "BEVEL")
            mod.width = bevel
            mod.harden_normals = False
            mod.segments = 1
            mod.limit_method = "ANGLE"
        return ob


def growth_material():
    m = bpy.data.materials.get("growth")
    if m:
        return m
    return material("growth", (0.014, 0.065, 0.024), rough=0.85, tint_var=0.3)


def leaf_material():
    return material("leaves", (0.016, 0.060, 0.020), rough=0.8, tint_var=0.35)


def surface_front(scn, deps, x, z):
    """Depth of the first surface seen from the camera at (x, z), or None over empty space."""
    hit, loc, nrm, *_ = scn.ray_cast(deps, Vector((x, -20.0, z)), Vector((0, 1, 0)))
    if not hit or loc.y > 1.0:
        return None
    return loc.y


def surface_top(scn, deps, x, y=-0.1):
    hit, loc, *_ = scn.ray_cast(deps, Vector((x, y, 30.0)), Vector((0, 0, -1)))
    return loc.z if hit else None


def ivy(scn, seed, starts=None, x_range=None, moss_floor=None, name="ivy", vine_len=(18, 95), moss_chance=0.75):
    """Hanging ivy and moss clumps grown onto whatever geometry is already in the scene."""
    bpy.context.view_layer.update()
    deps = bpy.context.evaluated_depsgraph_get()
    rng = random.Random(seed + 100)
    b = Builder(name, leaf_material(), seed + 100)

    def leaf(x, z, y, size):
        res = bmesh.ops.create_icosphere(b.bm, subdivisions=1, radius=size)
        m = (Matrix.Translation((x, y, z)) @ Matrix.Rotation(rng.uniform(0, 6.28), 4, "Y")
             @ Matrix.Diagonal((1.0, 0.45, rng.uniform(0.6, 1.0), 1)))
        bmesh.ops.transform(b.bm, matrix=m, verts=res["verts"])
        b._tag({f for v in res["verts"] for f in v.link_faces})

    # hanging vines
    vine_xs = starts or []
    for x_px in vine_xs:
        x = X(x_px)
        z = surface_top(scn, deps, x)
        if z is None:
            continue
        length = rng.uniform(*vine_len) * PX
        z0 = z
        drift = rng.uniform(-0.4, 0.4) * PX
        while z > z0 - length:
            y = surface_front(scn, deps, x, z)
            if y is None:
                break
            for _ in range(rng.choice((1, 2, 2, 3))):
                leaf(x + rng.uniform(-2.5, 2.5) * PX, z + rng.uniform(-1.5, 1.5) * PX, y - 0.06, rng.uniform(1.6, 2.6) * PX)
            x += drift + rng.uniform(-1.2, 1.2) * PX
            z -= rng.uniform(2.5, 4.0) * PX

    # moss clumps along every broken top in the ruins and on the cornice
    x = X(x_range[0])
    floor = Z(1546) if moss_floor is None else moss_floor
    while x < X(x_range[1]):
        z = surface_top(scn, deps, x)
        if z is not None and z > floor and rng.random() < moss_chance:
            y = surface_front(scn, deps, x, z - 2 * PX)
            if y is not None:
                for _ in range(rng.randint(1, 3)):
                    leaf(x + rng.uniform(-2, 2) * PX, z + rng.uniform(-1, 2.5) * PX, y - 0.05, rng.uniform(1.4, 2.4) * PX)
        x += rng.uniform(1.5, 4.0) * PX
    return b.finish(bevel=0)


def _growth_lump(b, rng, x, z, y, rx, rz, depth=0.45):
    """One rounded lump of moss or ivy: a flattened icosphere facing the camera."""
    res = bmesh.ops.create_icosphere(b.bm, subdivisions=2, radius=1.0)
    m = (Matrix.Translation((x, y, z)) @ Matrix.Rotation(rng.uniform(-0.4, 0.4), 4, "Y")
         @ Matrix.Diagonal((rx, depth * min(rx, rz), rz, 1)))
    bmesh.ops.transform(b.bm, matrix=m, verts=res["verts"])
    b._tag({f for v in res["verts"] for f in v.link_faces})


def moss_cushions(name, ledges, seed, cover=0.75):
    """Moss the way it grows on old stone: cushions sitting on ledges, bulging over the top edge,
    hanging a little over the front and now and then dripping down it.

    ledges: (x0_px, x1_px, top_py, front_y). Gaps are left where cover runs out, so ledges are
    never wrapped end to end."""
    rng = random.Random(seed)
    b = Builder(name, leaf_material(), seed)
    for x0_px, x1_px, top_py, front_y in ledges:
        x = x0_px + rng.uniform(0, 6)
        while x < x1_px:
            run = rng.uniform(10, 34)
            if rng.random() < cover:
                end = min(x + run, x1_px)
                cx = x
                while cx < end:
                    r = rng.uniform(3.0, 5.5)
                    _growth_lump(b, rng, X(cx), Z(top_py) + rng.uniform(0.8, 2.2) * PX, front_y - rng.uniform(0.02, 0.10),
                                 r * PX, rng.uniform(2.0, 3.4) * PX)
                    if rng.random() < 0.18:
                        # a drip of moss down the face below the ledge
                        drop = rng.uniform(3, 9)
                        for k in range(int(drop / 2)):
                            _growth_lump(b, rng, X(cx + rng.uniform(-0.8, 0.8)), Z(top_py + 2 + 2 * k), front_y - 0.1,
                                         rng.uniform(1.0, 1.8) * PX, rng.uniform(1.0, 1.6) * PX)
                    cx += r * rng.uniform(0.9, 1.5)
            x += run + rng.uniform(4, 22)
    return b.finish(bevel=0)


# an ivy leaf in leaf space: stem at the origin, tip at (0, 1), three lobes
IVY_LEAF = [(0.0, 1.0), (0.32, 0.62), (0.78, 0.55), (0.58, 0.18), (0.36, -0.08), (0.0, 0.02),
            (-0.36, -0.08), (-0.58, 0.18), (-0.78, 0.55), (-0.32, 0.62)]


def _ivy_leaf(b, rng, x, z, y, size, angle):
    """One flat three-lobed leaf, turned in the wall plane by angle and tipped out of it at random,
    so neighbouring leaves catch the light differently and read as separate leaves."""
    tip = Matrix.Rotation(rng.uniform(0.1, 0.55), 4, "X") @ Matrix.Rotation(rng.uniform(-0.4, 0.4), 4, "Z")
    turn = Matrix.Rotation(angle, 4, "Y")
    verts = []
    for lx, lz in IVY_LEAF:
        v = turn @ tip @ Vector((lx * size, 0.0, lz * size))
        verts.append(b.bm.verts.new((x + v.x, y + v.y, z + v.z)))
    b._tag([b.bm.faces.new(verts)])


def ivy_curtain(scn, name, seed, x_px, top_py, width_px, length_px):
    """Ivy pouring down a wall from a ledge: strands of vine hanging from the top, leaves along
    them, big and dense near the top, smaller and sparser towards the tips."""
    bpy.context.view_layer.update()
    deps = bpy.context.evaluated_depsgraph_get()
    rng = random.Random(seed)
    leaves = Builder(name, leaf_material(), seed)
    vines = Builder(name + "_vines", material(name + "_stem", (0.020, 0.030, 0.014), rough=0.9, tint_var=0.0), seed + 1)
    strands = max(2, int(width_px / 3.5))
    for k in range(strands):
        x = x_px + (k / max(strands - 1, 1) - 0.5) * width_px + rng.uniform(-2, 2)
        length = length_px * rng.uniform(0.35, 1.0)
        z_px = top_py + rng.uniform(0, 4)
        sway = rng.uniform(-0.25, 0.25)
        travelled = 0.0
        while travelled < length:
            t = travelled / length
            y = surface_front(scn, deps, X(x), Z(z_px))
            if y is None:
                break
            step = rng.uniform(2.0, 3.0)
            vines.box(X(x) - 0.35 * PX, X(x) + 0.35 * PX, Z(z_px + step), Z(z_px), y - 0.03, y - 0.01)
            for _ in range(rng.choice((2, 3, 3)) if t < 0.6 else rng.choice((1, 2))):
                if rng.random() < 0.1 + 0.5 * t:
                    continue
                side = rng.choice((-1, 1))
                size = rng.uniform(2.6, 4.0) * (1.0 - 0.4 * t) * PX
                angle = math.pi + side * rng.uniform(0.3, 1.2)     # tips point down and out
                _ivy_leaf(leaves, rng, X(x + side * rng.uniform(0.5, 2.0)), Z(z_px + rng.uniform(0, step)),
                          y - rng.uniform(0.05, 0.14), size, angle)
            x += sway + rng.uniform(-0.6, 0.6)
            z_px += step
            travelled += step
    vines.finish(bevel=0)
    return leaves.finish(bevel=0)


def crack_line(b, rng, x, z, length_px, y, branch_chance=0.3, depth=0):
    """A crack as dstar draws one: a jagged line one pixel wide, wandering down a face in single
    pixel steps, now and then splitting off a shorter branch. Joint dark (seam material), so it
    survives the conversion as a drawn line."""
    dx_bias = rng.choice((-1, 1)) * rng.uniform(0.2, 0.6)
    for _ in range(int(length_px)):
        b.box(x - 0.5 * PX, x + 0.5 * PX, z - 0.5 * PX, z + 0.5 * PX, y - 0.012, y + 0.02, material_index=1)
        x += (dx_bias + rng.uniform(-0.9, 0.9)) * PX
        z -= rng.uniform(0.6, 1.0) * PX
        if depth < 2 and rng.random() < branch_chance / max(length_px / 4, 1):
            crack_line(b, rng, x, z, length_px * rng.uniform(0.3, 0.5), y, branch_chance * 0.5, depth + 1)


def oblique(scn, slope, y_ref):
    """Looks a little down onto the scene without moving the camera: everything rises by `slope`
    per unit of depth behind the plane y_ref, so ledges, sills and capitals show their tops the
    way side-scroller scenery is drawn. Geometry on y_ref keeps its exact place, so a piece fitted
    to level tiles stays fitted."""
    shear = Matrix(((1, 0, 0, 0), (0, 1, 0, 0), (0, slope, 1, -slope * y_ref), (0, 0, 0, 1)))
    for ob in scn.objects:
        if ob.type == "MESH":
            ob.data.transform(shear)
        elif ob.type == "LIGHT" and ob.data.type != "SUN":
            ob.location = shear @ ob.location


def ashlar(b, x0, x1, z0, z1, course=13 * PX, min_w=18, max_w=38, keep=None, depth=(-0.35, 0.35), jitter=PX * 0.8, seam=PX, tilt=0.0):
    """Running-bond stone courses with dark seams.

    keep(x_mid, z_mid, x_a, x_b, z_a, z_b) returns False to drop a block, True to keep it, or an
    (x_a, x_b) pair to keep it clipped to that span.
    """
    z = z0
    row = 0
    while z < z1 - 1e-6:
        h = min(course * b.rng.uniform(0.85, 1.15), z1 - z)
        x = x0 - (b.rng.uniform(0.3, 0.7) * max_w * PX if row % 2 else 0)
        while x < x1 - 1e-6:
            w = b.rng.uniform(min_w, max_w) * PX
            xa, xb = max(x, x0), min(x + w, x1)
            verdict = keep((xa + xb) / 2, z + h / 2, xa, xb, z, z + h) if keep else True
            if isinstance(verdict, tuple):
                xa, xb = verdict
                verdict = True
            if verdict and xb - xa > 3 * PX:
                b.box(xa + seam, xb, z + seam, z + h, depth[0], depth[1], jitter=jitter,
                      tilt=b.rng.uniform(-tilt, tilt), seam=seam)
            x += w
        z += h
        row += 1



# the stage light; an asset config may override any key ("stage" in the asset json)
STAGE = dict(
    key_dir=(0.8, 0.35, -1.0), key_color=(0.70, 0.90, 1.0), key_energy=4.0,      # moonlight from the upper left, slightly in front
    fill_dir=(-0.5, 1.0, -0.2), fill_color=(0.62, 0.48, 1.0), fill_energy=0.55,  # weak cool fill from the front right
    world=(0.016, 0.012, 0.032),
)


def setup_stage(scn, cam_px):
    """Key light, fill, ambient and the 1:1 orthographic camera over the given level-pixel rectangle."""
    add_stage_lights(scn, Matrix.Identity(3))
    # orthographic camera framed on level pixels, one render pixel per level pixel
    cam = bpy.data.cameras.new("sprite_cam")
    cam.type = "ORTHO"
    w_px = cam_px["x1"] - cam_px["x0"]
    h_px = cam_px["y1"] - cam_px["y0"]
    cam.ortho_scale = max(w_px, h_px) * PX
    cam.clip_end = 100
    cob = bpy.data.objects.new("sprite_cam", cam)
    cob.location = (X((cam_px["x0"] + cam_px["x1"]) / 2), -40.0, Z((cam_px["y0"] + cam_px["y1"]) / 2))
    cob.rotation_euler = (math.radians(90), 0, 0)
    scn.collection.objects.link(cob)
    scn.camera = cob
    configure_render(scn, w_px, h_px)


def add_stage_lights(scn, view_turn):
    """The STAGE's key light, fill and ambient. Their directions are given for the front camera
    (x right, y into the picture, z up); view_turn (3x3) turns them along with another camera, so
    the moon keeps lighting from the upper left of the picture."""
    st = STAGE
    for name, kind in (("key", "key"), ("fill", "fill")):
        light = bpy.data.lights.new(name, "SUN")
        light.energy = st[kind + "_energy"]
        light.color = st[kind + "_color"]
        if kind == "key":
            light.angle = math.radians(2.0)
        ob = bpy.data.objects.new(name, light)
        ob.rotation_euler = (view_turn @ Vector(st[kind + "_dir"])).normalized().to_track_quat("-Z", "Y").to_euler()
        scn.collection.objects.link(ob)
    world = bpy.data.worlds.new("ambient")
    world.use_nodes = True
    bg = next(n for n in world.node_tree.nodes if n.type == "BACKGROUND")
    bg.inputs[0].default_value = (*st["world"], 1)
    bg.inputs[1].default_value = 1.0
    scn.world = world


def configure_render(scn, w_px, h_px):
    """Cycles at w_px x h_px level pixels (times RENDER_SCALE), transparent film, a box filter
    one sample wide and no colour transform, so pixels keep hard edges and true values."""
    r = scn.render
    r.resolution_x = w_px
    r.resolution_y = h_px
    r.resolution_percentage = RENDER_SCALE * 100
    r.film_transparent = True
    r.engine = "CYCLES"
    scn.cycles.samples = 96
    scn.cycles.use_denoising = False
    scn.cycles.pixel_filter_type = "BOX"
    scn.cycles.filter_width = 0.01
    scn.view_settings.view_transform = "Standard"
    scn.view_settings.look = "None"
    r.dither_intensity = 0.0


def render(path):
    bpy.context.scene.render.filepath = path
    bpy.ops.render.render(write_still=True)


def shader_group(name, build):
    """A shader node group (made once) whose shader output is the socket build(tree) returns, so one
    pass's shader can go into every material (see _route_outputs) and keep each material's alpha."""
    group = bpy.data.node_groups.get(name)
    if group is not None:
        return group
    group = bpy.data.node_groups.new(name, "ShaderNodeTree")
    group.interface.new_socket("Shader", in_out="OUTPUT", socket_type="NodeSocketShader")
    out = group.nodes.new("NodeGroupOutput")
    group.links.new(build(group), out.inputs[0])
    return group


def group_node(nt, group):
    node = nt.nodes.new("ShaderNodeGroup")
    node.node_tree = group
    return node


def _normal_shader(nt):
    geo = nt.nodes.new("ShaderNodeNewGeometry")
    # single-sided leaves seen from behind report the far side's normal: turn it towards the camera
    sign = nt.nodes.new("ShaderNodeMath"); sign.operation = "MULTIPLY_ADD"
    sign.inputs[1].default_value = -2.0
    sign.inputs[2].default_value = 1.0
    nt.links.new(geo.outputs["Backfacing"], sign.inputs[0])
    facing = nt.nodes.new("ShaderNodeVectorMath"); facing.operation = "SCALE"
    nt.links.new(geo.outputs["Normal"], facing.inputs[0])
    nt.links.new(sign.outputs[0], facing.inputs["Scale"])
    sep = nt.nodes.new("ShaderNodeSeparateXYZ")
    nt.links.new(facing.outputs[0], sep.inputs[0])
    neg = nt.nodes.new("ShaderNodeMath"); neg.operation = "MULTIPLY"; neg.inputs[1].default_value = -1.0
    nt.links.new(sep.outputs["Y"], neg.inputs[0])
    comb = nt.nodes.new("ShaderNodeCombineXYZ")
    nt.links.new(sep.outputs["X"], comb.inputs[0])
    nt.links.new(sep.outputs["Z"], comb.inputs[1])
    nt.links.new(neg.outputs[0], comb.inputs[2])
    remap = nt.nodes.new("ShaderNodeVectorMath"); remap.operation = "MULTIPLY_ADD"
    remap.inputs[1].default_value = (0.5, 0.5, 0.5)
    remap.inputs[2].default_value = (0.5, 0.5, 0.5)
    nt.links.new(comb.outputs[0], remap.inputs[0])
    em = nt.nodes.new("ShaderNodeEmission")
    nt.links.new(remap.outputs[0], em.inputs["Color"])
    return em.outputs[0]


def render_normals(path):
    """Tangent-space normal map for the engine's lighting: R = x, G = up, B = towards the camera."""
    group = shader_group("normal_out", _normal_shader)
    restore = _route_outputs(lambda m, nt: group_node(nt, group))
    try:
        _render_unlit(path, 16)
    finally:
        _restore_outputs(restore)


def _id_shader(nt):
    attr = nt.nodes.new("ShaderNodeAttribute"); attr.attribute_name = "block_rand"
    info = nt.nodes.new("ShaderNodeObjectInfo")
    comb = nt.nodes.new("ShaderNodeCombineXYZ")
    for i, (ka, kb) in enumerate(((97.1, 13.3), (31.7, 7.1), (57.3, 3.7))):
        a = nt.nodes.new("ShaderNodeMath"); a.operation = "MULTIPLY"; a.inputs[1].default_value = ka
        nt.links.new(attr.outputs["Fac"], a.inputs[0])
        b = nt.nodes.new("ShaderNodeMath"); b.operation = "MULTIPLY"; b.inputs[1].default_value = kb
        nt.links.new(info.outputs["Random"], b.inputs[0])
        s = nt.nodes.new("ShaderNodeMath"); s.operation = "ADD"
        nt.links.new(a.outputs[0], s.inputs[0]); nt.links.new(b.outputs[0], s.inputs[1])
        f = nt.nodes.new("ShaderNodeMath"); f.operation = "FRACT"
        nt.links.new(s.outputs[0], f.inputs[0])
        # seams carry block_rand 0: keep them black by gating with the block value
        g = nt.nodes.new("ShaderNodeMath"); g.operation = "GREATER_THAN"; g.inputs[1].default_value = 0.0
        nt.links.new(attr.outputs["Fac"], g.inputs[0])
        mm = nt.nodes.new("ShaderNodeMath"); mm.operation = "MULTIPLY"
        nt.links.new(f.outputs[0], mm.inputs[0]); nt.links.new(g.outputs[0], mm.inputs[1])
        nt.links.new(mm.outputs[0], comb.inputs[i])
    em = nt.nodes.new("ShaderNodeEmission")
    nt.links.new(comb.outputs[0], em.inputs["Color"])
    return em.outputs[0]


def render_ids(path):
    """One flat colour per stone (block value x object), for the pixel stylizer. Untagged seam slabs come out black."""
    group = shader_group("id_out", _id_shader)
    restore = _route_outputs(lambda m, nt: group_node(nt, group))
    try:
        _render_unlit(path, 1)
    finally:
        _restore_outputs(restore)


# material classes for the paint stage, by material name (first match wins); the class is written
# into the red channel as class / 16, the green channel is 1 for parts that stand back (side rooms)
MATERIAL_CLASSES = (("seam", 1), ("glass", 2), ("door", 2), ("roof", 3), ("growth", 4), ("leaves", 4),
                    ("bush", 4), ("ivy", 4), ("iron", 5), ("trim", 6))
STONE_CLASS = 7
RECEDING = ("annex",)
LIGHT_SAMPLES = 32


def material_class(name):
    return next((c for key, c in MATERIAL_CLASSES if key in name), STONE_CLASS)


# light the geometry's true normals instead of its shading normals (blender_kit/model.py sets it):
# imported characters often carry custom normals bent for cel shading (an anime face's normals point
# up and away), which a light reads as a face turned from it
LIGHT_TRUE_NORMALS = False


def _white_matt_shader(nt):
    bsdf = nt.nodes.new("ShaderNodeBsdfPrincipled")
    if LIGHT_TRUE_NORMALS:
        geo = nt.nodes.new("ShaderNodeNewGeometry")
        nt.links.new(geo.outputs["True Normal"], bsdf.inputs["Normal"])
    bsdf.inputs["Base Color"].default_value = (1, 1, 1, 1)
    bsdf.inputs["Roughness"].default_value = 1.0
    for name in ("Specular IOR Level", "Specular"):
        if name in bsdf.inputs:
            bsdf.inputs[name].default_value = 0.0
    return bsdf.outputs[0]


def render_light(path, shadows=True):
    """How much moonlight reaches each pixel: every surface plain white and matt, lit by the key
    light alone (no fill, no ambient), so the pass holds the light's direction and its cast shadows
    without texture or colour. With shadows=False only the surfaces' turn towards the light counts
    (the paint stage blends the two to set how strong cast shadows are)."""
    scn = bpy.context.scene
    key = bpy.data.objects["key"].data
    old_shadow = key.use_shadow
    group = shader_group("light_true_out" if LIGHT_TRUE_NORMALS else "light_out", _white_matt_shader)
    old_view, old_samples = scn.view_settings.view_transform, scn.cycles.samples
    old_world = scn.world
    hidden = [ob for ob in scn.objects if (ob.type == "LIGHT") and (ob.name != "key") and not ob.hide_render]
    for ob in hidden:
        ob.hide_render = True
    dark = bpy.data.worlds.get("no_ambient") or bpy.data.worlds.new("no_ambient")
    dark.use_nodes = True
    next(n for n in dark.node_tree.nodes if n.type == "BACKGROUND").inputs[1].default_value = 0.0
    scn.world = dark
    restore = _route_outputs(lambda m, nt: group_node(nt, group))
    scn.view_settings.view_transform = "Raw"
    scn.cycles.samples = LIGHT_SAMPLES
    key.use_shadow = shadows
    try:
        render(path)
    finally:
        key.use_shadow = old_shadow
        _restore_outputs(restore)
        scn.world = old_world
        scn.view_settings.view_transform = old_view
        scn.cycles.samples = old_samples
        for ob in hidden:
            ob.hide_render = False


def render_material_classes(path):
    """Each material emits its class (see MATERIAL_CLASSES) as a flat colour; the materials'
    own outputs are put back afterwards."""
    def make(m, nt):
        em = nt.nodes.new("ShaderNodeEmission")
        em.inputs["Color"].default_value = (material_class(m.name) / 16.0, 1.0 if any(k in m.name for k in RECEDING) else 0.0, 0.0, 1.0)
        return em
    restore = _route_outputs(make)
    try:
        _render_unlit(path, 1)
    finally:
        _restore_outputs(restore)


ALBEDO_SAMPLES = 16
# the camera stands 40 units in front of the scene (setup_stage); the depth pass spans 30 to 50 units,
# written at 16 bits so a relief step of a pixel or two still shows
DEPTH_NEAR_PLANE = 30.0
DEPTH_RANGE = 20.0


def material_alpha(nt):
    """(socket or value, inverted) carrying a material's coverage, or None when it is opaque: the
    factor of a mix with a Transparent BSDF (imported toon and unlit materials; a mix driven by the
    Light Path node only hides the surface from other rays and is not coverage), else the
    Principled BSDF's Alpha."""
    for mix in (n for n in nt.nodes if n.type == "MIX_SHADER"):
        fac = next((l for l in nt.links if l.to_socket == mix.inputs[0]), None)
        if (fac is not None) and (fac.from_node.type == "LIGHT_PATH"):
            continue
        for index, inverted in ((1, False), (2, True)):
            if any((l.to_socket == mix.inputs[index]) and (l.from_node.type == "BSDF_TRANSPARENT") for l in nt.links):
                return (fac.from_socket if fac else mix.inputs[0].default_value), inverted
    bsdf = next((n for n in nt.nodes if n.type == "BSDF_PRINCIPLED"), None)
    if bsdf is None:
        return None
    link = next((l for l in nt.links if l.to_socket == bsdf.inputs["Alpha"]), None)
    if link is not None:
        return link.from_socket, False
    value = bsdf.inputs["Alpha"].default_value
    return (value, False) if value < 1.0 else None


COVERED = 0.5   # alpha from which a surface counts as there in a pass


def _with_alpha(nt, shader_socket, alpha, added):
    """shader_socket where the material covers, transparent elsewhere. Coverage is all or nothing
    (alpha at least COVERED): the passes carry data (ids, classes, normals), and half-transparent
    decals (eyes, lashes) would blend them into values that mean something else."""
    source, inverted = alpha
    mix = nt.nodes.new("ShaderNodeMixShader")
    clear = nt.nodes.new("ShaderNodeBsdfTransparent")
    added += [mix, clear]
    if isinstance(source, float):
        covered = ((1.0 - source) if inverted else source) >= COVERED
        mix.inputs[0].default_value = 1.0 if covered else 0.0
    else:
        cut = nt.nodes.new("ShaderNodeMath")
        cut.operation = "LESS_THAN" if inverted else "GREATER_THAN"
        # inverted: the factor shows the transparent side, so coverage is factor < 1 - COVERED
        cut.inputs[1].default_value = (1.0 - COVERED) if inverted else (COVERED - 1e-6)
        nt.links.new(source, cut.inputs[0])
        nt.links.new(cut.outputs[0], mix.inputs[0])
        added.append(cut)
    nt.links.new(clear.outputs[0], mix.inputs[1])
    nt.links.new(shader_socket, mix.inputs[2])
    return mix.outputs[0]


def _route_outputs(make_emission):
    """Every material's surface output replaced by the shader node make_emission(material, tree)
    builds, kept transparent where the material is (material_alpha); returns what _restore_outputs
    needs to put the originals back."""
    restore = []
    for m in bpy.data.materials:
        if not m.use_nodes:
            continue
        nt = m.node_tree
        out = next((n for n in nt.nodes if n.type == "OUTPUT_MATERIAL"), None)
        if out is None:
            continue
        old = [l.from_socket for l in nt.links if l.to_socket == out.inputs["Surface"]]
        em = make_emission(m, nt)
        added = [em]
        alpha = material_alpha(nt)
        shader = em.outputs[0] if alpha is None else _with_alpha(nt, em.outputs[0], alpha, added)
        nt.links.new(shader, out.inputs["Surface"])
        restore.append((nt, added, out, old))
    return restore


def _restore_outputs(restore):
    for nt, added, out, old in restore:
        for node in added:
            nt.nodes.remove(node)
        for sock in old:
            nt.links.new(sock, out.inputs["Surface"])


def _render_unlit(path, samples):
    """Renders the current emission-only setup with lights off, raw values."""
    scn = bpy.context.scene
    old_view, old_samples = scn.view_settings.view_transform, scn.cycles.samples
    hidden = [ob for ob in scn.objects if (ob.type == "LIGHT") and not ob.hide_render]
    for ob in hidden:
        ob.hide_render = True
    scn.view_settings.view_transform = "Raw"
    scn.cycles.samples = samples
    try:
        render(path)
    finally:
        scn.view_settings.view_transform = old_view
        scn.cycles.samples = old_samples
        for ob in hidden:
            ob.hide_render = False


def render_albedo(path):
    """Each material's own colour without light: whatever feeds its BSDF's Base Color (stone scans,
    tint per block, grime, moss) emitted as is; emissive materials keep their emission colour."""
    def make(m, nt):
        em = nt.nodes.new("ShaderNodeEmission")
        bsdf = next((n for n in nt.nodes if n.type == "BSDF_PRINCIPLED"), None)
        # unlit and toon materials (imported models) carry their colour on an emission
        source = bsdf.inputs["Base Color"] if bsdf is not None else next(
            (n.inputs["Color"] for n in nt.nodes if (n.type == "EMISSION") and (n is not em)), None)
        if source is None:
            return em
        link = next((l for l in nt.links if l.to_socket == source), None)
        if link is not None:
            nt.links.new(link.from_socket, em.inputs["Color"])
        else:
            em.inputs["Color"].default_value = source.default_value
        return em
    restore = _route_outputs(make)
    try:
        _render_unlit(path, ALBEDO_SAMPLES)
    finally:
        _restore_outputs(restore)


def render_depth(path):
    """Distance from the camera, near = white, from DEPTH_NEAR_PLANE over DEPTH_RANGE scene units, as a
    16-bit image, for tracing silhouettes and relief edges."""
    extra = []

    def make(m, nt):
        cam = nt.nodes.new("ShaderNodeCameraData")
        scale = nt.nodes.new("ShaderNodeMapRange")
        scale.inputs["From Min"].default_value = DEPTH_NEAR_PLANE
        scale.inputs["From Max"].default_value = DEPTH_NEAR_PLANE + DEPTH_RANGE
        scale.inputs["To Min"].default_value = 1.0
        scale.inputs["To Max"].default_value = 0.0
        nt.links.new(cam.outputs["View Z Depth"], scale.inputs["Value"])
        em = nt.nodes.new("ShaderNodeEmission")
        nt.links.new(scale.outputs[0], em.inputs["Color"])
        extra.append((nt, cam, scale))
        return em
    settings = bpy.context.scene.render.image_settings
    old_depth, old_mode = settings.color_depth, settings.color_mode
    restore = _route_outputs(make)
    # one 16-bit grey channel: readers keep 16 bits for grey, not for RGBA (where the ids pass
    # already tells geometry from empty space)
    settings.color_mode = "BW"
    settings.color_depth = "16"
    try:
        _render_unlit(path, 1)
    finally:
        settings.color_depth, settings.color_mode = old_depth, old_mode
        for nt, cam, scale in extra:
            nt.nodes.remove(cam)
            nt.nodes.remove(scale)
        _restore_outputs(restore)


AO_DISTANCE = 0.5        # scene units an occluder may be away and still darken (12 level px)
AO_SAMPLES = 48


def render_ao(path):
    """Ambient occlusion alone, white = open, dark = enclosed (corners, under ledges, in reveals),
    for the paint stage's contact shadows."""
    def make(m, nt):
        ao = nt.nodes.new("ShaderNodeAmbientOcclusion")
        ao.inputs["Distance"].default_value = AO_DISTANCE
        ao.samples = 16
        em = nt.nodes.new("ShaderNodeEmission")
        nt.links.new(ao.outputs["AO"], em.inputs["Color"])
        made.append((nt, ao))
        return em
    made = []
    restore = _route_outputs(make)
    try:
        _render_unlit(path, AO_SAMPLES)
    finally:
        for nt, ao in made:
            nt.nodes.remove(ao)
        _restore_outputs(restore)
