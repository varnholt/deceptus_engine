"""Checks the detail pyramid: wall outlines stay exactly one map pixel at every level.

This mirrors LevelMap in the engine, so a regression here is a regression there.
"""

from pathlib import Path

import numpy as np
import pytest
from PIL import Image

import leveldata
import maprender
from maprender import DETAIL_LEVEL_BLOCK_SIZES, MapStyle

LEVEL_DIRECTORY = Path(__file__).resolve().parents[2] / "data" / "level-catacombs"
OUTPUT_DIRECTORY = Path(__file__).resolve().parent / "out"


@pytest.fixture(scope="session")
def level() -> leveldata.LevelData:
    return leveldata.load(LEVEL_DIRECTORY, "catacombs.tmx")


@pytest.fixture(scope="session")
def grid(level: leveldata.LevelData) -> maprender.SolidGrid:
    return maprender.rasterize_mesh(level, 3)


def _run_lengths(mask: np.ndarray) -> dict[int, int]:
    """Counts the lengths of all horizontal runs of set pixels."""
    counts: dict[int, int] = {}
    for row in mask:
        run = 0
        for value in row:
            if value:
                run += 1
            elif run:
                counts[run] = counts.get(run, 0) + 1
                run = 0
        if run:
            counts[run] = counts.get(run, 0) + 1
    return counts


@pytest.mark.parametrize("block_size", DETAIL_LEVEL_BLOCK_SIZES)
def test_wall_outline_is_one_pixel(grid: maprender.SolidGrid, block_size: int) -> None:
    style = MapStyle()
    image, world_px_per_map_px = maprender.build_detail_level_image(grid, style, block_size)

    wall = np.all(image == np.asarray(style.wall, dtype=np.uint8), axis=2)
    interior = np.all(image == np.asarray(style.interior, dtype=np.uint8), axis=2)

    assert wall.any(), "detail level has no wall outline at all"
    assert interior.any(), "detail level lost all walkable space"

    # a wall cell only exists where it borders walkable space, so no wall run may be enclosed by
    # further wall cells: every wall pixel must touch an interior pixel
    assert not (wall & ~maprender._dilate(interior)).any(), "found a wall pixel that borders no walkable space"

    print(
        f"block {block_size}: {image.shape[1]}x{image.shape[0]} px, "
        f"{world_px_per_map_px:.1f} world px per map px, wall={wall.sum()}, interior={interior.sum()}"
    )


def test_corridors_survive_to_the_coarsest_level(grid: maprender.SolidGrid) -> None:
    """Walkable space must never be lost when zooming out, that is the point of the any-merge.

    Each level merges the base grid directly and the block sizes are not multiples of each other,
    so the guarantee is against the base level rather than against the previous level.
    """
    base_ratio = float((~grid.solid).mean())
    print(f"base walkable ratio {base_ratio:.6f}")

    for block_size in DETAIL_LEVEL_BLOCK_SIZES:
        interior, _ = maprender.downsample_interior(grid, block_size)
        ratio = float(interior.mean())
        print(f"block {block_size}: walkable ratio {ratio:.6f}")
        assert ratio >= base_ratio, "detail level lost walkable space compared to the base grid"


def test_detail_level_preview(level: leveldata.LevelData, grid: maprender.SolidGrid) -> None:
    """Renders the same region at every detail level for a visual side by side."""
    OUTPUT_DIRECTORY.mkdir(exist_ok=True)

    style = MapStyle()
    checkpoint = next(marker for marker in level.markers if marker.kind == "checkpoint")

    tiles = []
    for block_size in DETAIL_LEVEL_BLOCK_SIZES:
        image, world_px_per_map_px = maprender.build_detail_level_image(grid, style, block_size)

        center_x = checkpoint.center[0] / world_px_per_map_px
        center_y = checkpoint.center[1] / world_px_per_map_px
        left = max(0, int(center_x) - 310)
        top = max(0, int(center_y) - 145)

        viewport = np.zeros((290, 620, 4), dtype=np.uint8)
        source = image[top : top + 290, left : left + 620]
        viewport[: source.shape[0], : source.shape[1]] = source
        tiles.append(viewport)

    stacked = np.concatenate(tiles, axis=0)
    output = Image.fromarray(stacked, mode="RGBA").resize((stacked.shape[1] * 2, stacked.shape[0] * 2), Image.NEAREST)
    backdrop = Image.new("RGBA", output.size, (10, 12, 26, 255))
    backdrop.alpha_composite(output)
    path = OUTPUT_DIRECTORY / "60_detail_levels.png"
    backdrop.save(path)
    print(path)
