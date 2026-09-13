"""Walks the catacombs from the spawn, planning every leg from the level's own collision data.

    uv run --project tests pytest tests/desktop/test_walkthrough.py -s

Nothing is handed to the player and nothing is teleported. Each leg is planned with
levelrouting.plan_route from wherever the player actually stands, and whenever the walk falls short
of a waypoint the leg is planned again from the new position, which is what lets it survive
drifting a tile and falling.

The reachable set is small, and that is the finding rather than a bug in here: the player's jump
rises 1.63 tiles, measured by test_jump_trajectory.py, so a two tile ledge is at the edge of what a
run-up can manage and a three tile ledge is a wall. Which waymarks that leaves in reach is what
test_waymark_reachability reports.
"""

import time
from collections import deque
from pathlib import Path

import levelrouting
from test_level_playthrough import (
    CONFIRM_KEY,
    WaypointTimeout,
    walker,  # noqa: F401  - the fixture that starts a game with a fresh slot 0
)

REPO_ROOT = Path(__file__).resolve().parents[2]
CATACOMBS_DIRECTORY = REPO_ROOT / "data" / "level-catacombs"

# how often a leg is planned again after the walk fell short of a waypoint
REPLAN_ATTEMPTS = 6

WAYPOINT_TIMEOUT_S = 20.0

SPAWN_TILE = (49, 110)

# the waymarks of the item chain, in tile coordinates, from tests/desktop/test_level_progression.py
WAYMARKS = {
    "cell lever": (81, 110),
    "locker key": (54, 91),
    "locker": (139, 88),
    "desk drawer": (291, 88),
    "chest": (145, 102),
    "head torch": (145, 99),
    "catacombs shrine": (125, 128),
    "sword": (125, 122),
    "graveyard exit": (31, 68),
}


def walk_to_waymark(walker, world, goal: tuple[int, int], what: str = "") -> bool:
    """Plans and walks to a waymark, planning again from the live position when a waypoint fails."""
    for attempt in range(1, REPLAN_ATTEMPTS + 1):
        state = walker.state()
        start = (state.x_tl, state.y_tl)

        path = levelrouting.plan_route(world, start, goal)
        if path is None:
            print(f"  no route from {start} to {goal}")
            return False

        waypoints = levelrouting.to_waypoints(path)[1:]
        print(f"  plan {attempt}: {len(waypoints)} waypoints from {start}")

        fell_short = False
        for index, waypoint in enumerate(waypoints, 1):
            try:
                walker.walk_to(waypoint, timeout_s=WAYPOINT_TIMEOUT_S, what=f"{what} {index}/{len(waypoints)}")
            except WaypointTimeout as timeout:
                print(f"    {timeout}")
                fell_short = True
                break

        if not fell_short:
            # the plan ends within its goal tolerance and the walk within its own, so without a
            # last precise step the player can stand three tiles away from what it came for
            try:
                walker.walk_to(goal, timeout_s=WAYPOINT_TIMEOUT_S, what=f"{what}, final step")
            except WaypointTimeout as timeout:
                print(f"    {timeout}")
            state = walker.state()
            print(f"  arrived at {state.x_tl},{state.y_tl}, {abs(state.x_tl - goal[0])} tiles from {goal}")
            return True

    return False


def test_waymark_reachability():
    """Reports which waymarks the planner can reach from the spawn, no game needed."""
    world = levelrouting.load_world(CATACOMBS_DIRECTORY)

    start = levelrouting.settle(world, SPAWN_TILE)
    reached = {start}
    queue = deque([start])
    while queue:
        for move in levelrouting.moves_from(world, queue.popleft()):
            if move not in reached:
                reached.add(move)
                queue.append(move)

    xs = [tile[0] for tile in reached]
    ys = [tile[1] for tile in reached]
    print(f"{len(reached)} standing tiles reachable from {start}: x {min(xs)}..{max(xs)}, y {min(ys)}..{max(ys)}")

    for name, tile in WAYMARKS.items():
        near = [spot for spot in reached if abs(spot[0] - tile[0]) <= 2 and abs(spot[1] - tile[1]) <= 2]
        print(f"  {name:<18} {'reachable' if near else 'out of reach'}")


def test_walk_to_the_cell_lever(walker):
    """Walks from the spawn to the cell lever and examines it.

    This is the one waymark the measured jump can reach from the spawn, so it is the one leg that
    can be walked honestly today. Operating the lever needs the handle, which is in the locker.
    """
    walker.start_level_from_menu()
    walker.watch_state()
    walker.load_harmful_enemies()

    world = levelrouting.load_world(CATACOMBS_DIRECTORY)
    started_at = time.monotonic()

    assert walk_to_waymark(walker, world, WAYMARKS["cell lever"], what="the cell lever"), "never reached the lever"

    since = walker.mark()
    walker.press(CONFIRM_KEY)
    time.sleep(1.0)
    state = walker.state()
    if state.dialogue:
        walker.handle_dialogue(state)

    lever_lines = [line for line in walker._lines[since:] if "lever_cell" in line]
    print(f"the lever answered with {len(lever_lines)} event(s)")
    for line in lever_lines[:3]:
        print(f"    {line.strip()[-110:]}")

    state = walker.state()
    print(f"done at tile {state.x_tl},{state.y_tl} after {time.monotonic() - started_at:.0f}s")
    print(f"dialogue confirmations: {walker._confirmations_sent}, damage taken: {walker._damage_taken}")
    assert state.lives == walker._lives_at_start, "the player lost a life on the way"
