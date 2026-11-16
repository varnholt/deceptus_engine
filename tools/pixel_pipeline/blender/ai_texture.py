"""Gives the AI model (ai_combine.py) a crisp image texture: his drawings projected along the view axis.

Run inside the live Blender after ai_atlas.py wrote the atlas: exec(open(path).read()); texture()

Hunyuan3D built each part so its front view is its input picture, so a vertex's (x, z) maps straight
into that picture: body vertices into his sprite, head vertices (moved back into the portrait
generation's space) into his portrait. The UV map "projection" stores that, the material samples the
atlas with nearest neighbour, so every one of his pixels stays a sharp block. Where the front picture
cannot know the surface, a per-face override color takes over: the back and the sides of the head are
hair (except his ear), the back of the jacket has no shirt stripe. The material shows atlas_natural.png;
sprite renders swap in atlas_sprite.png (the head in his sprite palette).
"""

import json

import bpy
import mathutils
import numpy

ATLAS_DIRECTORY = r"D:\ai3d\work\atlas"
BODY_PREFIX = "ai_adam_idle_no_outline"
HEAD_PREFIX = "ai_adam_portrait_input"

HAIR = (188, 54, 116)
JACKET = (100, 34, 58)
SHIRT_COLORS = ((31, 133, 125), (57, 173, 135), (13, 63, 83))
SKIN_COLORS = ((219, 150, 107), (190, 107, 69), (240, 190, 150), (232, 170, 128), (241, 192, 151))
# his visible ear in the portrait (pixel 25, 48 of 82 x 78), in the portrait canvas: 10x and centred on 1024
EAR_PIXEL = (102 + 25 * 10 + 5, 122 + 48 * 10 + 5)
EAR_RADIUS = 0.16
# his ear in the sprite picture of the atlas (idle frame pixel 5.5, 11.5: Scale2x 8x, centred on a 1.25x canvas, 1024 wide)
SPRITE_EAR_PIXEL = (385, 328)
SPRITE_EAR_RADIUS = 0.1
HEAD_START_Z = 0.27
# his four hair shades (darkest first) and where they start along the lock pattern's noise value
HAIR_SHADES = [((102, 30, 80), 0.0), ((140, 40, 92), 0.42), ((188, 54, 116), 0.5), ((214, 67, 118), 0.6)]
# the lock pattern: noise stretched along z, so it runs down the head in streaks
LOCK_SCALE = (9.0, 9.0, 2.2)


def linear(color):
    def channel(value):
        value /= 255.0
        return value / 12.92 if value <= 0.04045 else ((value + 0.055) / 1.055) ** 2.4
    return (channel(color[0]), channel(color[1]), channel(color[2]), 1.0)


def near(color, palette, tolerance):
    return any(sum(abs(int(first) - second) for first, second in zip(color, entry)) < tolerance for entry in palette)


def generation_bounds(prefix):
    obj = next(item for item in bpy.data.objects if item.name.startswith(prefix) and item.type == "MESH")
    points = [obj.matrix_world @ vertex.co for vertex in obj.data.vertices]
    return (min(point.x for point in points), max(point.x for point in points), min(point.z for point in points), max(point.z for point in points))


class Projection:
    """Maps a point of one generation to atlas pixels (column, row top down) and to UV."""

    def __init__(self, layout, part, bounds):
        self.width, self.height = layout["size"]
        self.offset = layout[part]["offset"]
        self.left, self.right, self.top, self.bottom = layout[part]["box"]
        self.bounds = bounds

    def pixel(self, point):
        x_low, x_high, z_low, z_high = self.bounds
        column = self.offset + self.left + (point.x - x_low) / (x_high - x_low) * (self.right - self.left)
        row = self.bottom - (point.z - z_low) / (z_high - z_low) * (self.bottom - self.top)
        return column, row

    def uv(self, point):
        column, row = self.pixel(point)
        return (column + 0.5) / self.width, 1.0 - (row + 0.5) / self.height


def atlas_color(pixels, column, row):
    """The atlas color at a pixel; just outside the figure (silhouette edges) the nearest figure pixel."""
    row, column = int(round(row)), int(round(column))
    for radius in range(0, 16):
        window = pixels[max(0, row - radius):row + radius + 1, max(0, column - radius):column + radius + 1]
        opaque = window[..., 3] > 0
        if opaque.any():
            return tuple(int(value) for value in window[opaque][0][:3]), radius
    return JACKET, 99


def material(atlas_path, name="ai_adam_material"):
    material = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    material.use_nodes = True
    tree = material.node_tree
    tree.nodes.clear()
    coordinates = tree.nodes.new("ShaderNodeUVMap")
    coordinates.uv_map = "projection"
    image_node = tree.nodes.new("ShaderNodeTexImage")
    image_node.image = bpy.data.images.load(atlas_path, check_existing=True)
    # keep the bled colors around the figures even where alpha is 0
    image_node.image.alpha_mode = "CHANNEL_PACKED"
    image_node.interpolation = "Closest"
    image_node.extension = "EXTEND"
    override = tree.nodes.new("ShaderNodeVertexColor")
    override.layer_name = "override"
    choose = tree.nodes.new("ShaderNodeMix")
    choose.data_type = "RGBA"
    diffuse = tree.nodes.new("ShaderNodeBsdfDiffuse")
    to_rgb = tree.nodes.new("ShaderNodeShaderToRGB")
    brightness = tree.nodes.new("ShaderNodeRGBToBW")
    ramp = tree.nodes.new("ShaderNodeValToRGB")
    ramp.color_ramp.interpolation = "CONSTANT"
    ramp.color_ramp.elements[0].position = 0.0
    ramp.color_ramp.elements[0].color = (0.6, 0.6, 0.6, 1.0)
    ramp.color_ramp.elements[1].position = 0.2
    ramp.color_ramp.elements[1].color = (1.0, 1.0, 1.0, 1.0)
    shade = tree.nodes.new("ShaderNodeMix")
    shade.data_type = "RGBA"
    shade.blend_type = "MULTIPLY"
    shade.inputs[0].default_value = 1.0
    emission = tree.nodes.new("ShaderNodeEmission")
    output = tree.nodes.new("ShaderNodeOutputMaterial")
    tree.links.new(coordinates.outputs["UV"], image_node.inputs["Vector"])
    # a color mix node repeats its socket names per data type: 0 is the float factor, 6 and 7 the colors, output 2 the color
    tree.links.new(override.outputs["Alpha"], choose.inputs[0])
    tree.links.new(image_node.outputs["Color"], choose.inputs[6])
    tree.links.new(override.outputs["Color"], choose.inputs[7])
    tree.links.new(diffuse.outputs["BSDF"], to_rgb.inputs["Shader"])
    tree.links.new(to_rgb.outputs["Color"], brightness.inputs["Color"])
    tree.links.new(brightness.outputs["Val"], ramp.inputs["Fac"])
    # hair the front picture cannot show: streaks in his four hair shades
    texture_coordinates = tree.nodes.new("ShaderNodeTexCoord")
    mapping = tree.nodes.new("ShaderNodeMapping")
    mapping.inputs["Scale"].default_value = LOCK_SCALE
    noise = tree.nodes.new("ShaderNodeTexNoise")
    noise.inputs["Scale"].default_value = 1.0
    noise.inputs["Detail"].default_value = 1.0
    shades = tree.nodes.new("ShaderNodeValToRGB")
    shades.color_ramp.interpolation = "CONSTANT"
    while len(shades.color_ramp.elements) < len(HAIR_SHADES):
        shades.color_ramp.elements.new(0.5)
    for element, (color, position) in zip(shades.color_ramp.elements, HAIR_SHADES):
        element.position = position
        element.color = linear(color)
    hair_flag = tree.nodes.new("ShaderNodeAttribute")
    hair_flag.attribute_name = "hair_pattern"
    with_hair = tree.nodes.new("ShaderNodeMix")
    with_hair.data_type = "RGBA"
    tree.links.new(texture_coordinates.outputs["Object"], mapping.inputs["Vector"])
    tree.links.new(mapping.outputs["Vector"], noise.inputs["Vector"])
    tree.links.new(noise.outputs["Fac"], shades.inputs["Fac"])
    tree.links.new(hair_flag.outputs["Fac"], with_hair.inputs[0])
    tree.links.new(choose.outputs[2], with_hair.inputs[6])
    tree.links.new(shades.outputs["Color"], with_hair.inputs[7])
    tree.links.new(with_hair.outputs[2], shade.inputs[6])
    tree.links.new(ramp.outputs["Color"], shade.inputs[7])
    tree.links.new(shade.outputs[2], emission.inputs["Color"])
    tree.links.new(emission.outputs["Emission"], output.inputs["Surface"])
    return material


def texture():
    model = bpy.data.objects["ai_adam"]
    mesh = model.data
    layout = json.load(open(ATLAS_DIRECTORY + r"\atlas.json"))
    atlas_path = ATLAS_DIRECTORY + r"\atlas_natural.png"
    image = bpy.data.images.load(atlas_path, check_existing=True)
    image.reload()
    width, height = image.size
    # rows top down, 0 to 255
    pixels = (numpy.array(image.pixels[:], dtype=numpy.float32).reshape(height, width, 4)[::-1] * 255).round().astype(numpy.uint8)
    body_count = model["body_vertex_count"]
    values = list(model.get("head_to_body", [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]))
    to_portrait = mathutils.Matrix([values[0:4], values[4:8], values[8:12], values[12:16]]).inverted()
    head_from_sprite = bool(model.get("head_from_sprite", False))
    body_projection = Projection(layout, "body", generation_bounds(BODY_PREFIX))
    x_low, x_high, z_low, z_high = body_projection.bounds
    sprite_ear_x = x_low + (SPRITE_EAR_PIXEL[0] - body_projection.left) / (body_projection.right - body_projection.left) * (x_high - x_low)
    sprite_ear_z = z_low + (body_projection.bottom - SPRITE_EAR_PIXEL[1]) / (body_projection.bottom - body_projection.top) * (z_high - z_low)
    projections = {"body": Projection(layout, "body", generation_bounds(BODY_PREFIX)), "head": Projection(layout, "head", generation_bounds(HEAD_PREFIX))}
    ear_column = layout["head"]["offset"] + EAR_PIXEL[0]
    ear = None

    uv_layer = mesh.uv_layers.get("projection") or mesh.uv_layers.new(name="projection")
    vertex_uv = []
    vertex_point = []
    for vertex in mesh.vertices:
        is_head = vertex.index >= body_count
        point = to_portrait @ vertex.co if is_head else vertex.co
        projection = projections["head" if is_head else "body"]
        vertex_uv.append(projection.uv(point))
        vertex_point.append((is_head, point))
    for loop in mesh.loops:
        uv_layer.data[loop.index].uv = vertex_uv[loop.vertex_index]

    # the ear center in the portrait generation's space
    head_projection = projections["head"]
    x_low, x_high, z_low, z_high = head_projection.bounds
    ear_x = x_low + (ear_column - head_projection.offset - head_projection.left) / (head_projection.right - head_projection.left) * (x_high - x_low)
    ear_z = z_low + (head_projection.bottom - EAR_PIXEL[1]) / (head_projection.bottom - head_projection.top) * (z_high - z_low)

    override = mesh.attributes.get("override") or mesh.attributes.new("override", "BYTE_COLOR", "CORNER")
    hair_pattern = mesh.attributes.get("hair_pattern") or mesh.attributes.new("hair_pattern", "FLOAT", "CORNER")
    overridden = 0
    for polygon in mesh.polygons:
        first = polygon.vertices[0]
        is_head, _ = vertex_point[first]
        center = mathutils.Vector((0.0, 0.0, 0.0))
        for index in polygon.vertices:
            center += vertex_point[index][1]
        center /= len(polygon.vertices)
        projection = projections["head" if is_head else "body"]
        color, _ = atlas_color(pixels, *projection.pixel(center))
        normal = polygon.normal
        replacement = None
        if is_head:
            at_ear = ((center.x - ear_x) ** 2 + (center.z - ear_z) ** 2) ** 0.5 < EAR_RADIUS
            if normal.y > 0.25 or (normal.y > -0.15 and not at_ear and near(color, SKIN_COLORS, 90)):
                replacement = HAIR
        elif head_from_sprite and center.z > HEAD_START_Z:
            # his own head: whatever turns away from the front is hair (his drawings never show it), except his ear
            # the ear: near his ear in the picture and facing sideways or forward, never backward
            at_ear = (((center.x - sprite_ear_x) ** 2 + (center.z - sprite_ear_z) ** 2) ** 0.5 < SPRITE_EAR_RADIUS
                      and normal.x < -0.3 and normal.y < 0.2)
            if normal.y > -0.35 and not at_ear:
                replacement = "pattern"
        elif normal.y > 0.25 and near(color, SHIRT_COLORS, 40):
            replacement = JACKET
        pattern = 1.0 if replacement == "pattern" else 0.0
        value = linear(replacement) if replacement and not pattern else (0.0, 0.0, 0.0, 0.0)
        if replacement:
            overridden += 1
        for loop_index in polygon.loop_indices:
            override.data[loop_index].color = value
            hair_pattern.data[loop_index].value = pattern
    mesh.materials.clear()
    mesh.materials.append(material(atlas_path))
    return len(mesh.polygons), overridden
