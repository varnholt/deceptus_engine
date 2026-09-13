"""Offline tile opacity classification and early-z prize estimation.

Answers the question that sizes the early-z work: of the tile quads a frame submits,
how many sit behind a fully opaque tile and could therefore be rejected before they
are ever shaded?

The classification runs here rather than in the engine on purpose: decoding every
tileset into a cpu side image at level load crashed the switch, which starts at
3281 MB of 3285 MB used.

Usage:
    uv run --with pillow --with numpy analyze_opacity.py <level.tmx>
"""

import argparse
import sys
import xml.etree.ElementTree as ElementTree
from pathlib import Path

import numpy as np
from PIL import Image

GID_FLIP_MASK = 0x1FFFFFFF

TILE_EMPTY = 0
TILE_PARTIAL = 1
TILE_OPAQUE = 2


class Tileset:
    def __init__(self, first_gid, tile_count, columns, tile_width, tile_height, image_path, name):
        self.first_gid = first_gid
        self.tile_count = tile_count
        self.columns = columns
        self.tile_width = tile_width
        self.tile_height = tile_height
        self.image_path = image_path
        self.name = name
        self.classification = None
        self.animated_ids = set()

    def classify(self):
        """Classify every tile of the tileset as empty, partially transparent or fully opaque."""
        classification = np.full(self.tile_count, TILE_EMPTY, dtype=np.uint8)

        if self.image_path is None or not self.image_path.exists():
            print(f"  ! missing tileset image, treating as non opaque: {self.image_path}")
            classification[:] = TILE_PARTIAL
            self.classification = classification
            return

        with Image.open(self.image_path) as image:
            alpha = np.array(image.convert("RGBA"))[:, :, 3]

        rows = alpha.shape[0] // self.tile_height
        columns = self.columns if self.columns > 0 else alpha.shape[1] // self.tile_width

        for local_id in range(self.tile_count):
            row = local_id // columns
            column = local_id % columns
            if row >= rows:
                break

            top = row * self.tile_height
            left = column * self.tile_width
            patch = alpha[top : top + self.tile_height, left : left + self.tile_width]
            if patch.size == 0:
                continue

            minimum_alpha = patch.min()
            maximum_alpha = patch.max()
            if maximum_alpha == 0:
                classification[local_id] = TILE_EMPTY
            elif minimum_alpha == 255:
                classification[local_id] = TILE_OPAQUE
            else:
                classification[local_id] = TILE_PARTIAL

        self.classification = classification


class Layer:
    def __init__(self, name, z_index, opacity, visible, width, height, gids, document_index):
        self.name = name
        self.z_index = z_index
        self.opacity = opacity
        self.visible = visible
        self.width = width
        self.height = height
        self.gids = gids
        self.document_index = document_index


def parse_tileset_element(element, first_gid, level_directory):
    """Read a tileset either from the inline element or from the .tsx it points at."""
    source = element.get("source")
    if source is not None:
        tileset_path = (level_directory / source).resolve()
        tileset_root = ElementTree.parse(tileset_path).getroot()
        base_directory = tileset_path.parent
    else:
        tileset_root = element
        base_directory = level_directory

    image_element = tileset_root.find("image")
    image_path = None
    if image_element is not None:
        image_path = (base_directory / image_element.get("source")).resolve()

    animated_ids = set()
    for tile_element in tileset_root.findall("tile"):
        if tile_element.find("animation") is not None:
            animated_ids.add(int(tile_element.get("id")))

    tileset = Tileset(
        first_gid=first_gid,
        tile_count=int(tileset_root.get("tilecount", "0")),
        columns=int(tileset_root.get("columns", "0")),
        tile_width=int(tileset_root.get("tilewidth", "0")),
        tile_height=int(tileset_root.get("tileheight", "0")),
        image_path=image_path,
        name=tileset_root.get("name", "unnamed"),
    )
    tileset.animated_ids = animated_ids
    return tileset


def parse_level(tmx_path):
    level_directory = tmx_path.parent
    root = ElementTree.parse(tmx_path).getroot()

    map_width = int(root.get("width"))
    map_height = int(root.get("height"))
    map_tile_width = int(root.get("tilewidth"))
    map_tile_height = int(root.get("tileheight"))

    tilesets = []
    for element in root.findall("tileset"):
        tilesets.append(parse_tileset_element(element, int(element.get("firstgid")), level_directory))

    layers = []
    for document_index, element in enumerate(root.findall("layer")):
        data_element = element.find("data")
        if data_element is None or data_element.get("encoding") != "csv":
            print(f"  ! skipping layer with unsupported encoding: {element.get('name')}")
            continue

        gids = np.fromstring(data_element.text.replace("\n", ""), dtype=np.int64, sep=",")
        width = int(element.get("width"))
        height = int(element.get("height"))
        if gids.size != width * height:
            print(f"  ! layer {element.get('name')} has {gids.size} entries, expected {width * height}")
            continue

        z_index = None
        properties_element = element.find("properties")
        if properties_element is not None:
            for property_element in properties_element.findall("property"):
                if property_element.get("name") == "z":
                    z_index = int(property_element.get("value"))

        layers.append(
            Layer(
                name=element.get("name"),
                z_index=z_index,
                opacity=float(element.get("opacity", "1")),
                visible=element.get("visible", "1") != "0",
                width=width,
                height=height,
                gids=(gids & GID_FLIP_MASK).astype(np.int64).reshape(height, width),
                document_index=document_index,
            )
        )

    return map_width, map_height, map_tile_width, map_tile_height, tilesets, layers


def build_gid_lookup(tilesets):
    """Map every global tile id onto its classification, so a layer becomes a lookup."""
    highest_gid = 0
    for tileset in tilesets:
        highest_gid = max(highest_gid, tileset.first_gid + tileset.tile_count)

    lookup = np.full(highest_gid + 1, TILE_EMPTY, dtype=np.uint8)
    for tileset in tilesets:
        if tileset.classification is None:
            continue
        start = tileset.first_gid
        end = start + tileset.tile_count
        lookup[start:end] = tileset.classification

    return lookup


def box_sum(array, window_height, window_width):
    """Sum every window_height x window_width window of the array."""
    padded = np.pad(array.astype(np.float64), ((1, 0), (1, 0)))
    integral = padded.cumsum(axis=0).cumsum(axis=1)
    height, width = array.shape
    rows = height - window_height + 1
    columns = width - window_width + 1
    if rows <= 0 or columns <= 0:
        return None

    return (
        integral[window_height : window_height + rows, window_width : window_width + columns]
        - integral[0:rows, window_width : window_width + columns]
        - integral[window_height : window_height + rows, 0:columns]
        + integral[0:rows, 0:columns]
    )


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("tmx", type=Path)
    parser.add_argument("--view-width", type=int, default=640)
    parser.add_argument("--view-height", type=int, default=360)
    arguments = parser.parse_args()

    if not arguments.tmx.exists():
        print(f"no such file: {arguments.tmx}")
        return 1

    print(f"parsing {arguments.tmx}")
    map_width, map_height, tile_width, tile_height, tilesets, layers = parse_level(arguments.tmx)
    print(f"map: {map_width} x {map_height} tiles of {tile_width} x {tile_height} px, {len(layers)} tile layers")

    print("classifying tilesets")
    for tileset in tilesets:
        tileset.classify()
        if tileset.classification is None:
            continue
        opaque_count = int((tileset.classification == TILE_OPAQUE).sum())
        partial_count = int((tileset.classification == TILE_PARTIAL).sum())
        empty_count = int((tileset.classification == TILE_EMPTY).sum())
        print(
            f"  {tileset.name:38s} {tileset.tile_count:5d} tiles"
            f"  opaque {opaque_count:5d}  partial {partial_count:5d}  empty {empty_count:5d}"
        )

    lookup = build_gid_lookup(tilesets)

    highest_gid = lookup.size
    animated_lookup = np.zeros(highest_gid, dtype=bool)
    for tileset in tilesets:
        for local_id in tileset.animated_ids:
            global_id = tileset.first_gid + local_id
            if global_id < highest_gid:
                animated_lookup[global_id] = True
    print(f"animated tile ids: {int(animated_lookup.sum())}")

    # parallax layers are drawn through a view of their own, so their coverage does not line up
    # with the level grid and they are reported separately rather than mixed into the estimate
    drawn_layers = [layer for layer in layers if layer.visible and not layer.name.startswith("parallax")]
    skipped = [layer.name for layer in layers if layer not in drawn_layers]
    print(f"\nlayers considered: {len(drawn_layers)}, skipped: {', '.join(skipped) if skipped else 'none'}")

    drawn_layers.sort(key=lambda layer: (layer.z_index if layer.z_index is not None else 0, layer.document_index))

    print("\nper layer coverage (share of map cells the layer puts a quad into)")
    total_cells = map_width * map_height
    classified_layers = []
    for layer in drawn_layers:
        classification = lookup[np.clip(layer.gids, 0, lookup.size - 1)]
        non_empty = classification != TILE_EMPTY
        # a layer drawn at less than full opacity never occludes what is behind it, however
        # opaque its tiles happen to be
        occluding = (classification == TILE_OPAQUE) & (layer.opacity >= 1.0)
        classified_layers.append((layer, non_empty, occluding))
        print(
            f"  z={str(layer.z_index):>4s}  {layer.name:28s} opacity {layer.opacity:4.2f}"
            f"  quads {non_empty.sum() / total_cells * 100:6.2f}%  occluding {occluding.sum() / total_cells * 100:6.2f}%"
        )

    # walk the layers back to front, tracking the frontmost occluder seen per cell. every quad
    # drawn before that occluder is one early z would reject
    print("\nbuilding per cell totals")
    quads_per_cell = np.zeros((map_height, map_width), dtype=np.int32)
    rejectable_per_cell = np.zeros((map_height, map_width), dtype=np.int32)

    for layer, non_empty, occluding in classified_layers:
        # anything counted so far in a cell that this layer now covers becomes rejectable
        rejectable_per_cell = np.where(occluding, quads_per_cell, rejectable_per_cell)
        quads_per_cell += non_empty

    view_columns = arguments.view_width // tile_width
    view_rows = arguments.view_height // tile_height
    cells_per_view = view_columns * view_rows
    print(f"view window: {view_columns} x {view_rows} tiles ({cells_per_view} cells)")

    quads_per_view = box_sum(quads_per_cell, view_rows, view_columns)
    rejectable_per_view = box_sum(rejectable_per_cell, view_rows, view_columns)
    if quads_per_view is None:
        print("view window larger than the map")
        return 1

    # windows over empty parts of the map are not places the camera ever sits, and averaging them
    # in would understate the load in the parts that are actually played
    populated = quads_per_view >= (0.25 * cells_per_view)
    if not populated.any():
        print("no populated windows found")
        return 1

    overdraw = quads_per_view[populated] / cells_per_view
    rejectable_share = rejectable_per_view[populated] / np.maximum(quads_per_view[populated], 1)

    print("\n--- colour pass, tile geometry only ---")
    print(f"populated camera windows       : {int(populated.sum())} of {populated.size}")
    print(f"tile overdraw   mean/median/p95: {overdraw.mean():5.2f}x / {np.median(overdraw):5.2f}x / {np.percentile(overdraw, 95):5.2f}x")
    print(
        f"rejectable      mean/median/p95: {rejectable_share.mean() * 100:5.1f}% / "
        f"{np.median(rejectable_share) * 100:5.1f}% / {np.percentile(rejectable_share, 95) * 100:5.1f}%"
    )
    print(f"overdraw after early z    mean: {(overdraw * (1 - rejectable_share)).mean():5.2f}x")

    # animated tiles do not go through the block window the static geometry uses. they are
    # collected around the player block instead, +-3 blocks across and +-2 down at 16 tiles per
    # block, and the whole collection is submitted every frame whether it is on screen or not
    animation_columns = (2 * 3 + 1) * 16
    animation_rows = (2 * 2 + 1) * 16
    print(f"\n--- animated tiles, submitted over a {animation_columns} x {animation_rows} tile window ---")

    animated_per_cell = np.zeros((map_height, map_width), dtype=np.int32)
    for layer, non_empty, _ in classified_layers:
        animated = animated_lookup[np.clip(layer.gids, 0, animated_lookup.size - 1)] & non_empty
        if animated.any():
            print(f"  {layer.name:28s} animated quads {animated.sum():7d}")
        animated_per_cell += animated

    animated_per_window = box_sum(animated_per_cell, animation_rows, animation_columns)
    animated_per_view = box_sum(animated_per_cell, view_rows, view_columns)
    if animated_per_window is None:
        print("  animation window larger than the map")
        return 0

    # only judge the windows the camera actually visits, same rule as above
    populated_window = box_sum(quads_per_cell, animation_rows, animation_columns) >= (
        0.25 * animation_columns * animation_rows
    )
    submitted = animated_per_window[populated_window] / cells_per_view
    on_screen = animated_per_view[populated] / cells_per_view

    print(f"  submitted per frame  mean/p95: {submitted.mean():6.2f}x / {np.percentile(submitted, 95):6.2f}x of the view area")
    print(f"  actually on screen   mean/p95: {on_screen.mean():6.2f}x / {np.percentile(on_screen, 95):6.2f}x of the view area")
    if submitted.mean() > 0:
        print(f"  wasted share of animated quads: {(1 - on_screen.mean() / submitted.mean()) * 100:5.1f}%")

    print("\n--- what the engine counter reports ---")
    print("the counter weights static blocks by how much of the block is on screen, but adds the")
    print("animated array whole, and both are counted once for the colour target and once for the")
    print("normal target, so the reported figure is roughly:")
    print(f"  2 x ({overdraw.mean():.2f} static + {submitted.mean():.2f} animated) = {2 * (overdraw.mean() + submitted.mean()):.2f}x")

    return 0


if __name__ == "__main__":
    sys.exit(main())
