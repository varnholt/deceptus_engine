"""Builds one piece of an asset inside Blender and renders the three passes the stylizer reads.

Headless:  blender -b --python run_passes.py -- <asset.json> <piece> <out_dir>
Live (e.g. from an MCP session):  exec this file with KIT_DIR in its globals, then run(asset_json, piece, out_dir)

Writes <out_dir>/<piece>_beauty.png, _ids.png and _normals.png.
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
    ns[piece_cfg["build"]](**piece_cfg.get("build_args", {}))
    return ns


def run(asset_json, piece, out_dir):
    ns = build(asset_json, piece)
    device = use_gpu()
    os.makedirs(out_dir, exist_ok=True)
    base = os.path.join(out_dir, piece)
    ns["render"](base + "_beauty.png")
    ns["render_ids"](base + "_ids.png")
    ns["render_normals"](base + "_normals.png")
    print(f"pixelart: rendered {piece} on {device} into {out_dir}")


if "--" in sys.argv:
    run(*sys.argv[sys.argv.index("--") + 1:][:3])
