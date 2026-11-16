"""Rigs the AI model (ai_adam) and carries the animation clips of player_rig_hires over to it.

Run inside the live Blender: exec(open(path).read()); build_rig(); bind(); retarget("Idle_Loop")

The skeleton "ai_rig" has the clip rig's bone names, placed at the model's own joints (measured from
its cross sections: arms hanging, his 3/4 stance). Blender's bone heat weights the mesh to it.

The clips were made for a rig in T-pose, this one rests with hanging arms. Arms are carried over as
directions: each arm bone turns from its rest direction to where the clip's bone points. Torso and legs,
which rest alike in both rigs, are carried over as rotation changes from the clip rig's rest pose (its
spine rests in an S-curve; copying directions would add that curve to the straight spine here). The pelvis also moves, by the clip's pelvis offset
scaled to the model's leg length.
"""

import math

import bpy
import mathutils

MODEL_NAME = "ai_adam"
RIG_NAME = "ai_rig"
SOURCE_RIG_NAME = "player_rig_hires"

# bone: (head, tail, parent) in the model's space; the face looks along -y, his left is +x
JOINTS = {
    "pelvis": ((-0.11, -0.04, -0.48), (-0.10, -0.06, -0.30), None),
    "spine_01": ((-0.10, -0.06, -0.30), (-0.09, -0.08, -0.12), "pelvis"),
    "spine_02": ((-0.09, -0.08, -0.12), (-0.09, -0.09, 0.02), "spine_01"),
    "spine_03": ((-0.09, -0.09, 0.02), (-0.08, -0.08, 0.18), "spine_02"),
    "neck_01": ((-0.08, -0.08, 0.18), (-0.06, -0.08, 0.30), "spine_03"),
    "Head": ((-0.06, -0.08, 0.30), (-0.02, -0.06, 0.95), "neck_01"),
    "clavicle_r": ((-0.12, -0.10, 0.08), (-0.33, -0.16, 0.02), "spine_03"),
    "upperarm_r": ((-0.33, -0.16, 0.02), (-0.40, -0.20, -0.15), "clavicle_r"),
    "lowerarm_r": ((-0.40, -0.20, -0.15), (-0.45, -0.21, -0.32), "upperarm_r"),
    "hand_r": ((-0.45, -0.21, -0.32), (-0.47, -0.22, -0.45), "lowerarm_r"),
    "clavicle_l": ((-0.02, -0.10, 0.08), (0.18, -0.08, 0.02), "spine_03"),
    "upperarm_l": ((0.18, -0.08, 0.02), (0.25, -0.10, -0.15), "clavicle_l"),
    "lowerarm_l": ((0.25, -0.10, -0.15), (0.28, -0.12, -0.32), "upperarm_l"),
    "hand_l": ((0.28, -0.12, -0.32), (0.29, -0.13, -0.45), "lowerarm_l"),
    "thigh_r": ((-0.31, -0.08, -0.48), (-0.31, -0.10, -0.70), "pelvis"),
    "calf_r": ((-0.31, -0.10, -0.70), (-0.32, -0.09, -0.87), "thigh_r"),
    "foot_r": ((-0.32, -0.09, -0.87), (-0.33, -0.18, -0.97), "calf_r"),
    "ball_r": ((-0.33, -0.18, -0.97), (-0.33, -0.24, -0.98), "foot_r"),
    "thigh_l": ((0.09, -0.04, -0.48), (0.09, -0.06, -0.70), "pelvis"),
    "calf_l": ((0.09, -0.06, -0.70), (0.09, -0.04, -0.87), "thigh_l"),
    "foot_l": ((0.09, -0.04, -0.87), (0.09, -0.10, -0.97), "calf_l"),
    "ball_l": ((0.09, -0.10, -0.97), (0.09, -0.16, -0.98), "foot_l"),
}
ORDER = list(JOINTS)
# bones whose rest pose differs between the rigs (T-pose arms against hanging arms): carried over as directions
DIRECTION_BONES = ("clavicle", "upperarm", "lowerarm", "hand")
# torso and head: how much of the clip's lean they keep (his run is drawn nearly upright, the head level)
UPRIGHT = {"pelvis": 0.35, "spine_01": 0.35, "spine_02": 0.35, "spine_03": 0.35, "neck_01": 0.25, "Head": 0.15}


def view3d_override():
    window = bpy.context.window_manager.windows[0]
    area = next(area for area in window.screen.areas if area.type == "VIEW_3D")
    return window, area


def build_rig():
    old = bpy.data.objects.get(RIG_NAME)
    if old:
        bpy.data.objects.remove(old, do_unlink=True)
    armature = bpy.data.armatures.new(RIG_NAME)
    rig = bpy.data.objects.new(RIG_NAME, armature)
    bpy.context.scene.collection.objects.link(rig)
    window, area = view3d_override()
    for obj in bpy.context.view_layer.objects:
        obj.select_set(False)
    rig.select_set(True)
    bpy.context.view_layer.objects.active = rig
    with bpy.context.temp_override(window=window, area=area, active_object=rig, object=rig):
        bpy.ops.object.mode_set(mode="EDIT")
        for name in ORDER:
            head, tail, parent = JOINTS[name]
            bone = armature.edit_bones.new(name)
            bone.head = head
            bone.tail = tail
            if parent:
                bone.parent = armature.edit_bones[parent]
                bone.use_connect = False
        bpy.ops.object.mode_set(mode="OBJECT")
    rig.show_in_front = True
    return rig


def bind():
    model = bpy.data.objects[MODEL_NAME]
    rig = bpy.data.objects[RIG_NAME]
    for modifier in list(model.modifiers):
        if modifier.type == "ARMATURE":
            model.modifiers.remove(modifier)
    model.vertex_groups.clear()
    model.parent = None
    window, area = view3d_override()
    for obj in bpy.context.view_layer.objects:
        obj.select_set(False)
    model.hide_set(False)
    rig.hide_set(False)
    model.select_set(True)
    rig.select_set(True)
    bpy.context.view_layer.objects.active = rig
    with bpy.context.temp_override(window=window, area=area, active_object=rig, selected_objects=[model, rig], selected_editable_objects=[model, rig]):
        bpy.ops.object.parent_set(type="ARMATURE_AUTO")
    unweighted = sum(1 for vertex in model.data.vertices if not vertex.groups)
    return len(model.vertex_groups), unweighted


def leg_length(rig, foot_name="foot_l"):
    pelvis = rig.data.bones["pelvis"].head_local
    foot = rig.data.bones[foot_name].head_local
    return pelvis.z - foot.z


def retarget(action_name, first=None, last=None, turn_degrees=90.0, upright=None):
    """Bakes action_name of the clip rig onto ai_rig as a new action "ai_<action_name>".

    The clips move toward -y (their toes point that way); the sprite camera looks along +y and he runs to the
    screen's right (+x), so every rotation change and the pelvis offset are turned by turn_degrees about z.
    upright replaces UPRIGHT (e.g. the crawl keeps the clip's horizontal torso).
    """
    upright = UPRIGHT if upright is None else upright
    turn = mathutils.Quaternion((0.0, 0.0, 1.0), math.radians(turn_degrees))
    scene = bpy.context.scene
    source = bpy.data.objects[SOURCE_RIG_NAME]
    target = bpy.data.objects[RIG_NAME]
    action = bpy.data.actions[action_name]
    # a rig disabled in the viewport is not evaluated, its clips would not play; hiding keeps it evaluated
    source.hide_viewport = False
    source.hide_set(True)
    source.animation_data_create()
    source.animation_data.action = action
    first = int(action.frame_range[0]) if first is None else first
    last = int(action.frame_range[1]) if last is None else last
    scale = leg_length(target) / leg_length(source)
    source_rest = {name: (source.matrix_world @ source.data.bones[name].matrix_local) for name in ORDER}
    target_rest = {name: (target.matrix_world @ target.data.bones[name].matrix_local) for name in ORDER}
    baked = bpy.data.actions.get("ai_" + action_name)
    if baked:
        bpy.data.actions.remove(baked)
    baked = bpy.data.actions.new("ai_" + action_name)
    target.animation_data_create()
    target.animation_data.action = baked
    for pose_bone in target.pose.bones:
        pose_bone.rotation_mode = "QUATERNION"
    scene.frame_set(first)
    source_pelvis_rest = source_rest["pelvis"].to_translation()
    for frame in range(first, last + 1):
        scene.frame_set(frame)
        for pose_bone in target.pose.bones:
            pose_bone.matrix_basis = mathutils.Matrix.Identity(4)
        bpy.context.view_layer.update()
        for name in ORDER:
            source_pose = source.matrix_world @ source.pose.bones[name].matrix
            target_rest_rotation = target_rest[name].to_quaternion()
            if name in upright:
                # torso and head: the clip's direction, but mostly upright like his run (the clips lean far forward)
                source_direction = turn @ (source_pose.to_quaternion() @ mathutils.Vector((0.0, 1.0, 0.0)))
                upright_direction = mathutils.Vector((0.0, 0.0, 1.0))
                direction = upright_direction.lerp(source_direction.normalized(), upright[name]).normalized()
                target_rest_direction = target_rest_rotation @ mathutils.Vector((0.0, 1.0, 0.0))
                rotation = target_rest_direction.rotation_difference(direction) @ target_rest_rotation
            elif name.startswith(DIRECTION_BONES):
                # the arms point where the clip's arms point: the clip rig rests in a T-pose, this one with hanging arms
                source_direction = turn @ (source_pose.to_quaternion() @ mathutils.Vector((0.0, 1.0, 0.0)))
                target_rest_direction = target_rest_rotation @ mathutils.Vector((0.0, 1.0, 0.0))
                rotation = target_rest_direction.rotation_difference(source_direction) @ target_rest_rotation
            else:
                # torso and legs rest alike in both rigs: the clip's change from its rest pose (its spine rests in an S-curve)
                change = turn @ source_pose.to_quaternion() @ source_rest[name].to_quaternion().inverted() @ turn.inverted()
                rotation = change @ target_rest_rotation
            rotation = rotation.to_matrix().to_4x4()
            pose_bone = target.pose.bones[name]
            if name == "pelvis":
                offset = turn @ ((source_pose.to_translation() - source_pelvis_rest) * scale)
                head = target_rest[name].to_translation() + offset
            else:
                head = target.matrix_world @ pose_bone.head
            pose_bone.matrix = target.matrix_world.inverted() @ (mathutils.Matrix.Translation(head) @ rotation)
            bpy.context.view_layer.update()
        for pose_bone in target.pose.bones:
            pose_bone.keyframe_insert("rotation_quaternion", frame=frame, group=pose_bone.name)
            if pose_bone.name == "pelvis":
                pose_bone.keyframe_insert("location", frame=frame, group=pose_bone.name)
    return baked.name, first, last


def distance_weights(falloff=4.0, keep=3):
    """Bone weights by distance, for meshes where bone heat finds no solution: every vertex follows its nearest
    bone segments, blended by inverse distance (power falloff), keeping the strongest few."""
    model = bpy.data.objects[MODEL_NAME]
    rig = bpy.data.objects[RIG_NAME]
    for modifier in list(model.modifiers):
        if modifier.type == "ARMATURE":
            model.modifiers.remove(modifier)
    model.vertex_groups.clear()
    segments = []
    for bone in rig.data.bones:
        segments.append((bone.name, rig.matrix_world @ bone.head_local, rig.matrix_world @ bone.tail_local))
    groups = {name: model.vertex_groups.new(name=name) for name, _, _ in segments}
    for vertex in model.data.vertices:
        point = model.matrix_world @ vertex.co
        distances = []
        for name, head, tail in segments:
            axis = tail - head
            fraction = max(0.0, min(1.0, (point - head).dot(axis) / axis.length_squared))
            distances.append(((point - (head + axis * fraction)).length + 1e-4, name))
        distances.sort()
        nearest = distances[:keep]
        weights = [(1.0 / distance ** falloff, name) for distance, name in nearest]
        total = sum(weight for weight, _ in weights)
        for weight, name in weights:
            groups[name].add([vertex.index], weight / total, "REPLACE")
    model.parent = rig
    model.matrix_parent_inverse = rig.matrix_world.inverted()
    modifier = model.modifiers.new("armature", "ARMATURE")
    modifier.object = rig
    return len(groups)
