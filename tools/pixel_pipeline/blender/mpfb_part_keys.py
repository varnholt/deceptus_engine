"""Shape keys that move or resize whole pieces of the MPFB character, as smooth parameters for mpfb_match.py.

Run inside the live Blender: exec(open(path).read()); make_part_keys()

Each key is one simple, even change of a whole piece (no local dents): the hair shifted back, shifted up or
grown about the head's center; the jacket's lower part pulled up toward the chest (a shorter jacket); the
boots grown about their own center. A key's value 1.0 is the full change below, values in between blend.
"""

import bpy
import numpy

# full change of each key, in rest units (meters) or as a factor
HAIR_SHIFT = 0.05
HAIR_GROWTH = 0.25
JACKET_SHORTENING = 0.3
BOOT_GROWTH = 0.3


def basis_coordinates(obj):
    if obj.data.shape_keys is None:
        obj.shape_key_add(name="Basis", from_mix=False)
    basis = obj.data.shape_keys.key_blocks[0]
    coordinates = numpy.zeros(len(basis.data) * 3)
    basis.data.foreach_get("co", coordinates)
    return coordinates.reshape(-1, 3)


def set_key(obj, name, coordinates):
    key = obj.data.shape_keys.key_blocks.get(name) or obj.shape_key_add(name=name, from_mix=False)
    key.slider_min, key.slider_max = 0.0, 2.0
    key.data.foreach_set("co", coordinates.ravel())
    key.value = 0.0
    return key.name


def make_part_keys():
    made = []
    for obj in bpy.data.objects:
        if obj.type != "MESH":
            continue
        name = obj.name.lower()
        if "hair" in name:
            basis = basis_coordinates(obj)
            center = basis.mean(axis=0)
            made.append(set_key(obj, "hair_back", basis + (0.0, HAIR_SHIFT, 0.0)))
            made.append(set_key(obj, "hair_up", basis + (0.0, 0.0, HAIR_SHIFT)))
            made.append(set_key(obj, "hair_grow", center + (basis - center) * (1.0 + HAIR_GROWTH)))
        elif "casualsuit" in name:
            basis = basis_coordinates(obj)
            chest = basis[:, 2].max() - 0.15
            group = obj.vertex_groups.get("jacket_volume")
            in_jacket = numpy.zeros(len(basis), bool)
            if group:
                for vertex in obj.data.vertices:
                    in_jacket[vertex.index] = any(element.group == group.index for element in vertex.groups)
            shortened = basis.copy()
            below = in_jacket & (basis[:, 2] < chest)
            # everything below the chest moves up by its distance to the chest times the shortening
            shortened[below, 2] = chest - (chest - basis[below, 2]) * (1.0 - JACKET_SHORTENING)
            made.append(set_key(obj, "jacket_short", shortened))
        elif "shoes" in name:
            basis = basis_coordinates(obj)
            grown = basis.copy()
            for side in (basis[:, 0] < 0, basis[:, 0] >= 0):
                center = basis[side].mean(axis=0)
                bottom = basis[side][:, 2].min()
                scaled = center + (basis[side] - center) * (1.0 + BOOT_GROWTH)
                # grown boots keep standing on the ground
                scaled[:, 2] += bottom - scaled[:, 2].min()
                grown[side] = scaled
            made.append(set_key(obj, "boots_grow", grown))
    return made
