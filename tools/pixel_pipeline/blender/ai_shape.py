"""Gives the AI model a natural body: free arms, straight legs and a smooth surface, then re-rigs it.

Run inside the live Blender: exec(open(path).read()); reshape()

Hunyuan3D read his 3/4 sprite as a front view, so the hips came out turned (the near hip forward) while the
feet stand nearly square: the legs lean and twist, which reads as bent knees in every pose. Each leg is cut
into height slices and every slice is moved so the leg runs straight down from its hip; the boots follow their
ankle. The body below the head gets a volume preserving smooth against the AI's lumps. The leg bones are then
put on the straight legs and the mesh is weighted again with bone heat (at 10x scale, where it finds a solution).
"""

import bpy
import mathutils
import numpy

MODEL_NAME = "ai_adam"
RIG_NAME = "ai_rig"
# legs: from the hem down; slices above this blend from no move (at LEG_TOP) to the full move (at LEG_FULL)
LEG_TOP = -0.50
LEG_FULL = -0.58
SLICE = 0.02
# the head (and its hair) is left exactly as it is
HEAD_START_Z = 0.22
SMOOTH_ROUNDS = 6
SMOOTH_FACTOR = 0.5
# below this height an arm touching the body is cut free (above it the arm joins the shoulder)
ARMPIT_Z = -0.06


def dominant_bones(model):
    names = {group.index: group.name for group in model.vertex_groups}
    result = []
    for vertex in model.data.vertices:
        best = max(vertex.groups, key=lambda element: element.weight, default=None)
        result.append(names[best.group] if best else "")
    return numpy.array(result)


def straighten_legs(model, coordinates, dominant):
    """Moves each leg's slices onto the vertical through the leg's top slice; returns each leg's new center."""
    centers = {}
    for side in ("l", "r"):
        leg = numpy.isin(dominant, [f"thigh_{side}", f"calf_{side}", f"foot_{side}", f"ball_{side}"]) & (coordinates[:, 2] < LEG_TOP)
        top = leg & (numpy.abs(coordinates[:, 2] - LEG_FULL) < SLICE)
        target = coordinates[top][:, :2].mean(axis=0)
        heights = numpy.arange(LEG_FULL, coordinates[leg][:, 2].min() - SLICE, -SLICE)
        offsets = []
        for height in heights:
            band = leg & (numpy.abs(coordinates[:, 2] - height) < SLICE) & ~numpy.isin(dominant, [f"foot_{side}", f"ball_{side}"])
            offsets.append(target - coordinates[band][:, :2].mean(axis=0) if band.sum() > 20 else None)
        # the boots have no thigh or calf vertices: they take the last slice's (the ankle's) move
        known = [index for index, offset in enumerate(offsets) if offset is not None]
        for index in range(len(offsets)):
            if offsets[index] is None:
                nearest = min(known, key=lambda other: abs(other - index))
                offsets[index] = offsets[nearest]
        offsets = numpy.array(offsets)
        for axis in range(2):
            offsets[:, axis] = numpy.convolve(numpy.pad(offsets[:, axis], 2, mode="edge"), numpy.ones(5) / 5, mode="valid")
        for index in numpy.nonzero(leg)[0]:
            height = coordinates[index, 2]
            if height > LEG_FULL:
                blend = (LEG_TOP - height) / (LEG_TOP - LEG_FULL)
                offset = offsets[0] * blend
            else:
                position = min(len(heights) - 1, int(round((LEG_FULL - height) / SLICE)))
                offset = offsets[position]
            coordinates[index, :2] += offset
        centers[side] = target
    return centers


def smooth_body(model):
    """Taubin smoothing (a shrinking and an inflating step per round, so the volume stays) of everything below
    the head, in numpy: bmesh crashed reading this mesh."""
    mesh = model.data
    coordinates = numpy.zeros(len(mesh.vertices) * 3)
    mesh.vertices.foreach_get("co", coordinates)
    coordinates = coordinates.reshape(-1, 3)
    edges = numpy.zeros(len(mesh.edges) * 2, dtype=numpy.int64)
    mesh.edges.foreach_get("vertices", edges)
    edges = edges.reshape(-1, 2)
    movable = coordinates[:, 2] < HEAD_START_Z
    counts = numpy.zeros(len(coordinates))
    numpy.add.at(counts, edges[:, 0], 1)
    numpy.add.at(counts, edges[:, 1], 1)
    for _ in range(SMOOTH_ROUNDS):
        for factor in (SMOOTH_FACTOR, -SMOOTH_FACTOR - 0.03):
            sums = numpy.zeros_like(coordinates)
            numpy.add.at(sums, edges[:, 0], coordinates[edges[:, 1]])
            numpy.add.at(sums, edges[:, 1], coordinates[edges[:, 0]])
            change = sums / numpy.maximum(counts, 1)[:, None] - coordinates
            coordinates[movable] += factor * change[movable]
    mesh.vertices.foreach_set("co", coordinates.ravel())
    mesh.update()


def separate_arms(model):
    """Cuts each arm free from the body below the armpit: the AI fused the hanging arms to the jacket where
    they touched (the inner arm, the hand on the hip), so a swinging arm pulled a web of jacket along. Faces
    joining arm vertices (mostly moved by the arm bones) to body vertices below ARMPIT_Z are deleted and both
    openings are closed."""
    mesh = model.data
    names = {group.index: group.name for group in model.vertex_groups}
    arm_side = []
    for vertex in mesh.vertices:
        shares = {"l": 0.0, "r": 0.0}
        total = 0.0
        for element in vertex.groups:
            name = names[element.group]
            total += element.weight
            if name.startswith(("upperarm", "lowerarm", "hand")):
                shares[name[-1]] += element.weight
        side = max(shares, key=shares.get)
        arm_side.append(side if total and shares[side] / total > 0.5 else "")
    for polygon in mesh.polygons:
        labels = {arm_side[index] for index in polygon.vertices}
        polygon.select = len(labels) > 1 and polygon.center.z < ARMPIT_Z
    selected = sum(1 for polygon in mesh.polygons if polygon.select)
    window = bpy.context.window_manager.windows[0]
    area = next(item for item in window.screen.areas if item.type == "VIEW_3D")
    for obj in bpy.context.view_layer.objects:
        obj.select_set(False)
    model.hide_set(False)
    model.select_set(True)
    bpy.context.view_layer.objects.active = model
    with bpy.context.temp_override(window=window, area=area, active_object=model, object=model):
        bpy.ops.object.mode_set(mode="EDIT")
        bpy.ops.mesh.select_mode(type="FACE")
        bpy.ops.mesh.delete(type="FACE")
        bpy.ops.mesh.select_all(action="SELECT")
        bpy.ops.mesh.fill_holes(sides=0)
        bpy.ops.mesh.select_all(action="DESELECT")
        bpy.ops.object.mode_set(mode="OBJECT")
    return selected


def place_leg_bones(rig, centers, coordinates, dominant):
    window = bpy.context.window_manager.windows[0]
    area = next(item for item in window.screen.areas if item.type == "VIEW_3D")
    for obj in bpy.context.view_layer.objects:
        obj.select_set(False)
    rig.hide_set(False)
    rig.select_set(True)
    bpy.context.view_layer.objects.active = rig
    with bpy.context.temp_override(window=window, area=area, active_object=rig, object=rig):
        bpy.ops.object.mode_set(mode="EDIT")
        bones = rig.data.edit_bones
        for side in ("l", "r"):
            x, y = centers[side]
            thigh, calf, foot, ball = (bones[f"{name}_{side}"] for name in ("thigh", "calf", "foot", "ball"))
            hip_z, knee_z, ankle_z = thigh.head.z, calf.head.z, foot.head.z
            thigh.head = (x, y, hip_z)
            thigh.tail = calf.head = (x, y, knee_z)
            calf.tail = foot.head = (x, y, ankle_z)
            # the toes point where the boot reaches furthest forward
            boot = coordinates[numpy.isin(dominant, [f"foot_{side}", f"ball_{side}"])]
            toe_y = boot[:, 1].min() if len(boot) else y - 0.15
            foot.tail = ball.head = (x, (y + toe_y) / 2, -0.97)
            ball.tail = (x, toe_y, -0.98)
        bpy.ops.object.mode_set(mode="OBJECT")


def heat_weights(model, rig):
    """Bone heat on 10x copies (at the model's own scale it finds no solution), weights copied back."""
    for pose_bone in rig.pose.bones:
        pose_bone.matrix_basis = mathutils.Matrix.Identity(4)
    mesh_copy = model.copy()
    mesh_copy.data = model.data.copy()
    mesh_copy.name = "heat_mesh"
    rig_copy = rig.copy()
    rig_copy.data = rig.data.copy()
    rig_copy.name = "heat_rig"
    rig_copy.animation_data_clear()
    for obj in (mesh_copy, rig_copy):
        bpy.context.scene.collection.objects.link(obj)
    mesh_copy.parent = None
    mesh_copy.modifiers.clear()
    mesh_copy.vertex_groups.clear()
    mesh_copy.matrix_world = mathutils.Matrix.Scale(10, 4)
    rig_copy.matrix_world = mathutils.Matrix.Scale(10, 4)
    window = bpy.context.window_manager.windows[0]
    area = next(item for item in window.screen.areas if item.type == "VIEW_3D")
    for obj in bpy.context.view_layer.objects:
        obj.select_set(False)
    for obj in (mesh_copy, rig_copy):
        with bpy.context.temp_override(window=window, area=area, active_object=obj, selected_objects=[obj], selected_editable_objects=[obj]):
            bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    mesh_copy.select_set(True)
    rig_copy.select_set(True)
    bpy.context.view_layer.objects.active = rig_copy
    with bpy.context.temp_override(window=window, area=area, active_object=rig_copy, selected_objects=[mesh_copy, rig_copy], selected_editable_objects=[mesh_copy, rig_copy]):
        bpy.ops.object.parent_set(type="ARMATURE_AUTO")
    model.vertex_groups.clear()
    groups = {group.name: model.vertex_groups.new(name=group.name) for group in mesh_copy.vertex_groups}
    index_to_name = {group.index: group.name for group in mesh_copy.vertex_groups}
    buckets = {}
    for vertex in mesh_copy.data.vertices:
        for element in vertex.groups:
            if element.weight > 0.001:
                buckets.setdefault((index_to_name[element.group], round(element.weight, 3)), []).append(vertex.index)
    for (name, weight), indices in buckets.items():
        groups[name].add(indices, weight, "REPLACE")
    unweighted = sum(1 for vertex in mesh_copy.data.vertices if not vertex.groups)
    for obj in (mesh_copy, rig_copy):
        bpy.data.objects.remove(obj, do_unlink=True)
    return unweighted


def reshape():
    model = bpy.data.objects[MODEL_NAME]
    rig = bpy.data.objects[RIG_NAME]
    rig.animation_data.action = None
    for pose_bone in rig.pose.bones:
        pose_bone.matrix_basis = mathutils.Matrix.Identity(4)
    cut = separate_arms(model)
    mesh = model.data
    coordinates = numpy.zeros(len(mesh.vertices) * 3, dtype=numpy.float64)
    mesh.vertices.foreach_get("co", coordinates)
    coordinates = coordinates.reshape(-1, 3)
    dominant = dominant_bones(model)
    centers = straighten_legs(model, coordinates, dominant)
    mesh.vertices.foreach_set("co", coordinates.ravel())
    mesh.update()
    smooth_body(model)
    mesh.vertices.foreach_get("co", coordinates.ravel())
    place_leg_bones(rig, centers, coordinates, dominant)
    unweighted = heat_weights(model, rig)
    return cut, {side: [round(float(value), 3) for value in center] for side, center in centers.items()}, unweighted
