"""Puts dstar's idle pixels onto the MPFB character: every face the sprite camera sees takes his pixel there.

Run inside the live Blender, with the character posed and turned like his idle (mpfb_match.py) and the sprite
camera set up by mpfb_sprite.render_sprite: exec(open(path).read()); project_idle(his_rgba_npy, shift)

Each face's posed center is projected into the 72 x 48 frame; when a ray from the camera hits that face first and
his pixel there is part of the figure (not his black outline), the face's three tones all become that pixel, so
it shows exactly his color whatever the light. Faces the camera does not see keep their colors. The colors live
on the faces, so they move with the mesh in every animation. shift: his frame's horizontal offset against the
render (from mpfb_match.score).
"""

import bpy
import mathutils
import numpy

FRAME = (72, 48)


def linear(color):
    values = numpy.asarray(color, dtype=numpy.float64) / 255.0
    return numpy.where(values <= 0.04045, values / 12.92, ((values + 0.055) / 1.055) ** 2.4)


def project_idle(his_rgba_npy, shift=0):
    his = numpy.load(his_rgba_npy)
    scene = bpy.context.scene
    camera = scene.camera
    depsgraph = bpy.context.evaluated_depsgraph_get()
    direction = camera.matrix_world.to_quaternion() @ mathutils.Vector((0.0, 0.0, -1.0))
    right = camera.matrix_world.to_quaternion() @ mathutils.Vector((1.0, 0.0, 0.0))
    up = camera.matrix_world.to_quaternion() @ mathutils.Vector((0.0, 1.0, 0.0))
    width = camera.data.ortho_scale
    pixel = width / FRAME[0]
    origin = camera.matrix_world.translation
    counts = {}
    for obj in bpy.data.objects:
        if obj.type != "MESH" or obj.hide_render:
            continue
        # MPFB hides body parts under the clothes with modifiers that change the face count: off while matching faces
        switched = [modifier for modifier in obj.modifiers if modifier.type not in ("ARMATURE", "DISPLACE") and modifier.show_viewport]
        for modifier in switched:
            modifier.show_viewport = False
        bpy.context.view_layer.update()
        depsgraph = bpy.context.evaluated_depsgraph_get()
        evaluated = obj.evaluated_get(depsgraph)
        posed = evaluated.to_mesh()
        for modifier in switched:
            modifier.show_viewport = True
        if len(posed.polygons) != len(obj.data.polygons):
            evaluated.to_mesh_clear()
            continue
        world = evaluated.matrix_world
        mesh = obj.data
        attributes = [mesh.attributes[name] for name in ("tone_light", "adam_color", "tone_dark")]
        stripes = mesh.attributes.get("stripes")
        projected = 0
        for polygon in posed.polygons:
            center = world @ polygon.center
            normal = (world.to_3x3() @ polygon.normal).normalized()
            if normal.dot(direction) >= 0.0:
                continue
            relative = center - origin
            column = int((relative.dot(right) + width / 2) / pixel) - shift
            row = int(FRAME[1] / 2 - relative.dot(up) / pixel)
            if not (0 <= column < FRAME[0] and 0 <= row < FRAME[1]):
                continue
            color = his[row, column]
            if color[3] == 0 or color[:3].sum() == 0:
                continue
            # visible: the first thing a camera ray meets on the way to this face is this face
            start = center - direction * 10.0
            hit, location, _, _, hit_object, _ = scene.ray_cast(depsgraph, start, direction)
            if not hit or hit_object.original != obj or (location - center).length > pixel * 0.6:
                continue
            value = (*linear(color[:3]), 1.0)
            for attribute in attributes:
                for loop_index in mesh.polygons[polygon.index].loop_indices:
                    attribute.data[loop_index].color = value
            if stripes:
                stripes.data[polygon.index].value = 0.0
            projected += 1
        evaluated.to_mesh_clear()
        mesh.update()
        counts[obj.name] = projected
    return counts
