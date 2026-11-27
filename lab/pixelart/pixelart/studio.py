"""What the GUI's studio page (studio_page.html) does: one workflow from a source to pixel art.

  source      a model file (glTF/GLB, FBX, OBJ, .blend; framed by blender_kit/model.py) or a level
              piece built by a scene script (an asset config's piece with a "paint" section)
  generation  Blender builds the source into the piece's scene file (<piece>_scene.blend)
  conversion  the scene's passes are rendered and reduced, the paint stage draws the art
  output      save the art anywhere, or apply it to its level (pieces with a "tiled" entry)

Pieces stylized by stylize.py (the grave rows) stay on the stylizer page (gui_page.html).
"""
import inspect
import json
import shutil
from pathlib import Path

import numpy as np
from PIL import Image

from . import model, paint, workbench
from .__main__ import load, render_dir, scene_path, stylize_piece

MODEL_TYPES = (".glb", ".gltf", ".fbx", ".obj", ".blend")
UPLOAD_DIR = model.MODELS_DIR / "uploads"
ASSETS_DIR = Path(__file__).resolve().parents[1] / "assets"
RECENT_FILE = Path.home() / ".pixelart" / "recent_models.json"
RECENT_LIMIT = 12
PASS_KINDS = ("light", "turn", "albedo", "ids", "normals", "ao", "depth")
MAX_SWATCHES = 96
# paint() parameters that are not drawing settings
NOT_SETTINGS = ("directory", "piece", "out_path", "palette", "seed")


def paint_defaults():
    """The paint stage's own defaults: what a piece gets for every setting its config leaves out."""
    return {name: p.default for name, p in inspect.signature(paint.paint).parameters.items() if name not in NOT_SETTINGS}


def is_model(cfg, piece):
    return cfg["pieces"][piece]["build"] == "build_model"


def level_pieces():
    """Every piece the paint stage draws, in the asset configs under assets/."""
    found = []
    for asset in sorted(ASSETS_DIR.glob("*/*.json")):
        try:
            cfg = json.loads(asset.read_text(encoding="utf-8"))
        except ValueError:
            continue
        for piece, entry in cfg.get("pieces", {}).items():
            if "paint" in entry:
                found.append({"asset": str(asset), "piece": piece, "title": entry.get("title", piece),
                              "group": cfg.get("title", asset.stem)})
    return found


def defaults():
    return {"view": {**model.VIEW_DEFAULTS, **model.LIGHT_DEFAULTS}, "paint": model.PAINT_DEFAULTS,
            "types": list(MODEL_TYPES), "recent": recent_models(), "pieces": level_pieces()}


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


def art_path(cfg, piece):
    return cfg["_work"] / f"{piece}_px.png"


def source_of(cfg, piece):
    """What the piece is generated from, in words: a model file, or scene scripts and their build."""
    entry = cfg["pieces"][piece]
    if is_model(cfg, piece):
        return {"kind": "model", "name": Path(entry["build_args"]["path"]).name, "where": entry["build_args"]["path"]}
    scripts = ", ".join(Path(s).name for s in cfg["scenes"])
    return {"kind": "scene", "name": entry.get("title", piece), "where": f"{scripts} · {entry['build']}()"}


def state(asset, piece):
    """Everything the page shows about a piece: its source, settings, scene file, passes and art."""
    cfg = load(asset)
    work = cfg["_work"]
    entry = cfg["pieces"][piece]
    art = art_path(cfg, piece)
    view = model.settings(asset)[0] if is_model(cfg, piece) else None
    out = {
        "asset": cfg["_path"],
        "piece": piece,
        "source": source_of(cfg, piece),
        "view": view,
        "paint": {**paint_defaults(), **entry["paint"]},
        "scene": stamped(scene_path(cfg, piece)),
        "render": stamped(render_dir(cfg, piece) / f"{piece}_beauty.png"),
        "passes": {kind: stamped(work / f"{piece}_{kind}.png") for kind in PASS_KINDS},
        "art": stamped(art),
        "normals": stamped(paint.normals_path(art)),
        "level": entry["tiled"]["layer"] if "tiled" in entry else None,
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
    return state(asset, model.PIECE)


def open_piece(asset, piece):
    """Opens a level piece of an asset config."""
    cfg = load(asset)
    if piece not in cfg["pieces"]:
        raise ValueError(f"{Path(asset).name} has no piece '{piece}'")
    if "paint" not in cfg["pieces"][piece]:
        raise ValueError(f"'{piece}' is drawn by the stylizer, not the paint stage: open it on the stylizer page")
    return state(asset, piece)


def save_upload(name, data):
    """A model dropped onto the page, kept under work/models/uploads; returns its path."""
    name = Path(name).name
    if Path(name).suffix.lower() not in MODEL_TYPES:
        raise ValueError(f"not a model file: {name}")
    UPLOAD_DIR.mkdir(parents=True, exist_ok=True)
    target = UPLOAD_DIR / name
    target.write_bytes(data)
    return str(target)


def store_paint_settings(asset, piece, paint_settings):
    """Writes the paint settings into the asset config: the keys it already has, plus every value
    that differs from the paint stage's default. The file is only written when something changed."""
    path = Path(asset)
    text = path.read_text(encoding="utf-8")
    cfg = json.loads(text)
    entry = cfg["pieces"][piece]
    stored = workbench.stored_values(entry["paint"], paint_settings, paint_defaults())
    if stored == entry["paint"]:
        return
    entry["paint"] = stored
    trailing_newline = "\n" if text.endswith("\n") else ""
    path.write_text(json.dumps(cfg, indent=2, ensure_ascii=False) + trailing_newline, encoding="utf-8")


def store_settings(asset, piece, view, paint_settings):
    cfg = load(asset)
    if is_model(cfg, piece):
        model.write_asset(model.model_file(asset), view, paint_settings)
    else:
        store_paint_settings(asset, piece, paint_settings)


def draw(asset, piece):
    """Conversion, last part: the paint stage draws the art from the passes (a second or two)."""
    stylize_piece(load(asset), piece)


def repaint(asset, piece, view, paint_settings):
    """Stores the settings and draws the art from the passes already rendered."""
    store_settings(asset, piece, view, paint_settings)
    if stamped(load(asset)["_work"] / f"{piece}_light.png") is None:
        return {**state(asset, piece), "needs_render": True}
    draw(asset, piece)
    return state(asset, piece)


def render(asset, piece, view, paint_settings, report):
    """Stores the settings, builds the scene file, renders its passes and draws the art."""
    store_settings(asset, piece, view, paint_settings)
    cfg = load(asset)
    report("Building the scene in Blender…")
    log = workbench.build_scene(cfg, piece)
    report("Rendering the passes…")
    log += workbench.render_passes(cfg, piece)
    report("Painting the pixel art…")
    draw(asset, piece)
    return state(asset, piece), log


def apply_to_level(asset, piece, report):
    """Writes the drawn art into the piece's level (its Tiled map) as its tileset and layer."""
    cfg = load(asset)
    if "tiled" not in cfg["pieces"][piece]:
        raise ValueError(f"'{piece}' has no level to go into")
    report("Putting the art into the level…")
    return state(asset, piece), workbench.integrate(cfg, piece)


def save_art(asset, piece, out_path):
    """Copies the art and its normal map to out_path (and <name>_normals.png beside it)."""
    art = art_path(load(asset), piece)
    if not art.is_file():
        raise ValueError("nothing painted yet")
    out = Path(out_path).expanduser()
    if out.suffix.lower() != ".png":
        out = out / art.name if out.is_dir() else out.with_suffix(".png")
    out.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(art, out)
    shutil.copyfile(paint.normals_path(art), paint.normals_path(out))
    return {"saved": str(out), "normals": str(paint.normals_path(out))}
