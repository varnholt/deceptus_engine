"""From a 3D model file (glTF/GLB, FBX, OBJ, .blend) to pixel art, with no scene script.

The model gets its own small asset config (work/models/<name>/<name>.json, one piece called
"model") built from the view and paint settings, so it runs through the same steps as the
scripted pieces: render the passes in Blender (blender_kit/model.py frames it), reduce them, and
draw the art with the paint stage in colour families (paint.colour_families).

usage: python -m pixelart.model <model file> [--height 64] [--yaw 0] [--pitch 0] [--colours 6]
writes work/models/<name>/model_px.png and model_px_normals.png
"""
import argparse
import json
import math
from pathlib import Path

from . import paint
from .__main__ import load, render

MODELS_DIR = Path(__file__).resolve().parents[1] / "work" / "models"
PIECE = "model"
VIEW_DEFAULTS = {"height_px": 64, "yaw": 0.0, "pitch": 0.0, "margin_px": 2, "frame": None}
# where the key light comes from, in the picture: degrees from straight ahead towards the left, and
# degrees above the horizon (the paint stage draws rims and cast lines for light from the upper left)
LIGHT_DEFAULTS = {"light_side": 40.0, "light_height": 30.0}
PAINT_DEFAULTS = {"colours": 8, "hue_shift": 0.25, "gain": 1.0, "contrast": 1.0, "saturation": 1.0,
                  "night": 0.0, "antialias": 0.6, "wall_marks": False, "grow_moss": False}
RENDER_SCALE = 4


def asset_path(model_file):
    stem = Path(model_file).stem
    return MODELS_DIR / stem / f"{stem}.json"


def key_direction(side_deg, height_deg):
    """The direction the key light travels in the front camera's frame (x right, y into the picture,
    z up), for light coming from side_deg towards the left of straight ahead, height_deg up."""
    side, height = math.radians(side_deg), math.radians(height_deg)
    return [math.sin(side) * math.cos(height), math.cos(side) * math.cos(height), -math.sin(height)]


def write_asset(model_file, view=None, paint_settings=None):
    """Writes (or updates) the model's asset config and returns its path. view holds VIEW_DEFAULTS
    and LIGHT_DEFAULTS keys, paint_settings PAINT_DEFAULTS keys; missing ones take the defaults."""
    model_file = str(Path(model_file).resolve())
    path = asset_path(model_file)
    path.parent.mkdir(parents=True, exist_ok=True)
    view = {**VIEW_DEFAULTS, **LIGHT_DEFAULTS, **(view or {})}
    light = {k: view.pop(k) for k in LIGHT_DEFAULTS}
    cfg = {
        "title": Path(model_file).stem,
        "origin": [0, 0],
        "kit": ["model"],
        "scenes": [],
        "work_dir": ".",
        "stage": {"key_dir": key_direction(light["light_side"], light["light_height"])},
        "light": light,
        "pieces": {PIECE: {
            "title": Path(model_file).name,
            "build": "build_model",
            "build_args": {"path": model_file, **view},
            "render_scale": RENDER_SCALE,
            "paint_passes": True,
            "paint": {**PAINT_DEFAULTS, **(paint_settings or {})},
        }},
    }
    path.write_text(json.dumps(cfg, indent=2), encoding="utf-8")
    return path


def settings(asset):
    """(view including the light, paint settings) stored in a model's asset config."""
    cfg = load(asset)
    piece = cfg["pieces"][PIECE]
    view = {k: v for k, v in piece["build_args"].items() if k != "path"}
    return {**view, **cfg["light"]}, piece["paint"]


def model_file(asset):
    """The model file an asset config was made from."""
    return load(asset)["pieces"][PIECE]["build_args"]["path"]


def art_path(asset):
    return Path(asset).parent / f"{PIECE}_px.png"


def render_model(asset):
    """Renders the passes in Blender and reduces them to level size (about half a minute)."""
    render(load(asset), PIECE)


def paint_model(asset, out_path=None):
    """Draws the art from the rendered passes (a second or two) and returns its path."""
    cfg = load(asset)
    out = Path(out_path) if out_path else art_path(asset)
    paint.paint(cfg["_work"], PIECE, out, **cfg["pieces"][PIECE]["paint"])
    return out


def main(argv=None):
    parser = argparse.ArgumentParser(prog="pixelart.model", description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("model")
    parser.add_argument("--height", type=int, default=VIEW_DEFAULTS["height_px"], help="picture height of the model in pixels")
    parser.add_argument("--yaw", type=float, default=VIEW_DEFAULTS["yaw"], help="degrees the camera turns around the model")
    parser.add_argument("--pitch", type=float, default=VIEW_DEFAULTS["pitch"], help="degrees the camera looks down")
    parser.add_argument("--colours", type=int, default=PAINT_DEFAULTS["colours"], help="colour families (ramps)")
    args = parser.parse_args(argv)
    asset = write_asset(args.model, {"height_px": args.height, "yaw": args.yaw, "pitch": args.pitch},
                        {"colours": args.colours})
    render_model(asset)
    print("wrote", paint_model(asset))


if __name__ == "__main__":
    main()
