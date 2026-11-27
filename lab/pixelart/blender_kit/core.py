"""Blender side of the pixel-art kit: coordinate frame, materials, stone builder, stage and render passes.

Runs inside Blender. Coordinates: 1 Blender unit = 24 px (one tile). X(px) and Z(py) map level pixels
onto the scene; set ORIGIN_X_PX / GROUND_PY (run_passes.py does that from the asset config) before
building. The camera looks along +Y, so Y is depth (negative = towards the viewer).
"""
import bpy, bmesh, math, random
from mathutils import Vector, Matrix

PX = 1.0 / 24.0
# render pixels per level pixel (run_passes sets it from the piece's render_scale); above 1 the
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


SCAN_DIR = ""   # set by run_passes.py: the textures folder next to the kit, or the asset's "scan_dir"
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
    CAM = cam_px
    st = STAGE
    sun = bpy.data.lights.new("key", "SUN")
    sun.energy = st["key_energy"]
    sun.color = st["key_color"]
    sun.angle = math.radians(2.0)
    sob = bpy.data.objects.new("key", sun)
    sob.rotation_euler = Vector(st["key_dir"]).normalized().to_track_quat("-Z", "Y").to_euler()
    scn.collection.objects.link(sob)
    fill = bpy.data.lights.new("fill", "SUN")
    fill.energy = st["fill_energy"]
    fill.color = st["fill_color"]
    fob = bpy.data.objects.new("fill", fill)
    fob.rotation_euler = Vector(st["fill_dir"]).normalized().to_track_quat("-Z", "Y").to_euler()
    scn.collection.objects.link(fob)

    world = bpy.data.worlds.new("ambient")
    world.use_nodes = True
    bg = next(n for n in world.node_tree.nodes if n.type == "BACKGROUND")
    bg.inputs[0].default_value = (*st["world"], 1)
    bg.inputs[1].default_value = 1.0
    scn.world = world

    # orthographic camera framed on level pixels, one render pixel per level pixel
    cam = bpy.data.cameras.new("sprite_cam")
    cam.type = "ORTHO"
    w_px = CAM["x1"] - CAM["x0"]
    h_px = CAM["y1"] - CAM["y0"]
    cam.ortho_scale = max(w_px, h_px) * PX
    cam.clip_end = 100
    cob = bpy.data.objects.new("sprite_cam", cam)
    cob.location = (X((CAM["x0"] + CAM["x1"]) / 2), -40.0, Z((CAM["y0"] + CAM["y1"]) / 2))
    cob.rotation_euler = (math.radians(90), 0, 0)
    scn.collection.objects.link(cob)
    scn.camera = cob

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


def render_normals(path):
    """Tangent-space normal map for the engine's lighting: R = x, G = up, B = towards the camera."""
    scn = bpy.context.scene
    m = bpy.data.materials.get("normal_out") or bpy.data.materials.new("normal_out")
    m.use_nodes = True
    nt = m.node_tree
    for n in list(nt.nodes):
        nt.nodes.remove(n)
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
    out = nt.nodes.new("ShaderNodeOutputMaterial")
    nt.links.new(em.outputs[0], out.inputs[0])

    layer = scn.view_layers[0]
    old_view = scn.view_settings.view_transform
    old_samples = scn.cycles.samples
    hidden = []
    for ob in scn.objects:
        if ob.type == "LIGHT" and not ob.hide_render:
            ob.hide_render = True
            hidden.append(ob)
    layer.material_override = m
    scn.view_settings.view_transform = "Raw"
    scn.cycles.samples = 16
    try:
        render(path)
    finally:
        layer.material_override = None
        scn.view_settings.view_transform = old_view
        scn.cycles.samples = old_samples
        for ob in hidden:
            ob.hide_render = False


def render_ids(path):
    """One flat colour per stone (block value x object), for the pixel stylizer. Untagged seam slabs come out black."""
    scn = bpy.context.scene
    m = bpy.data.materials.get("id_out") or bpy.data.materials.new("id_out")
    m.use_nodes = True
    nt = m.node_tree
    for n in list(nt.nodes):
        nt.nodes.remove(n)
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
    out = nt.nodes.new("ShaderNodeOutputMaterial")
    nt.links.new(em.outputs[0], out.inputs[0])
    layer = scn.view_layers[0]
    old_view, old_samples = scn.view_settings.view_transform, scn.cycles.samples
    hidden = [ob for ob in scn.objects if ob.type == "LIGHT" and not ob.hide_render]
    for ob in hidden:
        ob.hide_render = True
    layer.material_override = m
    scn.view_settings.view_transform = "Raw"
    scn.cycles.samples = 1
    try:
        render(path)
    finally:
        layer.material_override = None
        scn.view_settings.view_transform = old_view
        scn.cycles.samples = old_samples
        for ob in hidden:
            ob.hide_render = False
