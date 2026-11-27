"""Kit module for pieces made from a 3D model file instead of a scene script (glTF, FBX, OBJ, .blend).

Executed into the same namespace as core.py (see run_passes.py). build_model imports the file,
scales it so it stands height_px level pixels tall in the picture, gives every loose part of every
material its own id (the paint stage treats each as one "stone": one tone, its own edges), and
frames it with an orthographic camera turned by yaw and tilted by pitch. The stage's lights turn
with the camera, so the light always comes from the picture's upper left, as the paint stage expects.
"""
import os

MODEL_IMPORTERS = {
    ".glb": lambda path: bpy.ops.import_scene.gltf(filepath=path),
    ".gltf": lambda path: bpy.ops.import_scene.gltf(filepath=path),
    ".fbx": lambda path: bpy.ops.import_scene.fbx(filepath=path),
    ".obj": lambda path: bpy.ops.wm.obj_import(filepath=path),
}
# the model's centre sits in the middle of the depth pass's range (core.DEPTH_NEAR_PLANE / DEPTH_RANGE)
VIEW_DISTANCE = DEPTH_NEAR_PLANE + DEPTH_RANGE / 2
PART_SPREAD = 0.6180339887   # golden ratio step: neighbouring part numbers get far-apart id colours
MIN_PART_VALUE = 0.05        # ids above zero: block_rand 0 means joint (core.render_ids)
PLAIN_COLOUR = (0.6, 0.6, 0.6, 1.0)
COVERAGE_LONG_SIDE = 256     # pixels across the quick render that finds what is visible


def import_model(path):
    """Loads the model file into the empty scene."""
    ext = os.path.splitext(path)[1].lower()
    if ext == ".blend":
        with bpy.data.libraries.load(path) as (source, target):
            target.objects = list(source.objects)
        for ob in target.objects:
            if (ob is not None) and (ob.type not in ("CAMERA", "LIGHT")):
                bpy.context.scene.collection.objects.link(ob)
        return
    if ext not in MODEL_IMPORTERS:
        raise ValueError(f"cannot import {ext} files; use one of {sorted(MODEL_IMPORTERS) + ['.blend']}")
    MODEL_IMPORTERS[ext](path)
    for ob in [o for o in bpy.context.scene.objects if o.type in ("CAMERA", "LIGHT")]:
        bpy.data.objects.remove(ob)


def give_plain_material(meshes):
    """Meshes or slots without a material get a plain light grey one: the passes route each pass
    through the materials, so a surface without one would render as Blender's default instead."""
    plain = None
    for ob in meshes:
        if all(slot.material is not None for slot in ob.material_slots) and ob.material_slots:
            continue
        if plain is None:
            plain = bpy.data.materials.new("plain")
            plain.use_nodes = True
            next(n for n in plain.node_tree.nodes if n.type == "BSDF_PRINCIPLED").inputs["Base Color"].default_value = PLAIN_COLOUR
        if not ob.material_slots:
            ob.data.materials.append(plain)
        for slot in ob.material_slots:
            if slot.material is None:
                slot.material = plain


def loose_parts(mesh):
    """Part number per vertex: vertices joined by edges share one (label propagation with pointer
    jumping, so it runs in numpy without a per-vertex Python loop)."""
    import numpy as np
    n = len(mesh.vertices)
    edges = np.zeros(len(mesh.edges) * 2, dtype=np.int64)
    mesh.edges.foreach_get("vertices", edges)
    a, b = edges[0::2], edges[1::2]
    label = np.arange(n)
    while True:
        low = np.minimum(label[a], label[b])
        before = label.copy()
        np.minimum.at(label, a, low)
        np.minimum.at(label, b, low)
        label = label[label]
        if np.array_equal(label, before):
            return label


def tag_parts(mesh):
    """block_rand per face: one value per loose part and material, never 0 (0 is a joint)."""
    import numpy as np
    if "block_rand" in mesh.attributes:
        return
    part = loose_parts(mesh)
    first = np.zeros(len(mesh.polygons), dtype=np.int64)
    mesh.polygons.foreach_get("loop_start", first)
    loop_vertex = np.zeros(len(mesh.loops), dtype=np.int64)
    mesh.loops.foreach_get("vertex_index", loop_vertex)
    material = np.zeros(len(mesh.polygons), dtype=np.int64)
    mesh.polygons.foreach_get("material_index", material)
    _, number = np.unique(np.stack([part[loop_vertex[first]], material], axis=1), axis=0, return_inverse=True)
    value = MIN_PART_VALUE + (1.0 - MIN_PART_VALUE) * np.mod((number.ravel() + 1) * PART_SPREAD, 1.0)
    mesh.attributes.new("block_rand", "FLOAT", "FACE").data.foreach_set("value", value.astype(np.float32))


def view_turn(yaw_deg, pitch_deg):
    """Rotation from the front camera's frame (x right, y into the picture, z up) to the turned view:
    yaw turns around the model (positive = see more of its right side), pitch looks down on it."""
    return (Matrix.Rotation(math.radians(-yaw_deg), 3, "Z") @ Matrix.Rotation(math.radians(-pitch_deg), 3, "X"))


def picture_bounds(objects, turn):
    """(min, max) of the evaluated meshes' vertices in the view frame (x right, y depth, z up)."""
    deps = bpy.context.evaluated_depsgraph_get()
    back = turn.transposed()
    lo, hi = Vector((1e9, 1e9, 1e9)), Vector((-1e9, -1e9, -1e9))
    for ob in objects:
        ev = ob.evaluated_get(deps)
        mesh = ev.to_mesh()
        for v in mesh.vertices:
            p = back @ (ev.matrix_world @ v.co)
            lo = Vector(map(min, lo, p))
            hi = Vector(map(max, hi, p))
        ev.to_mesh_clear()
    return lo, hi


def place_camera(scn, turn, lo, hi, margin_px):
    """Camera and picture size for the view-frame box lo..hi (x across, z up) with margin_px around it."""
    w_px = int(math.ceil((hi.x - lo.x) / PX)) + 2 * margin_px
    h_px = int(math.ceil((hi.z - lo.z) / PX)) + 2 * margin_px
    scn.camera.data.ortho_scale = max(w_px, h_px) * PX
    scn.camera.location = turn @ ((lo + hi) / 2) - (turn @ Vector((0.0, 1.0, 0.0))) * VIEW_DISTANCE
    configure_render(scn, w_px, h_px)


def visible_box(scn, lo, hi):
    """lo..hi narrowed across and up to what actually renders (helper meshes and fully transparent
    parts are left out), from a quick render of the ids pass with the camera on lo..hi."""
    import numpy as np
    r = scn.render
    across = scn.camera.data.ortho_scale
    long_side = max(r.resolution_x, r.resolution_y)
    r.resolution_x = max(1, round(COVERAGE_LONG_SIDE * r.resolution_x / long_side))
    r.resolution_y = max(1, round(COVERAGE_LONG_SIDE * r.resolution_y / long_side))
    r.resolution_percentage = 100
    path = os.path.join(bpy.app.tempdir, "coverage.png")
    render_ids(path)
    image = bpy.data.images.load(path)
    w, h = image.size
    covered = np.array(image.pixels[:]).reshape(h, w, 4)[..., 3] > 0.5   # rows from the bottom
    bpy.data.images.remove(image)
    if not covered.any():
        raise ValueError("nothing of the model is visible from this view")
    unit = across / max(w, h)
    left = (lo.x + hi.x) / 2 - w * unit / 2
    bottom = (lo.z + hi.z) / 2 - h * unit / 2
    cols, rows = np.nonzero(covered.any(0))[0], np.nonzero(covered.any(1))[0]
    return (Vector((left + cols[0] * unit, lo.y, bottom + rows[0] * unit)),
            Vector((left + (cols[-1] + 1) * unit, hi.y, bottom + (rows[-1] + 1) * unit)))


def build_model(path, height_px=64, yaw=0.0, pitch=0.0, margin_px=2, frame=None):
    """Imports the model and sets up the stage for it: the visible model height_px pixels tall, the
    picture as wide as it needs."""
    scn = bpy.context.scene
    clear_scene()
    import_model(path)
    if frame is not None:
        scn.frame_set(int(frame))
    meshes = [ob for ob in scn.objects if ob.type == "MESH"]
    if not meshes:
        raise ValueError(f"{path} holds no meshes")
    give_plain_material(meshes)
    for mesh in {ob.data for ob in meshes}:
        tag_parts(mesh)
    holder = bpy.data.objects.new("model_scale", None)
    scn.collection.objects.link(holder)
    for ob in scn.objects:
        if (ob.parent is None) and (ob is not holder):
            ob.parent = holder
    turn = view_turn(yaw, pitch)
    add_stage_lights(scn, turn)
    cam = bpy.data.cameras.new("sprite_cam")
    cam.type = "ORTHO"
    cam.clip_end = 100
    scn.camera = bpy.data.objects.new("sprite_cam", cam)
    scn.camera.rotation_euler = (turn @ Matrix.Rotation(math.radians(90), 3, "X")).to_euler()
    scn.collection.objects.link(scn.camera)

    lo, hi = picture_bounds(meshes, turn)
    place_camera(scn, turn, lo, hi, 0)
    lo, hi = visible_box(scn, lo, hi)
    # the holder sits at the origin and the view only turns, so scaling it scales the box alike
    scale = height_px * PX / max(hi.z - lo.z, 1e-6)
    holder.scale = (scale, scale, scale)
    bpy.context.view_layer.update()
    lo, hi = lo * scale, hi * scale
    if (hi.y - lo.y) > DEPTH_RANGE:
        print("pixelart: the model is deeper than the depth pass covers; relief lines may be missing")
    place_camera(scn, turn, lo, hi, margin_px)
