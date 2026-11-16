"""Bakes the clip rig's animations onto the AI model and renders them from the sprite camera.

Run inside the live Blender, after ai_rig.py and ai_sprite_preview.py were executed:
exec(open(path).read()); render_cycle("crawl", output_directory)

Each cycle names its clip, the frames of one loop, how many frames he gets (his animations use 100 ms
frames, so the count sets the speed), the turn (90: he moves toward the screen's right, 180: his back
faces the camera) and how upright torso and head stay.
"""

import math
import os

import bpy
import numpy

# crawl keeps the clip's horizontal torso, the head looks ahead along the floor
CRAWL_UPRIGHT = {"pelvis": 1.0, "spine_01": 1.0, "spine_02": 1.0, "spine_03": 1.0, "neck_01": 0.8, "Head": 0.45}
# torso and head follow the clip fully (dance, sitting, ladder)
FREE = {}

CYCLES = {
    "run": {"action": "Sprint_Loop", "first": 0, "last": 16, "count": 12, "turn": 90.0, "upright": None},
    "idle": {"action": "Idle_Loop", "first": 0, "last": 60, "count": 10, "turn": 90.0, "upright": None},
    "crawl": {"action": "CMU_Crawl", "first": 24, "last": 72, "count": 12, "turn": 90.0, "upright": CRAWL_UPRIGHT},
    "dance": {"action": "Dance_Loop", "first": 0, "last": 24, "count": 8, "turn": 60.0, "upright": FREE},
    "read": {"action": "Player_Read", "first": 0, "last": 11, "count": 12, "turn": 60.0, "upright": FREE},
    "ladder_mocap": {"action": "CMU_Ladder", "first": 0, "last": 72, "count": 12, "turn": 0.0, "upright": FREE, "facing": 180.0},
    # key poses from ai_keyposes.py, one frame per pose
    "ladder": {"keyed": "ladder", "facing": 180.0},
}


def frame_numbers(first, last, count):
    """count frames spread over one loop; last is the loop's end, the same pose as first, so it is left out."""
    return [round(first + (last - first) * index / count) for index in range(count)]


def lowest_point(model):
    evaluated = model.evaluated_get(bpy.context.evaluated_depsgraph_get())
    mesh = evaluated.to_mesh()
    coordinates = numpy.zeros(len(mesh.vertices) * 3, dtype=numpy.float32)
    mesh.vertices.foreach_get("co", coordinates)
    evaluated.to_mesh_clear()
    world = numpy.array(model.matrix_world)
    return float((coordinates.reshape(-1, 3) @ world[:3, :3].T + world[:3, 3])[:, 2].min())


def render_cycle(name, output_directory):
    """Renders one cycle. The whole cycle is lifted (or lowered) by one amount so its lowest point over all frames
    touches the rest pose's ground: crouched and crawling poses would otherwise sink below the camera frame.
    facing turns the rig itself about its center (the ladder shows his back), not just the motion."""
    cycle = CYCLES[name]
    if "keyed" in cycle:
        count = key_cycle(cycle["keyed"])
        cycle = dict(cycle, first=0, last=count, count=count)
    else:
        retarget(cycle["action"], cycle["first"], cycle["last"], cycle["turn"], cycle["upright"])
    os.makedirs(output_directory, exist_ok=True)
    scene = bpy.context.scene
    rig = bpy.data.objects[RIG_NAME]
    model = bpy.data.objects[MODEL_NAME]
    coordinates = numpy.zeros(len(model.data.vertices) * 3, dtype=numpy.float32)
    model.data.vertices.foreach_get("co", coordinates)
    coordinates = coordinates.reshape(-1, 3)
    middle_x, ground = (coordinates[:, 0].min() + coordinates[:, 0].max()) / 2, coordinates[:, 2].min()
    saved_location, saved_rotation = rig.location.copy(), rig.rotation_euler.copy()
    facing = math.radians(cycle.get("facing", 0.0))
    # turning about z maps x to -x for 180 degrees; moving by twice the center keeps him in the frame
    rig.rotation_euler.z += facing
    rig.location.x += middle_x - (middle_x * math.cos(facing))
    rig.location.y -= middle_x * math.sin(facing)
    frames = frame_numbers(cycle["first"], cycle["last"], cycle["count"])
    lowest = []
    for frame in frames:
        scene.frame_set(frame)
        lowest.append(lowest_point(model))
    rig.location.z += ground - min(lowest)
    paths = []
    try:
        for index, frame in enumerate(frames):
            scene.frame_set(frame)
            paths.append(render_front(os.path.join(output_directory, f"frame_{index:02d}.png"), middle_x))
    finally:
        rig.location, rig.rotation_euler = saved_location, saved_rotation
    return paths
