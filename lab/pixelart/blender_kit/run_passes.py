"""Builds one piece of an asset inside Blender and renders the three passes the stylizer reads.

Headless:  blender -b --python run_passes.py -- <asset.json> <piece> <out_dir>
Live (e.g. from an MCP session):  exec this file with KIT_DIR in its globals, then run(asset_json, piece, out_dir)

Writes <out_dir>/<piece>_beauty.png, _ids.png and _normals.png, and _inscriptions.json when the
piece has lettering areas (see graves.lettering). A piece with a render_scale above 1 renders the passes
that many times larger (the caller reduces them, see pixelart/reduce.py); inscriptions stay in level pixels.
"""
import json
import os
import sys

import bpy

# exec'd into a live Blender there is no __file__: the caller puts KIT_DIR into the globals first
KIT_DIR = os.path.dirname(os.path.abspath(__file__)) if "__file__" in globals() else globals()["KIT_DIR"]


def _exec(path, ns):
    with open(path, encoding="utf-8") as f:
        exec(compile(f.read(), path, "exec"), ns)


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


def build(asset_json, piece):
    """Executes the kit and the asset's scene scripts into one namespace and builds the piece."""
    cfg = json.loads(open(asset_json, encoding="utf-8").read())
    asset_dir = os.path.dirname(os.path.abspath(asset_json))
    ns = {"__name__": "pixelart_scene"}
    _exec(os.path.join(KIT_DIR, "core.py"), ns)
    ns["ORIGIN_X_PX"], ns["GROUND_PY"] = cfg["origin"]
    ns["SCAN_DIR"] = cfg.get("scan_dir", os.path.join(os.path.dirname(KIT_DIR), "textures")).replace("\\", "/")
    ns["STAGE"].update({k: tuple(v) if isinstance(v, list) else v for k, v in cfg.get("stage", {}).items()})
    for module in cfg.get("kit", []):
        _exec(os.path.join(KIT_DIR, module + ".py"), ns)
    for scene in cfg["scenes"]:
        _exec(os.path.join(asset_dir, scene), ns)
    piece_cfg = cfg["pieces"][piece]
    ns["RENDER_SCALE"] = piece_cfg.get("render_scale", 1)
    ns[piece_cfg["build"]](**piece_cfg.get("build_args", {}))
    return ns


def image_point(scene, world_point):
    """Pixel position (x right, y down) of a world point in the rendered image."""
    from bpy_extras.object_utils import world_to_camera_view
    ndc = world_to_camera_view(scene, scene.camera, world_point)
    return [ndc.x * scene.render.resolution_x, (1.0 - ndc.y) * scene.render.resolution_y]


def write_inscriptions(ns, path):
    """The lettering areas as image coordinates: the top line's centre and one level pixel
    along the face to the right and up, so the carving can follow the face's turn and lean."""
    areas = ns.get("INSCRIPTIONS") or []
    if not areas:
        return
    scene = bpy.context.scene
    out = []
    for area in areas:
        centre = image_point(scene, area["centre"])
        right, up = image_point(scene, area["right"]), image_point(scene, area["up"])
        out.append(dict(centre=centre, right=[right[0] - centre[0], right[1] - centre[1]],
                        up=[up[0] - centre[0], up[1] - centre[1]], half_px=area["half_px"],
                        lines=area["lines"], line_pitch_px=area["line_pitch_px"], seed=area["seed"]))
    with open(path, "w", encoding="utf-8") as f:
        json.dump(out, f, indent=1)


def run(asset_json, piece, out_dir):
    ns = build(asset_json, piece)
    device = use_gpu()
    os.makedirs(out_dir, exist_ok=True)
    base = os.path.join(out_dir, piece)
    ns["render"](base + "_beauty.png")
    ns["render_ids"](base + "_ids.png")
    ns["render_normals"](base + "_normals.png")
    write_inscriptions(ns, base + "_inscriptions.json")
    print(f"pixelart: rendered {piece} on {device} into {out_dir}")


if "--" in sys.argv:
    run(*sys.argv[sys.argv.index("--") + 1:][:3])
