"""Generates the placeholder sprites for the harpoon: rope and hook.

    uv run --with pillow python make_harpoon_sprites.py

The rope palette and its 3px width are taken from the rope the Rope mechanism draws out of the
catacombs tileset (rect 971,73 3x138), so the harpoon rope reads exactly as thin as that one. The
hook is a small pirate style J hook, kept subtle to match.

The hook points along +x, which is what the renderer rotates from.

write_launcher() is kept for later but is not called: the launcher held by the player is not drawn
at the moment.
"""

import pathlib

from PIL import Image, ImageDraw

OUTPUT_DIRECTORY = pathlib.Path(__file__).resolve().parents[2] / "data" / "sprites"

# rope, sampled from the tileset rope
ROPE_SHADOW = (70, 55, 52, 255)
ROPE_MID = (93, 76, 72, 255)
ROPE_LIGHT = (112, 92, 87, 255)

# metal, kept cool so it reads against the warm rope
METAL_OUTLINE = (34, 32, 42, 255)
METAL_DARK = (88, 94, 110, 255)
METAL_MID = (146, 152, 168, 255)
METAL_LIGHT = (206, 212, 224, 255)

# wood for the launcher
WOOD_OUTLINE = (32, 22, 18, 255)
WOOD_DARK = (74, 50, 34, 255)
WOOD_MID = (108, 76, 50, 255)
WOOD_LIGHT = (142, 104, 70, 255)

TRANSPARENT = (0, 0, 0, 0)


def write_rope() -> None:
    """A 3x8 strip that tiles along the rope length, twisted so the repeat reads as a braid.

    3 px is the width of the tileset rope the Rope mechanism uses."""
    width = 3
    height = 8
    image = Image.new("RGBA", (width, height), TRANSPARENT)

    # one highlight pixel travelling across the strand per row makes the twist
    for y in range(height):
        highlight_x = y % width
        for x in range(width):
            distance = (x - highlight_x) % width
            if distance == 0:
                color = ROPE_LIGHT
            elif distance == 1:
                color = ROPE_MID
            else:
                color = ROPE_SHADOW
            image.putpixel((x, y), color)

    image.save(OUTPUT_DIRECTORY / "harpoon_rope.png")
    print(f"wrote harpoon_rope.png ({width}x{height})")


def write_hook() -> None:
    """A small pirate style hook: short shank on the left, hook curling back at the right.

    Deliberately tiny - roughly as thick as the rope, with no heavy outline. At this size an outline
    swallows the shape and the hook stops reading as subtle."""
    size = 16
    image = Image.new("RGBA", (size, size), TRANSPARENT)
    draw = ImageDraw.Draw(image)

    arc_box = (7, 4, 14, 12)

    draw.line((2, 8, 8, 8), fill=METAL_MID, width=2)
    draw.arc(arc_box, start=235, end=125, fill=METAL_MID, width=2)

    # a single highlight pixel row is all the shading this size carries
    draw.line((3, 7, 8, 7), fill=METAL_LIGHT, width=1)
    draw.arc(arc_box, start=250, end=10, fill=METAL_LIGHT, width=1)

    # the point curling back towards the shank
    draw.line((9, 12, 7, 13), fill=METAL_MID, width=1)
    draw.point((7, 13), fill=METAL_LIGHT)

    # the eye the rope is tied to
    draw.rectangle((0, 7, 2, 9), fill=METAL_DARK)
    draw.point((1, 8), fill=ROPE_MID)

    image.save(OUTPUT_DIRECTORY / "harpoon_hook.png")
    print(f"wrote harpoon_hook.png ({size}x{size})")


def write_launcher() -> None:
    """A short wooden speargun with a metal head, pointing right."""
    size = 24
    image = Image.new("RGBA", (size, size), TRANSPARENT)
    draw = ImageDraw.Draw(image)

    # stock and barrel
    draw.rectangle((3, 9, 19, 14), fill=WOOD_OUTLINE)
    draw.rectangle((4, 10, 18, 13), fill=WOOD_MID)
    draw.line((4, 10, 18, 10), fill=WOOD_LIGHT, width=1)
    draw.line((4, 13, 18, 13), fill=WOOD_DARK, width=1)

    # grip below the stock
    draw.rectangle((5, 14, 9, 20), fill=WOOD_OUTLINE)
    draw.rectangle((6, 15, 8, 19), fill=WOOD_DARK)

    # metal band and muzzle
    draw.rectangle((14, 8, 16, 15), fill=METAL_OUTLINE)
    draw.rectangle((15, 9, 15, 14), fill=METAL_MID)
    draw.rectangle((19, 10, 22, 13), fill=METAL_OUTLINE)
    draw.rectangle((19, 11, 21, 12), fill=METAL_LIGHT)

    # the loaded harpoon tip poking out
    draw.polygon([(22, 9), (23, 11), (22, 14)], fill=METAL_LIGHT)

    image.save(OUTPUT_DIRECTORY / "harpoon_launcher.png")
    print(f"wrote harpoon_launcher.png ({size}x{size})")


def write_preview() -> None:
    """Zoomed side by side sheet so the sprites can be judged without launching the game."""
    scale = 10
    sheet = Image.new("RGBA", (24 * 3 * scale, 24 * scale), (24, 20, 34, 255))
    for index, name in enumerate(["harpoon_rope.png", "harpoon_hook.png"]):
        sprite = Image.open(OUTPUT_DIRECTORY / name).convert("RGBA")
        zoomed = sprite.resize((sprite.width * scale, sprite.height * scale), Image.NEAREST)
        sheet.alpha_composite(zoomed, (index * 24 * scale, 0))

    preview_path = pathlib.Path(__file__).resolve().parent / "out" / "sprite_preview.png"
    preview_path.parent.mkdir(exist_ok=True)
    sheet.save(preview_path)
    print(f"wrote {preview_path}")


write_rope()
write_hook()
write_preview()
