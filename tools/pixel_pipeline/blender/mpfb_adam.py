"""Colors the MPFB character (models/adam_mpfb.blend) like dstar's player sprite, in his palette.

Run inside the live Blender: exec(open(path).read()); texture_adam()

Every piece gets his tones per face: the mid tone in "adam_color", his lit and shadow tones of the same material
in "tone_light" and "tone_dark"; one toon material shows the tone the light picks (flat emission). The outfit (male_casualsuit05) is one mesh
whose loose pieces are jacket, jeans, shirt and collar. Then his details: red trim along the jacket's open edges (hem, cuffs, collar),
the shirt's stripes (two dark rows, one light row, at his pixel size), dark gloves on the hands.
"""

import math

import bpy
import numpy

SKIN = (219, 150, 107)
GLOVES = (50, 22, 61)
JACKET = (100, 34, 58)
TRIM = (208, 67, 73)
SHIRT_DARK = (31, 133, 125)
SHIRT_LIGHT = (57, 173, 135)
JEANS = (6, 37, 101)
BOOTS = (32, 15, 44)
HAIR = (140, 40, 92)
EYEBROWS = (102, 30, 80)
# his tones per material (counted in his idle frame): mid tone -> (lit tone, shadow tone); the light picks one
TONES = {
    SKIN: ((219, 150, 107), (190, 107, 69)),
    GLOVES: ((60, 26, 77), (32, 15, 44)),
    JACKET: ((119, 49, 81), (71, 27, 51)),
    TRIM: ((208, 67, 73), (188, 73, 79)),
    SHIRT_DARK: ((31, 133, 125), (13, 63, 83)),
    JEANS: ((26, 60, 151), (2, 16, 41)),
    BOOTS: ((47, 22, 47), (20, 11, 39)),
    HAIR: ((188, 54, 116), (102, 30, 80)),
}
# his figure is 42 pixels for the character's height: one sprite pixel in model units
FIGURE_PIXELS = 42
# trim width along the jacket's open edges, in sprite pixels
TRIM_PIXELS = 1.0
# his standing collar: the jacket's top band around the neck shows his trim red, this many sprite pixels deep
COLLAR_PIXELS = 2.5
# his shirt shows 3 sprite pixels wide down the middle of the chest
OPENING_PIXELS = 4.5
# his boots reach this many sprite pixels above the shoes' top
BOOT_SHAFT_PIXELS = 0.0
BUCKLE = (100, 117, 50)
EYES = (51, 20, 35)
# his eyes are the smooth dark ovals adam_eye_l / adam_eye_r on the face; painting them into the face mesh gave
# jagged patches, so it is off
PAINT_EYES = False
# the built hair (mpfb_hair_build.py) covers the scalp; painting the scalp left a jagged band on the forehead
PAINT_SCALP = True
# his eyes: dark ovals, about one sprite pixel wide and two tall; painted on the face around each eyeball
EYE_WIDTH_FACTOR = 0.8
EYE_HEIGHT_FACTOR = 1.4
# hand bones move the gloves
GLOVE_BONES = ("hand", "index", "middle", "pinky", "ring", "thumb")


def to_linear(color):
    values = numpy.asarray(color, dtype=numpy.float64) / 255.0
    return numpy.where(values <= 0.04045, values / 12.92, ((values + 0.055) / 1.055) ** 2.4)


def toon_material():
    material = bpy.data.materials.get("adam_toon") or bpy.data.materials.new("adam_toon")
    material.use_nodes = True
    tree = material.node_tree
    tree.nodes.clear()
    # his three tones per face (lit, mid, shadow); the light's brightness picks one, thresholds "tone_low"/"tone_high"
    color = tree.nodes.new("ShaderNodeVertexColor")
    color.layer_name = "adam_color"
    lit = tree.nodes.new("ShaderNodeVertexColor")
    lit.layer_name = "tone_light"
    dark = tree.nodes.new("ShaderNodeVertexColor")
    dark.layer_name = "tone_dark"
    diffuse = tree.nodes.new("ShaderNodeBsdfDiffuse")
    to_rgb = tree.nodes.new("ShaderNodeShaderToRGB")
    brightness = tree.nodes.new("ShaderNodeRGBToBW")
    low = tree.nodes.new("ShaderNodeValue")
    low.name = "tone_low"
    low.outputs[0].default_value = 0.2
    high = tree.nodes.new("ShaderNodeValue")
    high.name = "tone_high"
    high.outputs[0].default_value = 0.8
    above_low = tree.nodes.new("ShaderNodeMath")
    above_low.operation = "GREATER_THAN"
    above_high = tree.nodes.new("ShaderNodeMath")
    above_high.operation = "GREATER_THAN"
    mid_or_dark = tree.nodes.new("ShaderNodeMix")
    mid_or_dark.data_type = "RGBA"
    toned = tree.nodes.new("ShaderNodeMix")
    toned.data_type = "RGBA"
    emission = tree.nodes.new("ShaderNodeEmission")
    output = tree.nodes.new("ShaderNodeOutputMaterial")
    tree.links.new(diffuse.outputs["BSDF"], to_rgb.inputs["Shader"])
    tree.links.new(to_rgb.outputs["Color"], brightness.inputs["Color"])
    tree.links.new(brightness.outputs["Val"], above_low.inputs[0])
    tree.links.new(low.outputs[0], above_low.inputs[1])
    tree.links.new(brightness.outputs["Val"], above_high.inputs[0])
    tree.links.new(high.outputs[0], above_high.inputs[1])
    tree.links.new(above_low.outputs[0], mid_or_dark.inputs[0])
    tree.links.new(dark.outputs["Color"], mid_or_dark.inputs[6])
    tree.links.new(color.outputs["Color"], mid_or_dark.inputs[7])
    tree.links.new(above_high.outputs[0], toned.inputs[0])
    tree.links.new(mid_or_dark.outputs[2], toned.inputs[6])
    tree.links.new(lit.outputs["Color"], toned.inputs[7])
    # a color mix node repeats its socket names per data type: 0 is the factor, 6 and 7 the colors, output 2 the color
    # his shirt stripes: rows one sprite pixel high counted from his feet (the sprite camera's bottom row), one light
    # row then two dark ones; mpfb_sprite.py sets the origin and the pixel height for every render
    position = tree.nodes.new("ShaderNodeNewGeometry")
    separate = tree.nodes.new("ShaderNodeSeparateXYZ")
    origin = tree.nodes.new("ShaderNodeValue")
    origin.name = "stripe_origin"
    pixel = tree.nodes.new("ShaderNodeValue")
    pixel.name = "stripe_pixel"
    pixel.outputs[0].default_value = 0.034
    above = tree.nodes.new("ShaderNodeMath")
    above.operation = "SUBTRACT"
    rows = tree.nodes.new("ShaderNodeMath")
    rows.operation = "DIVIDE"
    floor = tree.nodes.new("ShaderNodeMath")
    floor.operation = "FLOOR"
    modulo = tree.nodes.new("ShaderNodeMath")
    modulo.operation = "FLOORED_MODULO"
    modulo.inputs[1].default_value = 3.0
    is_light = tree.nodes.new("ShaderNodeMath")
    is_light.operation = "LESS_THAN"
    is_light.inputs[1].default_value = 0.5
    stripe_color = tree.nodes.new("ShaderNodeMix")
    stripe_color.data_type = "RGBA"
    stripe_color.inputs[6].default_value = (*to_linear(SHIRT_DARK), 1.0)
    stripe_color.inputs[7].default_value = (*to_linear(SHIRT_LIGHT), 1.0)
    stripe_flag = tree.nodes.new("ShaderNodeAttribute")
    stripe_flag.attribute_name = "stripes"
    with_stripes = tree.nodes.new("ShaderNodeMix")
    with_stripes.data_type = "RGBA"
    tree.links.new(position.outputs["Position"], separate.inputs["Vector"])
    tree.links.new(separate.outputs["Z"], above.inputs[0])
    tree.links.new(origin.outputs[0], above.inputs[1])
    tree.links.new(above.outputs[0], rows.inputs[0])
    tree.links.new(pixel.outputs[0], rows.inputs[1])
    tree.links.new(rows.outputs[0], floor.inputs[0])
    tree.links.new(floor.outputs[0], modulo.inputs[0])
    tree.links.new(modulo.outputs[0], is_light.inputs[0])
    tree.links.new(is_light.outputs[0], stripe_color.inputs[0])
    tree.links.new(stripe_flag.outputs["Fac"], with_stripes.inputs[0])
    tree.links.new(toned.outputs[2], with_stripes.inputs[6])
    tree.links.new(stripe_color.outputs[2], with_stripes.inputs[7])
    tree.links.new(with_stripes.outputs[2], emission.inputs["Color"])
    tree.links.new(emission.outputs["Emission"], output.inputs["Surface"])
    return material


def write_face_colors(obj, face_colors, striped=None):
    """face_colors: one sRGB color per face; written per corner so faces stay crisp. striped: faces showing his
    shirt stripes (drawn by the material)."""
    mesh = obj.data
    flags = mesh.attributes.get("stripes") or mesh.attributes.new("stripes", "FLOAT", "FACE")
    flags.data.foreach_set("value", numpy.asarray(striped if striped is not None else [0.0] * len(mesh.polygons), dtype=numpy.float64))
    starts = numpy.zeros(len(mesh.polygons), dtype=numpy.int64)
    totals = numpy.zeros(len(mesh.polygons), dtype=numpy.int64)
    mesh.polygons.foreach_get("loop_start", starts)
    mesh.polygons.foreach_get("loop_total", totals)
    # the mid tone, and his lit and shadow tones for the same material (unlisted colors keep one tone)
    tone_sets = {"tone_light": [TONES.get(tuple(color), (color, color))[0] for color in face_colors],
                 "adam_color": list(face_colors),
                 "tone_dark": [TONES.get(tuple(color), (color, color))[1] for color in face_colors]}
    for name, colors in tone_sets.items():
        attribute = mesh.attributes.get(name) or mesh.attributes.new(name, "BYTE_COLOR", "CORNER")
        linear = to_linear(numpy.asarray(colors, dtype=numpy.float64))
        corners = numpy.zeros((len(mesh.loops), 4))
        corners[:, 3] = 1.0
        for face, (start, total) in enumerate(zip(starts, totals)):
            corners[start:start + total, :3] = linear[face]
        attribute.data.foreach_set("color", corners.ravel())
    mesh.materials.clear()
    mesh.materials.append(toon_material())


def islands(mesh):
    """The mesh's loose pieces as lists of vertex indices, largest first."""
    parent = list(range(len(mesh.vertices)))

    def find(index):
        while parent[index] != index:
            parent[index] = parent[parent[index]]
            index = parent[index]
        return index

    for edge in mesh.edges:
        first, second = find(edge.vertices[0]), find(edge.vertices[1])
        if first != second:
            parent[first] = second
    groups = {}
    for index in range(len(mesh.vertices)):
        groups.setdefault(find(index), []).append(index)
    return sorted(groups.values(), key=len, reverse=True)


def boundary_loops(mesh, vertices):
    """Open edges among the given vertices, joined into loops (each a set of vertex indices)."""
    inside = set(vertices)
    edge_count = {}
    for polygon in mesh.polygons:
        if polygon.vertices[0] in inside:
            for key in polygon.edge_keys:
                edge_count[key] = edge_count.get(key, 0) + 1
    open_edges = [key for key, count in edge_count.items() if count == 1]
    parent = {}

    def find(index):
        parent.setdefault(index, index)
        while parent[index] != index:
            parent[index] = parent[parent[index]]
            index = parent[index]
        return index

    for first, second in open_edges:
        root_first, root_second = find(first), find(second)
        if root_first != root_second:
            parent[root_first] = root_second
    loops = {}
    for first, second in open_edges:
        loops.setdefault(find(first), set()).update((first, second))
    return list(loops.values())


def texture_outfit(obj, pixel, boots_top):
    """The outfit's loose pieces are jacket (largest), jeans (reaches the ankles), shirt and collar (the small piece at
    the neck). The jacket gets his trim along its hem, cuffs and collar edge only, not the pocket flaps."""
    mesh = obj.data
    coordinates = numpy.array([vertex.co for vertex in mesh.vertices])
    pieces = islands(mesh)
    piece_of = numpy.zeros(len(coordinates), dtype=numpy.int64)
    for number, members in enumerate(pieces):
        piece_of[members] = number
    lowest = [coordinates[members][:, 2].min() for members in pieces]
    jeans = int(numpy.argmin(lowest))
    others = [number for number in range(len(pieces)) if number != jeans]
    jacket = others[0]
    shirt = others[1] if len(others) > 1 else None
    names = {jacket: "jacket", jeans: "jeans"}
    if shirt is not None:
        names[shirt] = "shirt"
    for number in others[2:]:
        names[number] = "collar"
    # the jacket's trim edges: the lowest loop (hem), the two loops furthest out (cuffs), the highest loop (collar)
    loops = boundary_loops(mesh, pieces[jacket])
    described = [(loop, coordinates[list(loop)].mean(axis=0)) for loop in loops if len(loop) > 6]
    trim_loops = []
    if described:
        trim_loops.append(min(described, key=lambda item: item[1][2])[0])
        trim_loops.append(max(described, key=lambda item: item[1][2])[0])
        trim_loops.append(min(described, key=lambda item: item[1][0])[0])
        trim_loops.append(max(described, key=lambda item: item[1][0])[0])
    trim_points = coordinates[sorted(set().union(*trim_loops))] if trim_loops else numpy.zeros((0, 3))
    # the trim also runs down the front opening (the user preferred it to collar-only trim)
    jacket_top = coordinates[pieces[jacket]][:, 2].max()
    collar_loop = max(described, key=lambda item: item[1][2])[0] if described else set()
    keep = sorted(set().union(*trim_loops)) if trim_loops else []
    trim_points = coordinates[keep] if keep else numpy.zeros((0, 3))
    jacket_bottom = coordinates[pieces[jacket]][:, 2].min()
    jeans_top = coordinates[pieces[jeans]][:, 2].max()
    colors = []
    striped = []
    counts = {}
    for polygon in mesh.polygons:
        part = names[piece_of[polygon.vertices[0]]]
        counts[part] = counts.get(part, 0) + 1
        center = numpy.array(polygon.center)
        in_opening = (part == "jacket" and abs(center[0]) < OPENING_PIXELS * pixel / 2 and polygon.normal.y < -0.3
                      and jacket_bottom + pixel < center[2] < jacket_top - 2.0 * pixel)
        if part == "jeans":
            if center[2] < boots_top:
                colors.append(BOOTS)
            elif center[2] > jeans_top - pixel and abs(center[0]) < pixel / 2 and polygon.normal.y < -0.5:
                colors.append(BUCKLE)
            else:
                colors.append(JEANS)
        elif part == "shirt" or in_opening:
            colors.append(SHIRT_DARK)
        elif part == "collar":
            colors.append(TRIM)
        else:
            near = len(trim_points) and numpy.min(numpy.linalg.norm(trim_points - center, axis=1)) < TRIM_PIXELS * pixel
            in_collar = center[2] > jacket_top - COLLAR_PIXELS * pixel
            colors.append(TRIM if near or in_collar else JACKET)
        striped.append(1.0 if part == "shirt" or in_opening else 0.0)
    write_face_colors(obj, colors, striped)
    return counts, len(loops)


def shaped_coordinates(obj):
    """Vertex positions with the MakeHuman targets (shape keys) applied, without the rig's pose."""
    mesh = obj.data
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


def eye_centers():
    """The two eyeball centers (rest pose) and the eyeball radius, from the eyes asset."""
    eyes = next(item for item in bpy.data.objects if "poly" in item.name.lower() and item.type == "MESH")
    coordinates = numpy.array([vertex.co for vertex in eyes.data.vertices])
    centers = [coordinates[coordinates[:, 0] < 0].mean(axis=0), coordinates[coordinates[:, 0] >= 0].mean(axis=0)]
    radius = float(numpy.ptp(coordinates[coordinates[:, 0] < 0][:, 2]) / 2)
    return centers, radius


def texture_body(obj, rig):
    names = {group.index: group.name for group in obj.vertex_groups}
    # only bone groups count: the base mesh also has helper and joint groups at full weight
    bones = {bone.name for bone in rig.data.bones}
    glove_vertex = numpy.zeros(len(obj.data.vertices), bool)
    for vertex in obj.data.vertices:
        bone_elements = [element for element in vertex.groups if names[element.group] in bones]
        best = max(bone_elements, key=lambda element: element.weight, default=None)
        glove_vertex[vertex.index] = bool(best) and names[best.group].startswith(GLOVE_BONES)
    centers, radius = eye_centers()
    # the base mesh's own coordinates lack the targets (the enlarged head): faces are placed on the shaped body
    shaped = shaped_coordinates(obj)
    colors = []
    for polygon in obj.data.polygons:
        center = shaped[list(polygon.vertices)].mean(axis=0)
        at_eye = polygon.normal.y < -0.2 and any(
            ((center[0] - eye[0]) / (radius * EYE_WIDTH_FACTOR)) ** 2 + ((center[2] - eye[2]) / (radius * EYE_HEIGHT_FACTOR)) ** 2 < 1.0
            for eye in centers)
        # the scalp: head skin above the eyes or behind them is hair, so no view shows a bald spot under the haircut
        eye_height = (centers[0][2] + centers[1][2]) / 2
        eye_depth = (centers[0][1] + centers[1][1]) / 2
        # the same tilted hairline as the built hair (mpfb_hair_build.py), a little higher so the hair edge covers it
        hairline = eye_height + 0.055 - 0.75 * (center[1] - (eye_depth + 0.08) + 0.1)
        on_scalp = center[2] > hairline and center[2] > eye_height - 0.09
        if at_eye and PAINT_EYES:
            colors.append(EYES)
        elif PAINT_SCALP and on_scalp and not glove_vertex[list(polygon.vertices)].any():
            colors.append(HAIR)
        elif glove_vertex[list(polygon.vertices)].mean() > 0.5:
            colors.append(GLOVES)
        else:
            colors.append(SKIN)
    write_face_colors(obj, colors)


def texture_adam(posed_height=None):
    """posed_height: the posed figure's height (bigger head, shorter legs); one sprite pixel is that over 42, measured
    back in the rest body's units, so stripes and trim come out one sprite pixel wide in the render."""
    body = bpy.data.objects["adam_body"]
    coordinates = numpy.array([vertex.co for vertex in body.data.vertices])
    rest_height = coordinates[:, 2].max() - coordinates[:, 2].min()
    pixel = (posed_height or rest_height) / FIGURE_PIXELS
    result = {}
    for obj in bpy.data.objects:
        if obj.type != "MESH":
            continue
        name = obj.name.lower()
        if obj == body:
            texture_body(obj, body.parent)
        elif "casualsuit" in name:
            shoes = next(item for item in bpy.data.objects if "shoes" in item.name.lower())
            boots_top = max(vertex.co.z for vertex in shoes.data.vertices) + BOOT_SHAFT_PIXELS * pixel
            result["outfit"] = texture_outfit(obj, pixel, boots_top)
        elif "shoes" in name:
            write_face_colors(obj, [BOOTS] * len(obj.data.polygons))
        elif name.startswith("adam_eye_"):
            write_face_colors(obj, [EYES] * len(obj.data.polygons))
        elif "poly" in name or "eyes" in name:
            # the eyeballs sit behind the eye ovals: skin, so the eye sockets do not show
            write_face_colors(obj, [SKIN] * len(obj.data.polygons))
        elif "eyebrow" in name:
            # his sprite shows no eyebrows (the hair covers the brow)
            write_face_colors(obj, [EYEBROWS] * len(obj.data.polygons))
            obj.hide_render = True
            obj.hide_set(True)
        elif "short" in name or "hair" in name:
            write_face_colors(obj, [HAIR] * len(obj.data.polygons))
    return result
