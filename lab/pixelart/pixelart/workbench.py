"""Tuning logic shared by the GUI (gui.py) and the MCP server (mcp_server.py).

Everything here works on a Target: three render passes plus the places its previews and its written
art go. Previews never land on <piece>_px.png; only write_art() writes there, on request.
"""
import contextlib
import inspect
import io
import json
import math
import shutil
import sys
import threading
from dataclasses import dataclass
from pathlib import Path

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

OPTION_RANGES = {
    "shift": dict(kind="int", min=-4, max=4, step=1, help="whole tone steps for every stone, negative for pieces further back"),
    "k_scale": dict(kind="float", min=0.1, max=4.0, step=0.05, help="brightness change (in stone spreads) needed for a patch to step a tone"),
    "crack_k": dict(kind="float", min=0.5, max=6.0, step=0.1, help="how dark (in stone spreads) a pixel must be to become a crack; lower = more cracks"),
    "grain": dict(kind="float", min=0.0, max=1.0, step=0.05, help="0 = smoothed broad patches, 1 = raw per-pixel brightness"),
    "max_step": dict(kind="int", min=0, max=3, step=1, help="tone steps a patch may differ from its stone"),
    "micro": dict(kind="int", min=0, max=12, step=1, help="strength of the fine shade noise, 0 = calm"),
    "micro_k": dict(kind="float", min=0.2, max=4.0, step=0.1, help="contrast needed for the fine shade noise"),
    "moss_clumps": dict(kind="bool", help="gather moss into clumps"),
    "foot_rows": dict(kind="int", min=0, max=8, step=1, help="rows shaded at the foot of the stones"),
    "outline_level": dict(kind="int", min=0, max=7, step=1, help="stone ramp index used for outlines and cracks"),
    "base_shares": dict(kind="shares", min=0.0, max=1.0, step=0.05, help="share of stone bodies per base tone (levels 2..5)"),
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
        specs.append({"name": parameter.name, "default": default, **spec})
    return specs


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
    buffer = io.BytesIO()
    compare.reference_image(ref).save(buffer, "PNG")
    return buffer.getvalue()


# ---------------------------------------------------------------- config

def _stored_values(saved, values, defaults):
    """What the config keeps: keys it already had, plus every value that differs from its default."""
    return {name: value for name, value in values.items() if (name in saved) or (value != defaults.get(name))}


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
