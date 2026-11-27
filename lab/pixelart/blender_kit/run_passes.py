"""Conversion, Blender part: renders the passes of a scene file that build_scene.py saved.

Headless:  blender -b <scene.blend> --python run_passes.py -- <piece> <out_dir> [paint]
Live (e.g. from an MCP session):  exec this file with KIT_DIR in its globals, then
run_live(asset_json, piece, out_dir) builds the piece in the open Blender and renders it there.

Writes <out_dir>/<piece>_beauty.png, _ids.png and _normals.png at the scene's render size (a piece
with a render_scale above 1 was built that many times larger; the caller reduces the passes, see
pixelart/reduce.py). With "paint" it also writes the paint stage's passes: _light.png (key light
alone on white matt surfaces), _turn.png (the same without cast shadows), _classes.png (material
class per pixel), _albedo.png (each material's colour without light), _depth.png (16-bit distance
from the camera) and _ao.png (ambient occlusion).
"""
import json
import os
import sys

import bpy

KIT_DIR = os.path.dirname(os.path.abspath(__file__)) if "__file__" in globals() else globals()["KIT_DIR"]
TRUE_NORMALS_PROPERTY = "pixelart_true_normals"   # written by build_scene.py


def use_gpu():
    """Cycles on the first GPU backend this machine offers; stays on the CPU otherwise."""
    prefs = bpy.context.preferences.addons["cycles"].preferences
    for backend in ("OPTIX", "CUDA", "HIP", "ONEAPI", "METAL"):
        try:
            prefs.compute_device_type = backend
        except TypeError:
            continue
        prefs.get_devices()
        gpus = [d for d in prefs.devices if d.type != "CPU"]
        if gpus:
            for d in prefs.devices:
                d.use = d.type != "CPU"
            bpy.context.scene.cycles.device = "GPU"
            return backend
    return "CPU"


def pass_renderers():
    """The kit's pass functions, set up for the open scene."""
    ns = {"__name__": "pixelart_passes"}
    with open(os.path.join(KIT_DIR, "core.py"), encoding="utf-8") as f:
        exec(compile(f.read(), "core.py", "exec"), ns)
    ns["LIGHT_TRUE_NORMALS"] = bool(bpy.context.scene.get(TRUE_NORMALS_PROPERTY, False))
    return ns


def render_passes(piece, out_dir, paint_passes):
    ns = pass_renderers()
    device = use_gpu()
    os.makedirs(out_dir, exist_ok=True)
    base = os.path.join(out_dir, piece)
    ns["render"](base + "_beauty.png")
    ns["render_ids"](base + "_ids.png")
    ns["render_normals"](base + "_normals.png")
    if paint_passes:
        ns["render_light"](base + "_light.png")
        ns["render_light"](base + "_turn.png", shadows=False)
        ns["render_material_classes"](base + "_classes.png")
        ns["render_albedo"](base + "_albedo.png")
        ns["render_depth"](base + "_depth.png")
        ns["render_ao"](base + "_ao.png")
    print(f"pixelart: rendered {piece} on {device} into {out_dir}")


def run_live(asset_json, piece, out_dir):
    """Build and render in one Blender session (the live MCP route)."""
    ns = {"KIT_DIR": KIT_DIR}
    with open(os.path.join(KIT_DIR, "build_scene.py"), encoding="utf-8") as f:
        exec(compile(f.read(), "build_scene.py", "exec"), ns)
    built = ns["build"](asset_json, piece)
    ns["write_inscriptions"](built, os.path.join(out_dir, f"{piece}_inscriptions.json"))
    paint_passes = json.loads(open(asset_json, encoding="utf-8").read())["pieces"][piece].get("paint_passes", False)
    render_passes(piece, out_dir, paint_passes)


if "--" in sys.argv:
    args = sys.argv[sys.argv.index("--") + 1:]
    render_passes(args[0], args[1], (len(args) > 2) and (args[2] == "paint"))
