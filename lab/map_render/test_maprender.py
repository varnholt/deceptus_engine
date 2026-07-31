"""Renders map previews so the look can be iterated on without starting the game.

    uv run pytest

Outputs land in lab/map_render/out/.
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

# roughly the pixel area the map page has available between header and footer
VIEWPORT_SIZE_PX = (620, 290)


@pytest.fixture(scope="session")
def level() -> leveldata.LevelData:
    return leveldata.load(LEVEL_DIRECTORY, "catacombs.tmx")


@pytest.fixture(scope="session")
def grid(level: leveldata.LevelData) -> maprender.SolidGrid:
    return maprender.rasterize_mesh(level, MapStyle().map_px_per_tile)


def _save(image: np.ndarray, filename: str, scale: int = 1) -> Path:
    OUTPUT_DIRECTORY.mkdir(exist_ok=True)
    output = Image.fromarray(image, mode="RGBA")
    if scale != 1:
        output = output.resize((output.width * scale, output.height * scale), Image.NEAREST)

    # composite onto the menu background colour so the png is readable in a viewer
    backdrop = Image.new("RGBA", output.size, (10, 12, 26, 255))
    backdrop.alpha_composite(output)

    path = OUTPUT_DIRECTORY / filename
    backdrop.save(path)
    return path


def _crop_to_visited(image: np.ndarray, level: leveldata.LevelData, grid: maprender.SolidGrid, visited_room_ids: set[int]) -> np.ndarray:
    box = maprender.bounding_box_of_visited(level, visited_room_ids)
    assert box is not None
    left, top = grid.world_to_cell(box.x, box.y)
    right, bottom = grid.world_to_cell(box.right, box.bottom)
    margin = 4
    return image[
        max(0, int(top) - margin) : int(bottom) + margin,
        max(0, int(left) - margin) : int(right) + margin,
    ]


def test_level_loads(level: leveldata.LevelData) -> None:
    print(f"\nlevel {level.width_tl}x{level.height_tl} tiles ({level.width_px}x{level.height_px} px)")
    print(f"mesh faces: {len(level.mesh_faces)}")
    print(f"rooms: {len(level.rooms)} (sub rooms: {sum(len(room.sub_rooms) for room in level.rooms)})")

    marker_counts: dict[str, int] = {}
    for marker in level.markers:
        marker_counts[marker.kind] = marker_counts.get(marker.kind, 0) + 1
    print(f"markers: {marker_counts}")

    assert level.mesh_faces
    assert level.rooms


def test_mesh_rasterizes(grid: maprender.SolidGrid, level: leveldata.LevelData) -> None:
    solid_ratio = grid.solid.mean()
    print(f"\nsolid grid: {grid.solid.shape[1]}x{grid.solid.shape[0]} cells, {solid_ratio:.1%} solid")
    assert 0.02 < solid_ratio < 0.9

    image = np.zeros((*grid.solid.shape, 4), dtype=np.uint8)
    image[grid.solid] = (128, 176, 255, 255)
    print(_save(image, "00_solid_grid.png"))


def test_full_map_all_rooms_visited(level: leveldata.LevelData, grid: maprender.SolidGrid) -> None:
    visited_room_ids = {room.identifier for room in level.rooms}
    visited = maprender.build_visited_mask(level, grid, visited_room_ids)

    style = MapStyle()
    image = maprender.render(level, grid, visited, style)
    maprender.draw_markers(image, grid, visited, level.markers, style)

    print(_save(_crop_to_visited(image, level, grid, visited_room_ids), "01_full_map.png"))


def test_partial_exploration(level: leveldata.LevelData, grid: maprender.SolidGrid) -> None:
    visited_room_ids = {room.identifier for room in level.rooms[:14]}
    visited = maprender.build_visited_mask(level, grid, visited_room_ids)
    current_room_mask = maprender.build_visited_mask(level, grid, {level.rooms[3].identifier})

    style = MapStyle()
    image = maprender.render(level, grid, visited, style, current_room_mask)
    maprender.draw_markers(image, grid, visited, level.markers, style)

    sub_room = level.rooms[3].sub_rooms[0]
    maprender.draw_player(image, grid, (sub_room.rect.x + sub_room.rect.width * 0.5, sub_room.rect.y + sub_room.rect.height * 0.5), style)

    print(_save(_crop_to_visited(image, level, grid, visited_room_ids), "02_partial.png", scale=2))


def test_viewport_at_player(level: leveldata.LevelData, grid: maprender.SolidGrid) -> None:
    """Shows what actually fits on the map page at zoom 1 around the player."""
    visited_room_ids = {room.identifier for room in level.rooms}
    visited = maprender.build_visited_mask(level, grid, visited_room_ids)

    style = MapStyle()
    image = maprender.render(level, grid, visited, style)
    maprender.draw_markers(image, grid, visited, level.markers, style)

    sub_room = level.rooms[0].sub_rooms[0]
    player_position_px = (sub_room.rect.x + sub_room.rect.width * 0.5, sub_room.rect.y + sub_room.rect.height * 0.5)
    maprender.draw_player(image, grid, player_position_px, style)

    center_x, center_y = grid.world_to_cell(*player_position_px)
    left = int(center_x) - VIEWPORT_SIZE_PX[0] // 2
    top = int(center_y) - VIEWPORT_SIZE_PX[1] // 2
    viewport = image[
        max(0, top) : max(0, top) + VIEWPORT_SIZE_PX[1],
        max(0, left) : max(0, left) + VIEWPORT_SIZE_PX[0],
    ]

    print(_save(viewport, "03_viewport.png", scale=2))


@pytest.mark.parametrize("map_px_per_tile", [1, 2, 3, 4])
def test_resolution_variants(level: leveldata.LevelData, map_px_per_tile: int) -> None:
    style = MapStyle(map_px_per_tile=map_px_per_tile)
    resolution_grid = maprender.rasterize_mesh(level, map_px_per_tile)
    visited_room_ids = {room.identifier for room in level.rooms[:14]}
    visited = maprender.build_visited_mask(level, resolution_grid, visited_room_ids)

    image = maprender.render(level, resolution_grid, visited, style)
    cropped = _crop_to_visited(image, level, resolution_grid, visited_room_ids)
    print(_save(cropped, f"04_resolution_{map_px_per_tile}.png", scale=max(1, 4 // map_px_per_tile)))
