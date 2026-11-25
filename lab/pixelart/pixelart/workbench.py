"""Tuning logic shared by the GUI (gui.py) and the MCP server (mcp_server.py).

Everything here works on a Target: three render passes plus the places its previews and its written
art go. Previews never land on <piece>_px.png; only write_art() writes there, on request.
"""
import contextlib
import inspect
import io
import json
import math
import os
import shutil
import subprocess
import sys
import threading
from dataclasses import dataclass
from pathlib import Path
from xml.etree import ElementTree

from PIL import Image

from . import __main__ as pipeline
from . import compare, palette, stylize
from .metrics import metrics

# pixelize() parameters that are not style options
PASS_PARAMETERS = {"beauty_p", "ids_p", "normals_p", "out_p", "palette"}
RAMP_NAMES = ("stone", "foliage", "passage")
PREVIEW_FOLDER = "gui"
# larger MCP previews cost the model more than they show; the zoom drops until the image fits
MAX_PREVIEW_WIDTH = 2048

# group "fit" sits open under "Fit to the level"; "look" is folded away under "Stone look".
# needs names the option (and the condition on it) without which this one changes nothing.
OPTION_RANGES = {
    "gain": dict(kind="float", min=0.3, max=1.5, step=0.02, group="fit", label="Brightness",
                 help="Makes the whole piece lighter or darker, to sit it into the night of the level. Lower = darker."),
    "contrast": dict(kind="float", min=0.5, max=1.5, step=0.02, group="fit", label="Contrast",
                     help="Spreads the shades apart or pulls them together around the piece's middle tone. Lower = flatter, more misty."),
    "saturation": dict(kind="float", min=0.0, max=1.5, step=0.05, group="fit", label="Colour",
                       help="How colourful the piece is. 0 = grey, 1 = as painted, higher = stronger colours."),
    "night": dict(kind="float", min=0.0, max=0.8, step=0.02, group="fit", label="Night tint",
                  help="Pulls every colour towards the night blue of the scenery. Higher = bluer, more part of the background."),
    "shift": dict(kind="int", min=-4, max=4, step=1, group="fit", label="Tone shift",
                  help="Moves every stone whole shades darker or lighter, keeping the lit top edges. Negative = darker, for pieces further back."),
    "value_tones": dict(kind="bool", group="look", label="Tones from light",
                        help="On: each stone's shade follows how bright it is in the render, so lit stones stay light. Off: shades are dealt out in fixed shares (flatter, muddier)."),
    "base_shares": dict(kind="shares", min=0.0, max=1.0, step=0.05, group="look", label="Tone shares", needs=("value_tones", "==", False),
                        help="How much of the piece gets each of the four stone shades, darkest first. Only used while Tones from light is off."),
    "soft": dict(kind="float", min=0.0, max=1.0, step=0.05, group="look", label="Soft shading",
                 help="Keeps the render's own light and shade inside each stone as in-between shades. 0 = every stone one flat shade; higher = rounder stones."),
    "sub_steps": dict(kind="int", min=1, max=6, step=1, group="look", label="In-between shades", needs=("soft", ">", 0),
                      help="How many in-between shades soft shading may use between two palette colours. More = smoother, but more colours."),
    "bevel": dict(kind="int", min=0, max=5, step=1, group="look", label="Bevel", needs=("soft", ">", 0),
                  help="Width in pixels of the lit band along each stone's top and left edge and the shade towards its bottom right (dstar's pillowed stones). 0 = none."),
    "round_corners": dict(kind="bool", group="look", label="Rounded corners",
                          help="Cuts the corner pixel of every stone into the joint, so blocks look worn instead of machined."),
    "edge_light": dict(kind="int", min=0, max=3, step=1, group="look", label="Edge highlight",
                       help="How many shades lighter the top edge of each stone is than the stone itself. Lower = softer, fewer bright lines."),
    "texture": dict(kind="float", min=0.0, max=1.5, step=0.05, group="look", label="Surface patches", needs=("soft", ">", 0),
                    help="Irregular lighter and darker patches across each stone's face, so no stone is one flat tone. 0 = smooth."),
    "pits": dict(kind="float", min=0.0, max=0.05, step=0.002, group="look", label="Pits", needs=("soft", ">", 0),
                 help="Small dents: a dark pixel with a lighter one below it. The value is how many of the stone's pixels get one."),
    "scratches": dict(kind="float", min=0.0, max=0.02, step=0.001, group="look", label="Scratches", needs=("soft", ">", 0),
                      help="Short dark diagonal marks two or three pixels long. The value is how often one starts."),
    "texture_seed": dict(kind="int", min=0, max=99, step=1, group="look", label="Texture variation",
                         help="Picks another random layout of patches, pits and scratches; the look stays the same."),
    "form": dict(kind="float", min=0.0, max=2.0, step=0.1, group="look", label="Round forms", needs=("soft", ">", 0),
                 help="Shades round surfaces such as columns: light on the left, dark on the right. Higher = stronger."),
    "speck": dict(kind="float", min=0.0, max=1.0, step=0.05, group="look", label="Speckle", needs=("soft", ">", 0),
                  help="Fine speckle on the stone surface, taken from the render's texture. 0 = clean."),
    "k_scale": dict(kind="float", min=0.1, max=4.0, step=0.05, group="look", label="Calm patches",
                    help="How big a brightness difference inside a stone must be before part of it gets a lighter or darker patch. Higher = calmer, fewer patches."),
    "max_step": dict(kind="int", min=0, max=3, step=1, group="look", label="Patch depth",
                     help="How many shades a patch may be lighter or darker than the rest of its stone. 0 = no patches."),
    "grain": dict(kind="float", min=0.0, max=1.0, step=0.05, group="look", label="Grain",
                  help="0 = patches follow the broad light of the render; 1 = they follow every pixel (noisier)."),
    "crack_k": dict(kind="float", min=0.5, max=6.0, step=0.1, group="look", label="Cracks",
                    help="How much darker than its stone a spot must be to be drawn as a crack. Lower = more cracks."),
    "micro": dict(kind="int", min=0, max=12, step=1, group="look", label="Fine noise",
                  help="Strength of a fine one-pixel shade noise inside flat areas. 0 = calm."),
    "micro_k": dict(kind="float", min=0.2, max=4.0, step=0.1, group="look", label="Noise threshold", needs=("micro", ">", 0),
                    help="How much the render must vary before the fine noise shows. Higher = less noise."),
    "moss_clumps": dict(kind="bool", group="look", label="Moss clumps",
                        help="Gathers scattered moss pixels into clumps with a dark underside and a lit top, the way dstar paints moss."),
    "foot_rows": dict(kind="int", min=0, max=8, step=1, group="look", label="Ground shadow",
                      help="Darkens this many of the lowest rows where the piece meets the ground (contact shadow). 0 = none."),
    "outline_level": dict(kind="int", min=0, max=7, step=1, group="look", label="Outline",
                          help="How dark the outline around the piece is. 0 = black; higher = a dark stone colour, softer, for background pieces."),
}


@dataclass(frozen=True)
class Target:
    """Something to stylize: its three passes, a label for file names, and where results go."""
    beauty: Path
    ids: Path
    normals: Path
    label: str
    preview_dir: Path
    written_art: Path

    def preview_path(self, tag):
        return self.preview_dir / f"{self.label}_{tag}.png"


def target_for_piece(cfg, piece):
    if piece not in cfg["pieces"]:
        raise KeyError(f"no piece '{piece}' in {cfg['_path']}")
    beauty, ids, normals = pipeline.pass_paths(cfg, piece)
    return Target(beauty, ids, normals, piece, cfg["_work"] / PREVIEW_FOLDER, cfg["_work"] / f"{piece}_px.png")


def target_for_passes(beauty, ids, normals):
    """Passes from anywhere; results go next to the beauty pass, named after it without '_beauty'."""
    beauty = Path(beauty)
    label = beauty.stem.removesuffix("_beauty")
    return Target(beauty, Path(ids), Path(normals), label, beauty.parent / PREVIEW_FOLDER, beauty.parent / f"{label}_px.png")


def missing_passes(target):
    return [str(p) for p in (target.beauty, target.ids, target.normals) if not p.is_file()]


# ---------------------------------------------------------------- options

def _option_default(parameter):
    # base_shares defaults to None inside pixelize(), meaning the stylizer's own shares
    if parameter.name == "base_shares" and parameter.default is None:
        return list(stylize.BASE_SHARES)
    return parameter.default


def _inferred_range(default):
    """Range for an option pixelize() gained after OPTION_RANGES was written."""
    if isinstance(default, bool):
        return dict(kind="bool", help="")
    if isinstance(default, int):
        return dict(kind="int", min=min(default, 0) - 4, max=max(default, 0) + 4, step=1, help="")
    return dict(kind="float", min=0.0, max=max(float(default or 0), 1.0) * 3, step=0.05, help="")


def option_specs():
    """One spec per style option of pixelize(), read from its signature so new options show up."""
    specs = []
    for parameter in inspect.signature(stylize.pixelize).parameters.values():
        if parameter.name in PASS_PARAMETERS:
            continue
        default = _option_default(parameter)
        spec = OPTION_RANGES.get(parameter.name) or _inferred_range(default)
        specs.append({"name": parameter.name, "default": default, "group": "look", "label": parameter.name, **spec})
    # in the order OPTION_RANGES lists them (most used first), options it does not know at the end
    order = list(OPTION_RANGES)
    return sorted(specs, key=lambda spec: order.index(spec["name"]) if spec["name"] in order else len(order))


def default_options():
    return {spec["name"]: spec["default"] for spec in option_specs()}


def _coerce(spec, value):
    if spec["kind"] == "bool":
        return bool(value)
    if spec["kind"] == "int":
        return int(round(float(value)))
    if spec["kind"] == "shares":
        return [float(v) for v in value]
    return float(value)


def normalized_options(options):
    """Options typed the way pixelize() and the config expect them; unknown names are an error."""
    specs = {spec["name"]: spec for spec in option_specs()}
    unknown = set(options) - set(specs)
    if unknown:
        raise ValueError(f"unknown stylize options: {', '.join(sorted(unknown))}")
    return {name: _coerce(specs[name], value) for name, value in options.items()}


def piece_options(cfg, piece):
    """Every option with the value the piece uses: its saved ones over the stylizer defaults."""
    return {**default_options(), **normalized_options(cfg["pieces"][piece].get("stylize", {}))}


# ---------------------------------------------------------------- palette

def default_palette():
    return {"stone": stylize.RAMP.tolist(), "foliage": stylize.GREENS.tolist(), "passage": stylize.PASSAGE.tolist()}


def _as_ramps(source):
    return {name: [[int(c) for c in colour] for colour in source[name]] for name in RAMP_NAMES if name in source}


def piece_palette_source(cfg, piece):
    """Where the piece's palette comes from, in the order piece_palette() looks: piece, asset or default."""
    for scope, holder in (("piece", cfg["pieces"][piece]), ("asset", cfg)):
        if "palette" in holder or "palette_from" in holder:
            return scope
    return "default"


def resolved_palette(cfg, piece):
    """All three ramps the piece is stylized with; ramps its palette leaves out are the stylizer's."""
    return {**default_palette(), **_as_ramps(pipeline.piece_palette(cfg, piece) or {})}


def read_palette(image_path, box=None):
    return _as_ramps(palette.extract(image_path, tuple(box) if box else None))


# ---------------------------------------------------------------- stylize and look

_stylize_lock = threading.Lock()


def stylize_to(target, options, ramps, out_path):
    """Stylizes the target's passes with these options and ramps into out_path."""
    missing = missing_passes(target)
    if missing:
        raise FileNotFoundError(f"passes missing (render first): {', '.join(missing)}")
    out_path = Path(out_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    # previews of one target share a file, so two stylizes must not write it at once
    with _stylize_lock:
        stylize.pixelize(str(target.beauty), str(target.ids), str(target.normals), str(out_path),
                         palette=ramps, **normalized_options(options))
    return out_path


def write_art(target, options, ramps):
    """Writes the stylized art the pipeline integrates (<piece>_px.png)."""
    return stylize_to(target, options, ramps, target.written_art)


def keep_as_before(target, preview_path):
    """Keeps a preview as the 'before' image to flip against."""
    before = target.preview_path("before")
    shutil.copyfile(preview_path, before)
    return before


def _json_number(value):
    """A metric as a plain number; NaN (a box without opaque pixels) becomes None, which JSON can carry."""
    number = value.item() if hasattr(value, "item") else value
    return None if (isinstance(number, float) and math.isnan(number)) else number


def image_metrics(path, box=None):
    """Style numbers (metrics.py), ready for JSON."""
    numbers = metrics(str(path), tuple(box) if box else None)
    return {name: _json_number(value) for name, value in numbers.items()}


def zoomed_png(path, box=None, zoom=3, backdrop=compare.BACKDROP):
    """PNG bytes of an image (or a box of it) on the night backdrop, enlarged by whole pixels."""
    im = Image.open(path).convert("RGBA")
    if box:
        im = im.crop(tuple(box))
    zoom = max(1, min(int(zoom), MAX_PREVIEW_WIDTH // max(im.width, 1)))
    framed = Image.new("RGBA", im.size, tuple(backdrop))
    framed.alpha_composite(im)
    framed = framed.convert("RGB").resize((im.width * zoom, im.height * zoom), Image.NEAREST)
    buffer = io.BytesIO()
    framed.save(buffer, "PNG")
    return buffer.getvalue(), zoom


def reference_png(ref):
    """The reference as the artist painted it: the comparison sheets brighten dim parallax art (gain),
    but next to the level that only shows colours the game never draws."""
    buffer = io.BytesIO()
    compare.reference_image({**ref, "gain": None}).save(buffer, "PNG")
    return buffer.getvalue()


# ---------------------------------------------------------------- config

def _stored_values(saved, values, defaults):
    """What the config keeps: keys it already had, plus every value that differs from its default."""
    # keys the config already has keep their place, so a save only shows real changes in a diff
    kept = {name: values[name] for name in saved if name in values}
    added = {name: value for name, value in values.items() if (name not in saved) and (value != defaults.get(name))}
    return {**kept, **added}


def _palette_entry(existing, ramps):
    """A config palette entry for these ramps; notes ('_' keys) of the old entry stay."""
    stored = _stored_values(existing, ramps, default_palette())
    return {**{key: value for key, value in existing.items() if key.startswith("_")}, **stored}


def save_piece_settings(asset_path, piece, options, ramps=None, palette_scope="piece"):
    """Writes the piece's stylize options (and the palette, when given) into the asset JSON.

    The file is read fresh so edits made elsewhere since loading survive; other keys stay as they are.
    A palette saved at piece scope replaces a piece-level palette_from; at asset scope the asset's.
    """
    if palette_scope not in ("piece", "asset"):
        raise ValueError(f"palette scope must be 'piece' or 'asset', not '{palette_scope}'")
    path = Path(asset_path)
    text = path.read_text(encoding="utf-8")
    cfg = json.loads(text)
    entry = cfg["pieces"][piece]
    entry["stylize"] = _stored_values(entry.get("stylize", {}), normalized_options(options), default_options())
    if ramps is not None:
        holder = entry if palette_scope == "piece" else cfg
        holder["palette"] = _palette_entry(holder.get("palette", {}), _as_ramps(ramps))
        holder.pop("palette_from", None)
    trailing_newline = "\n" if text.endswith("\n") else ""
    path.write_text(json.dumps(cfg, indent=2, ensure_ascii=False) + trailing_newline, encoding="utf-8")
    return entry["stylize"]


def piece_summary(cfg, piece):
    target = target_for_piece(cfg, piece)
    entry = cfg["pieces"][piece]
    return {
        "name": piece,
        "title": entry.get("title", piece.capitalize()),
        "has_passes": not missing_passes(target),
        "passes": {"beauty": str(target.beauty), "ids": str(target.ids), "normals": str(target.normals)},
        "written_art": str(target.written_art),
        "saved_options": entry.get("stylize", {}),
        "options": piece_options(cfg, piece),
        "palette": resolved_palette(cfg, piece),
        "palette_source": piece_palette_source(cfg, piece),
        "compare_boxes": entry.get("compare_boxes", []),
        "capture_spots": entry.get("capture", []),
    }


# ---------------------------------------------------------------- pipeline steps

class _ThreadRoutedStream:
    """Stands in for sys.stdout so the pipeline's prints from one thread can be collected as that
    job's log while other threads keep printing normally."""

    def __init__(self, fallback):
        self.fallback = fallback
        self.sinks = threading.local()

    def write(self, text):
        return (getattr(self.sinks, "current", None) or self.fallback).write(text)

    def flush(self):
        self.fallback.flush()

    def __getattr__(self, name):
        return getattr(self.fallback, name)


_stdout_install_lock = threading.Lock()


@contextlib.contextmanager
def _collected_prints():
    with _stdout_install_lock:
        if not isinstance(sys.stdout, _ThreadRoutedStream):
            sys.stdout = _ThreadRoutedStream(sys.stdout)
    sink = io.StringIO()
    sys.stdout.sinks.current = sink
    try:
        yield sink
    finally:
        sys.stdout.sinks.current = None


class StepFailed(RuntimeError):
    def __init__(self, message, log):
        super().__init__(message)
        self.log = log


def _run_step(step, *args):
    """Runs a pipeline step, returning its result and what it printed. The pipeline ends failed steps
    with SystemExit (it is a command line tool); here that becomes StepFailed with the log."""
    with _collected_prints() as log:
        try:
            result = step(*args)
        except SystemExit as failure:
            raise StepFailed(str(failure), log.getvalue()) from None
    return result, log.getvalue()


def render(cfg, piece):
    """Headless Blender: writes the piece's three passes. Takes tens of seconds."""
    _, log = _run_step(pipeline.render, cfg, piece)
    return log


def integrate(cfg, piece):
    """Writes the piece's written art (<piece>_px.png) into the Tiled map of the engine checkout."""
    _, log = _run_step(pipeline.integrate, cfg, piece)
    return log


def capture(cfg, pieces, label):
    """Runs the game over the pieces' capture spots; returns the frame paths and the log."""
    frames_dir, log = _run_step(pipeline.capture, cfg, pieces, label)
    frames = sorted(str(p) for p in Path(frames_dir).glob("*.png")) if frames_dir else []
    return frames, log


def compare_sheet(cfg, piece, art=None, out=None):
    """Comparison sheet of the art (default: the written art) against the references; None without compare_boxes."""
    sheet, log = _run_step(pipeline.compare_piece, cfg, piece, art, out)
    return sheet, log


# ---------------------------------------------------------------- the piece in the level

RASTERIZER = os.environ.get("TMXRASTERIZER", "C:/Program Files/Tiled/tmxrasterizer.exe")
# effects drawn over the whole screen in the game; in a still they only hide the art
SCREEN_EFFECT_LAYERS = ("vignette", "atmosphere")
LEVEL_MARGIN_TILES = 10
MAP_LAYER_TAGS = ("layer", "imagelayer", "objectgroup", "group")


@dataclass(frozen=True)
class LevelView:
    """The level around a piece: what the game draws behind it and in front of it, and where the
    piece's top-left corner sits in those images."""
    behind: Path
    front: Path
    piece_offset: tuple


def _map_path(cfg):
    eng = cfg["engine"]
    return Path(eng["data_root"]) / eng["level_dir"] / eng["tmx"]


def _layer_names(tmx):
    root = ElementTree.parse(tmx).getroot()
    return [child.get("name") for child in root if child.tag in MAP_LAYER_TAGS]


def _layers_split_at_piece(names, tiled_entry):
    """Layer names drawn before and after the piece's layer. Before the first integrate the layer is
    not in the map yet; it will go right after its after_layer."""
    layer = tiled_entry["layer"]
    if layer in names:
        split = names.index(layer)
        return names[:split], names[split + 1:]
    split = names.index(tiled_entry["after_layer"]) + 1
    return names[:split], names[split:]


def _rasterize(tmx, hidden_layers, out_path):
    cmd = [RASTERIZER, "--hide-object-layers", *[arg for name in hidden_layers for arg in ("--hide-layer", name)],
           str(tmx), str(out_path)]
    done = subprocess.run(cmd, capture_output=True, text=True)
    if (done.returncode != 0) or not Path(out_path).is_file():
        raise RuntimeError(f"tmxrasterizer failed: {done.stderr[-1500:]}")


def _level_crop_box(cfg, piece, map_size, tile_size):
    tiled_entry = cfg["pieces"][piece]["tiled"]
    piece_width, piece_height = Image.open(target_for_piece(cfg, piece).beauty).size
    left, top = tiled_entry["map_tile_x"] * tile_size, tiled_entry["map_tile_y"] * tile_size
    margin = LEVEL_MARGIN_TILES * tile_size
    return (max(0, left - margin), max(0, top - margin),
            min(map_size[0], left + piece_width + margin), min(map_size[1], top + piece_height + margin))


def level_view(cfg, piece):
    """The level around the piece as Tiled renders it, without the piece's own layer, split into the
    layers behind and in front of it. Cached until the map file changes (integrate rewrites it)."""
    tmx = _map_path(cfg)
    target = target_for_piece(cfg, piece)
    behind, front = target.preview_path("level_behind"), target.preview_path("level_front")
    tiled_entry = cfg["pieces"][piece]["tiled"]
    root = ElementTree.parse(tmx).getroot()
    tile_size = int(root.get("tilewidth"))
    map_size = (int(root.get("width")) * tile_size, int(root.get("height")) * tile_size)
    box = _level_crop_box(cfg, piece, map_size, tile_size)
    offset = (tiled_entry["map_tile_x"] * tile_size - box[0], tiled_entry["map_tile_y"] * tile_size - box[1])
    # two page loads at once would rasterize into the same files
    with _level_lock:
        _refresh_level_images(cfg, piece, tmx, box, behind, front)
    return LevelView(behind, front, offset)


_level_lock = threading.Lock()


def _refresh_level_images(cfg, piece, tmx, box, behind, front):
    is_current = all(p.is_file() and (p.stat().st_mtime > tmx.stat().st_mtime) for p in (behind, front))
    if not is_current:
        tiled_entry = cfg["pieces"][piece]["tiled"]
        target = target_for_piece(cfg, piece)
        below, above = _layers_split_at_piece(_layer_names(tmx), tiled_entry)
        always_hidden = [*SCREEN_EFFECT_LAYERS, tiled_entry["layer"]]
        whole_map = target.preview_path("level_whole")
        behind.parent.mkdir(parents=True, exist_ok=True)
        # the whole map is 18432 px wide: rasterize it once per half and keep only the crop
        for hidden_layers, out_path in ((always_hidden + above, behind), (always_hidden + below, front)):
            _rasterize(tmx, hidden_layers, whole_map)
            Image.open(whole_map).crop(box).save(out_path)
        whole_map.unlink()


def latest_game_frames(cfg, piece):
    """The newest capture run that has frames of the piece's spots: [(spot, path)], oldest spot first."""
    spot_names = [(spot, f"spot_{spot[0]}_{spot[1]}.png") for spot in cfg["pieces"][piece].get("capture", [])]
    runs = [folder for folder in (cfg["_work"] / "capture").glob("*") if folder.is_dir()]
    runs_with_piece = [run for run in runs if any((run / name).is_file() for _, name in spot_names)]
    if not runs_with_piece:
        return []
    newest = max(runs_with_piece, key=lambda run: max((run / name).stat().st_mtime for _, name in spot_names if (run / name).is_file()))
    return [(spot, newest / name) for spot, name in spot_names if (newest / name).is_file()]


def apply_to_game(cfg, piece, options, ramps, save_palette, palette_scope="piece", report=print):
    """Saves the options (and the palette when save_palette) into the config, writes <piece>_px.png
    and integrates it into the map: the piece as the game will show it next time. report(text)
    hears which of the three steps is running."""
    report("Saving the settings…")
    save_piece_settings(cfg["_path"], piece, options, ramps if save_palette else None, palette_scope)
    report("Writing art…")
    write_art(target_for_piece(cfg, piece), options, ramps)
    report("Putting it into the level…")
    return integrate(cfg, piece)
