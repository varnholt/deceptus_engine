"""Sweeps the MPFB hair's front strands up and forward, like dstar's Adam (his bangs stand up in a spike).

Run inside the live Blender: exec(open(path).read()); sweep_hair(hair_object_name, degrees)

Hair in front of the head below the hairline is turned about a horizontal axis through the hairline (left-right,
at the brow), so it swings up over the forehead instead of hanging over the eyes. Strands lower down swing
further, so they fan out into a crest. Heights come from the eyes asset (rest pose).
"""

import math

import bpy
import mathutils
import numpy

# the hairline (pivot) above the eye centers, in eyeball radii
HAIRLINE_RADII = 3.0
# hair this far behind the eyes (eyeball radii) is the back of the head and stays
BACK_RADII = 4.0


def sweep_hair(hair_name, degrees=110.0):
    hair = bpy.data.objects[hair_name]
    eyes = next(item for item in bpy.data.objects if "poly" in item.name.lower() and item.type == "MESH")
    eye_points = numpy.array([vertex.co for vertex in eyes.data.vertices])
    radius = float(numpy.ptp(eye_points[eye_points[:, 0] < 0][:, 2]) / 2)
    pivot_z = eye_points[:, 2].mean() + HAIRLINE_RADII * radius
    pivot_y = eye_points[:, 1].mean() + 1.0 * radius
    back_y = eye_points[:, 1].mean() + BACK_RADII * radius
    moved = 0
    for vertex in hair.data.vertices:
        x, y, z = vertex.co
        if y > back_y or z > pivot_z:
            continue
        # the further below the hairline, the further it swings (up to the full angle at one hairline depth)
        depth = min(1.0, (pivot_z - z) / (HAIRLINE_RADII * radius * 1.5))
        angle = math.radians(degrees) * depth
        offset = mathutils.Vector((0.0, y - pivot_y, z - pivot_z))
        # turning about +x by a negative angle brings strands hanging in front (-y, below) up and over to the front-top
        offset.rotate(mathutils.Matrix.Rotation(-angle, 3, "X"))
        vertex.co = (x, pivot_y + offset.y, pivot_z + offset.z)
        moved += 1
    hair.data.update()
    return moved, len(hair.data.vertices)
