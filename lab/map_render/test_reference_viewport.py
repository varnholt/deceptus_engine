"""Renders the exact viewport the engine shows, so the C++ output can be compared against it.

The engine centers a 620x290 map pixel view on the player, at 3 map pixels per tile.
"""

from pathlib import Path

import numpy as np
import pytest
from PIL import Image

import leveldata
import maprender
from maprender import MapStyle

LEVEL_DIRECTORY = Path(__file__).resolve().parents[2] / "data" / "level-catacombs"
OUTPUT_DIRECTORY = Path(__file__).resolve().parent / "out"

VIEWPORT_MAP_PX = (620, 290)

# the rooms the drive_desktop run visits, in the order it teleports through them
VISITED_SUB_ROOM_NAMES = {
    "ct-the_cell",
    "ct-the_clockwok_antechamber",
    "ct-the_syphon",
    "ct-room11",
    "ct-the_widow_maker",
}

# checkpoint 3, where drive_desktop leaves the player
PLAYER_POSITION_PX = (8388.0, 2484.0)


@pytest.fixture(scope="session")
def level() -> leveldata.LevelData:
    return leveldata.load(LEVEL_DIRECTORY, "catacombs.tmx")


def test_reference_viewport(level: leveldata.LevelData) -> None:
    grid = maprender.rasterize_mesh(level, 3)

    visited_room_ids = {
        room.identifier
        for room in level.rooms
        if any(sub_room.name in VISITED_SUB_ROOM_NAMES for sub_room in room.sub_rooms)
    }
    print(f"visited rooms: {len(visited_room_ids)}")

    style = MapStyle()
    visited = maprender.build_visited_mask(level, grid, visited_room_ids)
    image = maprender.render(level, grid, visited, style)
    maprender.draw_markers(image, grid, visited, level.markers, style, kinds={"checkpoint", "portal", "door"})
    maprender.draw_player(image, grid, PLAYER_POSITION_PX, style)

    center_x, center_y = grid.world_to_cell(*PLAYER_POSITION_PX)
    left = int(center_x) - VIEWPORT_MAP_PX[0] // 2
    top = int(center_y) - VIEWPORT_MAP_PX[1] // 2

    viewport = np.zeros((VIEWPORT_MAP_PX[1], VIEWPORT_MAP_PX[0], 4), dtype=np.uint8)
    source = image[max(0, top) : top + VIEWPORT_MAP_PX[1], max(0, left) : left + VIEWPORT_MAP_PX[0]]
    viewport[: source.shape[0], : source.shape[1]] = source

    OUTPUT_DIRECTORY.mkdir(exist_ok=True)
    output = Image.fromarray(viewport, mode="RGBA").resize((VIEWPORT_MAP_PX[0] * 2, VIEWPORT_MAP_PX[1] * 2), Image.NEAREST)
    backdrop = Image.new("RGBA", output.size, (10, 12, 26, 255))
    backdrop.alpha_composite(output)
    path = OUTPUT_DIRECTORY / "40_reference_viewport.png"
    backdrop.save(path)
    print(path)
