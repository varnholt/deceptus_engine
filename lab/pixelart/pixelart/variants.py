"""Tries stylizer settings side by side on one rendered piece, without touching the piece's config.

usage: python -m pixelart.variants <asset.json> <piece> <out.png> <x0,y0,x1,y1> 'label={json options}' ...
Each variant's options are merged over the piece's own "stylize" options; "palette_from" may name
a reference image ({"path": ..., "box": [...]}) to read the palette from.
"""
import json
import sys

from PIL import Image, ImageDraw

from . import palette, stylize
from .__main__ import load, piece_palette
from .metrics import metrics

ZOOM = 3
BACKDROP = (24, 30, 70, 255)


def run(asset_json, piece, out_png, box, variants):
    cfg = load(asset_json)
    w = cfg["_work"]
    panels = []
    for label, opts in variants:
        opts = {**cfg["pieces"][piece].get("stylize", {}), **opts}
        pal = piece_palette(cfg, piece)
        if "palette_from" in opts:
            ref = opts.pop("palette_from")
            pal = palette.extract(ref["path"], tuple(ref["box"]) if ref.get("box") else None)
        out = w / f"variant_{piece}_{label}.png"
        stylize.pixelize(str(w / f"{piece}_beauty.png"), str(w / f"{piece}_ids.png"), str(w / f"{piece}_normals.png"),
                         str(out), palette=pal, **opts)
        m = metrics(str(out), box)
        im = Image.open(out).convert("RGBA").crop(box)
        bg = Image.new("RGBA", im.size, BACKDROP)
        bg.alpha_composite(im)
        z = bg.convert("RGB").resize((im.size[0] * ZOOM, im.size[1] * ZOOM), Image.NEAREST)
        panel = Image.new("RGB", (z.size[0], z.size[1] + 16), (10, 10, 16))
        panel.paste(z, (0, 16))
        ImageDraw.Draw(panel).text((4, 2), f"{label}: c/t {m['colours/tile']} flat {m['flat pairs']:.2f} dark {m['dark share']:.2f}", fill=(220, 220, 230))
        panels.append(panel)
    cols = 2
    pw, ph = panels[0].size
    sheet = Image.new("RGB", (cols * (pw + 8), ((len(panels) + cols - 1) // cols) * (ph + 8)), (0, 0, 0))
    for i, p in enumerate(panels):
        sheet.paste(p, ((i % cols) * (pw + 8), (i // cols) * (ph + 8)))
    sheet.save(out_png)
    print(out_png)


if __name__ == "__main__":
    a = sys.argv
    specs = []
    for spec in a[5:]:
        label, _, opts = spec.partition("=")
        specs.append((label, json.loads(opts or "{}")))
    run(a[1], a[2], a[3], tuple(int(v) for v in a[4].split(",")), specs)
