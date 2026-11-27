# pixelart: Blender scenes to dstar-style pixel art, into the game

All commands run from `lab/pixelart`; paths in an asset config are relative to the config file.

One command takes an asset from a Blender scene script to frames from the running game:

```
python -m pixelart assets/graveyard/graveyard.json chapel
python -m pixelart assets/graveyard/graveyard.json all --steps render,stylize,compare
```

Steps, in order (pick any with `--steps`):

| step | what it does | output in `work_dir` |
|---|---|---|
| render | builds the piece in headless Blender (GPU if there is one), renders three passes, at `render_scale` times level size reduced back to level size | `<piece>_beauty.png`, `_ids.png`, `_normals.png` |
| stylize | redraws the beauty pass in the target style (see below) | `<piece>_px.png` |
| inscribe | carves the epitaphs the render recorded into the art, as drawn glyphs (see below) | `<piece>_px.png` |
| compare | the piece next to the reference art, at 4x, with style numbers | `compare_<piece>.png` |
| integrate | writes the piece into the Tiled map as its own tileset and layer, with its normal map | map + `tilesets/` in the engine checkout |
| capture | starts the game, loads the level, teleports to the piece's spots, saves a frame per spot | `capture/<label>/spot_x_y.png` |

To try stylizer settings without touching the config:

```
python -m pixelart.variants assets/graveyard/graveyard.json lower out.png 1200,40,1460,192 \
    'now={}' 'calm={"micro":0,"moss_clumps":true}'
```

`python -m pixelart.palette <image> [x0,y0,x1,y1]` prints the stone and foliage ramps an artist used in a reference image.

## Snapping fake pixel art onto its grid

```
python -m pixelart.snap picture.png picture_snapped.png [--colours 16 | --palette colours.png] [--cell 4.5] [--preview 4]
```

For pictures that only look like pixel art: AI output, scaled-up screenshots, renders at the wrong
size, blurred copies (PNG; JPEG block noise is out of scope). It finds the real pixel grid, also at
non-integer scales and for uneven grids that mix block sizes, gives every block its most common colour
(counted inside the block, away from the blurred border, so one-pixel lines and dithering survive) and
limits the palette to `--colours` colours of the picture or to the colours of a palette picture (for
example a crop of dstar's art). A picture that already is pixel art keeps its size. The method is
described at the top of `pixelart/snap.py`. The GUI offers it under **More ▸ Snap to the pixel grid**,
the MCP server as `snap_image`; both write `<name>_snapped.png` next to the picture.

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
- **More** holds the rest: another art set or loose renders, snapping a picture to its pixel grid,
  rebuilding from the 3D model, a comparison sheet, writing the picture only, the 3D render views and
  the style numbers.
- Pieces may carry a `"title"` (and the asset a top-level `"title"`) for the names on the page.

## MCP server

`python -m pixelart.mcp_server` serves the same operations over MCP (stdio). Register it with Claude Code:

```
claude mcp add pixelart -- uv run --directory <repository>/lab/pixelart python -m pixelart.mcp_server
```

Tools: `list_pieces`, `get_piece_options`, `stylize` (options overrides, optional crop box and zoom; returns
the preview image and the style numbers, saves nothing), `set_piece_options` (saves into the config),
`write_art` (writes `<piece>_px.png` from the saved config), `render`, `integrate`, `capture` (returns frame
paths), `compare` (returns the sheet of the written art), `read_palette` and `snap_image`. The GUI and the server share
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

**Inscribe (`pixelart/inscribe.py`).** Modelled grooves blur into lines at one pixel per level pixel, so
monuments don't model their lettering: `graves.lettering` records where it goes (the top line's centre on the
face and the face's right and up directions, moved with the monument's turn and lean) and the render writes
those as image coordinates to `<piece>_inscriptions.json`. The inscribe step then draws small glyphs grouped
into words, the name line widest and more letters worn away lower down, each a darker cut with a lighter lip
on its lower right. It only carves pixels with the face's own normal (no side faces, moss or leaves) and only
uses colours the piece already has.

**Stylize (`pixelart/stylize.py`).** The render is used as information only. Each stone gets one flat tone from
the palette, at most one step lighter or darker for broad patches; its top row is lit two steps above the
stone, its bottom row shaded; joints become a dark tone of the stones beside them, darkest where joints cross;
engraved lettering and cracks become outline colour; moss stays green and can be gathered into clumps.
Thin strips (incised borders) are line work and get no edge light. Options per piece (`"stylize"`):
`shift` (whole tone steps, negative for pieces further back), `k_scale`, `micro` (fine shade noise, 0 = calm),
`moss_clumps`, `foot_rows` (contact shade where a piece meets the ground), `outline_level`, `tone_spread` (how far a
stone's tone may depart from the stones around it; lower = calmer walls), `drawn_edges` (block edges by dstar's wall
rules: sides lit only near the top, a glint at the top corners, no dark bottom edge, bottom corners cut into the joint
so a dark wedge opens where joints meet), `wear` (`pixelart/wear.py`: top lines stop short or break, a few stones get a
crack stroke, a pit or a chipped edge, decided per stone from `texture_seed`), `antialias` (`pixelart/antialias.py`:
an in-between tone in the concave notches of curves, inside the piece only, from the piece's own colours),
`ornament_faithful` (small carved parts and line work keep the render's light pixel by pixel, edges included, and get
no cracks: a boss or ring has its shape in its light and shade, which one flat tone per stone wipes out), and the
shading that keeps pieces from looking like flat EGA tiles: `soft` (the render's own light inside a
stone, in in-between shades), `bevel` (pillowed stones: a lit band along the top and left edges, falloff
towards the bottom right), `speck` (fine surface speckle) and `sub_steps`. Stones deep in shadow stay flat.

**Paint (`pixelart/paint.py`), the stage for architecture.** A piece with a `"paint"` section in its config is
drawn instead of converted: the render passes answer questions, they are not reduced. It needs `"paint_passes": true`
(and `render_scale`, 4 for the chapel), which adds four passes to the render: `light` (the key light alone on white
matt surfaces: the light's direction and cast shadows without texture), `albedo` (each material's colour without
light: tint per stone, grime, moss), `depth` (16-bit distance from the camera) and `classes` (material class per pixel:
wall stone, trim, glass, roof, foliage, metal, joint, and whether a part stands back). The stage:
1. light bands from the light pass (deep, shade, half, lit), patches smaller than a few pixels merged into their
   neighbour, so there is no speckle by construction;
2. a ramp step per band and material, a step up or down per stone from its albedo, side rooms a step lower;
3. lines traced from the depth pass: a lit rim where a raised part faces the moon, a dark cast line where it ends
   towards the lower right; lit top lines under joints and hand marks (`wear.py`);
4. joints under the stones beside them, moss green where the albedo is green on faces turned up, a dark silhouette;
5. curve smoothing (`antialias.py`) and the grade. Options: `gain`, `contrast`, `saturation`, `night`, `seed`, `antialias`.

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
goes after, z, tile position, optional placeholder clearing), `capture` spots in tiles, `compare_boxes`, and
`render_scale` (default 1): the passes are rendered that many times larger into `<work_dir>/<piece>_x<scale>/` and
reduced to level size by `pixelart/reduce.py`, so every level pixel is decided by the whole area it covers instead
of one sample (no speckle from scan texture or edges; joints stay one pixel wide). 4 suits architecture.

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
