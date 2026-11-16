"""Colors the combined AI model (ai_combine.py) from his own drawings, by projection along the view axis.

Run inside the live Blender: exec(open(path).read()); paint()

Hunyuan3D builds a mesh whose front view matches its input picture: the picture's figure box spans the
mesh's x and z extent. So every body vertex takes the color of the sprite input picture at its (x, z),
and every head vertex the color of the portrait input picture (the head is first moved back into the
portrait generation's space). Surfaces facing away get what the front cannot show: the back of the
head is hair, the back of the body has no shirt stripe. Two color sets: "natural" keeps the portrait's
colors on the head (for looking at the model), "outfit" has everything in his sprite palette (for
sprite renders). The material shades them flat (toon) and shows "natural"; sprite renders switch it.
"""

import bpy
import mathutils
import numpy

BODY_PICTURE = r"D:\ai3d\work\idle_no_outline.png"
HEAD_PICTURE = r"D:\ai3d\work\portrait_input.png"
BODY_PREFIX = "ai_adam_idle_no_outline"
HEAD_PREFIX = "ai_adam_portrait_input"

HAIR = (196, 42, 118)
JACKET = (100, 34, 58)
SHIRT_COLORS = ((31, 133, 125), (57, 173, 135), (13, 63, 83))
SKIN_COLORS = ((219, 150, 107), (190, 107, 69), (240, 190, 150), (232, 170, 128))
# the colors of his idle sprite below the head: the body is snapped to them for clean flat areas
BODY_PALETTE = [(100, 34, 58), (71, 27, 51), (119, 49, 81), (208, 67, 73), (31, 133, 125), (57, 173, 135), (13, 63, 83),
                (6, 37, 101), (26, 60, 151), (2, 16, 41), (32, 15, 44), (47, 22, 47), (50, 22, 61), (60, 26, 77), (20, 11, 39),
                (100, 117, 50)]
# his visible ear in the portrait (pixel 25, 48 of 82 x 78), in portrait_input.png: 10x and centred on 1024
EAR_PIXEL = (102 + 25 * 10 + 5, 122 + 48 * 10 + 5)
EAR_RADIUS = 0.16
# his head colors in the sprite: what the portrait colors become in the sprite color set
SPRITE_HAIR = [(140, 40, 92), (155, 44, 105), (188, 54, 116), (214, 67, 118), (102, 30, 80), (171, 50, 109)]
SPRITE_SKIN = [(219, 150, 107), (190, 107, 69)]
SPRITE_EYE = (51, 20, 35)


def load_picture(path):
    image = bpy.data.images.load(path, check_existing=True)
    width, height = image.size
    pixels = numpy.array(image.pixels[:], dtype=numpy.float32).reshape(height, width, 4)
    # rows are stored bottom up; an 8 bit picture's pixels are its stored sRGB values, not linear
    opaque = pixels[..., 3] > 0.5
    rows, columns = numpy.where(opaque)
    return pixels, (columns.min(), columns.max(), rows.min(), rows.max())


def generation_bounds(prefix):
    obj = next(item for item in bpy.data.objects if item.name.startswith(prefix) and item.type == "MESH")
    points = [obj.matrix_world @ vertex.co for vertex in obj.data.vertices]
    return (min(point.x for point in points), max(point.x for point in points), min(point.z for point in points), max(point.z for point in points))


def sample(picture, box, bounds, point):
    pixels, (left, right, bottom, top) = picture
    x_low, x_high, z_low, z_high = bounds
    column = left + (point.x - x_low) / (x_high - x_low) * (right - left)
    row = bottom + (point.z - z_low) / (z_high - z_low) * (top - bottom)
    column = int(min(max(round(column), 0), pixels.shape[1] - 1))
    row = int(min(max(round(row), 0), pixels.shape[0] - 1))
    # step inward until an opaque pixel: points at the silhouette edge can land just outside the figure
    for radius in range(0, 12):
        window = pixels[max(0, row - radius):row + radius + 1, max(0, column - radius):column + radius + 1]
        opaque = window[..., 3] > 0.5
        if opaque.any():
            return tuple(int(round(value * 255)) for value in window[opaque].mean(axis=0)[:3])
    return JACKET


def linear(color):
    def channel(value):
        value /= 255.0
        return value / 12.92 if value <= 0.04045 else ((value + 0.055) / 1.055) ** 2.4
    return (channel(color[0]), channel(color[1]), channel(color[2]), 1.0)


def nearest(color, palette):
    return min(palette, key=lambda entry: sum((first - second) ** 2 for first, second in zip(color, entry)))


def near(color, palette, tolerance=40):
    return any(sum(abs(first - second) for first, second in zip(color, entry)) < tolerance for entry in palette)


def toon_material(name="ai_adam_material"):
    material = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    material.use_nodes = True
    tree = material.node_tree
    tree.nodes.clear()
    attribute = tree.nodes.new("ShaderNodeVertexColor")
    attribute.layer_name = "natural"
    diffuse = tree.nodes.new("ShaderNodeBsdfDiffuse")
    to_rgb = tree.nodes.new("ShaderNodeShaderToRGB")
    brightness = tree.nodes.new("ShaderNodeRGBToBW")
    ramp = tree.nodes.new("ShaderNodeValToRGB")
    ramp.color_ramp.interpolation = "CONSTANT"
    ramp.color_ramp.elements[0].position = 0.0
    ramp.color_ramp.elements[0].color = (0.6, 0.6, 0.6, 1.0)
    ramp.color_ramp.elements[1].position = 0.2
    ramp.color_ramp.elements[1].color = (1.0, 1.0, 1.0, 1.0)
    multiply = tree.nodes.new("ShaderNodeMix")
    multiply.data_type = "RGBA"
    multiply.blend_type = "MULTIPLY"
    multiply.inputs[0].default_value = 1.0
    emission = tree.nodes.new("ShaderNodeEmission")
    output = tree.nodes.new("ShaderNodeOutputMaterial")
    tree.links.new(diffuse.outputs["BSDF"], to_rgb.inputs["Shader"])
    tree.links.new(to_rgb.outputs["Color"], brightness.inputs["Color"])
    tree.links.new(brightness.outputs["Val"], ramp.inputs["Fac"])
    # a color mix node repeats its socket names per data type: 6 and 7 are the colors, output 2 the color
    tree.links.new(attribute.outputs["Color"], multiply.inputs[6])
    tree.links.new(ramp.outputs["Color"], multiply.inputs[7])
    tree.links.new(multiply.outputs[2], emission.inputs["Color"])
    tree.links.new(emission.outputs["Emission"], output.inputs["Surface"])
    return material


def sprite_head_color(color):
    """A portrait color in his sprite palette: hair to his hair shades, skin to his two skin tones, eyes and brows dark."""
    red, green, blue = color
    if red > green + 60 and blue > green + 25:
        return nearest(color, SPRITE_HAIR)
    if red + green + blue < 200 or blue > red + 30:
        return SPRITE_EYE
    if min(color) > 190:
        # the white of the eye: at sprite size his eyes are dark pixels on skin
        return SPRITE_SKIN[0]
    return nearest(color, SPRITE_SKIN)


def write_colors(mesh, name, colors):
    attribute = mesh.attributes.get(name) or mesh.attributes.new(name, "BYTE_COLOR", "CORNER")
    for loop in mesh.loops:
        attribute.data[loop.index].color = linear(colors[loop.vertex_index])


def paint():
    model = bpy.data.objects["ai_adam"]
    mesh = model.data
    body_count = model["body_vertex_count"]
    values = list(model["head_to_body"])
    to_body = mathutils.Matrix([values[0:4], values[4:8], values[8:12], values[12:16]])
    to_portrait = to_body.inverted()
    body_picture = load_picture(BODY_PICTURE)
    head_picture = load_picture(HEAD_PICTURE)
    body_bounds = generation_bounds(BODY_PREFIX)
    head_bounds = generation_bounds(HEAD_PREFIX)
    # the ear in the portrait generation's space: picture column and row (top down) to x and z
    _, (left, right, bottom, top) = head_picture
    ear_row = head_picture[0].shape[0] - 1 - EAR_PIXEL[1]
    ear_x = head_bounds[0] + (EAR_PIXEL[0] - left) / (right - left) * (head_bounds[1] - head_bounds[0])
    ear_z = head_bounds[2] + (ear_row - bottom) / (top - bottom) * (head_bounds[3] - head_bounds[2])
    colors = []
    sprite_colors = []
    for vertex in mesh.vertices:
        facing_away = vertex.normal.y > 0.25
        if vertex.index >= body_count:
            point = to_portrait @ vertex.co
            color = HAIR if facing_away else sample(head_picture, None, head_bounds, point)
            # the face seen from the front spills onto the sides of the head; there he has hair, except at his ear
            facing_side = vertex.normal.y > -0.15
            at_ear = ((point.x - ear_x) ** 2 + (point.z - ear_z) ** 2) ** 0.5 < EAR_RADIUS
            if facing_side and not at_ear and near(color, SKIN_COLORS, 90):
                color = HAIR
            sprite_colors.append(sprite_head_color(color))
        else:
            color = nearest(sample(body_picture, None, body_bounds, vertex.co), BODY_PALETTE)
            if facing_away and near(color, SHIRT_COLORS):
                color = JACKET
            sprite_colors.append(color)
        colors.append(color)
    # "natural": his portrait colors on the head, for looking at the model; "outfit": everything in his sprite palette
    write_colors(mesh, "natural", colors)
    write_colors(mesh, "outfit", sprite_colors)
    mesh.materials.clear()
    mesh.materials.append(toon_material())
    return len(colors)
