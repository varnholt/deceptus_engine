"""Cuts the MPFB hair like dstar draws Adam's: the face and the ear free, the hair starting above the forehead.

Run inside the live Blender: exec(open(path).read()); cut_hair(hair_object_name)

Library haircuts hang bangs over the eyes; his hair sweeps up and back. Hair faces in front of the head's
center below the brow, and at the sides below eye height (his ear shows), are deleted. The back stays.
Heights come from the eyes asset (rest pose).
"""

import bpy
import numpy

# how far above the eye centers the hairline sits in front, in eyeball radii
BROW_RADII = 2.5
# hair at the sides ends this many eyeball radii above the eye centers (so the ear shows)
SIDE_RADII = 0.5
# faces whose center lies this far behind the eyes (in eyeball radii) count as the back of the head
BACK_RADII = 4.0


def cut_hair(hair_name):
    hair = bpy.data.objects[hair_name]
    eyes = next(item for item in bpy.data.objects if "poly" in item.name.lower() and item.type == "MESH")
    eye_points = numpy.array([vertex.co for vertex in eyes.data.vertices])
    eye_z = eye_points[:, 2].mean()
    eye_y = eye_points[:, 1].mean()
    radius = float(numpy.ptp(eye_points[eye_points[:, 0] < 0][:, 2]) / 2)
    brow_z = eye_z + BROW_RADII * radius
    side_z = eye_z + SIDE_RADII * radius
    back_y = eye_y + BACK_RADII * radius
    mesh = hair.data
    for polygon in mesh.polygons:
        x, y, z = polygon.center
        in_front = y < back_y
        polygon.select = (in_front and z < brow_z and abs(x) < 4.0 * radius) or (in_front and z < side_z)
    selected = sum(1 for polygon in mesh.polygons if polygon.select)
    window = bpy.context.window_manager.windows[0]
    area = next(item for item in window.screen.areas if item.type == "VIEW_3D")
    for obj in bpy.context.view_layer.objects:
        obj.select_set(False)
    hair.select_set(True)
    bpy.context.view_layer.objects.active = hair
    with bpy.context.temp_override(window=window, area=area, active_object=hair, object=hair):
        bpy.ops.object.mode_set(mode="EDIT")
        bpy.ops.mesh.select_mode(type="FACE")
        bpy.ops.mesh.delete(type="FACE")
        bpy.ops.object.mode_set(mode="OBJECT")
    return selected, len(mesh.polygons)
