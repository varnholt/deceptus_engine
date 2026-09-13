"""Rasterize a single tile layer of a TMX map to a PNG.

This replaces the "tmxrasterizer --show-layer <name>" step of the ambient
occlusion pipeline. Tiled's own rasterizer sizes the output canvas from the
bounding rectangle of *all* layers including their pixel offsets, which does
not match the world coordinate system the engine expects. This tool always
renders exactly map_width * tile_width by map_height * tile_height pixels with
the map origin at (0, 0).
"""

import argparse
import os
import xml.etree.ElementTree as ElementTree

from PIL import Image

Image.MAX_IMAGE_PIXELS = None

FLIPPED_HORIZONTALLY_FLAG = 0x80000000
FLIPPED_VERTICALLY_FLAG = 0x40000000
FLIPPED_DIAGONALLY_FLAG = 0x20000000
GID_MASK = ~(FLIPPED_HORIZONTALLY_FLAG | FLIPPED_VERTICALLY_FLAG | FLIPPED_DIAGONALLY_FLAG)


class Tileset:
    def __init__(self, first_gid, tileset_element, base_directory):
        self.first_gid = first_gid
        self.tile_width = int(tileset_element.get("tilewidth"))
        self.tile_height = int(tileset_element.get("tileheight"))
        self.tile_count = int(tileset_element.get("tilecount"))
        self.columns = int(tileset_element.get("columns"))
        self.margin = int(tileset_element.get("margin", 0))
        self.spacing = int(tileset_element.get("spacing", 0))

        image_element = tileset_element.find("image")
        image_path = os.path.join(base_directory, image_element.get("source"))
        self.image = Image.open(image_path).convert("RGBA")

        transparent_color = image_element.get("trans")
        if transparent_color is not None:
            self._apply_transparent_color(transparent_color)

    def _apply_transparent_color(self, transparent_color):
        transparent_color = transparent_color.lstrip("#")
        red = int(transparent_color[0:2], 16)
        green = int(transparent_color[2:4], 16)
        blue = int(transparent_color[4:6], 16)

        pixels = self.image.load()
        for y in range(self.image.height):
            for x in range(self.image.width):
                if pixels[x, y][:3] == (red, green, blue):
                    pixels[x, y] = (0, 0, 0, 0)

    def contains(self, gid):
        return self.first_gid <= gid < self.first_gid + self.tile_count

    def crop(self, gid):
        local_id = gid - self.first_gid
        column = local_id % self.columns
        row = local_id // self.columns
        left = self.margin + column * (self.tile_width + self.spacing)
        top = self.margin + row * (self.tile_height + self.spacing)
        return self.image.crop((left, top, left + self.tile_width, top + self.tile_height))


def load_tilesets(map_element, base_directory):
    tilesets = []

    for tileset_element in map_element.findall("tileset"):
        first_gid = int(tileset_element.get("firstgid"))
        source = tileset_element.get("source")

        if source is None:
            tilesets.append(Tileset(first_gid, tileset_element, base_directory))
            continue

        external_path = os.path.join(base_directory, source)
        external_root = ElementTree.parse(external_path).getroot()
        tilesets.append(Tileset(first_gid, external_root, os.path.dirname(external_path)))

    tilesets.sort(key=lambda tileset: tileset.first_gid)
    return tilesets


def find_layer(map_element, layer_name):
    for layer_element in map_element.findall("layer"):
        if layer_element.get("name") == layer_name:
            return layer_element
    raise SystemExit(f"[!] no tile layer named '{layer_name}' in map")


def read_layer_gids(layer_element):
    data_element = layer_element.find("data")
    encoding = data_element.get("encoding")

    if encoding != "csv":
        raise SystemExit(f"[!] unsupported layer encoding: {encoding}")

    return [int(entry) for entry in data_element.text.replace("\n", "").split(",") if entry.strip()]


def transform_tile(tile_image, gid):
    if gid & FLIPPED_DIAGONALLY_FLAG:
        tile_image = tile_image.transpose(Image.Transpose.TRANSPOSE)
    if gid & FLIPPED_HORIZONTALLY_FLAG:
        tile_image = tile_image.transpose(Image.Transpose.FLIP_LEFT_RIGHT)
    if gid & FLIPPED_VERTICALLY_FLAG:
        tile_image = tile_image.transpose(Image.Transpose.FLIP_TOP_BOTTOM)
    return tile_image


def rasterize(map_filename, layer_name, output_filename):
    base_directory = os.path.dirname(os.path.abspath(map_filename))
    map_element = ElementTree.parse(map_filename).getroot()

    map_width = int(map_element.get("width"))
    map_height = int(map_element.get("height"))
    tile_width = int(map_element.get("tilewidth"))
    tile_height = int(map_element.get("tileheight"))

    print(f"[x] map is {map_width}x{map_height} tiles at {tile_width}x{tile_height}px")

    tilesets = load_tilesets(map_element, base_directory)
    print(f"[x] loaded {len(tilesets)} tilesets")

    layer_element = find_layer(map_element, layer_name)
    if layer_element.get("offsetx") or layer_element.get("offsety"):
        print(f"[!] layer '{layer_name}' has a pixel offset which is not applied")

    gids = read_layer_gids(layer_element)
    expected_gid_count = map_width * map_height
    if len(gids) != expected_gid_count:
        raise SystemExit(f"[!] expected {expected_gid_count} tiles, got {len(gids)}")

    output_image = Image.new("RGBA", (map_width * tile_width, map_height * tile_height), (0, 0, 0, 0))
    tile_cache = {}
    drawn_tile_count = 0

    for tile_index, gid in enumerate(gids):
        if gid == 0:
            continue

        cached_tile = tile_cache.get(gid)
        if cached_tile is None:
            plain_gid = gid & GID_MASK
            tileset = next((candidate for candidate in tilesets if candidate.contains(plain_gid)), None)
            if tileset is None:
                raise SystemExit(f"[!] no tileset for gid {plain_gid}")
            cached_tile = transform_tile(tileset.crop(plain_gid), gid)
            tile_cache[gid] = cached_tile

        x_px = (tile_index % map_width) * tile_width
        y_px = (tile_index // map_width) * tile_height
        output_image.paste(cached_tile, (x_px, y_px), cached_tile)
        drawn_tile_count += 1

    print(f"[x] drew {drawn_tile_count} tiles ({len(tile_cache)} unique)")
    print(f"[x] writing {output_image.width}x{output_image.height} texture to {output_filename}")
    output_image.save(output_filename)


def main():
    parser = argparse.ArgumentParser(description="rasterize a single tile layer of a TMX map")
    parser.add_argument("map_filename", help="input .tmx map")
    parser.add_argument("output_filename", help="output .png image")
    parser.add_argument("--show-layer", dest="layer_name", required=True, help="name of the tile layer to render")
    arguments = parser.parse_args()

    rasterize(arguments.map_filename, arguments.layer_name, arguments.output_filename)


if __name__ == "__main__":
    main()
