"""Turns level mesh + room rectangles into a metroid style pixel art map.

The pipeline is deliberately kept close to what the engine can do at runtime:

  1. rasterize the collision mesh into a coarse solid grid   (once per level, cached)
  2. build a "visited" mask from the rectangles of visited rooms
  3. classify every grid cell into background / interior / wall
  4. paint the classified grid plus marker icons

Grid resolution is expressed in map pixels per tile. The existing map screen already assumes
3 map pixels per tile (player position is scaled by 0.125, doors and portals by 3.0), so that is
the default here as well.
"""

from dataclasses import dataclass, field

import numpy as np

from leveldata import LevelData, Marker, Rect

Color = tuple[int, int, int, int]


@dataclass
class MapStyle:
    map_px_per_tile: int = 3

    background: Color = (0, 0, 0, 0)

    # walkable space, from "deep inside a chamber" to "right next to a wall"
    interior: Color = (30, 44, 96, 255)
    interior_near_wall: Color = (20, 30, 70, 255)
    interior_shading_depth: int = 3

    wall: Color = (128, 176, 255, 255)
    wall_outer: Color = (52, 76, 140, 255)

    room_border: Color | None = None
    current_room_boost: tuple[int, int, int, int] | None = (18, 26, 52, 0)

    # both off by default: the flat silhouette reads cleanest at map scale and is the most
    # metroid-like. kept as knobs because the artist may want a softer look later.
    draw_interior_shading: bool = False
    draw_outer_wall_halo: bool = False

    player: Color = (255, 255, 255, 255)
    icon_outline: Color = (8, 10, 24, 255)
    marker_colors: dict[str, Color] = field(
        default_factory=lambda: {
            "checkpoint": (86, 240, 128, 255),
            "portal": (86, 200, 255, 255),
            "door": (255, 214, 92, 255),
            "extra": (255, 128, 200, 255),
            "treasure_chest": (255, 176, 64, 255),
            "level_transition": (200, 140, 255, 255),
        }
    )


@dataclass
class SolidGrid:
    """Collision mesh rasterized into a boolean grid, one cell per map pixel."""

    solid: np.ndarray
    map_px_per_tile: int
    tile_size_px: int

    @property
    def cell_size_px(self) -> float:
        return self.tile_size_px / self.map_px_per_tile

    def world_to_cell(self, x_px: float, y_px: float) -> tuple[float, float]:
        return (x_px / self.cell_size_px, y_px / self.cell_size_px)


def rasterize_mesh(level: LevelData, map_px_per_tile: int) -> SolidGrid:
    """Scanline-fills the mesh faces with the even-odd rule, so nested hole contours carve out space."""
    cell_size_px = level.tile_width_px / map_px_per_tile
    width = level.width_tl * map_px_per_tile
    height = level.height_tl * map_px_per_tile

    # collect every edge of every face as (y_top, y_bottom, x_at_y_top, slope)
    edge_y_min: list[float] = []
    edge_y_max: list[float] = []
    edge_x_at_y_min: list[float] = []
    edge_slope: list[float] = []

    for face in level.mesh_faces:
        point_count = len(face)
        for index in range(point_count):
            start_x, start_y = face[index]
            end_x, end_y = face[(index + 1) % point_count]

            if start_y == end_y:
                continue

            if start_y > end_y:
                start_x, start_y, end_x, end_y = end_x, end_y, start_x, start_y

            edge_y_min.append(start_y)
            edge_y_max.append(end_y)
            edge_x_at_y_min.append(start_x)
            edge_slope.append((end_x - start_x) / (end_y - start_y))

    solid = np.zeros((height, width), dtype=bool)

    if not edge_y_min:
        return SolidGrid(solid, map_px_per_tile, level.tile_width_px)

    y_min_array = np.asarray(edge_y_min)
    y_max_array = np.asarray(edge_y_max)
    x_at_y_min_array = np.asarray(edge_x_at_y_min)
    slope_array = np.asarray(edge_slope)

    column_centers_px = (np.arange(width) + 0.5) * cell_size_px

    for row in range(height):
        scanline_y_px = (row + 0.5) * cell_size_px

        crossing = (y_min_array <= scanline_y_px) & (y_max_array > scanline_y_px)
        if not crossing.any():
            continue

        crossings_x = x_at_y_min_array[crossing] + (scanline_y_px - y_min_array[crossing]) * slope_array[crossing]
        crossings_x.sort()

        # even-odd: a cell center is inside when the number of crossings left of it is odd
        inside_count = np.searchsorted(crossings_x, column_centers_px, side="right")
        solid[row] = (inside_count % 2) == 1

    return SolidGrid(solid, map_px_per_tile, level.tile_width_px)


def build_visited_mask(level: LevelData, grid: SolidGrid, visited_room_ids: set[int]) -> np.ndarray:
    """Marks all cells covered by the sub-room rectangles of the visited rooms."""
    visited = np.zeros_like(grid.solid, dtype=bool)
    height, width = visited.shape

    for room in level.rooms:
        if room.identifier not in visited_room_ids:
            continue
        for sub_room in room.sub_rooms:
            left, top = grid.world_to_cell(sub_room.rect.x, sub_room.rect.y)
            right, bottom = grid.world_to_cell(sub_room.rect.right, sub_room.rect.bottom)
            left_cell = max(0, int(np.floor(left)))
            top_cell = max(0, int(np.floor(top)))
            right_cell = min(width, int(np.ceil(right)))
            bottom_cell = min(height, int(np.ceil(bottom)))
            if right_cell <= left_cell or bottom_cell <= top_cell:
                continue
            visited[top_cell:bottom_cell, left_cell:right_cell] = True

    return visited


def _dilate(mask: np.ndarray) -> np.ndarray:
    """4-neighbour dilation."""
    dilated = mask.copy()
    dilated[1:, :] |= mask[:-1, :]
    dilated[:-1, :] |= mask[1:, :]
    dilated[:, 1:] |= mask[:, :-1]
    dilated[:, :-1] |= mask[:, 1:]
    return dilated


#! how many base cells collapse into one map pixel, one entry per detail level.
#! mirrors detail_level_block_sizes in src/game/level/levelmap.cpp
DETAIL_LEVEL_BLOCK_SIZES = (1, 2, 3, 4)


def downsample_interior(grid: SolidGrid, block_size: int) -> tuple[np.ndarray, float]:
    """Merges blocks of base cells into one map pixel, keeping a block walkable if any cell is.

    That is what lets a one tile corridor survive to the coarsest detail level, and it lets each
    level derive its own one pixel wall skin instead of scaling a finer one.
    """
    interior = ~grid.solid
    if block_size == 1:
        return interior, grid.cell_size_px

    height = interior.shape[0] // block_size
    width = interior.shape[1] // block_size
    trimmed = interior[: height * block_size, : width * block_size]
    merged = trimmed.reshape(height, block_size, width, block_size).any(axis=(1, 3))
    return merged, grid.cell_size_px * block_size


def _mix(first: Color, second: Color, factor: float) -> Color:
    return tuple(int(round(first[channel] + (second[channel] - first[channel]) * factor)) for channel in range(4))  # type: ignore[return-value]


def build_detail_level_image(grid: SolidGrid, style: MapStyle, block_size: int) -> tuple[np.ndarray, float]:
    """Paints one entry of the detail pyramid. Wall outlines are always exactly one map pixel."""
    interior, world_px_per_map_px = downsample_interior(grid, block_size)

    height, width = interior.shape
    image = np.zeros((height, width, 4), dtype=np.uint8)
    image[:, :] = np.asarray(style.background, dtype=np.uint8)

    image[interior] = np.asarray(style.interior, dtype=np.uint8)
    image[~interior & _dilate(interior)] = np.asarray(style.wall, dtype=np.uint8)

    return image, world_px_per_map_px


def build_map_image(grid: SolidGrid, style: MapStyle) -> np.ndarray:
    """Paints the whole level once, ignoring exploration.

    This is the expensive step and it only depends on the level geometry, so the engine can do it
    once at level load and keep the result in a texture. Exploration is applied later by only
    revealing the rectangles of the rooms that have been visited.
    """
    solid = grid.solid
    interior = ~solid

    height, width = solid.shape
    image = np.zeros((height, width, 4), dtype=np.uint8)
    image[:, :] = np.asarray(style.background, dtype=np.uint8)

    # walkable space, shaded from the wall inwards so large chambers do not look flat
    image[interior] = np.asarray(style.interior, dtype=np.uint8)
    if style.draw_interior_shading:
        ring = solid
        for step in range(style.interior_shading_depth):
            ring = _dilate(ring)
            factor = 1.0 - (step / style.interior_shading_depth)
            image[ring & interior] = np.asarray(_mix(style.interior, style.interior_near_wall, factor), dtype=np.uint8)

    # only solid cells bordering walkable space become visible wall, everything deeper stays background
    wall = solid & _dilate(interior)
    if style.draw_outer_wall_halo:
        image[solid & _dilate(wall) & ~wall] = np.asarray(style.wall_outer, dtype=np.uint8)
    image[wall] = np.asarray(style.wall, dtype=np.uint8)

    return image


def reveal(map_image: np.ndarray, visited: np.ndarray, background: Color = (0, 0, 0, 0)) -> np.ndarray:
    """Applies exploration: everything outside the visited rectangles falls back to the background."""
    revealed = np.zeros_like(map_image)
    revealed[:, :] = np.asarray(background, dtype=np.uint8)
    revealed[visited] = map_image[visited]
    return revealed


def _paint(image: np.ndarray, mask: np.ndarray, color: Color) -> None:
    image[mask] = np.asarray(color, dtype=np.uint8)


def render(
    level: LevelData,
    grid: SolidGrid,
    visited: np.ndarray,
    style: MapStyle,
    current_room_mask: np.ndarray | None = None,
) -> np.ndarray:
    image = reveal(build_map_image(grid, style), visited, style.background)

    if current_room_mask is not None and style.current_room_boost is not None:
        # brighten the current room without flattening its shading
        mask = current_room_mask & visited & ~grid.solid
        boost = np.asarray(style.current_room_boost, dtype=np.int16)
        image[mask] = np.clip(image[mask].astype(np.int16) + boost, 0, 255).astype(np.uint8)

    if style.room_border is not None:
        _paint(image, _room_border_mask(level, grid, visited), style.room_border)

    return image


def _room_border_mask(level: LevelData, grid: SolidGrid, visited: np.ndarray) -> np.ndarray:
    """Outline of every visited sub-room rectangle, clipped to cells that are actually visited."""
    border = np.zeros_like(grid.solid)
    height, width = border.shape

    for room in level.rooms:
        for sub_room in room.sub_rooms:
            left, top = grid.world_to_cell(sub_room.rect.x, sub_room.rect.y)
            right, bottom = grid.world_to_cell(sub_room.rect.right, sub_room.rect.bottom)
            left_cell = max(0, int(np.floor(left)))
            top_cell = max(0, int(np.floor(top)))
            right_cell = min(width, int(np.ceil(right))) - 1
            bottom_cell = min(height, int(np.ceil(bottom))) - 1
            if right_cell <= left_cell or bottom_cell <= top_cell:
                continue
            border[top_cell, left_cell : right_cell + 1] = True
            border[bottom_cell, left_cell : right_cell + 1] = True
            border[top_cell : bottom_cell + 1, left_cell] = True
            border[top_cell : bottom_cell + 1, right_cell] = True

    return border & visited


# 7x7 pixel art icons, one character per pixel: '.' transparent, 'x' marker color
ICONS: dict[str, list[str]] = {
    "checkpoint": [
        "..xxx..",
        ".x...x.",
        "x..xx.x",
        "x.x.x.x",
        "x.xx..x",
        ".x...x.",
        "..xxx..",
    ],
    "portal": [
        "..xxx..",
        ".x...x.",
        "x..x..x",
        "x.xxx.x",
        "x..x..x",
        ".x...x.",
        "..xxx..",
    ],
    "door": [
        ".......",
        "..xxx..",
        "..x.x..",
        "..x.x..",
        "..x.x..",
        "..xxx..",
        ".......",
    ],
    "extra": [
        "...x...",
        "..xxx..",
        ".xxxxx.",
        "xxxxxxx",
        ".xxxxx.",
        "..xxx..",
        "...x...",
    ],
    "treasure_chest": [
        ".......",
        ".xxxxx.",
        ".x...x.",
        ".xx.xx.",
        ".x...x.",
        ".xxxxx.",
        ".......",
    ],
    "level_transition": [
        ".......",
        "..x....",
        "..xx...",
        "..xxx..",
        "..xx...",
        "..x....",
        ".......",
    ],
    "player": [
        "...x...",
        "..xxx..",
        ".xxxxx.",
        "xxxxxxx",
        ".xxxxx.",
        "..xxx..",
        "...x...",
    ],
}


def _blit_icon(image: np.ndarray, icon: list[str], center_x: int, center_y: int, color: Color, outline: Color | None = None) -> None:
    """Draws a 7x7 icon. When an outline color is given, the icon gets a 1px dark halo first."""
    height, width = image.shape[:2]
    icon_height = len(icon)
    icon_width = len(icon[0])
    origin_x = center_x - icon_width // 2
    origin_y = center_y - icon_height // 2

    def set_pixel(x: int, y: int, pixel_color: Color) -> None:
        if x < 0 or y < 0 or x >= width or y >= height:
            return
        image[y, x] = np.asarray(pixel_color, dtype=np.uint8)

    if outline is not None:
        for row_index, row in enumerate(icon):
            for column_index, character in enumerate(row):
                if character == ".":
                    continue
                for offset_y in (-1, 0, 1):
                    for offset_x in (-1, 0, 1):
                        set_pixel(origin_x + column_index + offset_x, origin_y + row_index + offset_y, outline)

    for row_index, row in enumerate(icon):
        for column_index, character in enumerate(row):
            if character == ".":
                continue
            set_pixel(origin_x + column_index, origin_y + row_index, color)


def draw_markers(
    image: np.ndarray,
    grid: SolidGrid,
    visited: np.ndarray,
    markers: list[Marker],
    style: MapStyle,
    kinds: set[str] | None = None,
) -> None:
    height, width = visited.shape

    for marker in markers:
        if kinds is not None and marker.kind not in kinds:
            continue

        center_x_px, center_y_px = marker.center
        cell_x, cell_y = grid.world_to_cell(center_x_px, center_y_px)
        cell_x = int(cell_x)
        cell_y = int(cell_y)

        if cell_x < 0 or cell_y < 0 or cell_x >= width or cell_y >= height:
            continue

        # a marker is only known once its room has been visited
        if not visited[cell_y, cell_x]:
            continue

        icon = ICONS.get(marker.kind)
        if icon is None:
            continue

        _blit_icon(image, icon, cell_x, cell_y, style.marker_colors[marker.kind], style.icon_outline)


def draw_player(image: np.ndarray, grid: SolidGrid, player_position_px: tuple[float, float], style: MapStyle) -> None:
    cell_x, cell_y = grid.world_to_cell(*player_position_px)
    _blit_icon(image, ICONS["player"], int(cell_x), int(cell_y), style.player, style.icon_outline)


def bounding_box_of_visited(level: LevelData, visited_room_ids: set[int]) -> Rect | None:
    left = top = float("inf")
    right = bottom = float("-inf")

    for room in level.rooms:
        if room.identifier not in visited_room_ids:
            continue
        for sub_room in room.sub_rooms:
            left = min(left, sub_room.rect.x)
            top = min(top, sub_room.rect.y)
            right = max(right, sub_room.rect.right)
            bottom = max(bottom, sub_room.rect.bottom)

    if left > right:
        return None

    return Rect(left, top, right - left, bottom - top)
