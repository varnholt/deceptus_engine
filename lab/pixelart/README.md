# pixelart: Blender scenes to dstar-style pixel art, into the game

All commands run from `lab/pixelart`; paths in an asset config are relative to the config file.

One command takes an asset from a Blender scene script to frames from the running game:

```
python -m pixelart assets/graveyard/graveyard.json mausoleum
python -m pixelart assets/graveyard/graveyard.json all --steps render,stylize,compare
```

Steps, in order (pick any with `--steps`):

| step | what it does | output in `work_dir` |
|---|---|---|
| render | builds the piece in headless Blender (GPU if there is one), renders three passes | `<piece>_beauty.png`, `_ids.png`, `_normals.png` |
| stylize | redraws the beauty pass in the target style (see below) | `<piece>_px.png` |
| compare | the piece next to the reference art, at 4x, with style numbers | `compare_<piece>.png` |
| integrate | writes the piece into the Tiled map as its own tileset and layer, with its normal map | map + `tilesets/` in the engine checkout |
| capture | starts the game, loads the level, teleports to the piece's spots, saves a frame per spot | `capture/<label>/spot_x_y.png` |

To try stylizer settings without touching the config:

```
python -m pixelart.variants assets/graveyard/graveyard.json lower out.png 1200,40,1460,192 \
    'now={}' 'calm={"micro":0,"moss_clumps":true}'
```

`python -m pixelart.palette <image> [x0,y0,x1,y1]` prints the stone and foliage ramps an artist used in a reference image.

## Tuning GUI

```
uv run python -m pixelart.gui assets/graveyard/graveyard.json
```

(plain `python -m pixelart.gui` works too where numpy and pillow are installed). It serves a page on
`http://127.0.0.1:8765/` (another free port if that one is taken) and opens the browser; `--no-browser`
only serves, `--port` picks the port, the asset argument is optional.

The page opens on the first piece, shown **in the level**: the stylized piece at its map position,
between the layers Tiled draws behind and in front of it (rasterized with `tmxrasterizer`, cached in
`<work_dir>/gui/` until the map changes). The view switch also offers **Piece only** and **In game**
(the newest captured frames of the piece's capture spots).

- **Fit to the level** (open): brightness, contrast, colour, night tint, tone shift. **Stone look** and
  **Colours (advanced)** (folded) hold every other option and the palette. Each option explains itself
  under its slider and is greyed out while it has no effect. Texts, ranges and groups live in
  `workbench.OPTION_RANGES`, which the MCP server shows too.
- **Apply to game** saves the options (and edited colours) into the config, writes `<piece>_px.png` and
  integrates it into the map. **Show in game** runs the capture (1-2 minutes; `GAME_EXE` is passed on)
  and switches to the new frame. Hold **B** to flip against the last applied version.
- An "Updating…" indicator runs from a change until the new image is shown, with the time it took.
- **More** holds the rest: another art set or loose renders, rebuilding from the 3D model, a comparison
  sheet, writing the picture only, the 3D render views and the style numbers.
- Pieces may carry a `"title"` (and the asset a top-level `"title"`) for the names on the page.

## MCP server

`python -m pixelart.mcp_server` serves the same operations over MCP (stdio). Register it with Claude Code:

```
claude mcp add pixelart -- uv run --directory <repository>/lab/pixelart python -m pixelart.mcp_server
```

Tools: `list_pieces`, `get_piece_options`, `stylize` (options overrides, optional crop box and zoom; returns
the preview image and the style numbers, saves nothing), `set_piece_options` (saves into the config),
`write_art` (writes `<piece>_px.png` from the saved config), `render`, `integrate`, `capture` (returns frame
paths), `compare` (returns the sheet of the written art) and `read_palette`. The GUI and the server share
their logic in `pixelart/workbench.py`.

## How a piece is made

**Model (Blender, `blender_kit/`).** Scene scripts build geometry in level pixels: `X(px)` and `Z(py)` map a
level coordinate onto the scene; 1 Blender unit is one 24 px tile. The kit gives you:

- `core.py`: the `Builder` (bevelled stone boxes and prisms, each with a one-pixel dark joint slab behind it),
  `ashlar()` walls, `ivy()` grown by ray casting, scanned stone materials (one random scan patch per stone,
  so nothing repeats), the stage (key light from the upper left, cool fill, ambient) and the three render passes.
- `graves.py`: monuments (headstones with frames, emblems and lettering, Celtic and fleury crosses,
  chest tombs, pillars with urns), cracks, chips, moss streaks, weeds, iron fences, bushes,
  and `build_layout()` to stand a list of them on a ground line.

Every stone carries a `block_rand` attribute; the ids pass turns it into one flat colour per stone, which is
how the stylizer knows where one stone ends and the next begins.

**Stylize (`pixelart/stylize.py`).** The render is used as information only. Each stone gets one flat tone from
the palette, at most one step lighter or darker for broad patches; its top row is lit two steps above the
stone, its bottom row shaded; joints become a dark tone of the stones beside them, darkest where joints cross;
engraved lettering and cracks become outline colour; moss stays green and can be gathered into clumps.
Thin strips (incised borders) are line work and get no edge light. Options per piece (`"stylize"`):
`shift` (whole tone steps, negative for pieces further back), `k_scale`, `micro` (fine shade noise, 0 = calm),
`moss_clumps`, `foot_rows` (contact shade where a piece meets the ground), `outline_level`, and the
shading that keeps pieces from looking like flat EGA tiles: `soft` (the render's own light inside a
stone, in in-between shades), `bevel` (pillowed stones: a lit band along the top and left edges, falloff
towards the bottom right), `speck` (fine surface speckle) and `sub_steps`. Stones deep in shadow stay flat.

**Tidy.** Every stylized piece ends with `tidy.py`, which removes what a render leaves behind and reports
it: tiny enclosed holes between meshes, specks of dark that are not part of a joint or crack, short
dark slivers, and single pixels no neighbour shares.

**Palette.** Default is dstar's owl shrine. An asset or piece can set `"palette": {"stone": [8 colours],
"foliage": [6 colours]}` or `"palette_from": {"path": ..., "box": [...]}` to read it from his art.

## Asset config

See `assets/graveyard/graveyard.json`. Top level: `origin` (level x, ground y of Blender's origin),
`kit` modules, `scenes` scripts (relative to the config), `work_dir`, `engine` (engine root for the test
harness, the checkout whose `data/` gets the art, level name, level dir, tmx), `references` for comparison
sheets. Each piece: `build` (function in a scene script that builds the scene and calls `setup_stage` with its
camera rectangle in level pixels), `stylize` options, `tiled` placement (tileset name, layer, the layer it
goes after, z, tile position, optional placeholder clearing), `capture` spots in tiles, `compare_boxes`.

## Gotchas

- The camera rectangle must start on a tile boundary and be a whole number of tiles; `map_tile_x/y` is its
  top-left corner divided by 24.
- The engine binds one tileset per tile layer, so every piece gets its own layer.
- Capturing rewrites the caches of every level the game loads; the tool puts those files back afterwards
  (only the level you work on stays modified). The dev build sometimes crashes switching levels right after
  its caches were regenerated; the tool retries once.
- The capture backs up and restores the dev save (`AppData/Roaming/deceptus-dev/settings/savestate.json`).
- Captures run `<engine root>/build_rel/deceptus.exe` (a desktop build with the dev console); set
  `GAME_EXE` to use another build, e.g. from a worktree without one.
- Set `BLENDER` if Blender is not in `C:/Program Files/Blender Foundation/Blender 5.0/`.
- Scans in `textures/` are Poly Haven CC0, prefiltered to 384 px.
