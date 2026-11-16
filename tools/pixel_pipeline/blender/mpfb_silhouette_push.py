"""Pulls one piece of the MPFB character (the hair) onto dstar's idle outline, smoothly, as a shape key.

Run inside the live Blender, posed like his idle with the sprite camera set:
exec(open(path).read()); push_to_outline(object_name, his_rgba_npy, shift)

His figure (inside his black outline) gives a signed distance field in sprite pixels. Every vertex of the piece
that lies on my outline (as seen from the sprite camera) gets the camera-plane move that brings it onto his
outline; the moves are then spread smoothly over the whole piece (neighbour averaging), so the shape stays even.
The posed move is carried back into rest space through the bone that moves the vertex most and stored as the
shape key "outline_fit" (value 1.0).
"""

import bpy
import mathutils
import numpy

FRAME = (72, 48)
SPREAD_ROUNDS = 40
# vertices within this many sprite pixels of my outline count as outline vertices
EDGE_PIXELS = 0.8


def distance_to(mask):
    """For every pixel, the distance to the nearest pixel of mask (brute force: the frame is only 72 x 48)."""
    rows, columns = numpy.nonzero(mask)
    grid_rows, grid_columns = numpy.mgrid[0:mask.shape[0], 0:mask.shape[1]]
    if len(rows) == 0:
        return numpy.full(mask.shape, 1e6)
    squared = (grid_rows.ravel()[:, None] - rows[None, :]) ** 2 + (grid_columns.ravel()[:, None] - columns[None, :]) ** 2
    return numpy.sqrt(squared.min(axis=1)).reshape(mask.shape)


def blur(values):
    padded = numpy.pad(values, 1, mode="edge")
    return sum(padded[row:row + values.shape[0], column:column + values.shape[1]] for row in range(3) for column in range(3)) / 9.0


def dilate(mask):
    padded = numpy.pad(mask, 1)
    return numpy.any([padded[row:row + mask.shape[0], column:column + mask.shape[1]] for row in range(3) for column in range(3)], axis=0)


def erode(mask):
    return ~dilate(~mask)


def fill_holes(mask):
    """Background reachable from the frame border stays background; enclosed gaps become part of the figure."""
    outside = numpy.zeros_like(mask)
    outside[0, :], outside[-1, :], outside[:, 0], outside[:, -1] = ~mask[0, :], ~mask[-1, :], ~mask[:, 0], ~mask[:, -1]
    while True:
        grown = dilate(outside) & ~mask
        if (grown == outside).all():
            return ~outside
        outside = grown


def signed_distance(mask):
    """Negative inside his figure, positive outside, in pixels; and its gradient (pointing outward)."""
    outside = distance_to(mask)
    inside = distance_to(~mask)
    distance = outside - inside
    gradient_row, gradient_column = numpy.gradient(blur(distance))
    return distance, gradient_row, gradient_column


def push_to_outline(object_name, his_rgba_npy, shift=0, strength=1.0):
    obj = bpy.data.objects[object_name]
    his = numpy.load(his_rgba_npy)
    his_figure = (his[..., 3] > 0) & (his[..., :3].sum(axis=-1) > 0)
    distance, gradient_row, gradient_column = signed_distance(his_figure)
    scene = bpy.context.scene
    camera = scene.camera
    rotation = camera.matrix_world.to_quaternion()
    right = rotation @ mathutils.Vector((1.0, 0.0, 0.0))
    up = rotation @ mathutils.Vector((0.0, 1.0, 0.0))
    width = camera.data.ortho_scale
    pixel = width / FRAME[0]
    origin = camera.matrix_world.translation
    depsgraph = bpy.context.evaluated_depsgraph_get()
    # my own outline: all visible meshes rendered as one mask
    mine = numpy.zeros((FRAME[1], FRAME[0]), bool)
    piece_points = None
    for item in bpy.data.objects:
        if item.type != "MESH" or item.hide_render:
            continue
        evaluated = item.evaluated_get(depsgraph)
        mesh = evaluated.to_mesh()
        world = numpy.array(evaluated.matrix_world)
        coordinates = numpy.zeros(len(mesh.vertices) * 3)
        mesh.vertices.foreach_get("co", coordinates)
        points = coordinates.reshape(-1, 3) @ world[:3, :3].T + world[:3, 3]
        evaluated.to_mesh_clear()
        relative = points - numpy.array(origin)
        columns = relative @ numpy.array(right) / pixel + FRAME[0] / 2 + shift
        rows = FRAME[1] / 2 - relative @ numpy.array(up) / pixel
        inside = (columns >= 0) & (columns < FRAME[0]) & (rows >= 0) & (rows < FRAME[1])
        mine[rows[inside].astype(int), columns[inside].astype(int)] = True
        if item == obj:
            piece_points = (points, columns, rows)
    mine = fill_holes(erode(dilate(mine)))
    my_edge_distance = distance_to(~mine)
    points, columns, rows = piece_points
    row_index = numpy.clip(rows.astype(int), 0, FRAME[1] - 1)
    column_index = numpy.clip(columns.astype(int), 0, FRAME[0] - 1)
    on_my_outline = (my_edge_distance[row_index, column_index] <= EDGE_PIXELS + 1.0)
    # the move onto his outline: along his distance gradient by his signed distance (in pixels)
    signed = distance[row_index, column_index]
    move_row = -signed * gradient_row[row_index, column_index]
    move_column = -signed * gradient_column[row_index, column_index]
    norm = numpy.maximum(numpy.hypot(gradient_row[row_index, column_index], gradient_column[row_index, column_index]), 1e-6)
    move_row, move_column = move_row / norm, move_column / norm
    moves = numpy.zeros((len(points), 3))
    moves[on_my_outline] = (numpy.outer(move_column[on_my_outline], numpy.array(right)) - numpy.outer(move_row[on_my_outline], numpy.array(up))) * pixel * strength
    weights = on_my_outline.astype(numpy.float64)
    # spread the outline moves over the piece: averaging with neighbours, outline vertices keep their own move
    mesh = obj.data
    edges = numpy.zeros(len(mesh.edges) * 2, dtype=numpy.int64)
    mesh.edges.foreach_get("vertices", edges)
    edges = edges.reshape(-1, 2)
    counts = numpy.zeros(len(points))
    numpy.add.at(counts, edges[:, 0], 1)
    numpy.add.at(counts, edges[:, 1], 1)
    fixed = on_my_outline.copy()
    spread = moves.copy()
    for _ in range(SPREAD_ROUNDS):
        sums = numpy.zeros_like(spread)
        numpy.add.at(sums, edges[:, 0], spread[edges[:, 1]])
        numpy.add.at(sums, edges[:, 1], spread[edges[:, 0]])
        averaged = sums / numpy.maximum(counts, 1)[:, None]
        spread = numpy.where(fixed[:, None], 0.5 * moves + 0.5 * averaged, averaged)
    # back into rest space through each vertex's strongest bone
    rig = obj.parent
    names = {group.index: group.name for group in obj.vertex_groups}
    rest_moves = numpy.zeros_like(spread)
    world_to_rig = rig.matrix_world.inverted().to_3x3()
    for vertex in mesh.vertices:
        bone_elements = [element for element in vertex.groups if names[element.group] in rig.pose.bones]
        best = max(bone_elements, key=lambda element: element.weight, default=None)
        move = world_to_rig @ mathutils.Vector(spread[vertex.index])
        if best:
            pose_bone = rig.pose.bones[names[best.group]]
            deform = (pose_bone.matrix @ pose_bone.bone.matrix_local.inverted()).to_3x3()
            move = deform.inverted() @ move
        rest_moves[vertex.index] = move
    if mesh.shape_keys is None:
        obj.shape_key_add(name="Basis", from_mix=False)
    basis = mesh.shape_keys.key_blocks[0]
    coordinates = numpy.zeros(len(basis.data) * 3)
    basis.data.foreach_get("co", coordinates)
    key = mesh.shape_keys.key_blocks.get("outline_fit") or obj.shape_key_add(name="outline_fit", from_mix=False)
    key.data.foreach_set("co", (coordinates.reshape(-1, 3) + rest_moves).ravel())
    key.value = 1.0
    return int(on_my_outline.sum()), float(numpy.abs(spread).max())
