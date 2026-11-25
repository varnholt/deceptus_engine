"""Puts a finished piece into a Tiled map as its own tileset and tile layer.

The engine binds one tileset per layer, so the piece gets a layer of its own, inserted right
after `after_layer` with the same z. Old placeholder tiles inside the piece's rectangle are
removed from the given background layers when their sheet row is at least `clear_min_row`.

Running it again replaces the piece's image, tiles and layer in place (same tileset gid, same layer id).
Placeholder clearing is optional: leave out "clear_layers" when there is nothing to remove.
"""
import json
import re
import sys
from pathlib import Path

import numpy as np
from PIL import Image

TILE = 24


def csv_of(layer_xml):
    return re.search(r'<data encoding="csv">\s*(.*?)\s*</data>', layer_xml, re.S)


def integrate(cfg):
    """cfg: level_dir, tmx, art, normals, name, layer, after_layer, z, map_tile_x, map_tile_y,
    optionally clear_layers, clear_rect_tiles, clear_min_row, clear_max_row, clear_tileset_firstgid, clear_tileset_columns."""
    level = Path(cfg["level_dir"])
    tmx_path = level / cfg["tmx"]
    tmx = tmx_path.read_text(encoding="utf-8")

    art = Image.open(cfg["art"]).convert("RGBA")
    nrm = Image.open(cfg["normals"]).convert("RGBA")
    assert art.size == nrm.size and art.size[0] % TILE == 0 and art.size[1] % TILE == 0, art.size
    a = np.array(art)
    n = np.array(nrm)
    solid = a[..., 3] >= 128
    n[..., 3] = np.where(solid, 255, 0)
    n[~solid] = 0
    name = cfg["name"]
    Image.fromarray(a).save(level / "tilesets" / f"{name}.png")
    Image.fromarray(n).save(level / "tilesets" / f"{name}_normals.png")

    cols, rows = art.size[0] // TILE, art.size[1] // TILE
    tsx = (
        '<?xml version="1.0" encoding="UTF-8"?>\n'
        f'<tileset version="1.10" tiledversion="1.10.2" name="{name}" tilewidth="{TILE}" tileheight="{TILE}" tilecount="{cols * rows}" columns="{cols}">\n'
        f' <image source="tilesets/{name}.png" width="{art.size[0]}" height="{art.size[1]}"/>\n'
        "</tileset>\n"
    )
    (level / f"{name}.tsx").write_text(tsx, encoding="utf-8", newline="\r\n")

    # first gid after the last tileset already in the map
    if f'source="{name}.tsx"' in tmx:
        firstgid = int(re.search(rf'<tileset firstgid="(\d+)" source="{re.escape(name)}.tsx"/>', tmx).group(1))
    else:
        last = None
        for m in re.finditer(r'<tileset firstgid="(\d+)" source="([^"]+)"/>', tmx):
            last = m
        last_tsx = (level / last.group(2)).read_text(encoding="utf-8")
        count = int(re.search(r'tilecount="(\d+)"', last_tsx).group(1))
        firstgid = int(last.group(1)) + count
        tmx = tmx[: last.end()] + f'\n <tileset firstgid="{firstgid}" source="{name}.tsx"/>' + tmx[last.end():]

    width = int(re.search(r'<map [^>]*\swidth="(\d+)"', tmx).group(1))
    height = int(re.search(r'<map [^>]*\sheight="(\d+)"', tmx).group(1))
    tx0, ty0 = cfg["map_tile_x"], cfg["map_tile_y"]

    grid = [0] * (width * height)
    for r in range(rows):
        for c in range(cols):
            if solid[r * TILE:(r + 1) * TILE, c * TILE:(c + 1) * TILE].any():
                grid[(ty0 + r) * width + tx0 + c] = firstgid + r * cols + c

    lines = []
    for y in range(height):
        row = ",".join(str(v) for v in grid[y * width:(y + 1) * width])
        lines.append(row + ("," if y < height - 1 else ""))
    data = "\n".join(lines)

    # replace a previous version of the layer (keeping its id), then insert it after the anchor layer
    old = re.search(rf'\n <layer id="(\d+)" name="{re.escape(cfg["layer"])}".*?</layer>', tmx, flags=re.S)
    if old:
        layer_id = int(old.group(1))
        tmx = tmx[: old.start()] + tmx[old.end():]
    else:
        layer_id = int(re.search(r'nextlayerid="(\d+)"', tmx).group(1))
        tmx = re.sub(r'nextlayerid="\d+"', f'nextlayerid="{layer_id + 1}"', tmx, count=1)
    layer_xml = (
        f'\n <layer id="{layer_id}" name="{cfg["layer"]}" width="{width}" height="{height}">\n'
        "  <properties>\n"
        f'   <property name="z" type="int" value="{cfg["z"]}"/>\n'
        "  </properties>\n"
        f'  <data encoding="csv">\n{data}\n</data>\n'
        " </layer>"
    )
    anchor = re.search(rf'<layer id="\d+" name="{re.escape(cfg["after_layer"])}".*?</layer>', tmx, re.S)
    tmx = tmx[: anchor.end()] + layer_xml + tmx[anchor.end():]

    # clear placeholder tiles under the piece
    bg_first = cfg.get("clear_tileset_firstgid", 1)
    bg_cols = cfg.get("clear_tileset_columns", 1)
    removed = 0
    for layer_name in cfg.get("clear_layers", []):
        m = re.search(rf'(<layer id="\d+" name="{re.escape(layer_name)}".*?<data encoding="csv">\s*)(.*?)(\s*</data>)', tmx, re.S)
        values = [int(v) for v in m.group(2).replace("\n", "").split(",")]
        x0, y0, x1, y1 = cfg["clear_rect_tiles"]
        for y in range(y0, y1):
            for x in range(x0, x1):
                i = y * width + x
                g = values[i] & 0x1FFFFFFF
                if bg_first <= g < bg_first + 4096 and cfg["clear_min_row"] <= (g - bg_first) // bg_cols <= cfg.get("clear_max_row", 10**6):
                    values[i] = 0
                    removed += 1
        out = []
        for y in range(height):
            row = ",".join(str(v) for v in values[y * width:(y + 1) * width])
            out.append(row + ("," if y < height - 1 else ""))
        tmx = tmx[: m.start(2)] + "\n".join(out) + tmx[m.end(2):]

    tmx_path.write_text(tmx, encoding="utf-8", newline="\r\n")
    print(f"tileset {name} firstgid {firstgid}, {sum(1 for g in grid if g)} tiles placed, {removed} placeholder tiles removed")


if __name__ == "__main__":
    integrate(json.loads(Path(sys.argv[1]).read_text()))
