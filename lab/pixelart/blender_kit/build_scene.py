"""Generation: builds one piece of an asset in Blender and saves it as a scene file.

Headless:  blender -b --python build_scene.py -- <asset.json> <piece> <scene.blend>
Live (e.g. from an MCP session):  exec this file with KIT_DIR in its globals, then build(asset_json, piece)

The piece's "build" names a function of the kit or of the asset's scene scripts (a scripted building,
or build_model for a model file). Whatever it makes (geometry, materials, camera, lights, render
size) goes into the scene file; the conversion (run_passes.py, then the pixelart package) only reads
that file, so a scene from any source converts the same way. Lettering areas the build recorded go
to <piece>_inscriptions.json next to the scene file.
"""
import json
import os
import sys

import bpy

# exec'd into a live Blender there is no __file__: the caller puts KIT_DIR into the globals first
KIT_DIR = os.path.dirname(os.path.abspath(__file__)) if "__file__" in globals() else globals()["KIT_DIR"]
# scene property the conversion reads back: light the true normals (see core.LIGHT_TRUE_NORMALS)
TRUE_NORMALS_PROPERTY = "pixelart_true_normals"


def _exec(path, ns):
    with open(path, encoding="utf-8") as f:
        exec(compile(f.read(), path, "exec"), ns)


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
    bpy.context.scene[TRUE_NORMALS_PROPERTY] = bool(ns["LIGHT_TRUE_NORMALS"])
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


def keep_images_with_scene():
    """Images that exist only in memory (textures unpacked from a .glb) go into the scene file;
    images on disk stay linked by path."""
    for image in bpy.data.images:
        on_disk = image.filepath and os.path.isfile(bpy.path.abspath(image.filepath))
        if (image.source == "FILE") and (image.packed_file is None) and not on_disk and image.has_data:
            image.pack()


def save_scene(scene_path):
    os.makedirs(os.path.dirname(os.path.abspath(scene_path)), exist_ok=True)
    keep_images_with_scene()
    bpy.ops.wm.save_as_mainfile(filepath=os.path.abspath(scene_path), check_existing=False)


def run(asset_json, piece, scene_path):
    ns = build(asset_json, piece)
    write_inscriptions(ns, os.path.join(os.path.dirname(os.path.abspath(scene_path)), f"{piece}_inscriptions.json"))
    save_scene(scene_path)
    print(f"pixelart: built {piece} into {scene_path}")


if "--" in sys.argv:
    run(*sys.argv[sys.argv.index("--") + 1:][:3])
