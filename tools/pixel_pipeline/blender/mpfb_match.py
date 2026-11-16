"""Fits the MPFB character to dstar's idle frame: proportions, volumes, stance and turn, scored pixel by pixel.

Run inside the live Blender after mpfb_sprite.py: exec(open(path).read()); match(his_rgba_npy, palette_npy)

Each candidate is rendered from the sprite camera at his size (8x), every 8x8 block takes its most common
color, snapped to his palette, and compared with his frame. The score is the silhouette overlap plus the share
of his inner pixels (not his black outline) that match exactly, so both shape and colors count. A coordinate
search moves one parameter at a time while the score improves; the best values stay applied.
"""

import math

import bpy
import numpy

FRAME = (72, 48)
SCALE = 8
RIG_NAME = "adam_body.rig"
HAIR_HINT = "hair"
JACKET_HINT = "casualsuit"
# name -> (start, step, low, high)
PARAMETERS = {
    "turn": (25.0, 5.0, 0.0, 60.0),
    "head": (2.2, 0.1, 1.5, 3.2),
    "hair": (0.0, 0.01, 0.0, 0.08),
    "jacket": (0.0, 0.005, 0.0, 0.08),
    "legs": (0.8, 0.05, 0.35, 1.0),
    "leg_spread": (6.0, 2.0, 0.0, 20.0),
    "arm_spread": (10.0, 3.0, 0.0, 35.0),
    # dstar turns the face further toward the camera than the body: the head turns back about its own axis
    "head_turn": (-20.0, 5.0, -40.0, 0.0),
    # his fists are big dark blocks
    "hands": (1.6, 0.2, 1.0, 2.6),
    # his collar stands up around the neck
    "collar": (0.03, 0.01, 0.0, 0.1),
    # the head's own width and depth on top of its overall size (his head is wide)
    "head_width": (1.0, 0.05, 0.7, 1.4),
    "head_depth": (1.0, 0.05, 0.7, 1.4),
    # the light decides which sides take his dark tones
    "light_pitch": (50.0, 10.0, 0.0, 90.0),
    "light_yaw": (-30.0, 15.0, -180.0, 180.0),
    # brightness below which his shadow tone shows, and above which his lit tone shows
    "shadow": (0.2, 0.05, 0.0, 0.7),
    "highlight": (0.8, 0.05, 0.3, 1.0),
    # his face ends two rows above the shirt: the head moves up along the neck (in the head bone's own units)
    "head_lift": (0.0, 0.02, 0.0, 0.2),
    # whole-piece shape keys from mpfb_part_keys.py (0 = as loaded)
    "hair_back": (0.0, 0.25, -1.0, 2.0),
    "hair_up": (0.0, 0.25, -1.0, 2.0),
    "hair_grow": (0.0, 0.25, -1.0, 2.0),
    "jacket_short": (0.0, 0.2, 0.0, 1.5),
    "boots_grow": (0.0, 0.25, -1.0, 2.0),
}
# his eyes weigh as much as this many silhouette points (they are a few pixels but carry the face)
EYE_WEIGHT = 0.3


def jacket_group(obj):
    """A vertex group with the outfit's jacket and collar pieces (not the jeans or shirt), so only the jacket grows."""
    group = obj.vertex_groups.get("jacket_volume")
    if group:
        return group.name
    mesh = obj.data
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
    pieces = {}
    for vertex in mesh.vertices:
        pieces.setdefault(find(vertex.index), []).append(vertex.index)
    ordered = sorted(pieces.values(), key=len, reverse=True)
    lowest = [min(mesh.vertices[index].co.z for index in members) for members in ordered]
    jeans = lowest.index(min(lowest))
    others = [members for number, members in enumerate(ordered) if number != jeans]
    group = obj.vertex_groups.new(name="jacket_volume")
    # the jacket is the largest remaining piece, the collar the smallest pieces; the shirt (second) stays
    group.add(others[0], 1.0, "REPLACE")
    for members in others[2:]:
        group.add(members, 1.0, "REPLACE")
    return group.name


def collar_group(obj):
    """The outfit's collar pieces (the smallest loose pieces, around the neck)."""
    group = obj.vertex_groups.get("collar_volume")
    if group:
        return group.name
    jacket_group(obj)
    jacket = obj.vertex_groups["jacket_volume"]
    mesh = obj.data
    members = [vertex.index for vertex in mesh.vertices if any(element.group == jacket.index for element in vertex.groups)]
    top = max(mesh.vertices[index].co.z for index in members)
    group = obj.vertex_groups.new(name="collar_volume")
    # the jacket's collar region: its top few centimeters
    group.add([index for index in members if mesh.vertices[index].co.z > top - 0.08], 1.0, "REPLACE")
    return group.name


def displace(obj, name, strength, vertex_group=""):
    modifier = obj.modifiers.get(name) or obj.modifiers.new(name, "DISPLACE")
    modifier.vertex_group = vertex_group
    modifier.direction = "NORMAL"
    modifier.mid_level = 0.0
    # a new modifier comes last, after the armature, so the volume grows around the posed surface
    modifier.strength = strength


def apply_values(values):
    rig = bpy.data.objects[RIG_NAME]
    bones = rig.pose.bones
    bones["head"].scale = (values["head"] * values.get("head_width", 1.0), values["head"], values["head"] * values.get("head_depth", 1.0))
    bones["head"].location = (0.0, values.get("head_lift", 0.0), 0.0)
    bones["head"].rotation_mode = "XYZ"
    bones["head"].rotation_euler = (0.0, math.radians(values.get("head_turn", 0.0)), 0.0)
    for side in ("l", "r"):
        thigh = bones[f"thigh_{side}"]
        thigh.rotation_mode = "QUATERNION"
        thigh.rotation_quaternion = (1.0, 0.0, 0.0, 0.0)
        thigh.scale = (1.0, values["legs"], 1.0)
    for side in ("l", "r"):
        bones[f"hand_{side}"].scale = (values.get("hands", 1.0),) * 3
    point_arms(rig, values["arm_spread"])
    spread_legs(rig, values["leg_spread"])
    rig.rotation_euler = (0.0, 0.0, math.radians(values["turn"]))
    sun = bpy.data.objects.get("sprite_sun")
    if sun:
        sun.rotation_euler = (math.radians(values.get("light_pitch", 50.0)), 0.0, math.radians(values.get("light_yaw", -30.0)))
    material = bpy.data.materials.get("adam_toon")
    if material:
        material.node_tree.nodes["tone_low"].outputs[0].default_value = values.get("shadow", 0.2)
        material.node_tree.nodes["tone_high"].outputs[0].default_value = values.get("highlight", 0.8)
    for obj in bpy.data.objects:
        if obj.type == "MESH" and obj.data.shape_keys:
            for key_name in ("hair_back", "hair_up", "hair_grow", "jacket_short", "boots_grow"):
                block = obj.data.shape_keys.key_blocks.get(key_name)
                if block:
                    block.slider_min = -1.0
                    block.value = values.get(key_name, 0.0)
    for obj in bpy.data.objects:
        if obj.type != "MESH":
            continue
        if HAIR_HINT in obj.name.lower():
            displace(obj, "volume", values["hair"])
        elif JACKET_HINT in obj.name.lower():
            displace(obj, "volume", values["jacket"], jacket_group(obj))
            displace(obj, "collar_volume", values.get("collar", 0.0), collar_group(obj))
    bpy.context.view_layer.update()


def spread_legs(rig, spread_degrees):
    """Each thigh points down and out sideways by spread_degrees (his idle stands with the feet apart)."""
    import mathutils
    spread = math.radians(spread_degrees)
    for side, sign in (("l", 1), ("r", -1)):
        pose_bone = rig.pose.bones[f"thigh_{side}"]
        current = pose_bone.matrix.to_quaternion()
        current_direction = current @ mathutils.Vector((0.0, 1.0, 0.0))
        rotation = current_direction.rotation_difference(mathutils.Vector((math.sin(spread) * sign, 0.0, -math.cos(spread)))) @ current
        pose_bone.matrix = mathutils.Matrix.LocRotScale(pose_bone.matrix.to_translation(), rotation, pose_bone.matrix.to_scale())
        bpy.context.view_layer.update()


def point_arms(rig, spread_degrees):
    """Upper arms hang down, swung out sideways by spread_degrees; forearms continue slightly forward."""
    import mathutils
    bones = rig.pose.bones
    for side, sign in (("l", 1), ("r", -1)):
        for name in (f"upperarm_{side}", f"lowerarm_{side}"):
            bones[name].rotation_mode = "QUATERNION"
            bones[name].rotation_quaternion = (1.0, 0.0, 0.0, 0.0)
    bpy.context.view_layer.update()
    spread = math.radians(spread_degrees)
    for side, sign in (("l", 1), ("r", -1)):
        for name, direction in ((f"upperarm_{side}", (math.sin(spread) * sign, 0.02, -math.cos(spread))),
                                (f"lowerarm_{side}", (math.sin(spread) * 0.6 * sign, -0.12, -1.0))):
            pose_bone = bones[name]
            current = pose_bone.matrix.to_quaternion()
            current_direction = current @ mathutils.Vector((0.0, 1.0, 0.0))
            rotation = current_direction.rotation_difference(mathutils.Vector(direction).normalized()) @ current
            pose_bone.matrix = mathutils.Matrix.LocRotScale(pose_bone.matrix.to_translation(), rotation, pose_bone.matrix.to_scale())
            bpy.context.view_layer.update()


def sprite_pixels(path, palette):
    """The render as his sprite: majority color of each 8x8 block (alpha majority), snapped to his palette."""
    image = bpy.data.images.load(path, check_existing=False)
    pixels = numpy.array(image.pixels[:]).reshape(FRAME[1] * SCALE, FRAME[0] * SCALE, 4)[::-1]
    bpy.data.images.remove(image)
    colors = numpy.round(pixels[..., :3] * 255.0)
    distances = ((colors[..., None, :] - palette[None, None, :, :]) ** 2).sum(axis=-1)
    labels = distances.argmin(axis=-1)
    labels[pixels[..., 3] < 0.5] = -1
    blocks = labels.reshape(FRAME[1], SCALE, FRAME[0], SCALE).transpose(0, 2, 1, 3).reshape(FRAME[1], FRAME[0], -1)
    result = numpy.full((FRAME[1], FRAME[0]), -1)
    for row in range(FRAME[1]):
        for column in range(FRAME[0]):
            values, counts = numpy.unique(blocks[row, column], return_counts=True)
            result[row, column] = values[counts.argmax()]
    return result


def eye_mask(his):
    """His eyes: dark pixels inside the face, skin directly left and right of them."""
    skin = (his[..., :3] == (219, 150, 107)).all(axis=-1) | (his[..., :3] == (190, 107, 69)).all(axis=-1)
    dark = (his[..., :3].sum(axis=-1) < 200) & (his[..., 3] > 0) & ~skin
    return dark & numpy.roll(skin, 1, axis=1) & numpy.roll(skin, -1, axis=1)


def score(mine, his_labels, his_on, his_inner, his_eyes=None, black_label=0):
    best = (0.0, 0.0, 0, 0.0)
    for shift in range(-12, 13):
        moved = numpy.roll(mine, shift, axis=1)
        on = moved >= 0
        # my render has no outline yet (finish_frames adds it around the figure): compare with his figure inside his outline
        silhouette = (on & his_inner).sum() / (on | his_inner).sum()
        exact = ((moved == his_labels) & his_inner).sum() / his_inner.sum()
        eyes = ((moved == his_labels) & his_eyes).sum() / max(his_eyes.sum(), 1) if his_eyes is not None else 0.0
        if silhouette + exact + EYE_WEIGHT * eyes > best[0] + best[1] + EYE_WEIGHT * best[3]:
            best = (silhouette, exact, shift, eyes)
    return best


def match(his_rgba_npy, palette_npy, rounds=2, path=r"D:\\ai3d\\work\\mpfb\\match.png", start=None, fixed=()):
    """start: values to begin from (e.g. the last best); fixed: parameters the search leaves alone."""
    his = numpy.load(his_rgba_npy)
    palette = numpy.load(palette_npy).astype(numpy.float64)
    his_on = his[..., 3] > 0
    his_labels = ((his[..., None, :3] - palette[None, None]) ** 2).sum(axis=-1).argmin(axis=-1)
    his_labels[~his_on] = -1
    his_inner = his_on & (his[..., :3].sum(axis=-1) > 0)
    his_eyes = eye_mask(his)
    black_label = int(((palette - 0.0) ** 2).sum(axis=1).argmin())
    values = {name: first for name, (first, _, _, _) in PARAMETERS.items()}
    values.update(start or {})

    def evaluate(candidate):
        apply_values(candidate)
        render_sprite(path, candidate["turn"])
        silhouette, exact, _, eyes = score(sprite_pixels(path, palette), his_labels, his_on, his_inner, his_eyes, black_label)
        return silhouette + exact + EYE_WEIGHT * eyes, silhouette, exact, eyes

    best = evaluate(values)
    history = [tuple(round(value, 3) for value in best)]
    for round_index in range(rounds):
        # every second round halves the steps: coarse first, then finer
        refine = 0.5 ** (round_index // 2)
        for name, (_, base_step, low, high) in PARAMETERS.items():
            step = base_step * refine
            if name in fixed:
                continue
            for direction in (1, -1):
                while True:
                    candidate = dict(values)
                    candidate[name] = min(high, max(low, values[name] + direction * step))
                    if candidate[name] == values[name]:
                        break
                    result = evaluate(candidate)
                    if result[0] <= best[0]:
                        break
                    best, values = result, candidate
        history.append(tuple(round(value, 3) for value in best))
    apply_values(values)
    render_sprite(path, values["turn"])
    return {name: round(value, 3) for name, value in values.items()}, history


def refine(his_rgba_npy, palette_npy, start, iterations=120, seed=1, path=r"D:\\ai3d\\work\\mpfb\\match.png", fixed=(), shape_only=False):
    """Random steps on two or three parameters at once (moves that only pay off together, like a higher and a
    smaller head), kept when the score improves. Steps shrink when a while passes without improvement."""
    import random
    generator = random.Random(seed)
    his = numpy.load(his_rgba_npy)
    palette = numpy.load(palette_npy).astype(numpy.float64)
    his_on = his[..., 3] > 0
    his_labels = ((his[..., None, :3] - palette[None, None]) ** 2).sum(axis=-1).argmin(axis=-1)
    his_labels[~his_on] = -1
    his_inner = his_on & (his[..., :3].sum(axis=-1) > 0)
    his_eyes = eye_mask(his)

    def evaluate(candidate):
        apply_values(candidate)
        render_sprite(path, candidate["turn"])
        silhouette, exact, _, eyes = score(sprite_pixels(path, palette), his_labels, his_on, his_inner, his_eyes)
        if shape_only:
            # shape fitting before his pixels are projected: only the outline counts
            return silhouette, silhouette, exact, eyes
        return silhouette + exact + EYE_WEIGHT * eyes, silhouette, exact, eyes

    names = [name for name in PARAMETERS if name not in fixed]
    values = {name: first for name, (first, _, _, _) in PARAMETERS.items()}
    values.update(start)
    best = evaluate(values)
    history = [tuple(round(value, 3) for value in best)]
    scale, since = 1.0, 0
    for _ in range(iterations):
        candidate = dict(values)
        for name in generator.sample(names, generator.choice((2, 3))):
            _, step, low, high = PARAMETERS[name]
            candidate[name] = min(high, max(low, values[name] + generator.choice((-1, 1)) * step * scale * generator.uniform(0.5, 1.5)))
        result = evaluate(candidate)
        if result[0] > best[0]:
            best, values, since = result, candidate, 0
            history.append(tuple(round(value, 3) for value in best))
        else:
            since += 1
            if since >= 25:
                scale, since = scale * 0.6, 0
    apply_values(values)
    render_sprite(path, values["turn"])
    return {name: round(value, 3) for name, value in values.items()}, history
