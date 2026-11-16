"""Renders the MPFB character (models/adam_mpfb.blend) from the sprite camera at dstar's sprite size.

Run inside the live Blender: exec(open(path).read()); render_sprite(out_path, turn_degrees)

He faces -y; turning him by turn_degrees about z shows him like dstar's 3/4 sprite (facing screen right, a
bit of his face toward the camera). The camera looks along +y, orthographic; his evaluated height maps onto
the 42 pixels of dstar's figure in a 72 x 48 frame rendered at 8x, feet on the bottom row. finish_frames.py
then snaps to his palette and draws his outline.
"""

import math

import bpy
import mathutils
import numpy

FRAME = (72, 48)
FIGURE_PIXELS = 42
SCALE = 8
RIG_NAME = "adam_body.rig"


def evaluated_bounds():
    depsgraph = bpy.context.evaluated_depsgraph_get()
    low, high = numpy.full(3, numpy.inf), numpy.full(3, -numpy.inf)
    for obj in bpy.data.objects:
        if obj.type != "MESH" or obj.hide_render:
            continue
        evaluated = obj.evaluated_get(depsgraph)
        mesh = evaluated.to_mesh()
        coordinates = numpy.zeros(len(mesh.vertices) * 3)
        mesh.vertices.foreach_get("co", coordinates)
        evaluated.to_mesh_clear()
        world = numpy.array(obj.matrix_world)
        points = coordinates.reshape(-1, 3) @ world[:3, :3].T + world[:3, 3]
        low, high = numpy.minimum(low, points.min(axis=0)), numpy.maximum(high, points.max(axis=0))
    return low, high


def sprite_camera(scene):
    camera = bpy.data.objects.get("sprite_camera")
    if camera is None:
        camera = bpy.data.objects.new("sprite_camera", bpy.data.cameras.new("sprite_camera"))
        scene.collection.objects.link(camera)
    camera.data.type = "ORTHO"
    camera.rotation_euler = mathutils.Vector((0.0, 1.0, 0.0)).to_track_quat("-Z", "Y").to_euler()
    return camera


def light(scene):
    """One sun from the front left and above, so the toon ramp shades the side turned away."""
    sun = bpy.data.objects.get("sprite_sun")
    if sun is None:
        sun = bpy.data.objects.new("sprite_sun", bpy.data.lights.new("sprite_sun", "SUN"))
        scene.collection.objects.link(sun)
        # set once: mpfb_match.py fits the direction afterwards
        sun.rotation_euler = (math.radians(50.0), 0.0, math.radians(-30.0))
    sun.data.energy = 3.0
    return sun


def render_sprite(out_path, turn_degrees=55.0, frame_bounds=None):
    """frame_bounds: (low, high) to keep one camera for a whole animation; by default this frame's own bounds."""
    scene = bpy.context.scene
    rig = bpy.data.objects[RIG_NAME]
    rig.rotation_euler = (0.0, 0.0, math.radians(turn_degrees))
    bpy.context.view_layer.update()
    low, high = frame_bounds or evaluated_bounds()
    units_per_pixel = (high[2] - low[2]) / FIGURE_PIXELS
    camera = sprite_camera(scene)
    light(scene)
    material = bpy.data.materials.get("adam_toon")
    if material:
        # the shirt stripes follow the sprite rows: counted from the frame's bottom row in sprite pixels
        material.node_tree.nodes["stripe_origin"].outputs[0].default_value = low[2]
        material.node_tree.nodes["stripe_pixel"].outputs[0].default_value = units_per_pixel
    camera.data.ortho_scale = FRAME[0] * units_per_pixel
    camera.location = mathutils.Vector(((low[0] + high[0]) / 2, low[1] - 5.0, low[2] + FRAME[1] / 2 * units_per_pixel))
    scene.camera = camera
    scene.render.engine = "BLENDER_EEVEE"
    scene.render.resolution_x, scene.render.resolution_y = FRAME[0] * SCALE, FRAME[1] * SCALE
    scene.render.film_transparent = True
    scene.view_settings.view_transform = "Standard"
    scene.render.dither_intensity = 0.0
    scene.render.filepath = out_path
    bpy.ops.render.render(write_still=True)
    return out_path
