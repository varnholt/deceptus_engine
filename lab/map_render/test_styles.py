"""Side by side style comparison on one region, so the final look can be picked quickly."""

from pathlib import Path

import numpy as np
import pytest
from PIL import Image

import leveldata
import maprender
from maprender import MapStyle

LEVEL_DIRECTORY = Path(__file__).resolve().parents[2] / "data" / "level-catacombs"
OUTPUT_DIRECTORY = Path(__file__).resolve().parent / "out"

REGION_CELLS = (520, 300)


@pytest.fixture(scope="session")
def level() -> leveldata.LevelData:
    return leveldata.load(LEVEL_DIRECTORY, "catacombs.tmx")


@pytest.fixture(scope="session")
def grid(level: leveldata.LevelData) -> maprender.SolidGrid:
    return maprender.rasterize_mesh(level, 3)


VARIANTS: dict[str, MapStyle] = {
    "a_plain": MapStyle(room_border=None, draw_interior_shading=False, draw_outer_wall_halo=False),
    "b_shaded": MapStyle(room_border=None),
    "c_room_border": MapStyle(room_border=(58, 84, 150, 160)),
    "d_bright_rooms": MapStyle(
        interior=(30, 46, 100, 235),
        interior_near_wall=(22, 32, 74, 235),
        wall=(150, 196, 255, 255),
        wall_outer=(48, 72, 132, 255),
        room_border=(72, 104, 178, 130),
    ),
}


def test_style_variants(level: leveldata.LevelData, grid: maprender.SolidGrid) -> None:
    OUTPUT_DIRECTORY.mkdir(exist_ok=True)

    visited_room_ids = {room.identifier for room in level.rooms}
    visited = maprender.build_visited_mask(level, grid, visited_room_ids)

    # pick a dense region: centre on the first checkpoint
    checkpoint = next(marker for marker in level.markers if marker.kind == "checkpoint")
    center_x, center_y = grid.world_to_cell(*checkpoint.center)
    left = max(0, int(center_x) - REGION_CELLS[0] // 2)
    top = max(0, int(center_y) - REGION_CELLS[1] // 2)

    tiles: list[np.ndarray] = []
    for name, style in VARIANTS.items():
        image = maprender.render(level, grid, visited, style)
        maprender.draw_markers(image, grid, visited, level.markers, style)
        maprender.draw_player(image, grid, checkpoint.center, style)
        crop = image[top : top + REGION_CELLS[1], left : left + REGION_CELLS[0]]
        tiles.append(crop)
        print(name)

    stacked = np.concatenate(tiles, axis=0)
    output = Image.fromarray(stacked, mode="RGBA").resize((stacked.shape[1] * 2, stacked.shape[0] * 2), Image.NEAREST)
    backdrop = Image.new("RGBA", output.size, (10, 12, 26, 255))
    backdrop.alpha_composite(output)
    path = OUTPUT_DIRECTORY / "10_style_variants.png"
    backdrop.save(path)
    print(path)
