"""Key poses for the AI model (ai_adam), written as bone directions, for cycles no clip covers well.

Run inside the live Blender after ai_rig.py: exec(open(path).read()); key_cycle("ladder")

A pose names, for the bones it changes, the direction the bone points in the rig's space (the face looks
along -y, his left is +x, up is +z) plus an optional pelvis offset. Bones it leaves out keep their rest
direction relative to their parent. Like dstar's (and Commander Keen's) animations, a cycle is a handful of
bold, readable poses; each pose is held for its number of 100 ms frames.
"""

import bpy
import mathutils

LADDER = [
    # right hand on the rung above the head, left hand at the shoulder, left knee up on a rung
    ({"upperarm_r": (0.15, -0.35, 1.0), "lowerarm_r": (0.0, -0.3, 1.0), "hand_r": (0.0, -0.5, 1.0),
      "upperarm_l": (0.2, -0.6, 0.3), "lowerarm_l": (0.0, -0.4, 1.0), "hand_l": (0.0, -0.6, 0.8),
      "thigh_l": (0.0, -1.0, -0.2), "calf_l": (0.0, 0.1, -1.0), "foot_l": (0.0, -1.0, -0.2),
      "thigh_r": (0.0, -0.1, -1.0), "calf_r": (0.0, 0.05, -1.0)}, (0.0, 0.0, 0.0), 2),
    # both hands meet: pulling up, the left hand travels past the right
    ({"upperarm_r": (0.2, -0.6, 0.6), "lowerarm_r": (0.0, -0.4, 1.0), "hand_r": (0.0, -0.5, 1.0),
      "upperarm_l": (-0.15, -0.5, 0.75), "lowerarm_l": (0.0, -0.3, 1.0), "hand_l": (0.0, -0.5, 1.0),
      "thigh_l": (0.0, -0.8, -0.6), "calf_l": (0.0, 0.2, -1.0), "foot_l": (0.0, -1.0, -0.2),
      "thigh_r": (0.0, -0.5, -0.8), "calf_r": (0.0, 0.3, -1.0), "foot_r": (0.0, -1.0, -0.2)}, (0.0, 0.0, -0.04), 2),
]


def mirrored(pose):
    """The same pose with left and right swapped (x negated), for the second half of a climb."""
    directions, offset, hold = pose
    swapped = {}
    for name, direction in directions.items():
        other = name[:-2] + ("_l" if name.endswith("_r") else "_r") if name.endswith(("_l", "_r")) else name
        swapped[other] = (-direction[0], direction[1], direction[2])
    return swapped, (-offset[0], offset[1], offset[2]), hold


KEY_CYCLES = {
    "ladder": LADDER + [mirrored(pose) for pose in LADDER],
}


def apply_pose(rig, directions, offset):
    for pose_bone in rig.pose.bones:
        pose_bone.rotation_mode = "QUATERNION"
        pose_bone.matrix_basis = mathutils.Matrix.Identity(4)
    bpy.context.view_layer.update()
    for name in ORDER:
        if name not in directions and name != "pelvis":
            continue
        pose_bone = rig.pose.bones[name]
        rest = rig.data.bones[name].matrix_local
        if name in directions:
            rest_direction = (rest.to_quaternion() @ mathutils.Vector((0.0, 1.0, 0.0)))
            # the bone's current direction after its parents moved, turned onto the wanted one
            current = pose_bone.matrix.to_quaternion()
            current_direction = current @ mathutils.Vector((0.0, 1.0, 0.0))
            rotation = current_direction.rotation_difference(mathutils.Vector(directions[name]).normalized()) @ current
        else:
            rotation = pose_bone.matrix.to_quaternion()
        head = pose_bone.matrix.to_translation()
        if name == "pelvis":
            head = rest.to_translation() + mathutils.Vector(offset)
        pose_bone.matrix = mathutils.Matrix.Translation(head) @ rotation.to_matrix().to_4x4()
        bpy.context.view_layer.update()


def key_cycle(name):
    """Keys the cycle's poses as action "key_<name>", one pose per frame (holds repeat it); returns the frame count."""
    rig = bpy.data.objects[RIG_NAME]
    action = bpy.data.actions.get("key_" + name)
    if action:
        bpy.data.actions.remove(action)
    action = bpy.data.actions.new("key_" + name)
    action.use_fake_user = True
    rig.animation_data_create()
    rig.animation_data.action = action
    frame = 0
    for directions, offset, hold in KEY_CYCLES[name]:
        apply_pose(rig, directions, offset)
        for _ in range(hold):
            for pose_bone in rig.pose.bones:
                pose_bone.keyframe_insert("rotation_quaternion", frame=frame, group=pose_bone.name)
                pose_bone.keyframe_insert("location", frame=frame, group=pose_bone.name)
            frame += 1
    for fcurve in action.layers[0].strips[0].channelbags[0].fcurves:
        for point in fcurve.keyframe_points:
            point.interpolation = "CONSTANT"
    return frame
