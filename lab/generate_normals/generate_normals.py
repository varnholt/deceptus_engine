"""generate tangent space normal maps for tile atlases.

the engine loads "<tileset>_normals.png" next to "<tileset>.png" (see TileMap::load).
tiles that sit next to each other in the atlas are usually not neighbours in the level,
so every filter here is clamped to the tile it belongs to - otherwise detail bleeds
across tiles that never touch on screen.

    atlas                              one tile, filtered on its own
    +----+----+----+                   +--------+
    | t0 | t1 | t2 |    ---->          |  clamp | <- border pixels are replicated,
    +----+----+----+                   |  edges |    never sampled from t1
    | t3 | t4 | t5 |                   +--------+
    +----+----+----+

the mapping from diffuse to normal reproduces the one behind the existing catacombs
maps: a luminance height field for the surface detail plus an alpha derived dome that
rounds off sprite silhouettes.
"""

import argparse
import pathlib

import numpy as np
from PIL import Image
from scipy.ndimage import gaussian_filter

DETAIL_GAIN = 7.0
DETAIL_SIGMA = 0.5
BEVEL_GAIN = 280.0
BEVEL_SIGMA = 1.6


def split_into_tiles(image_array: np.ndarray, tile_size: int) -> np.ndarray:
    height, width = image_array.shape[:2]
    tile_rows = height // tile_size
    tile_columns = width // tile_size
    trailing_shape = image_array.shape[2:]
    reshaped = image_array.reshape((tile_rows, tile_size, tile_columns, tile_size) + trailing_shape)
    moved = np.moveaxis(reshaped, 2, 1)
    return moved.reshape((tile_rows * tile_columns, tile_size, tile_size) + trailing_shape)


def join_from_tiles(tiles: np.ndarray, tile_rows: int, tile_columns: int) -> np.ndarray:
    tile_size = tiles.shape[1]
    trailing_shape = tiles.shape[3:]
    reshaped = tiles.reshape((tile_rows, tile_columns, tile_size, tile_size) + trailing_shape)
    moved = np.moveaxis(reshaped, 2, 1)
    return moved.reshape((tile_rows * tile_size, tile_columns * tile_size) + trailing_shape)


def blur_per_tile(tiles: np.ndarray, sigma: float) -> np.ndarray:
    if sigma <= 0.0:
        return tiles
    return gaussian_filter(tiles, sigma=(0.0, sigma, sigma), mode="nearest")


def gradient_per_tile(tiles: np.ndarray) -> tuple[np.ndarray, np.ndarray]:
    """central differences with the tile borders replicated instead of wrapped."""
    padded = np.pad(tiles, ((0, 0), (1, 1), (1, 1)), mode="edge")
    gradient_row = (padded[:, 2:, 1:-1] - padded[:, :-2, 1:-1]) * 0.5
    gradient_column = (padded[:, 1:-1, 2:] - padded[:, 1:-1, :-2]) * 0.5
    return gradient_row, gradient_column


def build_normal_map(
    diffuse: np.ndarray,
    tile_size: int,
    detail_gain: float,
    bevel_gain: float,
) -> np.ndarray:
    height, width = diffuse.shape[:2]
    tile_rows = height // tile_size
    tile_columns = width // tile_size

    tiles = split_into_tiles(diffuse.astype(np.float64), tile_size)
    luminance = 0.299 * tiles[..., 0] + 0.587 * tiles[..., 1] + 0.114 * tiles[..., 2]
    coverage = tiles[..., 3] / 255.0

    # a plain blur would drag the height down to zero across every silhouette and drown the
    # surface detail in one huge edge gradient. weighting by coverage extends the interior
    # height outwards instead, which leaves the silhouette to the bevel term below.
    weighted_height = blur_per_tile(luminance * coverage, DETAIL_SIGMA)
    weight = blur_per_tile(coverage, DETAIL_SIGMA)
    surface_height = np.where(weight > 1e-3, weighted_height / np.maximum(weight, 1e-6), 0.0)

    # a fully opaque tile blurs to a constant, so its bevel gradient is zero and the tile
    # stays flat - no embossed grid where solid tiles meet in the level
    dome_height = blur_per_tile(coverage, BEVEL_SIGMA)

    surface_gradient_row, surface_gradient_column = gradient_per_tile(surface_height)
    dome_gradient_row, dome_gradient_column = gradient_per_tile(dome_height)

    normal_x = detail_gain * surface_gradient_column - bevel_gain * dome_gradient_column
    normal_y = detail_gain * surface_gradient_row - bevel_gain * dome_gradient_row
    normal_z = np.full_like(normal_x, 127.0)

    length = np.sqrt(normal_x * normal_x + normal_y * normal_y + normal_z * normal_z)
    normal_x = normal_x / length
    normal_y = normal_y / length
    normal_z = normal_z / length

    encoded = np.empty(tiles.shape, dtype=np.uint8)
    encoded[..., 0] = np.clip(np.round(normal_x * 127.0 + 128.0), 0, 255)
    encoded[..., 1] = np.clip(np.round(normal_y * 127.0 + 128.0), 0, 255)
    encoded[..., 2] = np.clip(np.round(normal_z * 127.0 + 128.0), 0, 255)
    encoded[..., 3] = tiles[..., 3].astype(np.uint8)

    return join_from_tiles(encoded, tile_rows, tile_columns)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("diffuse", type=pathlib.Path, nargs="+")
    parser.add_argument("--tile-size", type=int, default=24)
    parser.add_argument("--detail-gain", type=float, default=DETAIL_GAIN)
    parser.add_argument("--bevel-gain", type=float, default=BEVEL_GAIN)
    parser.add_argument("--output-dir", type=pathlib.Path, default=None)
    parser.add_argument("--suffix", type=str, default="_normals")
    arguments = parser.parse_args()

    for diffuse_path in arguments.diffuse:
        diffuse = np.array(Image.open(diffuse_path).convert("RGBA"))
        normal_map = build_normal_map(
            diffuse,
            arguments.tile_size,
            arguments.detail_gain,
            arguments.bevel_gain,
        )
        output_dir = arguments.output_dir or diffuse_path.parent
        output_path = output_dir / f"{diffuse_path.stem}{arguments.suffix}{diffuse_path.suffix}"
        Image.fromarray(normal_map).save(output_path)
        print(f"{diffuse_path} -> {output_path}")


if __name__ == "__main__":
    main()
