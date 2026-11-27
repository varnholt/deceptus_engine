"""Blender scene to in-game frame, one command per asset.

usage: python -m pixelart <asset.json> <piece[,piece...]|all> [--steps render,stylize,compare,integrate,capture] [--label name]

  render     builds the piece in headless Blender and writes beauty, ids and normals passes
  stylize    turns the passes into pixel art in the target style (palette, edge and joint rules)
  inscribe   carves the epitaphs the render recorded into that art, as drawn glyphs
  compare    side-by-side sheet with the reference art and the style numbers
  integrate  writes the piece into the Tiled map as its own tileset and layer
  capture    starts the game, visits the pieces' capture spots and saves the frames

Outputs go to <work_dir>/<piece>_*.png, <work_dir>/compare_<piece>.png and <work_dir>/capture/<label>/.
See README.md for the asset config.
"""
import argparse
import json
import os
import subprocess
import sys
import time
from pathlib import Path

from . import compare, inscribe, paint, palette, reduce, stylize, tiled
from .metrics import metrics

KIT = Path(__file__).resolve().parent.parent / "blender_kit"
BLENDER = os.environ.get("BLENDER", "C:/Program Files/Blender Foundation/Blender 5.0/blender.exe")
STEPS = ["render", "stylize", "inscribe", "compare", "integrate", "capture"]


def _resolve(base, path):
    """Paths in an asset config may be relative to the config file, so a config travels with its repository."""
    return str((base / path).resolve()) if not Path(path).is_absolute() else path


def load(asset_json):
    cfg = json.loads(Path(asset_json).read_text(encoding="utf-8"))
    base = Path(asset_json).resolve().parent
    cfg["_path"] = str(Path(asset_json).resolve())
    cfg["_work"] = Path(_resolve(base, cfg["work_dir"]))
    cfg["_work"].mkdir(parents=True, exist_ok=True)
    for key in ("root", "data_root"):
        if key in cfg.get("engine", {}):
            cfg["engine"][key] = _resolve(base, cfg["engine"][key])
    for ref in cfg.get("references", []):
        ref["path"] = _resolve(base, ref["path"])
    for source in [cfg] + list(cfg["pieces"].values()):
        if "palette_from" in source:
            source["palette_from"]["path"] = _resolve(base, source["palette_from"]["path"])
    return cfg


def piece_palette(cfg, piece):
    """The piece's own palette, else the asset's, else one read from a reference image, else the stylizer default."""
    for source in (cfg["pieces"][piece], cfg):
        if "palette" in source:
            return {k: v for k, v in source["palette"].items() if not k.startswith("_")}
        if "palette_from" in source:
            ref = source["palette_from"]
            return palette.extract(ref["path"], tuple(ref["box"]) if ref.get("box") else None)
    return None


def render(cfg, piece):
    """Renders the piece's passes into work_dir; a piece with a render_scale above 1 renders them
    larger into work_dir/<piece>_x<scale> and reduces them to level size (see reduce.py)."""
    scale = cfg["pieces"][piece].get("render_scale", 1)
    out_dir = cfg["_work"] / f"{piece}_x{scale}" if scale > 1 else cfg["_work"]
    cmd = [BLENDER, "-b", "--python", str(KIT / "run_passes.py"), "--", cfg["_path"], piece, str(out_dir.resolve())]
    t0 = time.monotonic()
    done = subprocess.run(cmd, capture_output=True, text=True)
    if done.returncode != 0 or f"pixelart: rendered {piece}" not in done.stdout:
        print(done.stdout[-3000:], done.stderr[-3000:])
        raise SystemExit(f"render of {piece} failed")
    if scale > 1:
        reduce.reduce_passes(out_dir, cfg["_work"], piece, scale)
    print(f"{piece}: rendered in {time.monotonic() - t0:.0f}s" + (f" at {scale}x, reduced" if scale > 1 else ""))


def pass_paths(cfg, piece):
    """The piece's render passes in work_dir: beauty, ids, normals."""
    return tuple(cfg["_work"] / f"{piece}_{kind}.png" for kind in ("beauty", "ids", "normals"))


def stylize_piece(cfg, piece):
    """Writes <piece>_px.png: drawn by paint.py when the piece has a "paint" section (it needs the
    paint passes, see render_scale/paint_passes), otherwise converted by stylize.py."""
    w = cfg["_work"]
    if "paint" in cfg["pieces"][piece]:
        paint.paint(w, piece, w / f"{piece}_px.png", palette=piece_palette(cfg, piece), **cfg["pieces"][piece]["paint"])
        print(f"{piece}: {metrics(str(w / f'{piece}_px.png'))}")
        return
    opts = dict(cfg["pieces"][piece].get("stylize", {}))
    tidied = stylize.pixelize(*(str(p) for p in pass_paths(cfg, piece)), str(w / f"{piece}_px.png"),
                     palette=piece_palette(cfg, piece), **opts)
    print(f"{piece}: tidied {tidied}")
    print(f"{piece}: {metrics(str(w / f'{piece}_px.png'))}")


def inscribe_piece(cfg, piece):
    """Carve the piece's epitaphs, if its render recorded any."""
    w = cfg["_work"]
    areas = w / f"{piece}_inscriptions.json"
    if areas.exists():
        carved = inscribe.inscribe(str(w / f"{piece}_px.png"), str(w / f"{piece}_normals.png"), str(areas))
        print(f"{piece}: carved {carved} lettering pixels")


def compare_piece(cfg, piece, art=None, out=None):
    """Sheet of the piece's art (default: its written <piece>_px.png) next to the references; returns its path."""
    w = cfg["_work"]
    boxes = cfg["pieces"][piece].get("compare_boxes")
    if not boxes:
        return None
    out = out or w / f"compare_{piece}.png"
    compare.sheet(str(art or w / f"{piece}_px.png"), str(out), boxes, cfg.get("references", []))
    print(f"{piece}: comparison sheet {out}")
    return out


def integrate(cfg, piece):
    eng = cfg["engine"]
    w = cfg["_work"]
    t = dict(cfg["pieces"][piece]["tiled"])
    t.update(level_dir=str(Path(eng["data_root"]) / eng["level_dir"]), tmx=eng["tmx"],
             art=str(w / f"{piece}_px.png"), normals=str(w / f"{piece}_normals.png"))
    tiled.integrate(t)


def restore_game_side_effects(eng):
    """The game rewrites caches of every level it loads (collision meshes, crc files). Put back every
    tracked file it changed outside the level being worked on, so only the new art stays modified."""
    root = eng["data_root"]
    status = subprocess.run(["git", "-C", root, "status", "--porcelain", "--", "data"], capture_output=True, text=True).stdout
    touched = [line[3:] for line in status.splitlines() if line[:2] in (" M", " D") and not line[3:].startswith(eng["level_dir"])]
    if touched:
        subprocess.run(["git", "-C", root, "checkout", "--", *touched], check=True)
        print(f"restored {len(touched)} files the game rewrote: {', '.join(sorted({str(Path(t).parent) for t in touched}))}")


def game_executable(eng):
    """The game to capture with: GAME_EXE if set (a worktree usually has no build of its own),
    else the engine's build_rel, the build the desktop test harness uses."""
    return Path(os.environ.get("GAME_EXE", Path(eng["root"]) / "build_rel" / "deceptus.exe"))


def capture(cfg, pieces, label):
    eng = cfg["engine"]
    spots = [s for p in pieces for s in cfg["pieces"][p].get("capture", [])]
    if not spots:
        return None
    out = cfg["_work"] / "capture" / label
    game = game_executable(eng)
    if not game.exists():
        raise SystemExit(f"no game build at {game}: build the desktop target there, or point GAME_EXE at a deceptus.exe")
    env = dict(os.environ, GAME_DATA_ROOT=eng["data_root"], ENGINE_ROOT=eng["root"], GAME_EXE=str(game))
    cmd = ["uv", "run", "--project", str(Path(eng["root"]) / "tests"), "--with", "pillow", "python", "-m", "pixelart.capture",
           str(out), eng["level"], *[f"{x},{y}" for x, y in spots]]
    cwd = Path(__file__).resolve().parent.parent
    try:
        # the game occasionally crashes while switching levels; one retry tells that apart from a broken map
        for attempt in (1, 2):
            done = subprocess.run(cmd, env=env, cwd=cwd, capture_output=True, text=True)
            print(done.stdout[-1500:])
            if done.returncode == 0:
                print(f"frames in {out}")
                return out
            print(f"capture attempt {attempt} failed:\n{done.stderr[-1500:]}")
        raise SystemExit("capture failed twice")
    finally:
        restore_game_side_effects(eng)


def main(argv=None):
    ap = argparse.ArgumentParser(prog="pixelart", description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("asset")
    ap.add_argument("pieces")
    ap.add_argument("--steps", default=",".join(STEPS))
    ap.add_argument("--label", default=time.strftime("%Y%m%d-%H%M"))
    a = ap.parse_args(argv)
    cfg = load(a.asset)
    pieces = list(cfg["pieces"]) if a.pieces == "all" else a.pieces.split(",")
    steps = a.steps.split(",")
    unknown = set(steps) - set(STEPS)
    if unknown:
        raise SystemExit(f"unknown steps: {', '.join(sorted(unknown))}")
    for piece in pieces:
        if "render" in steps:
            render(cfg, piece)
        if "stylize" in steps:
            stylize_piece(cfg, piece)
        if "inscribe" in steps:
            inscribe_piece(cfg, piece)
        if "compare" in steps:
            compare_piece(cfg, piece)
        if "integrate" in steps:
            integrate(cfg, piece)
    if "capture" in steps:
        capture(cfg, pieces, a.label)


if __name__ == "__main__":
    sys.exit(main())
