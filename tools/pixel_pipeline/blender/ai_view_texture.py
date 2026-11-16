"""Puts the Zero123++ view colors (view_colors.py) on the AI model wherever his sprite cannot show it.

Run inside the live Blender after ai_texture.py and ai_parts.py: exec(open(path).read()); apply_view_colors(npy)

Faces turned toward the sprite camera keep his projected pixels. Every other face (the sides, the back, the
top of the hair) shows the colors the views painted there, through the "override" attribute; vertices no view
saw keep ai_parts.py's material color. The hair lock pattern is switched off wherever view colors take over,
since the views painted real locks.
"""

import bpy
import numpy

# faces whose normal.y is below this look at the sprite camera (it looks along +y) and keep his pixels
FRONT_FACING = -0.45


def linear(values):
    values = numpy.asarray(values, dtype=numpy.float64) / 255.0
    return numpy.where(values <= 0.04045, values / 12.92, ((values + 0.055) / 1.055) ** 2.4)


def apply_view_colors(npy_path):
    model = bpy.data.objects["ai_adam"]
    mesh = model.data
    data = numpy.load(npy_path)
    colors, weights = linear(data[:, :3]), data[:, 3]
    override = mesh.attributes["override"]
    hair_pattern = mesh.attributes["hair_pattern"]
    changed = 0
    for polygon in mesh.polygons:
        if polygon.normal.y < FRONT_FACING:
            continue
        if not all(weights[index] > 0 for index in polygon.vertices):
            continue
        changed += 1
        for loop_index, vertex_index in zip(polygon.loop_indices, polygon.vertices):
            red, green, blue = colors[vertex_index]
            override.data[loop_index].color = (red, green, blue, 1.0)
            hair_pattern.data[loop_index].value = 0.0
    mesh.update()
    return changed, len(mesh.polygons)
