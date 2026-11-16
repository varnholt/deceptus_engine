"""Gives the MPFB character dstar's chibi proportions with MakeHuman's own body targets, then refits the clothes.

Run inside the live Blender: exec(open(path).read()); apply_proportions(PROPORTIONS)

His sprite is 42 pixels tall and the head with hair takes about 15 of them; the legs are short, the neck hardly
shows. MakeHuman's targets reshape the mesh itself (unlike bone scales, which also thinned the limbs), and
MPFB refits the clothes, hair and eyes onto the reshaped body. What the targets cannot reach, an extra uniform
head scale on the head bone adds (hair, eyes and eyebrows follow that bone).
"""

import os

import bpy

from bl_ext.blender_org.mpfb.services.humanservice import HumanService
from bl_ext.blender_org.mpfb.services.locationservice import LocationService
from bl_ext.blender_org.mpfb.services.targetservice import TargetService

BODY_NAME = "adam_body"
RIG_NAME = "adam_body.rig"
# target file (relative to MPFB's targets) -> weight 0..1
PROPORTIONS = {
    r"head\head-scale-vert-incr.target.gz": 1.0,
    r"head\head-scale-horiz-incr.target.gz": 1.0,
    r"head\head-scale-depth-incr.target.gz": 1.0,
    r"head\head-age-decr.target.gz": 1.0,
    r"legs\measure-upperleg-height-decr.target.gz": 1.0,
    r"legs\measure-lowerleg-height-decr.target.gz": 1.0,
    r"arms\measure-upperarm-length-decr.target.gz": 0.6,
    r"arms\measure-lowerarm-length-decr.target.gz": 0.6,
    r"neck\measure-neck-height-decr.target.gz": 1.0,
}
# on top of the targets: the head bone's uniform scale
HEAD_BONE_SCALE = 1.0


def apply_proportions(proportions, head_bone_scale=HEAD_BONE_SCALE):
    body = bpy.data.objects[BODY_NAME]
    targets = LocationService.get_mpfb_data("targets")
    for relative, weight in proportions.items():
        TargetService.load_target(body, os.path.join(targets, relative), weight=weight)
    HumanService.refit(body)
    rig = bpy.data.objects[RIG_NAME]
    rig.pose.bones["head"].scale = (head_bone_scale,) * 3
    bpy.context.view_layer.update()
    coordinates = [vertex.co.z for vertex in body.data.vertices]
    return round(max(coordinates) - min(coordinates), 3)
