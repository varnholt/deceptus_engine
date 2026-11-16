"""Bakes motion capture joint positions (exported per frame as JSON) onto the player's rig as an action.

Run inside the live Blender: exec(open(path).read()); bake(json_path, action_name, first, last, step)

Each mapped bone is turned (shortest rotation, no twist) so it points where the captured limb points;
the pelvis follows the captured hips, its height scaled by the ratio of the two leg lengths. The
performer's average facing is turned to the model's forward (-y), so the pixel camera sees the
motion from the same 3/4 angle as dstar's frames.
"""

import json
import math

import bpy
from mathutils import Matrix, Vector

# target bone: (from joint, to joint) in the capture
BONE_MAP = [
    ("spine_01", "hips", "chest"), ("spine_02", "hips", "chest"), ("spine_03", "chest", "neck"),
    ("neck_01", "neck", "head"), ("Head", "head", "head_top"),
    ("upperarm_l", "l_shoulder", "l_elbow"), ("lowerarm_l", "l_elbow", "l_wrist"),
    ("upperarm_r", "r_shoulder", "r_elbow"), ("lowerarm_r", "r_elbow", "r_wrist"),
    ("thigh_l", "l_hip", "l_knee"), ("calf_l", "l_knee", "l_ankle"), ("foot_l", "l_ankle", "l_toe"),
    ("thigh_r", "r_hip", "r_knee"), ("calf_r", "r_knee", "r_ankle"), ("foot_r", "r_ankle", "r_toe"),
]


def facing_rotation(frames):
    """Rotation around z that turns the performer's average forward to -y."""
    left = Vector((0.0, 0.0, 0.0))
    for frame in frames:
        left += Vector(frame["l_hip"]) - Vector(frame["r_hip"])
    left.z = 0.0
    left.normalize()
    forward = left.cross(Vector((0.0, 0.0, 1.0)))
    angle = math.atan2(forward.y, forward.x) - math.atan2(-1.0, 0.0)
    return Matrix.Rotation(-angle, 3, "Z")


def align_bone(pose_bone, direction):
    matrix = pose_bone.matrix.copy()
    current = matrix.col[1].to_3d().normalized()
    rotation = current.rotation_difference(direction.normalized()).to_matrix().to_4x4()
    head = matrix.translation.copy()
    pose_bone.matrix = Matrix.Translation(head) @ rotation @ Matrix.Translation(-head) @ matrix


def bake(json_path, action_name, first=0, last=None, step=1, scales=None, head_upright=False):
    clip = json.load(open(json_path))
    frames = clip["frames"][first:last:step]
    rig = bpy.data.objects["player_rig_hires"]
    to_model = facing_rotation(frames)
    to_armature = rig.matrix_world.inverted()
    pelvis = rig.pose.bones["pelvis"]
    # captured leg length against the model's (posed, styled) leg length
    captured_leg = sum((Vector(frames[0][a]) - Vector(frames[0][b])).length for a, b in (("l_hip", "l_knee"), ("l_knee", "l_ankle")))
    model_leg = (rig.pose.bones["thigh_l"].head - rig.pose.bones["calf_l"].head).length + (rig.pose.bones["calf_l"].head - rig.pose.bones["foot_l"].head).length
    ratio = model_leg / captured_leg
    start_hips = Vector(frames[0]["hips"])
    ground = min(min(frame["l_ankle"][2], frame["r_ankle"][2]) for frame in frames)
    rest_pelvis = rig.data.bones["pelvis"].head_local.copy()
    old_action = bpy.data.actions.get(action_name)
    if old_action:
        bpy.data.actions.remove(old_action)
    action = bpy.data.actions.new(action_name)
    action.use_fake_user = True
    rig.animation_data.action = action
    for pose_bone in rig.pose.bones:
        pose_bone.rotation_mode = "QUATERNION"
        pose_bone.rotation_quaternion = (1.0, 0.0, 0.0, 0.0)
        pose_bone.location = (0.0, 0.0, 0.0)
    bpy.context.view_layer.update()
    for index, frame in enumerate(frames):
        joints = {name: to_model @ (Vector(position) - start_hips) for name, position in frame.items()}
        for pose_bone in rig.pose.bones:
            pose_bone.rotation_quaternion = (1.0, 0.0, 0.0, 0.0)
            pose_bone.location = (0.0, 0.0, 0.0)
            if scales and pose_bone.name in scales:
                pose_bone.scale = scales[pose_bone.name]
        bpy.context.view_layer.update()
        # pelvis: captured hip height above the captured ground, in model units, stays in place horizontally
        height = (Vector(frame["hips"]).z - ground) * ratio
        target = Vector((0.0, 0.0, height))
        pelvis_matrix = pelvis.matrix.copy()
        pelvis_matrix.translation = to_armature @ (rig.matrix_world @ Vector((rest_pelvis.x, rest_pelvis.y, 0.0)) + target)
        # pelvis orientation from the hips line and the spine
        left = (joints["l_hip"] - joints["r_hip"]).normalized()
        up = (joints["chest"] - joints["hips"]).normalized()
        forward = left.cross(up).normalized()
        left = up.cross(forward).normalized()
        rest_matrix = rig.data.bones["pelvis"].matrix_local.to_3x3()
        # rest pelvis frame: model left is +x, up is +z, forward is -y
        rest_frame = Matrix((Vector((1.0, 0.0, 0.0)), Vector((0.0, 0.0, 1.0)), Vector((0.0, -1.0, 0.0)))).transposed()
        captured_frame = Matrix((left, up, forward)).transposed()
        orientation = captured_frame @ rest_frame.inverted() @ rest_matrix
        pelvis.matrix = Matrix.Translation(pelvis_matrix.translation) @ orientation.to_4x4() @ Matrix.Diagonal(pelvis.matrix.to_scale()).to_4x4()
        bpy.context.view_layer.update()
        for bone_name, joint_from, joint_to in BONE_MAP:
            direction = to_armature.to_3x3() @ (joints[joint_to] - joints[joint_from])
            if head_upright and bone_name in ("neck_01", "Head"):
                # on all fours the face looks ahead instead of at the floor
                direction = to_armature.to_3x3() @ Vector((0.0, -0.35, 1.0))
            if direction.length < 1e-6:
                continue
            align_bone(rig.pose.bones[bone_name], direction)
            bpy.context.view_layer.update()
        for pose_bone in rig.pose.bones:
            pose_bone.keyframe_insert("rotation_quaternion", frame=index)
            pose_bone.keyframe_insert("location", frame=index)
    return action, len(frames)
