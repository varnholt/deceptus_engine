"""Paints the AI model's body by material, so every side of it has his colors, not front-projection leftovers.

Run inside the live Blender after ai_texture.py: exec(open(path).read()); paint_parts()

The front projection (ai_texture.py) is only right from his drawing's angle. Once a limb swings or the torso
turns, surfaces that were edge-on in his sprite face the camera and show stretched streaks (the coral trim and
the green shirt spread over the jacket). Here every body face gets a material: what his drawing shows at its
place decides jacket, trousers or boots (bones cannot: the coat tails hang over the thighs), and faces the arm
bones move almost alone are sleeve, cuff or glove. Faces turned toward the camera keep his projected pixels
(shirt stripes, trim, folds), all others show their material's mid tone; the material's toon ramp shades it.
The head keeps everything ai_texture.py gave it.
"""

import bpy

# his mid tones per material (his run frames' most used color of it)
JACKET = (100, 34, 58)
SLEEVE = (83, 38, 82)
CUFF = (188, 73, 79)
GLOVE = (32, 15, 44)
TROUSERS = (6, 37, 101)
BOOTS = (32, 15, 44)
# his palette sorted into materials, for reading what his drawing shows
MATERIAL_COLORS = {
    "trousers": ((6, 37, 101), (26, 60, 151), (12, 27, 53), (10, 21, 42), (12, 34, 72), (7, 32, 76), (2, 16, 41)),
    "boots": ((32, 15, 44), (22, 13, 39), (47, 22, 47), (13, 10, 19), (20, 11, 39), (33, 17, 47)),
    "jacket": ((100, 34, 58), (119, 49, 81), (71, 27, 51), (51, 20, 35), (78, 35, 55), (82, 32, 55), (83, 38, 82),
               (95, 34, 78), (50, 25, 60), (61, 30, 74), (60, 26, 77), (64, 26, 71), (50, 22, 61),
               (188, 73, 79), (208, 67, 73), (31, 133, 125), (57, 173, 135), (13, 63, 83),
               (219, 150, 107), (190, 107, 69), (0, 0, 0)),
}
MATERIAL_TONES = {"trousers": TROUSERS, "boots": BOOTS, "jacket": JACKET}
# heights (model space, feet at -1) above which boots and trousers cannot be: his far arm is drawn in boot tones
BOOTS_TOP = -0.8
TROUSERS_TOP = -0.4
# how far a face may turn from the camera (normal.y, the camera looks along +y) and keep the projection
FRONT_FACING = -0.45
# share of a vertex's weight the arm bones must hold for it to be sleeve (shoulders stay jacket)
ARM_SHARE = 0.8
# the cuff: the last part of the lower arm toward the hand
CUFF_FRACTION = 0.75
SMOOTHING_ROUNDS = 2


def linear(color):
    def channel(value):
        value /= 255.0
        return value / 12.92 if value <= 0.04045 else ((value + 0.055) / 1.055) ** 2.4
    return (channel(color[0]), channel(color[1]), channel(color[2]), 1.0)


def bone_part(name):
    if name.startswith(("upperarm", "lowerarm")):
        return "arm"
    if name.startswith("hand"):
        return "hand"
    if name in ("Head", "neck_01"):
        return "head"
    return "body"


def drawn_material(color):
    """The material whose palette entry is closest to a color of his drawing."""
    best = None
    for name, entries in MATERIAL_COLORS.items():
        for entry in entries:
            distance = sum(abs(first - second) for first, second in zip(color, entry))
            if best is None or distance < best[0]:
                best = (distance, name)
    return best[1]


def smooth(labels, neighbours, rounds):
    for _ in range(rounds):
        smoothed = list(labels)
        for index, others in enumerate(neighbours):
            votes = {labels[index]: 1}
            for other in others:
                votes[labels[other]] = votes.get(labels[other], 0) + 1
            smoothed[index] = max(votes, key=votes.get)
        labels = smoothed
    return labels


def paint_parts():
    model = bpy.data.objects["ai_adam"]
    rig = bpy.data.objects["ai_rig"]
    mesh = model.data
    group_names = {group.index: group.name for group in model.vertex_groups}
    vertex_part = []
    vertex_bone = []
    for vertex in mesh.vertices:
        weights = {}
        for element in vertex.groups:
            part = bone_part(group_names[element.group])
            weights[part] = weights.get(part, 0.0) + element.weight
        total = sum(weights.values()) or 1.0
        strongest = max(vertex.groups, key=lambda element: element.weight, default=None)
        part = max(weights, key=weights.get) if weights else "body"
        if part in ("arm", "hand") and weights[part] / total < ARM_SHARE:
            part = "body"
        vertex_part.append(part)
        vertex_bone.append(group_names[strongest.group] if strongest else "")
    # his drawing at each face, read through the projection UV from the atlas (rows bottom up, sRGB 0..1)
    image = next(node for node in mesh.materials[0].node_tree.nodes if node.type == "TEX_IMAGE").image
    width, height = image.size
    pixels = image.pixels[:]
    uv_layer = mesh.uv_layers["projection"]
    face_part = []
    for polygon in mesh.polygons:
        votes = {}
        for index in polygon.vertices:
            votes[vertex_part[index]] = votes.get(vertex_part[index], 0) + 1
        part = max(votes, key=votes.get)
        if part == "body":
            uv = uv_layer.data[polygon.loop_indices[0]].uv
            column, row = min(width - 1, max(0, int(uv.x * width))), min(height - 1, max(0, int(uv.y * height)))
            offset = (row * width + column) * 4
            part = drawn_material(tuple(round(value * 255) for value in pixels[offset:offset + 3]))
            if (part == "boots" and polygon.center.z > BOOTS_TOP) or (part == "trousers" and polygon.center.z > TROUSERS_TOP):
                part = "jacket"
        face_part.append(part)
    # neighbours by shared edges; a few majority rounds remove single stray faces on material borders
    edge_faces = {}
    for polygon in mesh.polygons:
        for key in polygon.edge_keys:
            edge_faces.setdefault(key, []).append(polygon.index)
    neighbours = [[] for _ in mesh.polygons]
    for faces in edge_faces.values():
        for face in faces:
            neighbours[face].extend(other for other in faces if other != face)
    face_part = smooth(face_part, neighbours, SMOOTHING_ROUNDS)
    override = mesh.attributes["override"]
    counts = {}
    for polygon in mesh.polygons:
        part = face_part[polygon.index]
        counts[part] = counts.get(part, 0) + 1
        if part == "head":
            continue
        color = None
        if part == "arm":
            color = SLEEVE
            # the cuff: faces of the lower arm close to the hand
            bone_name = vertex_bone[polygon.vertices[0]]
            if bone_name.startswith("lowerarm"):
                bone = rig.data.bones[bone_name]
                axis = bone.tail_local - bone.head_local
                if (polygon.center - bone.head_local).dot(axis) / axis.length_squared > CUFF_FRACTION:
                    color = CUFF
        elif part == "hand":
            color = GLOVE
        elif polygon.normal.y >= FRONT_FACING:
            # turned away from the camera: his drawing cannot know it, so it shows the material's tone
            color = MATERIAL_TONES[part]
        value = linear(color) if color else (0.0, 0.0, 0.0, 0.0)
        for loop_index in polygon.loop_indices:
            override.data[loop_index].color = value
    return counts
