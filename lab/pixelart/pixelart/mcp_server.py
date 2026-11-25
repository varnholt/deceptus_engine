"""MCP server (stdio) for tuning and running the pixel art pipeline from an agent.

usage: python -m pixelart.mcp_server
register: claude mcp add pixelart -- uv run --directory <repository>/lab/pixelart python -m pixelart.mcp_server

The tools share their logic with the GUI (workbench.py). Stylize previews go to <work_dir>/gui/,
never over <piece>_px.png; write_art() is the one tool that writes that file.
"""
import functools
import time

from mcp.server.mcpserver import Image, MCPServer
from mcp.server.mcpserver.exceptions import ToolError

from . import workbench
from .__main__ import load

server = MCPServer("pixelart")
# the end of a failed step's log usually names the cause (Blender's or the game's last lines)
FAILURE_LOG_TAIL = 3000


def reported_failures(tool):
    """Expected failures (bad piece or option, missing file, failed step) reach the caller with their
    message; the SDK would otherwise hide any exception behind a generic error."""
    @functools.wraps(tool)
    def run(*args, **kwargs):
        try:
            return tool(*args, **kwargs)
        except workbench.StepFailed as failure:
            raise ToolError(f"{failure}\n{failure.log[-FAILURE_LOG_TAIL:]}") from failure
        except (ValueError, KeyError, OSError) as failure:
            raise ToolError(f"{type(failure).__name__}: {failure}") from failure
    return run


def _effective_options(cfg, piece, overrides):
    return {**workbench.piece_options(cfg, piece), **workbench.normalized_options(overrides or {})}


@server.tool()
@reported_failures
def list_pieces(asset: str) -> list[dict]:
    """Pieces of an asset config (path to its JSON): whether their passes exist, their saved stylize
    options, palette source, compare boxes and capture spots."""
    cfg = load(asset)
    return [{key: value for key, value in workbench.piece_summary(cfg, piece).items() if key != "palette"}
            for piece in cfg["pieces"]]


@server.tool()
@reported_failures
def get_piece_options(asset: str, piece: str) -> dict:
    """Every stylize option with the value the piece uses, what the config stores, the palette ramps
    it is stylized with, and each option's range and meaning."""
    cfg = load(asset)
    summary = workbench.piece_summary(cfg, piece)
    return {
        "options": summary["options"],
        "saved_options": summary["saved_options"],
        "palette": summary["palette"],
        "palette_source": summary["palette_source"],
        "option_specs": workbench.option_specs(),
    }


@server.tool(structured_output=False)
@reported_failures
def stylize(asset: str, piece: str, options: dict | None = None, box: list[int] | None = None, zoom: int = 3) -> list:
    """Stylizes the piece with its saved options overridden by `options`, without saving anything.
    Returns the preview (cropped to box [x0, y0, x1, y1] if given, enlarged by whole-pixel zoom,
    reduced when the image would get wider than 2048 px) and the style metrics of the box and the whole piece."""
    cfg = load(asset)
    target = workbench.target_for_piece(cfg, piece)
    preview = workbench.stylize_to(target, _effective_options(cfg, piece, options),
                                   workbench.resolved_palette(cfg, piece), target.preview_path("mcp"))
    png, used_zoom = workbench.zoomed_png(preview, box, zoom)
    report = {"preview_file": str(preview), "zoom": used_zoom, "whole": workbench.image_metrics(preview)}
    if box:
        report["box"] = workbench.image_metrics(preview, box)
    return [Image(data=png, format="png"), report]


@server.tool()
@reported_failures
def set_piece_options(asset: str, piece: str, options: dict, palette: dict | None = None, palette_scope: str = "piece") -> dict:
    """Saves stylize options (merged over the piece's current ones) into the asset JSON. A palette
    ({"stone": 8 colours, "foliage": 6, "passage": 6} as [r, g, b]) is saved too when given, for the
    piece or (palette_scope "asset") the whole asset. Other keys of the config stay untouched."""
    cfg = load(asset)
    saved = workbench.save_piece_settings(asset, piece, _effective_options(cfg, piece, options), palette, palette_scope)
    return {"saved_options": saved}


@server.tool()
@reported_failures
def write_art(asset: str, piece: str) -> str:
    """Stylizes the piece with its saved config into <piece>_px.png, the art integrate puts into the map."""
    cfg = load(asset)
    target = workbench.target_for_piece(cfg, piece)
    return str(workbench.write_art(target, workbench.piece_options(cfg, piece), workbench.resolved_palette(cfg, piece)))


@server.tool()
@reported_failures
def render(asset: str, piece: str) -> dict:
    """Renders the piece's beauty, ids and normals passes in headless Blender (about 30 s)."""
    cfg = load(asset)
    log = workbench.render(cfg, piece)
    target = workbench.target_for_piece(cfg, piece)
    return {"passes": [str(target.beauty), str(target.ids), str(target.normals)], "log": log}


@server.tool()
@reported_failures
def integrate(asset: str, piece: str) -> dict:
    """Writes the piece's written art (<piece>_px.png, see write_art) into the Tiled map of the engine checkout."""
    return {"log": workbench.integrate(load(asset), piece)}


@server.tool()
@reported_failures
def capture(asset: str, pieces: list[str], label: str | None = None) -> dict:
    """Runs the game (1-2 minutes), visits the pieces' capture spots and returns the saved frame paths."""
    frames, log = workbench.capture(load(asset), pieces, label or time.strftime("%Y%m%d-%H%M%S"))
    return {"frames": frames, "log": log}


@server.tool(structured_output=False)
@reported_failures
def compare(asset: str, piece: str) -> list:
    """Comparison sheet of the piece's written art against the config's references, with style numbers."""
    sheet, _ = workbench.compare_sheet(load(asset), piece)
    if sheet is None:
        raise ValueError(f"piece '{piece}' has no compare_boxes")
    return [Image(path=sheet), {"sheet_file": str(sheet)}]


@server.tool()
@reported_failures
def read_palette(image_path: str, box: list[int] | None = None) -> dict:
    """Stone and foliage ramps an artist used in an image (optionally a box [x0, y0, x1, y1] of it)."""
    return workbench.read_palette(image_path, box)


if __name__ == "__main__":
    server.run()
