"""Projects dstar's idle frame onto the MPFB character as a texture, pixel exact from the sprite camera.

Run inside the live Blender, posed like his idle with the sprite camera set (mpfb_match.py, mpfb_sprite.py):
exec(open(path).read()); project_texture(his_rgba_npy, shift)

Every vertex gets UV coordinates where the sprite camera sees it in the 72 x 48 frame ("idle_projection"); the
material shows his frame there with nearest-neighbour sampling, so his pixels keep their exact edges. Only faces
the camera sees take the texture (face attribute "projected", from a ray test), and only where his pixel is part
of the figure: his black outline and the empty background are transparent in the image, so the model's own
colors show there (finish_frames.py draws the outline). The UVs stay on the surface, so the pixels move with
the mesh in every animation. shift: his frame's horizontal offset against the render (from mpfb_match.score).
"""

import bpy
import mathutils
import numpy

FRAME = (72, 48)
# how directly a face must look at the camera (cosine) to take his pixels
FACING = 0.35
IMAGE_PATH = r"D:\\ai3d\\work\\mpfb\\his_idle_projection.png"


def his_image(his_rgba_npy):
    his = numpy.load(his_rgba_npy).astype(numpy.float64)
    rgba = his / 255.0
    outline = his[..., :3].sum(axis=-1) == 0
    rgba[outline, 3] = 0.0
    image = bpy.data.images.get("his_idle_projection") or bpy.data.images.new("his_idle_projection", FRAME[0], FRAME[1], alpha=True)
    image.colorspace_settings.name = "sRGB"
    image.alpha_mode = "STRAIGHT"
    image.pixels.foreach_set(rgba[::-1].ravel().astype(numpy.float32))
    image.filepath_raw = IMAGE_PATH
    image.file_format = "PNG"
    image.save()
    return image


def add_to_material(image):
    material = bpy.data.materials["adam_toon"]
    tree = material.node_tree
    if "projection_image" in tree.nodes:
        tree.nodes["projection_image"].image = image
        return
    emission = next(node for node in tree.nodes if node.type == "EMISSION")
    current = emission.inputs["Color"].links[0].from_socket
    coordinates = tree.nodes.new("ShaderNodeUVMap")
    coordinates.uv_map = "idle_projection"
    texture = tree.nodes.new("ShaderNodeTexImage")
    texture.name = "projection_image"
    texture.image = image
    texture.interpolation = "Closest"
    texture.extension = "CLIP"
    flag = tree.nodes.new("ShaderNodeAttribute")
    flag.attribute_name = "projected"
    weight = tree.nodes.new("ShaderNodeMath")
    weight.operation = "MULTIPLY"
    mix = tree.nodes.new("ShaderNodeMix")
    mix.data_type = "RGBA"
    tree.links.new(coordinates.outputs["UV"], texture.inputs["Vector"])
    tree.links.new(flag.outputs["Fac"], weight.inputs[0])
    tree.links.new(texture.outputs["Alpha"], weight.inputs[1])
    tree.links.new(weight.outputs[0], mix.inputs[0])
    tree.links.new(current, mix.inputs[6])
    tree.links.new(texture.outputs["Color"], mix.inputs[7])
    tree.links.new(mix.outputs[2], emission.inputs["Color"])


def project_texture(his_rgba_npy, shift=0):
    scene = bpy.context.scene
    camera = scene.camera
    rotation = camera.matrix_world.to_quaternion()
    direction = rotation @ mathutils.Vector((0.0, 0.0, -1.0))
    right = rotation @ mathutils.Vector((1.0, 0.0, 0.0))
    up = rotation @ mathutils.Vector((0.0, 1.0, 0.0))
    width = camera.data.ortho_scale
    pixel = width / FRAME[0]
    origin = camera.matrix_world.translation
    image = his_image(his_rgba_npy)
    add_to_material(image)
    counts = {}
    for obj in bpy.data.objects:
        if obj.type != "MESH" or obj.hide_render:
            continue
        switched = [modifier for modifier in obj.modifiers if modifier.type not in ("ARMATURE", "DISPLACE") and modifier.show_viewport]
        for modifier in switched:
            modifier.show_viewport = False
        bpy.context.view_layer.update()
        depsgraph = bpy.context.evaluated_depsgraph_get()
        evaluated = obj.evaluated_get(depsgraph)
        posed = evaluated.to_mesh()
        world = evaluated.matrix_world.copy()
        same_topology = len(posed.vertices) == len(obj.data.vertices) and len(posed.polygons) == len(obj.data.polygons)
        points = [world @ vertex.co for vertex in posed.vertices] if same_topology else []
        centers = [(world @ polygon.center, (world.to_3x3() @ polygon.normal).normalized()) for polygon in posed.polygons] if same_topology else []
        evaluated.to_mesh_clear()
        for modifier in switched:
            modifier.show_viewport = True
        if not same_topology:
            continue
        mesh = obj.data
        uv_layer = mesh.uv_layers.get("idle_projection") or mesh.uv_layers.new(name="idle_projection")
        # the score rolls my render by shift to meet his frame: my render column c shows his column c + shift
        vertex_uv = []
        for point in points:
            relative = point - origin
            column = (relative.dot(right) + width / 2) / pixel + shift
            row_from_bottom = relative.dot(up) / pixel + FRAME[1] / 2
            vertex_uv.append((column / FRAME[0], row_from_bottom / FRAME[1]))
        for loop in mesh.loops:
            uv_layer.data[loop.index].uv = vertex_uv[loop.vertex_index]
        flags = mesh.attributes.get("projected") or mesh.attributes.new("projected", "FLOAT", "FACE")
        values = numpy.zeros(len(mesh.polygons))
        depsgraph = bpy.context.evaluated_depsgraph_get()
        for index, (center, normal) in enumerate(centers):
            # faces seen at a grazing angle would stretch his pixels across the side: only faces turned to the camera
            if normal.dot(direction) > -FACING:
                continue
            hit, location, _, _, hit_object, _ = scene.ray_cast(depsgraph, center - direction * 10.0, direction)
            if hit and hit_object.original == obj and (location - center).length < pixel * 0.6:
                values[index] = 1.0
        flags.data.foreach_set("value", values)
        mesh.update()
        counts[obj.name] = int(values.sum())
    return counts
