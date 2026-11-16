"""Combines two Hunyuan3D generations into one figure: the head from his portrait on the body from his sprite.

Run inside the live Blender with both generations imported (glTF): exec(open(path).read()); combine()

The portrait bust is cut at the neck (its collar fluff and the hair hanging below the neck go), scaled
so its chin-to-crown height matches the sprite body's head and placed with its chin on the body's
chin; the neck reaches down into the body's collar. The body loses everything above its chin.
Heights are in the generations' own units (about 2 tall, the face looking along -y).
"""

import bmesh
import bpy
import mathutils

BODY_PREFIX = "ai_adam_idle_no_outline"
HEAD_PREFIX = "ai_adam_portrait_input"

BODY_CHIN_Z = 0.27
BODY_CROWN_Z = 0.994
HEAD_CHIN_Z = -0.53
HEAD_CROWN_Z = 0.913
HEAD_NECK_BOTTOM_Z = -0.75     # the neck continues this far below the chin into the collar
NECK_RADIUS = 0.15             # in portrait units: only the neck survives below the chin
HEAD_SIZE = 1.15               # his sprite head is a bit bigger than the body's generated one


def mesh_object(prefix):
    return next(obj for obj in bpy.data.objects if obj.name.startswith(prefix) and obj.type == "MESH")


def world_copy(source, name):
    mesh = source.data.copy()
    mesh.transform(source.matrix_world)
    copy = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(copy)
    return copy


def neck_axis(head_object):
    band = [vertex.co for vertex in head_object.data.vertices if HEAD_CHIN_Z - 0.06 < vertex.co.z < HEAD_CHIN_Z - 0.01 and abs(vertex.co.x) < 0.3]
    return mathutils.Vector((sum(point.x for point in band) / len(band), sum(point.y for point in band) / len(band)))


def combine():
    for name in ("ai_head_part", "ai_body_part"):
        old = bpy.data.objects.get(name)
        if old:
            bpy.data.objects.remove(old, do_unlink=True)
    head = world_copy(mesh_object(HEAD_PREFIX), "ai_head_part")
    body = world_copy(mesh_object(BODY_PREFIX), "ai_body_part")
    axis = neck_axis(head)

    builder = bmesh.new()
    builder.from_mesh(head.data)
    remove = [vertex for vertex in builder.verts if vertex.co.z < HEAD_NECK_BOTTOM_Z or
              (vertex.co.z < HEAD_CHIN_Z and (mathutils.Vector((vertex.co.x, vertex.co.y)) - axis).length > NECK_RADIUS)]
    bmesh.ops.delete(builder, geom=remove, context="VERTS")
    builder.to_mesh(head.data)
    builder.free()

    builder = bmesh.new()
    builder.from_mesh(body.data)
    remove = [vertex for vertex in builder.verts if vertex.co.z > BODY_CHIN_Z]
    body_axis_points = [vertex.co for vertex in builder.verts if BODY_CHIN_Z - 0.08 < vertex.co.z < BODY_CHIN_Z]
    body_axis = mathutils.Vector((sum(point.x for point in body_axis_points) / len(body_axis_points), sum(point.y for point in body_axis_points) / len(body_axis_points)))
    bmesh.ops.delete(builder, geom=remove, context="VERTS")
    builder.to_mesh(body.data)
    builder.free()

    scale = HEAD_SIZE * (BODY_CROWN_Z - BODY_CHIN_Z) / (HEAD_CROWN_Z - HEAD_CHIN_Z)
    # move the head's neck axis and chin onto the body's
    transform = (mathutils.Matrix.Translation((body_axis.x, body_axis.y, BODY_CHIN_Z)) @ mathutils.Matrix.Scale(scale, 4)
                 @ mathutils.Matrix.Translation((-axis.x, -axis.y, -HEAD_CHIN_Z)))
    head.data.transform(transform)
    # ai_colors.py maps the head back to the portrait generation's space to project the portrait
    head["to_body"] = [value for row in transform for value in row]
    return scale, tuple(axis), tuple(body_axis), len(head.data.vertices), len(body.data.vertices)



def evaluated_mesh(source_mesh, name, modifiers):
    """source_mesh with the given modifiers applied: [(type, {property: value})]."""
    temporary = bpy.data.objects.new(name, source_mesh)
    bpy.context.scene.collection.objects.link(temporary)
    for modifier_type, settings in modifiers:
        modifier = temporary.modifiers.new(modifier_type.lower(), modifier_type)
        for key, value in settings.items():
            setattr(modifier, key, value)
    result = bpy.data.meshes.new_from_object(temporary.evaluated_get(bpy.context.evaluated_depsgraph_get()))
    bpy.data.objects.remove(temporary, do_unlink=True)
    return result


def closed_copy(part):
    builder = bmesh.new()
    builder.from_mesh(part.data)
    # the cuts at the neck and chin left openings
    bmesh.ops.holes_fill(builder, edges=[edge for edge in builder.edges if edge.is_boundary], sides=0)
    bmesh.ops.recalc_face_normals(builder, faces=builder.faces[:])
    mesh = bpy.data.meshes.new(part.name + "_closed")
    builder.to_mesh(mesh)
    builder.free()
    return mesh


def fuse(body_voxel_size=0.008, body_smoothing=10, head_vertices=150000):
    """One object "ai_adam": the body resampled and smoothed (the generation is faceted), the head as generated.

    The head's thin hair locks are open sheets a voxel remesh cannot fill, so the head is not welded on:
    its neck reaches deep into the collar, so no seam shows.
    """
    head = bpy.data.objects["ai_head_part"]
    body = bpy.data.objects["ai_body_part"]
    body_closed = closed_copy(body)
    body_mesh = evaluated_mesh(body_closed, "ai_body_smooth", [
        ("REMESH", {"mode": "VOXEL", "voxel_size": body_voxel_size}),
        ("SMOOTH", {"factor": 0.6, "iterations": body_smoothing}),
    ])
    head_mesh = evaluated_mesh(head.data.copy(), "ai_head_light", [
        ("DECIMATE", {"ratio": min(1.0, head_vertices / max(1, len(head.data.vertices)))}),
    ])
    builder = bmesh.new()
    builder.from_mesh(body_mesh)
    builder.from_mesh(head_mesh)
    old = bpy.data.objects.get("ai_adam")
    if old:
        bpy.data.objects.remove(old, do_unlink=True)
    fused_mesh = bpy.data.meshes.new("ai_adam")
    builder.to_mesh(fused_mesh)
    builder.free()
    for polygon in fused_mesh.polygons:
        polygon.use_smooth = True
    fused = bpy.data.objects.new("ai_adam", fused_mesh)
    # body vertices come first, then the head's
    fused["body_vertex_count"] = len(body_mesh.vertices)
    fused["head_to_body"] = head["to_body"]
    bpy.context.scene.collection.objects.link(fused)
    for mesh in (body_closed, body_mesh, head_mesh):
        bpy.data.meshes.remove(mesh)
    for part in (head, body):
        part.hide_set(True)
        part.hide_render = True
    return len(fused_mesh.vertices)


def whole_body(voxel_size=0.008, smoothing=10):
    """The model from the sprite generation alone, head included: his pixel art is the ground truth.

    The generation is closed but faceted: resampled and smoothed like the body in fuse(). Every vertex
    belongs to the body projection (ai_texture.py); the head starts above BODY_CHIN_Z.
    """
    body = world_copy(mesh_object(BODY_PREFIX), "ai_whole_part")
    smooth_mesh = evaluated_mesh(body.data, "ai_whole_smooth", [
        ("REMESH", {"mode": "VOXEL", "voxel_size": voxel_size}),
        ("SMOOTH", {"factor": 0.6, "iterations": smoothing}),
    ])
    bpy.data.objects.remove(body, do_unlink=True)
    old = bpy.data.objects.get("ai_adam")
    if old:
        bpy.data.objects.remove(old, do_unlink=True)
    smooth_mesh.name = "ai_adam"
    for polygon in smooth_mesh.polygons:
        polygon.use_smooth = True
    model = bpy.data.objects.new("ai_adam", smooth_mesh)
    bpy.context.scene.collection.objects.link(model)
    model["body_vertex_count"] = len(smooth_mesh.vertices)
    model["head_from_sprite"] = True
    return len(smooth_mesh.vertices)
