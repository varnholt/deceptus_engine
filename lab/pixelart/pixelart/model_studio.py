"""What the GUI's model page (model_page.html) does with a model: open it, keep its settings, paint
it, render it, save the art. The page shows one model at a time: its input file, its settings and
the render beside the pixel art.
"""
import json
import shutil
from pathlib import Path

import numpy as np
from PIL import Image

from . import model, paint, workbench
from .__main__ import load

MODEL_TYPES = (".glb", ".gltf", ".fbx", ".obj", ".blend")
UPLOAD_DIR = model.MODELS_DIR / "uploads"
RECENT_FILE = Path.home() / ".pixelart" / "recent_models.json"
RECENT_LIMIT = 12
PASS_KINDS = ("light", "turn", "albedo", "ids", "normals", "ao", "depth")
MAX_SWATCHES = 96


def defaults():
    return {"view": {**model.VIEW_DEFAULTS, **model.LIGHT_DEFAULTS}, "paint": model.PAINT_DEFAULTS,
            "types": list(MODEL_TYPES), "recent": recent_models()}


def recent_models():
    try:
        return json.loads(RECENT_FILE.read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return []


def remember_model(path):
    entries = [path] + [p for p in recent_models() if p != path]
    RECENT_FILE.parent.mkdir(parents=True, exist_ok=True)
    RECENT_FILE.write_text(json.dumps(entries[:RECENT_LIMIT], indent=2), encoding="utf-8")


def stamped(path):
    """A path with its change time, so the page reloads an image only when it changed."""
    return {"path": str(path), "time": path.stat().st_mtime} if path.is_file() else None


def palette_of(art):
    """The art's colours, darkest first (at most MAX_SWATCHES)."""
    rgba = np.asarray(Image.open(art).convert("RGBA"))
    colours = np.unique(rgba[rgba[..., 3] >= 128][:, :3], axis=0)
    order = np.argsort(colours @ np.array([0.299, 0.587, 0.114]))
    return ["#%02x%02x%02x" % tuple(int(v) for v in c) for c in colours[order][:MAX_SWATCHES]]


def model_name(asset):
    return Path(model.model_file(asset)).name


def state(asset):
    """Everything the page shows about a model: its settings, render, passes and art."""
    asset = Path(asset)
    work = asset.parent
    view, paint_settings = model.settings(asset)
    art = model.art_path(asset)
    out = {
        "asset": str(asset),
        "model": model.model_file(asset),
        "name": model_name(asset),
        "view": view,
        "paint": paint_settings,
        "render": stamped(work / f"{model.PIECE}_x{model.RENDER_SCALE}" / f"{model.PIECE}_beauty.png"),
        "passes": {kind: stamped(work / f"{model.PIECE}_{kind}.png") for kind in PASS_KINDS},
        "art": stamped(art),
        "normals": stamped(paint.normals_path(art)),
    }
    if art.is_file():
        with Image.open(art) as im:
            out["size"] = list(im.size)
        out["palette"] = palette_of(art)
    return out


def open_model(path):
    """Opens a model file, keeping the settings it had last time."""
    path = Path(path).expanduser().resolve()
    if path.suffix.lower() not in MODEL_TYPES:
        raise ValueError(f"not a model file: {path.name} (use {', '.join(MODEL_TYPES)})")
    if not path.is_file():
        raise ValueError(f"no such file: {path}")
    asset = model.asset_path(path)
    if (not asset.is_file()) or (model.model_file(asset) != str(path)):
        model.write_asset(path)
    remember_model(str(path))
    return state(asset)


def save_upload(name, data):
    """A model dropped onto the page, kept under work/models/uploads; returns its path."""
    name = Path(name).name
    if Path(name).suffix.lower() not in MODEL_TYPES:
        raise ValueError(f"not a model file: {name}")
    UPLOAD_DIR.mkdir(parents=True, exist_ok=True)
    target = UPLOAD_DIR / name
    target.write_bytes(data)
    return str(target)


def store_settings(asset, view, paint_settings):
    model.write_asset(model.model_file(asset), view, paint_settings)


def repaint(asset, view, paint_settings):
    """Stores the settings and draws the art from the passes already rendered (a second or two)."""
    store_settings(asset, view, paint_settings)
    if stamped(Path(asset).parent / f"{model.PIECE}_light.png") is None:
        return {**state(asset), "needs_render": True}
    model.paint_model(asset)
    return state(asset)


def render(asset, view, paint_settings, report):
    """Stores the settings, renders the passes in Blender and draws the art."""
    store_settings(asset, view, paint_settings)
    report("Rendering the model in Blender…")
    log = workbench.render(load(asset), model.PIECE)
    report("Painting the pixel art…")
    model.paint_model(asset)
    return state(asset), log


def save_art(asset, out_path):
    """Copies the art and its normal map to out_path (and <name>_normals.png beside it)."""
    art = model.art_path(asset)
    if not art.is_file():
        raise ValueError("nothing painted yet")
    out = Path(out_path).expanduser()
    if out.suffix.lower() != ".png":
        out = out / art.name if out.is_dir() else out.with_suffix(".png")
    out.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(art, out)
    shutil.copyfile(paint.normals_path(art), paint.normals_path(out))
    return {"saved": str(out), "normals": str(paint.normals_path(out))}
