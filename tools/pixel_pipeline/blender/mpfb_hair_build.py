"""Builds Adam's spiky hair as one clean mesh: a cap following the skull plus swept, tapered spikes.

Run inside the live Blender (rig in rest or any pose; everything is built in rest space):
exec(open(path).read()); build_hair()

The library haircuts are flat cards that look choppy up close. Stylised game hair is a smooth cap with spikes:
the cap is the scalp of the shaped body (MakeHuman targets applied) pushed outward; each spike is a curved,
tapered tube from a point on the cap along its direction (his sprite: big spikes swept up and back over the
crown, spikes to the sides and the back, the quiff standing up over the forehead). All parts are fused with a
voxel remesh and smoothed into one surface, weighted fully to the head bone.
"""

import math

import bpy
import bmesh
import mathutils
import numpy

BODY_NAME = "adam_body"
RIG_NAME = "adam_body.rig"
HAIR_NAME = "adam_hair"
# the cap sits this far outside the scalp (rest units)
CAP_OFFSET = 0.018
VOXEL_SIZE = 0.005
# the hairline above the eyes at the front, and how fast it drops toward the back (per rest unit of depth)
HAIRLINE_FRONT = 0.045
HAIRLINE_TILT = 0.75
SMOOTH_ITERATIONS = 6
# spikes: (azimuth, elevation) of the root on the head (degrees; azimuth 0 = the face's front, 90 = his left,
# 180 = back), then the spike's direction as (back, up, side) weights, its length and root radius (rest units)
SPIKES = [
    # the quiff over the forehead, standing up and a little forward
    ((0.0, 50.0), (-0.2, 1.0, 0.0), 0.11, 0.05),
    ((30.0, 45.0), (0.2, 1.0, 0.3), 0.09, 0.045),
    ((-30.0, 45.0), (0.2, 1.0, -0.3), 0.09, 0.045),
    # the big mass swept up and back over the crown
    ((0.0, 75.0), (1.0, 0.8, 0.0), 0.14, 0.055),
    ((50.0, 65.0), (1.0, 0.6, 0.35), 0.13, 0.052),
    ((-50.0, 65.0), (1.0, 0.6, -0.35), 0.13, 0.052),
    ((180.0, 65.0), (1.0, 0.45, 0.0), 0.14, 0.055),
    ((130.0, 45.0), (1.0, 0.25, 0.45), 0.12, 0.05),
    ((-130.0, 45.0), (1.0, 0.25, -0.45), 0.12, 0.05),
    # the sides and the back, lower
    ((95.0, 30.0), (0.9, 0.2, 0.6), 0.09, 0.045),
    ((-95.0, 30.0), (0.9, 0.2, -0.6), 0.09, 0.045),
    ((165.0, 25.0), (1.0, -0.15, 0.2), 0.11, 0.05),
    ((-165.0, 25.0), (1.0, -0.15, -0.2), 0.11, 0.05),
]
SPIKE_SEGMENTS = 10
SPIKE_SIDES = 10
# how much a spike curls (its tip turns further up/back than its root, as a fraction of its length)
SPIKE_CURL = 0.25


def shaped_body_coordinates(body):
    mesh = body.data
    coordinates = numpy.zeros(len(mesh.vertices) * 3)
    mesh.vertices.foreach_get("co", coordinates)
    coordinates = coordinates.reshape(-1, 3)
    if mesh.shape_keys:
        basis = coordinates.copy()
        for block in mesh.shape_keys.key_blocks[1:]:
            if block.value == 0.0 or block.mute:
                continue
            shaped = numpy.zeros(len(mesh.vertices) * 3)
            block.data.foreach_get("co", shaped)
            coordinates += block.value * (shaped.reshape(-1, 3) - basis)
    return coordinates


def body_surface_mask(body):
    """Vertices of the visible body (vertex group "body"); MPFB's base mesh also holds hidden helper geometry
    (joint cubes, teeth, eyelash and hair helpers) inside and around the head."""
    group = body.vertex_groups.get("body")
    mask = numpy.zeros(len(body.data.vertices), bool)
    for vertex in body.data.vertices:
        mask[vertex.index] = any(element.group == group.index and element.weight > 0.5 for element in vertex.groups)
    return mask


def head_frame():
    """Head center, radius and the eye height from the eyes asset (rest pose)."""
    eyes = next(item for item in bpy.data.objects if "poly" in item.name.lower() and item.type == "MESH")
    eye_points = numpy.array([vertex.co for vertex in eyes.data.vertices])
    body = bpy.data.objects[BODY_NAME]
    coordinates = shaped_body_coordinates(body)
    eye_height = eye_points[:, 2].mean()
    surface = body_surface_mask(body)
    head = coordinates[surface & (coordinates[:, 2] > eye_height - 0.12)]
    center = numpy.array([0.0, (head[:, 1].min() + head[:, 1].max()) / 2, (head[:, 2].min() + head[:, 2].max()) / 2 + 0.01])
    radius = float(head[:, 2].max() - center[2])
    return center, radius, eye_height, coordinates, surface


def cap_mesh(coordinates, center, eye_height, surface):
    """The scalp faces of the shaped body (above the brow, or behind the ears above the nape), pushed outward."""
    body = bpy.data.objects[BODY_NAME]
    mesh = body.data
    work = bmesh.new()
    vertex_map = {}
    for polygon in mesh.polygons:
        if not surface[list(polygon.vertices)].all():
            continue
        points = coordinates[list(polygon.vertices)]
        middle = points.mean(axis=0)
        relative = middle - center
        # a straight hairline tilted down toward the back: high over the forehead, low at the nape, clear of the ears
        hairline = eye_height + HAIRLINE_FRONT - HAIRLINE_TILT * (relative[1] + 0.1)
        if middle[2] < hairline or middle[2] < eye_height - 0.09:
            continue
        verts = []
        for index in polygon.vertices:
            if index not in vertex_map:
                point = mathutils.Vector(coordinates[index])
                outward = (point - mathutils.Vector(center)).normalized()
                vertex_map[index] = work.verts.new(point + outward * CAP_OFFSET)
            verts.append(vertex_map[index])
        try:
            work.faces.new(verts)
        except ValueError:
            pass
    # close the cap's thickness so the voxel remesh sees a solid
    bmesh.ops.solidify(work, geom=work.faces[:], thickness=0.02)
    return work


def add_spike(work, root, direction, length, radius, curl_axis):
    """A curved, tapered tube from root along direction; the tip curls about curl_axis."""
    direction = direction.normalized()
    side = direction.cross(mathutils.Vector((0.0, 0.0, 1.0)))
    if side.length < 1e-3:
        side = mathutils.Vector((1.0, 0.0, 0.0))
    side.normalize()
    other = direction.cross(side).normalized()
    rings = []
    center = root.copy()
    current = direction.copy()
    for segment in range(SPIKE_SEGMENTS + 1):
        fraction = segment / SPIKE_SEGMENTS
        ring_radius = radius * (1.0 - fraction) ** 1.3 + 0.0015
        ring = []
        for step in range(SPIKE_SIDES):
            angle = 2.0 * math.pi * step / SPIKE_SIDES
            # a little flatter across the spike, like a lock of hair
            ring.append(work.verts.new(center + side * math.cos(angle) * ring_radius + other * math.sin(angle) * ring_radius * 0.7))
        rings.append(ring)
        current = mathutils.Quaternion(curl_axis, SPIKE_CURL * 2.0 / SPIKE_SEGMENTS) @ current
        center = center + current * (length / SPIKE_SEGMENTS)
    for first, second in zip(rings[:-1], rings[1:]):
        for step in range(SPIKE_SIDES):
            work.faces.new((first[step], first[(step + 1) % SPIKE_SIDES], second[(step + 1) % SPIKE_SIDES], second[step]))
    work.faces.new(rings[0][::-1])
    tip = work.verts.new(center)
    for step in range(SPIKE_SIDES):
        work.faces.new((rings[-1][step], rings[-1][(step + 1) % SPIKE_SIDES], tip))


def build_hair():
    center, radius, eye_height, coordinates, surface = head_frame()
    work = cap_mesh(coordinates, center, eye_height, surface)
    head_center = mathutils.Vector(center)
    for (azimuth, elevation), (back, up, sideways), length, root_radius in SPIKES:
        azimuth_radians, elevation_radians = math.radians(azimuth), math.radians(elevation)
        # azimuth 0 faces -y (the face), 90 is his left (+x), 180 the back (+y)
        outward = mathutils.Vector((math.sin(azimuth_radians) * math.cos(elevation_radians),
                                    -math.cos(azimuth_radians) * math.cos(elevation_radians),
                                    math.sin(elevation_radians)))
        # the root sits on the real head surface in that direction (the head is no sphere), sunk into the cap
        head_points = coordinates[surface & (coordinates[:, 2] > eye_height - 0.1)]
        relative = head_points - center
        lengths = numpy.linalg.norm(relative, axis=1)
        facing = (relative / lengths[:, None]) @ numpy.array(outward)
        surface_point = head_points[int(numpy.argmax(facing + 0.002 * lengths))]
        root = mathutils.Vector(surface_point) + outward * (CAP_OFFSET * 0.4)
        direction = mathutils.Vector((sideways * 0.6, back, up)) + outward * 0.5
        curl_axis = direction.cross(mathutils.Vector((0.0, 1.0, 0.0)) if abs(direction.y) < 0.9 else mathutils.Vector((1.0, 0.0, 0.0))).normalized()
        add_spike(work, root, direction, length * radius / 0.12, root_radius * radius / 0.12, curl_axis)
    mesh = bpy.data.meshes.get(HAIR_NAME) or bpy.data.meshes.new(HAIR_NAME)
    work.to_mesh(mesh)
    work.free()
    old = bpy.data.objects.get(HAIR_NAME)
    if old:
        bpy.data.objects.remove(old, do_unlink=True)
    hair = bpy.data.objects.new(HAIR_NAME, mesh)
    bpy.context.scene.collection.objects.link(hair)
    # one surface: voxel remesh, then smoothing
    remesh = hair.modifiers.new("remesh", "REMESH")
    remesh.mode = "VOXEL"
    remesh.voxel_size = VOXEL_SIZE
    smooth = hair.modifiers.new("smooth", "CORRECTIVE_SMOOTH")
    smooth.iterations = SMOOTH_ITERATIONS
    smooth.smooth_type = "LENGTH_WEIGHTED"
    smooth.use_only_smooth = True
    depsgraph = bpy.context.evaluated_depsgraph_get()
    evaluated = hair.evaluated_get(depsgraph)
    fused = bpy.data.meshes.new_from_object(evaluated)
    hair.modifiers.clear()
    old_mesh = hair.data
    hair.data = fused
    bpy.data.meshes.remove(old_mesh)
    fused.name = HAIR_NAME
    for polygon in fused.polygons:
        polygon.use_smooth = True
    rig = bpy.data.objects[RIG_NAME]
    hair.parent = rig
    # vertex group names live on the mesh data: clear leftovers from earlier builds so the weights sit in "head"
    hair.vertex_groups.clear()
    group = hair.vertex_groups.new(name="head")
    group.add(list(range(len(fused.vertices))), 1.0, "REPLACE")
    modifier = hair.modifiers.new("Armature", "ARMATURE")
    modifier.object = rig
    return len(fused.vertices), round(radius, 3)
