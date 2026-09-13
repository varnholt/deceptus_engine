"""Plans walkable routes through a level from its own data, for the desktop playthrough tests.

Two things matter for getting this right.

The collision the engine hands box2d comes from the obj meshes next to the tmx, not from the tile
layers - the "level" layer draws plenty of tiles that never collide, because Level::parse only
turns the tiles listed in ParseData::colliding_tiles into physics. And of the two meshes that ship
per layer, only the *not optimised* one can be rasterised safely: the optimised mesh merges the
whole level into a few dozen concave chains whose interiors are the rooms, so filling those chains
face by face marks the rooms as rock. The not optimised mesh is one 24x24 quad per solid tile.

On top of the mesh, anything the player can stand on that is a box2d body counts as ground:
crumbling stones, on off blocks, box colliders, moveable boxes, moving platforms and the rest. An
on off block that the tmx marks `inverted` starts out passable, see OnOffBlock::setEnabled, and one
with `enabled` set to false does too.

The movement model is jump-then-gravity: every move ends by falling to whatever supports the
player, so walking off a ledge, leaping a gap and dropping down a shaft are the same shape of move.
Its numbers come from the real build, measured by tests/desktop/test_level_playthrough.py: a jump
reaches a bit over three tiles up, a running jump covers about six across, and a leap that descends
covers more the further it falls.
"""

import heapq
import xml.etree.ElementTree as ElementTree
from pathlib import Path

PIXELS_PER_TILE = 24

SOLID_MESH_NAME = "layer_level_solid_not_optimised.obj"
ONE_WAY_MESH_NAME = "layer_level_solid_onesided_solid_onesided_not_optimised.obj"

TILE_EMPTY = 0
TILE_SOLID = 1
TILE_ONE_WAY = 2

# the player body occupies its own tile and the one above it
PLAYER_HEIGHT_TL = 2

# the player clears one tile, never two. playersim reproduces the jump from the engine's own
# parameters and it peaks at 1.61 tiles no matter how long the run-up is - the branch in
# PlayerJump that would jump higher when running is marked "probably dead code" and never fires,
# so no press timing exists that clears a two tile step
JUMP_HEIGHT_TL = 1

# how far a running jump carries horizontally. the measured reach is six tiles, and the same margin
# applies: a plan that needs the full six fails whenever the run up is a little short
RUN_REACH_TL = 3

# one tile of fall buys this much extra horizontal travel, because a leap keeps its speed
DRIFT_PER_DROP_TL = 1

# how far down a single leap is followed before gravity is left to finish the job
MAX_DROP_TL = 14

# object groups whose mechanisms create a box2d body the player can stand on. taken from the
# CreateBody call sites in src/game/mechanisms
CARRIER_GROUPS = (
    "blocking_rects",
    "bouncers",
    "box_colliders",
    "bubble_cubes",
    "collapsing_platforms",
    "conveyor_belts",
    "crushers",
    "destructible_blocking_rects",
    "doors",
    "moveable_objects",
    "moving_platforms",
    "on_off_blocks",
)


class World:
    """The walkable state of one level, as a tile grid."""

    def __init__(self, grid: list[bytearray], width_tl: int, height_tl: int):
        self._grid = grid
        self._width_tl = width_tl
        self._height_tl = height_tl

    @property
    def width_tl(self) -> int:
        return self._width_tl

    @property
    def height_tl(self) -> int:
        return self._height_tl

    def is_solid(self, tile_x: int, tile_y: int) -> bool:
        """True for a tile the player can never enter. Outside the level counts as solid."""
        if tile_x < 0 or tile_y < 0 or tile_x >= self._width_tl or tile_y >= self._height_tl:
            return True
        return self._grid[tile_y][tile_x] == TILE_SOLID

    def is_one_way(self, tile_x: int, tile_y: int) -> bool:
        """True for a platform that carries the player from above but is entered from below."""
        if tile_x < 0 or tile_y < 0 or tile_x >= self._width_tl or tile_y >= self._height_tl:
            return False
        return self._grid[tile_y][tile_x] == TILE_ONE_WAY

    def fits(self, tile_x: int, tile_y: int) -> bool:
        """True when the player body fits with its feet in this tile."""
        return all(not self.is_solid(tile_x, tile_y - offset) for offset in range(PLAYER_HEIGHT_TL))

    def is_supported(self, tile_x: int, tile_y: int) -> bool:
        return self.is_solid(tile_x, tile_y + 1) or self.is_one_way(tile_x, tile_y + 1)

    def is_standable(self, tile_x: int, tile_y: int) -> bool:
        return self.fits(tile_x, tile_y) and self.is_supported(tile_x, tile_y)


def _read_mesh_quads(path: Path) -> list[tuple[float, float, float, float]]:
    vertices: list[tuple[float, float]] = []
    quads: list[tuple[float, float, float, float]] = []

    with path.open("r", encoding="utf-8") as mesh_file:
        for line in mesh_file:
            if line.startswith("v "):
                parts = line.split()
                vertices.append((float(parts[1]), float(parts[2])))
            elif line.startswith("f "):
                indices = [int(token.split("/")[0]) for token in line.split()[1:]]
                points = [vertices[index - 1] for index in indices]
                xs = [point[0] for point in points]
                ys = [point[1] for point in points]
                quads.append((min(xs), min(ys), max(xs), max(ys)))

    return quads


def _mark_quads(grid: list[bytearray], quads, width_tl: int, height_tl: int, value: int) -> int:
    marked = 0
    for left, top, right, bottom in quads:
        for tile_y in range(int(top // PIXELS_PER_TILE), int((bottom - 1) // PIXELS_PER_TILE) + 1):
            for tile_x in range(int(left // PIXELS_PER_TILE), int((right - 1) // PIXELS_PER_TILE) + 1):
                if 0 <= tile_x < width_tl and 0 <= tile_y < height_tl and grid[tile_y][tile_x] == TILE_EMPTY:
                    grid[tile_y][tile_x] = value
                    marked += 1
    return marked


def _read_object_properties(game_object: ElementTree.Element) -> dict[str, str]:
    return {node.get("name"): node.get("value") for node in game_object.findall("properties/property")}


def _object_tiles(game_object: ElementTree.Element) -> list[tuple[int, int]]:
    """Every tile an object covers, following its polyline when it has one.

    A moving platform is only ever at one point of its path at a time, but every point of that path
    is ground at some point, which is what a route may rely on.
    """
    origin_x = float(game_object.get("x", 0))
    origin_y = float(game_object.get("y", 0))
    width = max(PIXELS_PER_TILE, float(game_object.get("width", PIXELS_PER_TILE)))
    height = max(PIXELS_PER_TILE, float(game_object.get("height", PIXELS_PER_TILE)))

    anchors = [(origin_x, origin_y)]
    polyline = game_object.find("polyline")
    if polyline is not None:
        anchors = []
        for point in polyline.get("points", "").split():
            offset_x, offset_y = point.split(",")
            anchors.append((origin_x + float(offset_x), origin_y + float(offset_y)))

    tiles = []
    for index, (anchor_x, anchor_y) in enumerate(anchors):
        next_x, next_y = anchors[min(index + 1, len(anchors) - 1)]
        steps = int(max(abs(next_x - anchor_x), abs(next_y - anchor_y)) // PIXELS_PER_TILE) + 1
        for step in range(steps + 1):
            fraction = step / steps if steps else 0.0
            position_x = anchor_x + (next_x - anchor_x) * fraction
            position_y = anchor_y + (next_y - anchor_y) * fraction
            first_y = int(position_y // PIXELS_PER_TILE)
            last_y = int((position_y + height - 1) // PIXELS_PER_TILE)
            first_x = int(position_x // PIXELS_PER_TILE)
            last_x = int((position_x + width - 1) // PIXELS_PER_TILE)
            for tile_y in range(first_y, last_y + 1):
                for tile_x in range(first_x, last_x + 1):
                    tiles.append((tile_x, tile_y))
    return tiles


def _is_solid_at_level_start(properties: dict[str, str]) -> bool:
    """Whether a mechanism body blocks and carries the player when the level has just loaded."""
    if properties.get("inverted") == "true":
        return False
    if properties.get("enabled") == "false":
        return False
    return True


_world_cache: dict[Path, World] = {}


def load_world(level_directory: Path) -> World:
    """Builds the walkable grid for a level, cached per directory."""
    level_directory = Path(level_directory)
    if level_directory in _world_cache:
        return _world_cache[level_directory]

    tmx_path = next(level_directory.glob("*.tmx"))
    root = ElementTree.parse(tmx_path).getroot()
    width_tl = int(root.get("width"))
    height_tl = int(root.get("height"))

    grid = [bytearray(width_tl) for _ in range(height_tl)]
    _mark_quads(grid, _read_mesh_quads(level_directory / SOLID_MESH_NAME), width_tl, height_tl, TILE_SOLID)

    one_way_path = level_directory / ONE_WAY_MESH_NAME
    if one_way_path.exists():
        _mark_quads(grid, _read_mesh_quads(one_way_path), width_tl, height_tl, TILE_ONE_WAY)

    for object_group in root.findall(".//objectgroup"):
        if (object_group.get("name") or "") not in CARRIER_GROUPS:
            continue
        for game_object in object_group.findall("object"):
            if not _is_solid_at_level_start(_read_object_properties(game_object)):
                continue
            for tile_x, tile_y in _object_tiles(game_object):
                if 0 <= tile_x < width_tl and 0 <= tile_y < height_tl and grid[tile_y][tile_x] == TILE_EMPTY:
                    grid[tile_y][tile_x] = TILE_ONE_WAY

    world = World(grid, width_tl, height_tl)
    _world_cache[level_directory] = world
    return world


def _path_is_clear(world: World, steps: list[tuple[int, int]]) -> bool:
    return all(world.fits(tile_x, tile_y) for tile_x, tile_y in steps)


def _climb_variants(from_tile: tuple[int, int], to_tile: tuple[int, int]) -> list[list[tuple[int, int]]]:
    """Three shapes for a jump that gains height: rise first, move first, and a diagonal."""
    from_x, from_y = from_tile
    to_x, to_y = to_tile
    step_x = 1 if to_x >= from_x else -1
    climb = from_y - to_y
    across = abs(to_x - from_x)

    rise_first = [(from_x, from_y - height) for height in range(1, climb + 1)]
    rise_first += [(from_x + step_x * reach, to_y) for reach in range(1, across + 1)]

    move_first = [(from_x + step_x * reach, from_y) for reach in range(1, across + 1)]
    move_first += [(to_x, from_y - height) for height in range(1, climb + 1)]

    diagonal = []
    position_x, position_y = from_x, from_y
    while position_x != to_x or position_y != to_y:
        if position_y > to_y:
            position_y -= 1
            diagonal.append((position_x, position_y))
        if position_x != to_x:
            position_x += step_x
            diagonal.append((position_x, position_y))

    return [rise_first, move_first, diagonal]


def _descent_variants(from_tile: tuple[int, int], to_tile: tuple[int, int]) -> list[list[tuple[int, int]]]:
    """Two shapes for a leap that ends lower than it started: across then down, and a diagonal."""
    from_x, from_y = from_tile
    to_x, to_y = to_tile
    step_x = 1 if to_x >= from_x else -1
    across = abs(to_x - from_x)
    drop = to_y - from_y

    across_then_down = [(from_x + step_x * reach, from_y) for reach in range(1, across + 1)]
    across_then_down += [(to_x, from_y + fallen) for fallen in range(1, drop + 1)]

    diagonal = []
    position_x, position_y = from_x, from_y
    while position_x != to_x or position_y != to_y:
        if position_x != to_x:
            position_x += step_x
            diagonal.append((position_x, position_y))
        if position_y < to_y:
            position_y += 1
            diagonal.append((position_x, position_y))

    return [across_then_down, diagonal]


def settle(world: World, tile: tuple[int, int]) -> tuple[int, int]:
    """Where the player comes to rest from this tile, falling straight down."""
    tile_x, tile_y = tile
    while not world.is_supported(tile_x, tile_y) and world.fits(tile_x, tile_y + 1):
        tile_y += 1
    return (tile_x, tile_y)


def moves_from(world: World, tile: tuple[int, int]) -> list[tuple[int, int]]:
    """Every tile the player can end up standing on with one move from here."""
    tile_x, tile_y = tile
    found = []

    for direction in (-1, 1):
        if world.fits(tile_x + direction, tile_y):
            found.append(settle(world, (tile_x + direction, tile_y)))

    if not world.is_supported(tile_x, tile_y):
        found.append(settle(world, tile))
        return found

    for drop in range(1, MAX_DROP_TL + 1):
        for direction in (-1, 1):
            for across in range(1, RUN_REACH_TL + DRIFT_PER_DROP_TL * drop + 1):
                landing = (tile_x + direction * across, tile_y + drop)
                if not world.is_standable(*landing):
                    continue
                if any(_path_is_clear(world, steps) for steps in _descent_variants(tile, landing)):
                    found.append(landing)

    for climb in range(0, JUMP_HEIGHT_TL + 1):
        for direction in (-1, 1):
            for across in range(0, RUN_REACH_TL + 1):
                if climb == 0 and across == 0:
                    continue
                landing = (tile_x + direction * across, tile_y - climb)
                if not world.fits(*landing):
                    continue
                if any(_path_is_clear(world, steps) for steps in _climb_variants(tile, landing)):
                    found.append(settle(world, landing))

    return found


def plan_route(
    world: World,
    start: tuple[int, int],
    goal: tuple[int, int],
    goal_tolerance_tl: int = 2,
    node_budget: int = 200_000,
) -> list[tuple[int, int]] | None:
    """A* from a standing tile to a goal, returning the tiles to walk or nothing when unreachable."""
    start = settle(world, start)

    def heuristic(tile: tuple[int, int]) -> int:
        return abs(tile[0] - goal[0]) + abs(tile[1] - goal[1])

    open_set = [(heuristic(start), 0, start)]
    came_from: dict[tuple[int, int], tuple[int, int] | None] = {start: None}
    cost_so_far = {start: 0}
    expanded = 0

    while open_set and expanded < node_budget:
        _, cost, current = heapq.heappop(open_set)
        expanded += 1

        if abs(current[0] - goal[0]) <= goal_tolerance_tl and abs(current[1] - goal[1]) <= goal_tolerance_tl:
            path = []
            walker: tuple[int, int] | None = current
            while walker is not None:
                path.append(walker)
                walker = came_from[walker]
            return list(reversed(path))

        for neighbour in moves_from(world, current):
            step_cost = cost + 1 + abs(neighbour[0] - current[0]) + abs(neighbour[1] - current[1])
            if neighbour not in cost_so_far or step_cost < cost_so_far[neighbour]:
                cost_so_far[neighbour] = step_cost
                came_from[neighbour] = current
                heapq.heappush(open_set, (step_cost + heuristic(neighbour), step_cost, neighbour))

    return None


def to_waypoints(path: list[tuple[int, int]], max_spacing_tl: int = 8) -> list[tuple[int, int]]:
    """Thins a tile path down to the turns, keeping a waypoint at least every few tiles.

    Straight stretches do not need steering, but a long one without any waypoint lets the walk
    drift, and drifting past a pit is how a route fails.
    """
    if not path:
        return []

    waypoints = [path[0]]
    for index in range(1, len(path) - 1):
        previous = path[index - 1]
        current = path[index]
        following = path[index + 1]
        turned = (current[0] - previous[0], current[1] - previous[1]) != (
            following[0] - current[0],
            following[1] - current[1],
        )
        far_from_last = abs(current[0] - waypoints[-1][0]) + abs(current[1] - waypoints[-1][1]) >= max_spacing_tl
        if turned or far_from_last:
            waypoints.append(current)

    waypoints.append(path[-1])
    return waypoints
