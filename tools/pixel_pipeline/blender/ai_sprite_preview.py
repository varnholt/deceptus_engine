"""Renders the AI model (ai_adam) straight from the front at 8x sprite size, for finish_frames.py.

Run inside the live Blender: exec(open(path).read()); render_front(out_path)

Hunyuan3D built the model so its front view is his sprite drawing (a 3/4 view facing right), so the
sprite camera looks along +y. Scale: his idle figure is 42 pixels tall, a 72 x 48 frame rendered at 8x
(576 x 384); the model's feet to hair top span maps onto those 42 pixels. finish_frames.py --ground
puts the feet on the bottom row afterwards.
"""

import bpy
import mathutils
import numpy

FIGURE_PIXELS = 42
FRAME = (72, 48)
SCALE = 8


def render_front(out_path, center_x=None):
    scene = bpy.context.scene
    model = bpy.data.objects["ai_adam"]
    coordinates = numpy.zeros(len(model.data.vertices) * 3, dtype=numpy.float32)
    model.data.vertices.foreach_get("co", coordinates)
    coordinates = coordinates.reshape(-1, 3)
    low, high = coordinates.min(axis=0), coordinates.max(axis=0)
    units_per_pixel = (high[2] - low[2]) / FIGURE_PIXELS
    camera = bpy.data.objects.get("sprite_front_camera")
    if camera is None:
        camera = bpy.data.objects.new("sprite_front_camera", bpy.data.cameras.new("sprite_front_camera"))
        scene.collection.objects.link(camera)
    camera.data.type = "ORTHO"
    camera.data.ortho_scale = FRAME[0] * units_per_pixel
    middle_x = (low[0] + high[0]) / 2 if center_x is None else center_x
    # the frame's bottom row is the feet; the frame center sits half a frame above them
    camera.location = mathutils.Vector((middle_x, -6.0, low[2] + FRAME[1] / 2 * units_per_pixel))
    camera.rotation_euler = mathutils.Vector((0.0, 1.0, 0.0)).to_track_quat("-Z", "Y").to_euler()
    hidden = {}
    for obj in scene.objects:
        if obj.type in ("MESH", "CURVES") and obj != model:
            hidden[obj.name] = obj.hide_render
            obj.hide_render = True
    saved = (scene.camera, scene.render.resolution_x, scene.render.resolution_y, scene.render.film_transparent, scene.render.filepath,
             scene.view_settings.view_transform, scene.render.dither_intensity)
    scene.camera = camera
    scene.render.resolution_x, scene.render.resolution_y = FRAME[0] * SCALE, FRAME[1] * SCALE
    scene.render.film_transparent = True
    scene.view_settings.view_transform = "Standard"
    scene.render.dither_intensity = 0.0
    scene.render.filepath = out_path
    # sprites use the atlas in his sprite palette (ai_texture.py)
    image_node = next(node for node in model.data.materials[0].node_tree.nodes if node.type == "TEX_IMAGE")
    natural = image_node.image
    image_node.image = bpy.data.images.load(natural.filepath.replace("atlas_natural", "atlas_sprite"), check_existing=True)
    image_node.image.alpha_mode = "CHANNEL_PACKED"
    try:
        bpy.ops.render.render(write_still=True)
    finally:
        image_node.image = natural
        (scene.camera, scene.render.resolution_x, scene.render.resolution_y, scene.render.film_transparent, scene.render.filepath,
         scene.view_settings.view_transform, scene.render.dither_intensity) = saved
        for name, value in hidden.items():
            bpy.data.objects[name].hide_render = value
    return out_path
