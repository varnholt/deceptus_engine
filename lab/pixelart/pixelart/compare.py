"""Quality comparison sheet: crops of a stylized piece next to the target artist's work, same zoom,
each panel labelled with its style numbers (see metrics.py).

Top row: the references from the asset config. Bottom row: the given crops of the piece.
"""
import numpy as np
from PIL import Image, ImageDraw

from .metrics import metrics

ZOOM = 4
BACKDROP = (24, 30, 70, 255)


def _on_backdrop(im, backdrop):
    bg = Image.new("RGBA", im.size, tuple(backdrop))
    bg.alpha_composite(im.convert("RGBA"))
    return bg.convert("RGB")


def _panel(im, label):
    z = im.resize((im.size[0] * ZOOM, im.size[1] * ZOOM), Image.NEAREST)
    out = Image.new("RGB", (z.size[0], z.size[1] + 18), (10, 10, 16))
    out.paste(z, (0, 18))
    ImageDraw.Draw(out).text((4, 3), label, fill=(220, 220, 230))
    return out


def _stats(path, box):
    m = metrics(path, box)
    return f"c/t {m['colours/tile']} flat {m['flat pairs']:.2f} near {m['near pairs']:.2f} dark {m['dark share']:.2f}"


def reference_image(ref):
    """A reference entry as it is compared: cropped to its box, brightened by its gain."""
    im = Image.open(ref["path"]).convert("RGBA")
    if ref.get("box"):
        im = im.crop(tuple(ref["box"]))
    if ref.get("gain"):
        a = np.array(im)
        a[..., :3] = np.clip(a[..., :3].astype(int) * ref["gain"], 0, 255).astype(np.uint8)
        im = Image.fromarray(a)
    return im


def sheet(piece_png, out_png, boxes, references, backdrop=BACKDROP):
    """references: [{"path", "box" (optional), "label", "gain" (optional brightness factor for dim parallax art)}]"""
    refs = []
    for ref in references:
        box = tuple(ref["box"]) if ref.get("box") else None
        refs.append(_panel(_on_backdrop(reference_image(ref), backdrop), f'{ref["label"]}  {_stats(ref["path"], box)}'))
    src = Image.open(piece_png).convert("RGBA")
    mine = [_panel(_on_backdrop(src.crop(tuple(b)), backdrop), "mine  " + _stats(piece_png, tuple(b))) for b in boxes]

    def row_width(row):
        return sum(p.size[0] for p in row) + 10 * len(row)
    w = max(row_width(refs), row_width(mine))
    top_h = max(p.size[1] for p in refs) if refs else 0
    h = top_h + max(p.size[1] for p in mine) + 10
    out = Image.new("RGB", (w, h), (0, 0, 0))
    x = 0
    for p in refs:
        out.paste(p, (x, 0))
        x += p.size[0] + 10
    x = 0
    for p in mine:
        out.paste(p, (x, top_h + 10))
        x += p.size[0] + 10
    out.save(out_png)
