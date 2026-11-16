"""Cleans the AI model's surface and colors: smooth shapes and solid color regions in his palette.

Run inside the live Blender after ai_texture.py, ai_parts.py and ai_view_texture.py:
exec(open(path).read()); clean()

The colors came from three sources (his projected sprite, the painted views, part colors) and showed as
speckles and streaks. Every face now gets one color of his palette: what it showed before, snapped to the
palette. Then a few majority rounds over neighbouring faces and the removal of regions smaller than about one
sprite pixel leave solid regions, the way he paints clusters. His face (eyes, mouth, ear) is kept exactly as
projected. The mesh gets more volume preserving smoothing against the AI's lumps, the face left as it is.
"""

import bpy
import numpy

# his palette without the book's prop colors
PALETTE = numpy.array([
    (0, 0, 0), (2, 16, 41), (6, 37, 101), (7, 32, 76), (10, 21, 42), (12, 27, 53), (12, 34, 72), (13, 10, 19),
    (13, 63, 83), (20, 11, 39), (22, 13, 39), (26, 60, 151), (31, 133, 125), (32, 15, 44), (33, 17, 47), (47, 22, 47),
    (50, 22, 61), (50, 25, 60), (51, 20, 35), (57, 173, 135), (60, 26, 77), (61, 30, 74), (64, 26, 71), (71, 27, 51),
    (78, 35, 55), (82, 32, 55), (83, 38, 82), (95, 34, 78), (100, 34, 58), (100, 117, 50), (102, 30, 80), (119, 49, 81),
    (140, 40, 92), (155, 44, 105), (171, 50, 109), (188, 54, 116), (188, 73, 79), (190, 107, 69), (194, 59, 115),
    (208, 67, 73), (212, 66, 117), (214, 67, 118), (219, 150, 107)], dtype=numpy.float64)
# black is his outline and his eyes: only the protected face may keep it
BLACK = 0
HAIR = (188, 54, 116)
# his face: head faces looking at the sprite camera keep their projected pixels
HEAD_START_Z = 0.27
FACE_FACING = -0.3
# away from the sprite camera only the material counts: one base tone each, the toon ramp shades it
MATERIALS = {
    "hair": ((188, 54, 116), [(102, 30, 80), (140, 40, 92), (155, 44, 105), (171, 50, 109), (188, 54, 116), (194, 59, 115), (212, 66, 117), (214, 67, 118)]),
    "skin": ((219, 150, 107), [(219, 150, 107), (190, 107, 69)]),
    "trim": ((188, 73, 79), [(188, 73, 79), (208, 67, 73)]),
    "shirt": ((31, 133, 125), [(31, 133, 125), (57, 173, 135), (13, 63, 83), (100, 117, 50)]),
    "trousers": ((6, 37, 101), [(2, 16, 41), (6, 37, 101), (7, 32, 76), (10, 21, 42), (12, 27, 53), (12, 34, 72), (26, 60, 151)]),
}
JACKET = (100, 34, 58)
BOOTS = (32, 15, 44)
# boots below this height, everything else not listed above is jacket
BOOTS_TOP = -0.8
# his ear faces screen left (-x): skin on the head away from the camera stays skin only there
EAR_FACING_X = -0.5
# faces whose normal.y is below this look at the sprite camera and keep his cleaned detail colors
FRONT_FACING = -0.45
MAJORITY_ROUNDS = 3
# regions smaller than this many faces (about one sprite pixel of surface) melt into their neighbours
SMALLEST_REGION = 30
SMOOTH_ROUNDS = 15
SMOOTH_FACTOR = 0.5
SMOOTH_TOP_Z = 0.22


def to_srgb(values):
    values = numpy.clip(numpy.asarray(values, dtype=numpy.float64), 0.0, 1.0)
    return numpy.where(values <= 0.0031308, values * 12.92, 1.055 * values ** (1 / 2.4) - 0.055) * 255.0


def to_linear(values):
    values = numpy.asarray(values, dtype=numpy.float64) / 255.0
    return numpy.where(values <= 0.04045, values / 12.92, ((values + 0.055) / 1.055) ** 2.4)


def taubin(mesh, movable, rounds, factor):
    coordinates = numpy.zeros(len(mesh.vertices) * 3)
    mesh.vertices.foreach_get("co", coordinates)
    coordinates = coordinates.reshape(-1, 3)
    edges = numpy.zeros(len(mesh.edges) * 2, dtype=numpy.int64)
    mesh.edges.foreach_get("vertices", edges)
    edges = edges.reshape(-1, 2)
    counts = numpy.zeros(len(coordinates))
    numpy.add.at(counts, edges[:, 0], 1)
    numpy.add.at(counts, edges[:, 1], 1)
    for _ in range(rounds):
        for step in (factor, -factor - 0.03):
            sums = numpy.zeros_like(coordinates)
            numpy.add.at(sums, edges[:, 0], coordinates[edges[:, 1]])
            numpy.add.at(sums, edges[:, 1], coordinates[edges[:, 0]])
            change = sums / numpy.maximum(counts, 1)[:, None] - coordinates
            coordinates[movable] += step * change[movable]
    mesh.vertices.foreach_set("co", coordinates.ravel())
    mesh.update()


def face_colors(mesh):
    """Each face's current color in sRGB 0..255: override where set, else his projected atlas pixel (hair where
    the shader drew the lock pattern)."""
    image = next(node for node in mesh.materials[0].node_tree.nodes if node.type == "TEX_IMAGE").image
    width, height = image.size
    pixels = numpy.array(image.pixels[:]).reshape(height, width, 4)
    loop_count = len(mesh.loops)
    override = numpy.zeros(loop_count * 4)
    mesh.attributes["override"].data.foreach_get("color", override)
    override = override.reshape(-1, 4)
    pattern = numpy.zeros(loop_count)
    mesh.attributes["hair_pattern"].data.foreach_get("value", pattern)
    uv = numpy.zeros(loop_count * 2)
    mesh.uv_layers["projection"].data.foreach_get("uv", uv)
    uv = uv.reshape(-1, 2)
    starts = numpy.zeros(len(mesh.polygons), dtype=numpy.int64)
    mesh.polygons.foreach_get("loop_start", starts)
    columns = numpy.clip((uv[starts, 0] * width).astype(int), 0, width - 1)
    rows = numpy.clip((uv[starts, 1] * height).astype(int), 0, height - 1)
    # the atlas holds sRGB values
    colors = pixels[rows, columns, :3] * 255.0
    overridden = override[starts, 3] > 0.5
    colors[overridden] = to_srgb(override[starts][overridden, :3])
    colors[pattern[starts] > 0.5] = HAIR
    return colors


def neighbours_of(mesh):
    edge_faces = {}
    for polygon in mesh.polygons:
        for key in polygon.edge_keys:
            edge_faces.setdefault(key, []).append(polygon.index)
    neighbours = [[] for _ in mesh.polygons]
    for faces in edge_faces.values():
        for face in faces:
            neighbours[face].extend(other for other in faces if other != face)
    return neighbours


def clean_labels(labels, neighbours, protected):
    for _ in range(MAJORITY_ROUNDS):
        updated = labels.copy()
        for index, others in enumerate(neighbours):
            if protected[index] or not others:
                continue
            votes = numpy.bincount(labels[others], minlength=len(PALETTE))
            votes[labels[index]] += 1
            updated[index] = int(numpy.argmax(votes))
        labels = updated
    # small regions melt into the neighbour label they touch most
    seen = numpy.zeros(len(labels), bool)
    for start in range(len(labels)):
        if seen[start] or protected[start]:
            continue
        region, stack = [], [start]
        seen[start] = True
        while stack:
            face = stack.pop()
            region.append(face)
            for other in neighbours[face]:
                if not seen[other] and labels[other] == labels[start] and not protected[other]:
                    seen[other] = True
                    stack.append(other)
        if len(region) < SMALLEST_REGION:
            members = set(region)
            border = [labels[other] for face in region for other in neighbours[face] if other not in members]
            if border:
                labels[region] = numpy.bincount(border, minlength=len(PALETTE)).argmax()
    return labels


def hand_dominant_faces(model):
    """Faces whose vertices the hand bones move most: his gloves."""
    mesh = model.data
    names = {group.index: group.name for group in model.vertex_groups}
    is_hand = numpy.zeros(len(mesh.vertices), bool)
    for vertex in mesh.vertices:
        best = max(vertex.groups, key=lambda element: element.weight, default=None)
        is_hand[vertex.index] = bool(best) and names[best.group].startswith("hand")
    return numpy.array([is_hand[list(polygon.vertices)].mean() > 0.5 for polygon in mesh.polygons])


def clean():
    model = bpy.data.objects["ai_adam"]
    mesh = model.data
    coordinates = numpy.zeros(len(mesh.vertices) * 3)
    mesh.vertices.foreach_get("co", coordinates)
    coordinates = coordinates.reshape(-1, 3)
    normals = numpy.zeros(len(mesh.polygons) * 3)
    mesh.polygons.foreach_get("normal", normals)
    normals = normals.reshape(-1, 3)
    centers = numpy.zeros(len(mesh.polygons) * 3)
    mesh.polygons.foreach_get("center", centers)
    centers = centers.reshape(-1, 3)
    # his face stays as projected; the smoothing leaves it alone too
    protected = (centers[:, 2] > HEAD_START_Z) & (normals[:, 1] < FACE_FACING)
    face_vertex = numpy.zeros(len(mesh.vertices), bool)
    for polygon in numpy.nonzero(protected)[0]:
        face_vertex[list(mesh.polygons[polygon].vertices)] = True
    taubin(mesh, (coordinates[:, 2] < SMOOTH_TOP_Z) | ((coordinates[:, 2] >= SMOOTH_TOP_Z) & ~face_vertex), SMOOTH_ROUNDS, SMOOTH_FACTOR)
    colors = face_colors(mesh)
    distances = ((colors[:, None, :] - PALETTE[None, :, :]) ** 2).sum(axis=-1)
    distances[~protected, BLACK] = numpy.inf
    labels = distances.argmin(axis=1)
    neighbours = neighbours_of(mesh)
    labels = clean_labels(labels, neighbours, protected)
    # away from the camera: the material's base tone (as a palette index), cleaned once more at material level
    base_index = {}
    for name, (base, members) in MATERIALS.items():
        for member in members:
            base_index[tuple(member)] = int(numpy.argmin(((PALETTE - base) ** 2).sum(axis=1)))
    jacket_index = int(numpy.argmin(((PALETTE - JACKET) ** 2).sum(axis=1)))
    boots_index = int(numpy.argmin(((PALETTE - BOOTS) ** 2).sum(axis=1)))
    away = ~protected & (normals[:, 1] >= FRONT_FACING)
    skin_colors = [tuple(member) for member in MATERIALS["skin"][1]]
    hair_index = base_index[tuple(MATERIALS["hair"][0])]
    hand_faces = hand_dominant_faces(model)
    for index in numpy.nonzero(away)[0]:
        color = tuple(int(value) for value in PALETTE[labels[index]])
        is_skin = color in skin_colors
        if hand_faces[index]:
            # his gloves
            labels[index] = boots_index
        elif centers[index, 2] > HEAD_START_Z:
            # the head away from the camera is hair, except his ear (on the side turned to screen left)
            at_ear = is_skin and normals[index, 0] < EAR_FACING_X and normals[index, 1] < 0.2
            labels[index] = base_index[skin_colors[0]] if at_ear else hair_index
        elif is_skin:
            # skin below the head away from the camera is a stray ear or hand from the views: jacket
            labels[index] = jacket_index
        elif centers[index, 2] < BOOTS_TOP:
            labels[index] = boots_index
        else:
            labels[index] = base_index.get(color, jacket_index)
    labels = clean_labels(labels, neighbours, ~away)
    linear = to_linear(PALETTE[labels])
    override = mesh.attributes["override"]
    pattern = mesh.attributes["hair_pattern"]
    corner_colors = numpy.zeros((len(mesh.loops), 4))
    corner_pattern = numpy.zeros(len(mesh.loops))
    old = numpy.zeros(len(mesh.loops) * 4)
    override.data.foreach_get("color", old)
    old = old.reshape(-1, 4)
    for polygon in mesh.polygons:
        loops = list(polygon.loop_indices)
        if protected[polygon.index]:
            corner_colors[loops] = old[loops]
        else:
            corner_colors[loops, :3] = linear[polygon.index]
            corner_colors[loops, 3] = 1.0
    override.data.foreach_set("color", corner_colors.ravel())
    pattern.data.foreach_set("value", corner_pattern)
    mesh.update()
    return int(protected.sum()), len(numpy.unique(labels))
