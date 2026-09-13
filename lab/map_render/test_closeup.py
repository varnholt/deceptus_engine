"""4x close-up of a single chamber, to judge shading and icon legibility."""

from pathlib import Path

import numpy as np
import pytest
from PIL import Image

import leveldata
import maprender
from maprender import MapStyle

LEVEL_DIRECTORY = Path(__file__).resolve().parents[2] / "data" / "level-catacombs"
OUTPUT_DIRECTORY = Path(__file__).resolve().parent / "out"

REGION_CELLS = (300, 170)


@pytest.fixture(scope="session")
def level() -> leveldata.LevelData:
    return leveldata.load(LEVEL_DIRECTORY, "catacombs.tmx")


@pytest.fixture(scope="session")
def grid(level: leveldata.LevelData) -> maprender.SolidGrid:
    return maprender.rasterize_mesh(level, 3)


SHADING_VARIANTS: dict[str, MapStyle] = {
    "flat": MapStyle(draw_interior_shading=False, draw_outer_wall_halo=False),
    "subtle": MapStyle(),
    "strong": MapStyle(
        interior=(38, 56, 118, 255),
        interior_near_wall=(17, 24, 58, 255),
        interior_shading_depth=4,
    ),
}


def test_closeup(level: leveldata.LevelData, grid: maprender.SolidGrid) -> None:
    OUTPUT_DIRECTORY.mkdir(exist_ok=True)

    visited_room_ids = {room.identifier for room in level.rooms}
    checkpoint = next(marker for marker in level.markers if marker.kind == "checkpoint")
    center_x, center_y = grid.world_to_cell(*checkpoint.center)
    left = max(0, int(center_x) - REGION_CELLS[0] // 2)
    top = max(0, int(center_y) - REGION_CELLS[1] // 2)

    tiles: list[np.ndarray] = []
    for name, style in SHADING_VARIANTS.items():
        visited = maprender.build_visited_mask(level, grid, visited_room_ids)
        image = maprender.render(level, grid, visited, style)
        maprender.draw_markers(image, grid, visited, level.markers, style)
        maprender.draw_player(image, grid, checkpoint.center, style)
        tiles.append(image[top : top + REGION_CELLS[1], left : left + REGION_CELLS[0]])
        print(name)

    stacked = np.concatenate(tiles, axis=0)
    output = Image.fromarray(stacked, mode="RGBA").resize((stacked.shape[1] * 3, stacked.shape[0] * 3), Image.NEAREST)
    backdrop = Image.new("RGBA", output.size, (10, 12, 26, 255))
    backdrop.alpha_composite(output)
    path = OUTPUT_DIRECTORY / "11_closeup_shading.png"
    backdrop.save(path)
    print(path)
