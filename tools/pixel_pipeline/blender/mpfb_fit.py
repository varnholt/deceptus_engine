"""Fits the MPFB character's proportions and turn to dstar's idle sprite, by silhouette.

Run inside the live Blender: exec(open(path).read()); fit_proportions(alpha_npy)

The character stands in his idle pose (arms hanging) and turns toward screen right like his 3/4 sprite. Bone
scales make the head bigger and the legs, arms and torso shorter (his figure is a chibi: the head is about a
third of his height). Each candidate is rendered from the sprite camera at his 42 pixel height, feet on the
bottom row, and scored by silhouette overlap with his idle frame; a coordinate search keeps what improves it.
"""

import math

import bpy
import mathutils
import numpy

FRAME = (72, 48)
FIGURE_PIXELS = 42
# render scale for the silhouette (a pixel is on when most of its 8x8 block is covered)
SCALE = 8
RIG_NAME = "adam_body.rig"
# the idle pose: upper arms down along the body, a little away from it
ARM_DOWN_DEGREES = 62.0
# parameters: name -> (start, step, low, high)
PARAMETERS = {
    "head": (2.0, 0.2, 1.0, 3.5),
    "legs": (0.75, 0.05, 0.4, 1.1),
    "arms": (0.85, 0.05, 0.5, 1.1),
    "torso": (0.9, 0.05, 0.6, 1.2),
    "turn": (55.0, 5.0, 0.0, 90.0),
}


def silhouette_camera(scene):
    camera = bpy.data.objects.get("fit_camera")
    if camera is None:
        camera = bpy.data.objects.new("fit_camera", bpy.data.cameras.new("fit_camera"))
        scene.collection.objects.link(camera)
    camera.data.type = "ORTHO"
    camera.rotation_euler = mathutils.Vector((0.0, 1.0, 0.0)).to_track_quat("-Z", "Y").to_euler()
    return camera


def pose(values):
    rig = bpy.data.objects[RIG_NAME]
    for pose_bone in rig.pose.bones:
        pose_bone.rotation_mode = "XYZ"
        pose_bone.rotation_euler = (0.0, 0.0, 0.0)
        pose_bone.scale = (1.0, 1.0, 1.0)
    bones = rig.pose.bones
    bones["head"].scale = (values["head"],) * 3
    for side in ("l", "r"):
        bones[f"thigh_{side}"].scale = (values["legs"],) * 3
        bones[f"upperarm_{side}"].scale = (values["arms"],) * 3
        # the A-pose arms come down to his hanging arms (about the bone's local z)
        bones[f"upperarm_{side}"].rotation_euler = (0.0, 0.0, math.radians(ARM_DOWN_DEGREES if side == "l" else -ARM_DOWN_DEGREES))
    bones["spine_01"].scale = (values["torso"],) * 3
    # the head and legs keep their own size when the torso scales
    bones["head"].scale = tuple(value / values["torso"] for value in bones["head"].scale)
    rig.rotation_euler = (0.0, 0.0, math.radians(values["turn"]))
    bpy.context.view_layer.update()


def evaluated_bounds():
    depsgraph = bpy.context.evaluated_depsgraph_get()
    low, high = numpy.full(3, numpy.inf), numpy.full(3, -numpy.inf)
    for obj in bpy.data.objects:
        if obj.type != "MESH" or obj.hide_render:
            continue
        evaluated = obj.evaluated_get(depsgraph)
        mesh = evaluated.to_mesh()
        coordinates = numpy.zeros(len(mesh.vertices) * 3)
        mesh.vertices.foreach_get("co", coordinates)
        evaluated.to_mesh_clear()
        world = numpy.array(obj.matrix_world)
        points = coordinates.reshape(-1, 3) @ world[:3, :3].T + world[:3, 3]
        low, high = numpy.minimum(low, points.min(axis=0)), numpy.maximum(high, points.max(axis=0))
    return low, high


def render_silhouette(scene, camera, path):
    low, high = evaluated_bounds()
    units_per_pixel = (high[2] - low[2]) / FIGURE_PIXELS
    camera.data.ortho_scale = FRAME[0] * units_per_pixel
    camera.location = mathutils.Vector(((low[0] + high[0]) / 2, low[1] - 5.0, low[2] + FRAME[1] / 2 * units_per_pixel))
    scene.camera = camera
    scene.render.resolution_x, scene.render.resolution_y = FRAME[0] * SCALE, FRAME[1] * SCALE
    scene.render.film_transparent = True
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)
    image = bpy.data.images.load(path, check_existing=False)
    alpha = numpy.array(image.pixels[:]).reshape(FRAME[1] * SCALE, FRAME[0] * SCALE, 4)[::-1, :, 3]
    bpy.data.images.remove(image)
    blocks = alpha.reshape(FRAME[1], SCALE, FRAME[0], SCALE).mean(axis=(1, 3))
    return blocks > 0.5


def overlap(mine, his):
    """Best overlap over horizontal shifts (his figure is not centered in its frame)."""
    best = 0.0
    for shift in range(-12, 13):
        moved = numpy.roll(mine, shift, axis=1)
        best = max(best, (moved & his).sum() / (moved | his).sum())
    return best


def fit_proportions(alpha_npy, rounds=3, path=r"D:\\ai3d\\work\\fit_silhouette.png"):
    scene = bpy.context.scene
    try:
        scene.render.engine = "BLENDER_WORKBENCH"
    except TypeError:
        pass
    his = numpy.load(alpha_npy)
    camera = silhouette_camera(scene)
    values = {name: start for name, (start, _, _, _) in PARAMETERS.items()}

    def score(candidate):
        pose(candidate)
        return overlap(render_silhouette(scene, camera, path), his)

    best = score(values)
    history = [round(best, 3)]
    for _ in range(rounds):
        for name, (_, step, low, high) in PARAMETERS.items():
            for direction in (1, -1):
                while True:
                    candidate = dict(values)
                    candidate[name] = min(high, max(low, values[name] + direction * step))
                    if candidate[name] == values[name]:
                        break
                    value = score(candidate)
                    if value <= best:
                        break
                    best, values = value, candidate
        history.append(round(best, 3))
    pose(values)
    return {name: round(value, 3) for name, value in values.items()}, history
