"""Renders animations of the matched high resolution player at 8x, plus per frame feature positions.

Run inside the live Blender: exec(open(path).read()); render_animation(action, count, directory)

Writes <directory>/<index>.png and <directory>/<index>.json with the sprite position of every eye
anchor (vertices that carry dstar's eye pixels in the matched idle) and whether the face points at
the camera, so the downscale can stamp his exact eye pixels where the eyes are.
"""

import json
import math
import os

import bpy
from bpy_extras.object_utils import world_to_camera_view
from mathutils import Vector

FRAME_WIDTH, FRAME_HEIGHT = 72, 48


def eye_positions(scene, camera, body, anchors):
    depsgraph = bpy.context.evaluated_depsgraph_get()
    evaluated = body.evaluated_get(depsgraph)
    mesh = evaluated.to_mesh()
    positions = []
    view_direction = (camera.matrix_world.to_3x3() @ Vector((0.0, 0.0, -1.0))).normalized()
    facing = []
    for vertex_index in anchors:
        world = evaluated.matrix_world @ mesh.vertices[vertex_index].co
        normal = (evaluated.matrix_world.to_3x3() @ mesh.vertices[vertex_index].normal).normalized()
        view = world_to_camera_view(scene, camera, world)
        positions.append([view.x * FRAME_WIDTH, (1.0 - view.y) * FRAME_HEIGHT])
        facing.append(float(normal.dot(-view_direction)))
    evaluated.to_mesh_clear()
    return positions, facing


def head_position(scene, camera, rig):
    """Sprite position of the neck end of the head bone, its on-screen tilt (degrees, 0 = upright) and facing."""
    head_bone = rig.pose.bones["Head"]
    neck = rig.matrix_world @ head_bone.head
    top = rig.matrix_world @ head_bone.tail
    neck_view = world_to_camera_view(scene, camera, neck)
    top_view = world_to_camera_view(scene, camera, top)
    dx = (top_view.x - neck_view.x) * FRAME_WIDTH
    dy = (top_view.y - neck_view.y) * FRAME_HEIGHT
    view_direction = (camera.matrix_world.to_3x3() @ Vector((0.0, 0.0, -1.0))).normalized()
    # the head bone's local +z points out of the face in this rig (the idle pose gives +0.83)
    face = (rig.matrix_world.to_3x3() @ head_bone.matrix.to_3x3() @ Vector((0.0, 0.0, 1.0))).normalized()
    return [neck_view.x * FRAME_WIDTH, (1.0 - neck_view.y) * FRAME_HEIGHT], math.degrees(math.atan2(dx, dy)), float(face.dot(-view_direction))


def frame_features(scene, camera, rig, body):
    positions, facing = eye_positions(scene, camera, body, list(body["eye_anchors"]))
    head, tilt, head_facing = head_position(scene, camera, rig)
    return {"eyes": positions, "eye_facing": facing, "head": head, "head_tilt": tilt, "head_facing": head_facing}


def render_still(path):
    """Renders the current pose to path (.png) plus its features (.json)."""
    scene = bpy.context.scene
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)
    features = frame_features(scene, scene.camera, bpy.data.objects["player_rig_hires"], bpy.data.objects["SuperHero_Male"])
    json.dump(features, open(path[:-4] + ".json", "w"))
    return features


def lowest_point(objects):
    depsgraph = bpy.context.evaluated_depsgraph_get()
    lowest = None
    for mesh_object in objects:
        evaluated = mesh_object.evaluated_get(depsgraph)
        mesh = evaluated.to_mesh()
        value = min((evaluated.matrix_world @ vertex.co).z for vertex in mesh.vertices)
        evaluated.to_mesh_clear()
        lowest = value if lowest is None else min(lowest, value)
    return lowest


def render_animation(action_name, count, directory, scales, first=None, last=None, follow_ground=False):
    scene = bpy.context.scene
    camera = scene.camera
    rig = bpy.data.objects["player_rig_hires"]
    body = bpy.data.objects["SuperHero_Male"]
    anchors = list(body["eye_anchors"])
    animation_data = rig.animation_data
    animation_data.action = bpy.data.actions[action_name]
    animation_data.action_slot = animation_data.action.slots[0]
    start, end = [int(value) for value in animation_data.action.frame_range]
    start = start if first is None else first
    end = end if last is None else last
    os.makedirs(directory, exist_ok=True)
    rest_camera_height = camera.location.z
    pixel_size = camera.data.ortho_scale / FRAME_WIDTH
    for index in range(count):
        scene.frame_set(start + (end - start) * index // count)
        for bone_name, scale in scales.items():
            rig.pose.bones[bone_name].scale = scale
        bpy.context.view_layer.update()
        if follow_ground:
            # floor animations (crawl, sitting): the lowest point of the model on the bottom row
            camera.location.z = lowest_point([body]) + (FRAME_HEIGHT * 0.5 - camera.data.shift_y * FRAME_WIDTH) * pixel_size
            bpy.context.view_layer.update()
        scene.render.filepath = os.path.join(directory, f"{index:02d}.png")
        bpy.ops.render.render(write_still=True)
        json.dump(frame_features(scene, camera, rig, body), open(os.path.join(directory, f"{index:02d}.json"), "w"))
    camera.location.z = rest_camera_height
