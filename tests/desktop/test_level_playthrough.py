"""Walks the catacombs and graveyard item chain instead of teleporting through it.

    uv run --project tests pytest tests/desktop/test_level_playthrough.py -s

This is the recordable sibling of test_level_progression.py: the same chain, the same log markers,
but the player walks between the mechanisms so the run can be captured to video. Walking needs a
position readout, which the console provides with `pwatch <interval_ms>`; every line it writes is
parsed back into a player state, and the walk is closed loop - it steers towards the next waypoint,
jumps when it stops making progress and gives up on a waypoint rather than on the whole run.

The same watch line carries the health, the life count and whether a dialogue is on screen, so the
walk can hold still while a message box is up, confirm it once the text had time to be read, and
tell a run that lost a life from one that merely took a hit.

Needs a Windows desktop build with DEVELOPMENT_MODE, same as the progression test.
"""

import json
import os
import re
import shutil
import time
from pathlib import Path
from typing import NamedTuple

import pytest
import win32api
import win32con

from test_level_progression import (
    Game,
    SAVE_STATE_BACKUP_PATH,
    SAVE_STATE_PATH,
    SETTINGS_DIRECTORY,
)

# the console writes one of these per watch interval, see Console::updatePlayerWatch
STATE_PATTERN = re.compile(
    r"player position: tile (-?\d+) (-?\d+) px (-?[\d.]+) (-?[\d.]+) health (\d+)/(\d+) lives (-?\d+) dialogue ([01])"
)

ENEMY_PATTERN = re.compile(r"player enemies: (\d+)(.*)$")
ENEMY_ENTRY_PATTERN = re.compile(r"\| (\S+) (-?\d+) (-?\d+)")

# the enemy scripts that have hurt the player before. nobody tells the bot which enemies are
# dangerous, it finds out by being hit once and remembers across runs
HARMFUL_ENEMIES_PATH = Path(__file__).with_name("harmful_enemies.json")

# an enemy this close when the health drops is held responsible for it
BLAME_RANGE_TL = 4

# a harmful enemy this many tiles ahead gets jumped, which either clears it or lands on top of it
JUMP_RANGE_TL = 4

# how often the game logs the player state, and how often the walk loop looks at it
WATCH_INTERVAL_MS = 100
STEER_INTERVAL_S = 0.05

# a waypoint counts as reached inside this many tiles
ARRIVAL_TOLERANCE_TL = 1

# the player is considered stuck when its tile position has not changed for this long
STUCK_TIMEOUT_S = 0.5

# a 0.2s hold reaches the full jump height of a bit over 3 tiles, longer holds add nothing
JUMP_HOLD_S = 0.2

# a jump takes about this long to land, and jumping again mid air does nothing
JUMP_COOLDOWN_S = 0.9

# jumps that change neither the tile nor the height before the waypoint is called unreachable.
# without this the bot stands in one place hammering the jump key until the timeout runs out
FRUITLESS_JUMP_LIMIT = 3

# a jump from a standstill barely moves sideways, so a ledge is taken with a run-up: back off this
# far, then come at it running. two tries before the waypoint is given up
RUN_UP_SECONDS = 0.45
RUN_UP_ATTEMPTS = 2

# how long a dialogue is left on screen before it is confirmed, so the text stays readable in a
# recording rather than being skipped the frame it appears
DIALOGUE_READ_S = 2.5
DIALOGUE_TIMEOUT_S = 30.0

WALK_KEY_LEFT = win32con.VK_LEFT
WALK_KEY_RIGHT = win32con.VK_RIGHT
JUMP_KEY = win32con.VK_SPACE
CONFIRM_KEY = win32con.VK_RETURN


class PlayerState(NamedTuple):
    x_tl: int
    y_tl: int
    x_px: float
    y_px: float
    health: int
    health_max: int
    lives: int
    dialogue: bool


class WaypointTimeout(AssertionError):
    """Raised when the walk did not reach a waypoint in time."""


class PlayerDied(AssertionError):
    """Raised when the player lost a life, which invalidates a recorded run."""


class Walker(Game):
    """A running game that can be steered towards a tile instead of teleported onto it."""

    def __init__(self):
        super().__init__()
        self._held_keys: set[int] = set()
        self._lives_at_start: int | None = None
        self._confirmations_sent = 0
        self._damage_taken = 0
        self._harmful_enemies: set[str] = set()
        self._dialogue_read_s = DIALOGUE_READ_S

    def hold(self, virtual_key: int) -> None:
        if virtual_key not in self._held_keys:
            win32api.PostMessage(self._handle, win32con.WM_KEYDOWN, virtual_key, 0)
            self._held_keys.add(virtual_key)

    def release(self, virtual_key: int) -> None:
        if virtual_key in self._held_keys:
            win32api.PostMessage(self._handle, win32con.WM_KEYUP, virtual_key, 0)
            self._held_keys.discard(virtual_key)

    def release_all(self) -> None:
        for virtual_key in list(self._held_keys):
            self.release(virtual_key)

    def state(self, timeout_s: float = 3.0) -> PlayerState:
        """Returns the newest player state the watch logged."""
        deadline = time.monotonic() + timeout_s
        while time.monotonic() < deadline:
            for line in reversed(self._lines[-60:]):
                match = STATE_PATTERN.search(line)
                if match:
                    return PlayerState(
                        int(match.group(1)),
                        int(match.group(2)),
                        float(match.group(3)),
                        float(match.group(4)),
                        int(match.group(5)),
                        int(match.group(6)),
                        int(match.group(7)),
                        match.group(8) == "1",
                    )
            time.sleep(0.05)
        raise AssertionError(f"no player state in the log, is the watch on?{os.linesep}{self.tail()}")

    def watch_state(self, interval_ms: int = WATCH_INTERVAL_MS) -> None:
        self.console(f"pwatch {interval_ms}")
        self._lives_at_start = self.state().lives

    def enemies(self) -> list[tuple[str, int, int]]:
        """Returns the enemies the watch last reported, as (script, dx_tl, dy_tl) around the player."""
        for line in reversed(self._lines[-60:]):
            match = ENEMY_PATTERN.search(line)
            if match:
                return [
                    (entry[0], int(entry[1]), int(entry[2])) for entry in ENEMY_ENTRY_PATTERN.findall(match.group(2))
                ]
        return []

    def load_harmful_enemies(self) -> None:
        """Reads back which enemy scripts have hurt the player before."""
        if HARMFUL_ENEMIES_PATH.exists():
            self._harmful_enemies = set(json.loads(HARMFUL_ENEMIES_PATH.read_text()))
        print(f"enemies known to hurt: {sorted(self._harmful_enemies) or 'none yet'}")

    def note_damage(self, enemies: list[tuple[str, int, int]]) -> None:
        """Files every enemy that was next to the player when its health dropped as harmful."""
        for script_name, distance_x_tl, distance_y_tl in enemies:
            if abs(distance_x_tl) > BLAME_RANGE_TL or abs(distance_y_tl) > BLAME_RANGE_TL:
                continue
            if script_name not in self._harmful_enemies:
                print(f"  {Path(script_name).stem} hurt the player, it gets jumped from now on")
            self._harmful_enemies.add(script_name)
        HARMFUL_ENEMIES_PATH.write_text(json.dumps(sorted(self._harmful_enemies), indent=4))

    def enemy_to_jump(self, heading: int) -> tuple[str, int, int] | None:
        """The nearest harmful enemy in the direction of travel, close enough to jump over or onto."""
        candidates = [
            enemy
            for enemy in self.enemies()
            if enemy[0] in self._harmful_enemies and abs(enemy[2]) <= 2 and 0 < enemy[1] * heading <= JUMP_RANGE_TL
        ]
        return min(candidates, key=lambda enemy: abs(enemy[1])) if candidates else None

    def handle_dialogue(self, state: PlayerState) -> bool:
        """Reads and confirms a message box, and reports whether one was on screen.

        A box swallows every key while it is up, movement included, so the walk has to notice it
        rather than treating the standstill as a platforming problem.
        """
        if not state.dialogue:
            return False

        self.release_all()
        deadline = time.monotonic() + DIALOGUE_TIMEOUT_S
        while time.monotonic() < deadline:
            time.sleep(self._dialogue_read_s)
            self.press(CONFIRM_KEY)
            self._confirmations_sent += 1
            time.sleep(0.4)
            if not self.state().dialogue:
                return True
        raise AssertionError(f"a dialogue stayed on screen for {DIALOGUE_TIMEOUT_S}s{os.linesep}{self.tail()}")

    def take_run_up(self, heading: int) -> None:
        """Backs away from the target, so the next approach arrives at running speed."""
        self.release_all()
        self.hold(WALK_KEY_LEFT if heading > 0 else WALK_KEY_RIGHT)
        time.sleep(RUN_UP_SECONDS)
        self.release_all()
        time.sleep(0.1)

    def check_alive(self, state: PlayerState) -> None:
        if self._lives_at_start is not None and state.lives < self._lives_at_start:
            self.release_all()
            raise PlayerDied(f"the player lost a life at tile {state.x_tl},{state.y_tl}")

    def walk_to(self, target: tuple[int, int], timeout_s: float = 20.0, what: str = "", on_state=None) -> None:
        """Steers the player towards a tile until it stands there.

        The loop only ever holds a direction and taps jump: it reads the state the game logs, holds
        the direction that shortens the horizontal distance, and jumps whenever the tile position
        stopped changing or the target is above the player. Nothing about the route is assumed, so a
        waypoint that turns out to be a tile off still resolves.
        """
        target_x_tl, target_y_tl = target
        deadline = time.monotonic() + timeout_s
        last_tile = None
        last_progress_at = time.monotonic()
        last_jump_at = 0.0
        fruitless_jumps = 0
        tile_at_last_jump = None
        run_ups_taken = 0
        health_before = self.state().health

        while time.monotonic() < deadline:
            state = self.state()
            self.check_alive(state)
            if on_state is not None:
                on_state(state)

            if self.handle_dialogue(state):
                # the dialogue ate part of the budget without the player moving, so give it back
                deadline += self._dialogue_read_s + 1.0
                last_progress_at = time.monotonic()
                continue

            if state.health < health_before:
                self._damage_taken += health_before - state.health
                health_before = state.health
                self.note_damage(self.enemies())

            distance_x_tl = target_x_tl - state.x_tl
            distance_y_tl = target_y_tl - state.y_tl
            if abs(distance_x_tl) <= ARRIVAL_TOLERANCE_TL and abs(distance_y_tl) <= ARRIVAL_TOLERANCE_TL:
                self.release_all()
                print(f"  reached {target} as {state.x_tl},{state.y_tl}" + (f" ({what})" if what else ""))
                return

            if distance_x_tl > ARRIVAL_TOLERANCE_TL:
                self.release(WALK_KEY_LEFT)
                self.hold(WALK_KEY_RIGHT)
            elif distance_x_tl < -ARRIVAL_TOLERANCE_TL:
                self.release(WALK_KEY_RIGHT)
                self.hold(WALK_KEY_LEFT)
            else:
                self.release(WALK_KEY_RIGHT)
                self.release(WALK_KEY_LEFT)

            if (state.x_tl, state.y_tl) != last_tile:
                last_tile = (state.x_tl, state.y_tl)
                last_progress_at = time.monotonic()

            heading = 1 if distance_x_tl > 0 else -1
            enemy = self.enemy_to_jump(heading)

            # a jump needs a reason. walking into something that does not move is one, an enemy
            # that hurts is another, and a target above is only a reason once the horizontal
            # distance is closed, otherwise the bot jumps its way along a corridor for nothing
            is_blocked = time.monotonic() - last_progress_at > STUCK_TIMEOUT_S
            # a climb is jumped while approaching, two to four tiles out, so the arc carries the
            # player onto the ledge rather than into its side
            is_below_target = distance_y_tl < -ARRIVAL_TOLERANCE_TL and 1 <= abs(distance_x_tl) <= JUMP_RANGE_TL
            is_under_target = distance_y_tl < -ARRIVAL_TOLERANCE_TL and abs(distance_x_tl) < 1
            has_reason = enemy is not None or is_blocked or is_below_target
            if is_under_target and not is_blocked:
                # right below the ledge: no amount of jumping helps, so make room for a run-up
                self.take_run_up(heading)
                last_progress_at = time.monotonic()
                continue
            if has_reason and time.monotonic() - last_jump_at > JUMP_COOLDOWN_S:
                # a jump that lands the player on the same tile it started from got nowhere
                if tile_at_last_jump == (state.x_tl, state.y_tl):
                    fruitless_jumps += 1
                else:
                    fruitless_jumps = 0
                tile_at_last_jump = (state.x_tl, state.y_tl)

                if fruitless_jumps >= FRUITLESS_JUMP_LIMIT:
                    if distance_y_tl < -ARRIVAL_TOLERANCE_TL and run_ups_taken < RUN_UP_ATTEMPTS:
                        run_ups_taken += 1
                        fruitless_jumps = 0
                        print(f"  taking a run-up at {target} ({run_ups_taken}/{RUN_UP_ATTEMPTS})")
                        self.take_run_up(heading)
                        last_progress_at = time.monotonic()
                        continue

                    self.release_all()
                    raise WaypointTimeout(
                        f"{what or target}: {state.x_tl},{state.y_tl} is as far as it goes, "
                        f"{FRUITLESS_JUMP_LIMIT} jumps changed nothing"
                    )

                last_jump_at = time.monotonic()
                self.hold(JUMP_KEY)
                time.sleep(JUMP_HOLD_S)
                self.release(JUMP_KEY)

            time.sleep(STEER_INTERVAL_S)

        self.release_all()
        state = self.state()
        raise WaypointTimeout(
            f"{what or target}: stopped at {state.x_tl},{state.y_tl}, "
            f"{target_x_tl - state.x_tl:+d},{target_y_tl - state.y_tl:+d} short of {target}"
        )

    def walk_route(self, waypoints: list[tuple[int, int]], timeout_s: float = 20.0, what: str = "") -> None:
        for index, waypoint in enumerate(waypoints):
            self.walk_to(waypoint, timeout_s=timeout_s, what=f"{what} waypoint {index + 1}/{len(waypoints)}")


@pytest.fixture
def walker():
    """A started game with a fresh slot 0, same preparation as the progression test."""
    SETTINGS_DIRECTORY.mkdir(parents=True, exist_ok=True)
    if SAVE_STATE_PATH.exists() and not SAVE_STATE_BACKUP_PATH.exists():
        shutil.copy2(SAVE_STATE_PATH, SAVE_STATE_BACKUP_PATH)
    slots = json.loads(SAVE_STATE_PATH.read_text()) if SAVE_STATE_PATH.exists() else [{}, {}, {}]
    slots[0] = {"levelindex": 0, "checkpoints": {}, "levelstate": None, "playerinfo": {}}
    SAVE_STATE_PATH.write_text(json.dumps(slots, indent=4))

    running_game = Walker()
    running_game.start()
    try:
        yield running_game
    finally:
        running_game.release_all()
        running_game.stop()
        if SAVE_STATE_BACKUP_PATH.exists():
            shutil.move(str(SAVE_STATE_BACKUP_PATH), str(SAVE_STATE_PATH))


def test_walk_within_start_room(walker):
    """Walks a few tiles inside the start room, there and back, to prove the loop arrives."""
    walker.start_level_from_menu()
    walker.watch_state()
    walker.load_harmful_enemies()

    start = walker.state()
    print(f"start state: {start}")

    for target in ((start.x_tl + 6, start.y_tl), (start.x_tl - 3, start.y_tl), (start.x_tl, start.y_tl)):
        started_at = time.monotonic()
        walker.walk_to(target, timeout_s=15.0)
        print(f"    took {time.monotonic() - started_at:.1f}s")

    enemy_lines = [line for line in walker._lines if "player enemies" in line]
    print(f"enemy report lines seen: {len(enemy_lines)}, newest: {enemy_lines[-1][-80:] if enemy_lines else 'none'}")
    print(f"dialogue confirmations: {walker._confirmations_sent}, damage taken: {walker._damage_taken}")


def test_walk_corridor_east(walker):
    """Walks the long open stretch east of the start room, the first real piece of route."""
    walker.start_level_from_menu()
    walker.watch_state()
    walker.load_harmful_enemies()

    start = walker.state()
    print(f"start state: {start}")

    # from the collision mesh: the start room floor runs east to tile 90 before the wall at 91
    route = [(60, 110), (70, 109), (80, 109), (88, 109)]
    started_at = time.monotonic()
    walker.walk_route(route, timeout_s=25.0, what="corridor east")
    print(f"route took {time.monotonic() - started_at:.1f}s")
    print(f"dialogue confirmations: {walker._confirmations_sent}, damage taken: {walker._damage_taken}")


def test_explore_start_area(walker):
    """Lets the bot wander from the spawn and reports every tile it managed to stand on.

    This is the honest answer to "where can the player actually get": the collision mesh says one
    thing, the game with all its mechanisms says another, and only the second one counts.
    """
    walker.start_level_from_menu()
    walker.watch_state()
    walker.load_harmful_enemies()

    visited: set[tuple[int, int]] = set()
    start = walker.state()
    print(f"start state: {start}")

    directions = [WALK_KEY_RIGHT, WALK_KEY_LEFT]
    exploration_seconds = 120.0
    started_at = time.monotonic()
    direction_index = 0
    last_switch_at = time.monotonic()
    last_jump_at = 0.0

    while time.monotonic() - started_at < exploration_seconds:
        state = walker.state()
        visited.add((state.x_tl, state.y_tl))
        if walker.handle_dialogue(state):
            continue
        if state.lives < (walker._lives_at_start or state.lives):
            print("the player died while exploring")
            break

        # switch direction every few seconds, and jump constantly, which is what gets a bot up ledges
        if time.monotonic() - last_switch_at > 6.0:
            last_switch_at = time.monotonic()
            walker.release(directions[direction_index])
            direction_index = (direction_index + 1) % len(directions)
        walker.hold(directions[direction_index])

        if time.monotonic() - last_jump_at > 0.55:
            last_jump_at = time.monotonic()
            walker.hold(JUMP_KEY)
            time.sleep(JUMP_HOLD_S)
            walker.release(JUMP_KEY)

        time.sleep(STEER_INTERVAL_S)

    walker.release_all()

    xs = [tile[0] for tile in visited]
    ys = [tile[1] for tile in visited]
    print(f"visited {len(visited)} tiles, x {min(xs)}..{max(xs)}, y {min(ys)}..{max(ys)}")
    print(f"dialogue confirmations: {walker._confirmations_sent}, damage taken: {walker._damage_taken}")
    for tile_y in range(min(ys), max(ys) + 1):
        row = "".join("o" if (tile_x, tile_y) in visited else "." for tile_x in range(min(xs), max(xs) + 1))
        print(f"{tile_y:>4} {row}")


def test_climb_shaft_to_key_corridor(walker):
    """Tries to climb the shaft east of the spawn, the only opening towards the locker key corridor.

    The collision mesh shows a shaft around x 104..120 with a stack of blocks in it at y 95..97,
    connecting the spawn floor at y 104 up to the corridor at y 90 where the locker key sits. This
    reports the highest tile the bot manages to stand on while trying.
    """
    walker.start_level_from_menu()
    walker.watch_state()
    walker.load_harmful_enemies()

    walker.walk_to((100, 105), timeout_s=30.0, what="east end of the spawn floor")

    best_y_tl = walker.state().y_tl
    best_tile = None
    started_at = time.monotonic()
    last_jump_at = 0.0

    while time.monotonic() - started_at < 60.0:
        state = walker.state()
        if walker.handle_dialogue(state):
            continue
        if state.y_tl < best_y_tl:
            best_y_tl = state.y_tl
            best_tile = (state.x_tl, state.y_tl)
            print(f"  new best height: {best_tile}")

        # steer towards the shaft and keep jumping, which is all a bot can do without wall jump
        target_x_tl = 110
        if state.x_tl < target_x_tl - 1:
            walker.release(WALK_KEY_LEFT)
            walker.hold(WALK_KEY_RIGHT)
        elif state.x_tl > target_x_tl + 1:
            walker.release(WALK_KEY_RIGHT)
            walker.hold(WALK_KEY_LEFT)

        if time.monotonic() - last_jump_at > 0.55:
            last_jump_at = time.monotonic()
            walker.hold(JUMP_KEY)
            time.sleep(JUMP_HOLD_S)
            walker.release(JUMP_KEY)

        time.sleep(STEER_INTERVAL_S)

    walker.release_all()
    final = walker.state()
    print(f"highest tile reached: {best_tile}, ended at {final.x_tl},{final.y_tl}")
    print(f"the locker key sits at 53,90 - {'above' if best_y_tl > 90 else 'at or below'} the best height")


def test_cell_lever_and_shaft(walker):
    """Pulls the cell lever from a fresh save and checks whether the shaft below opens.

    lever_cell targets the three on_off_blocks down_1..down_3 that plug the floor at x 84..86, and
    the tmx marks it `handle_available: false`. If it needs the handle first, a fresh save cannot
    leave the cell at all, and that is what this reports.
    """
    walker.start_level_from_menu()
    walker.watch_state()
    walker.load_harmful_enemies()

    walker.walk_to((81, 110), timeout_s=30.0, what="the cell lever")

    since = walker.mark()
    for attempt in range(6):
        walker.press(CONFIRM_KEY)
        time.sleep(0.8)
        state = walker.state()
        if walker.handle_dialogue(state):
            continue
    print("log after pulling the lever:")
    for line in walker._lines[since:]:
        if "player position" in line or "player enemies" in line:
            continue
        print(f"    | {line}")

    # now stand over the plugged floor and see whether the player drops through
    walker.walk_to((85, 110), timeout_s=30.0, what="over the shaft")
    time.sleep(2.0)
    walker.release_all()
    state = walker.state()
    print(f"after standing over the shaft: tile {state.x_tl},{state.y_tl}")
    print("the shaft is " + ("OPEN, the player dropped through" if state.y_tl > 111 else "still plugged"))



def test_reach_the_spike_lever(walker):
    """Walks east to lever_spike_01 and tries to pull it, without being handed anything.

    The reachability report built from the collision mesh plus every standable mechanism says the
    player can get to x 44..141 at y 102..110, and that this lever at tiles 104..106, 99..100 is the
    only gate in that area besides the cell lever. Its target_ids include the four ct-crossroads
    blocks at x 105, y 104..107, which is a column right next to the reachable floor - so if the
    lever can be pulled, that column is worth trying as a way up.
    """
    walker.start_level_from_menu()
    walker.watch_state()
    walker.load_harmful_enemies()

    for target in ((100, 105), (105, 103), (105, 102)):
        try:
            walker.walk_to(target, timeout_s=30.0, what="towards the spike lever")
        except WaypointTimeout as timeout:
            print(f"  {timeout}")

    since = walker.mark()
    for attempt in range(6):
        walker.press(CONFIRM_KEY)
        time.sleep(0.9)
        state = walker.state()
        if walker.handle_dialogue(state):
            continue

    lever_lines = [line for line in walker._lines[since:] if "lever" in line.lower()]
    block_lines = [line for line in walker._lines[since:] if "OnOffBlocks" in line or "crossroads" in line]
    for line in lever_lines[:4]:
        print(f"  lever: {line.strip()[-120:]}")
    for line in block_lines[:4]:
        print(f"  blocks: {line.strip()[-120:]}")
    if not lever_lines and not block_lines:
        print("  the lever did not react, it is out of the player's reach")

    # if the column became solid, this is the way up out of the starting area
    highest = walker.state()
    for target in ((105, 100), (105, 97), (105, 94)):
        try:
            walker.walk_to(target, timeout_s=25.0, what="up the crossroads column")
            highest = walker.state()
        except WaypointTimeout as timeout:
            print(f"  {timeout}")
            break

    walker.release_all()
    print(f"highest tile after the lever: {highest.x_tl},{highest.y_tl}")
    print(f"dialogue confirmations: {walker._confirmations_sent}, damage taken: {walker._damage_taken}")


# the interactive spots the tmx places inside the starting cell: an Examine hint at the west wall,
# the cell lever, and the rope hanging above the spawn
CELL_INTERACTION_SPOTS = [
    ((46, 109), "the examine hint at the west wall"),
    ((49, 109), "under the rope above the spawn"),
    ((71, 108), "the monk sensor"),
    ((81, 110), "the cell lever"),
]


def test_examine_everything_in_the_cell(walker):
    """Walks to every interactive spot in the cell and presses everything, reporting what reacts.

    This is the last thing the cell offers that has not been tried: the Examine hint at tile 44,108,
    the rope at 49,106, the monk sensor and the lever. If none of them changes the world, the cell is
    sealed on a fresh save and the chain cannot start on foot.
    """
    walker.start_level_from_menu()
    walker.watch_state()
    walker.load_harmful_enemies()

    reactions: list[str] = []
    for target, description in CELL_INTERACTION_SPOTS:
        print(f"trying {description} at {target}")
        try:
            walker.walk_to(target, timeout_s=25.0, what=description)
        except WaypointTimeout as timeout:
            print(f"  {timeout}")
            continue

        since = walker.mark()
        for key in (CONFIRM_KEY, win32con.VK_UP, win32con.VK_DOWN, win32con.VK_CONTROL, win32con.VK_MENU):
            walker.press(key)
            time.sleep(0.7)
            state = walker.state()
            if state.dialogue:
                walker.handle_dialogue(state)

        for line in walker._lines[since:]:
            if "player position" in line or "player enemies" in line:
                continue
            if any(word in line for word in ("mechanism", "Dialogue", "dialogue", "received", "open", "lever", "Blocks")):
                reactions.append(f"{description}: {line.strip()[-110:]}")

    walker.release_all()
    print("reactions:")
    for reaction in reactions[:20]:
        print(f"  {reaction}")
    if not reactions:
        print("  nothing in the cell reacted to anything")

    state = walker.state()
    print(f"ended at tile {state.x_tl},{state.y_tl}")


# planned against layer_level_solid_not_optimised.obj, the mesh with one quad per solid tile. the
# optimised 69 face version cannot be rasterised by filling each face: its concave chains enclose
# the rooms, so filling them marks the rooms themselves as rock
ROUTE_SPAWN_TO_LOCKER_KEY = [
    (50, 110), (86, 110), (92, 109), (98, 107), (104, 107), (110, 105), (114, 102), (118, 100),
    (118, 97), (114, 94), (108, 94), (102, 92), (54, 92),
]


def test_walk_from_spawn_to_the_locker_key(walker):
    """Walks out of the cell and up to the locker key, taking it, without being handed anything.

    This is the leg that decides whether the chain can start on foot at all.
    """
    walker.start_level_from_menu()
    walker.watch_state()
    walker.load_harmful_enemies()

    started_at = time.monotonic()
    reached_waypoints = 0
    for index, waypoint in enumerate(ROUTE_SPAWN_TO_LOCKER_KEY, 1):
        try:
            walker.walk_to(waypoint, timeout_s=30.0, what=f"waypoint {index}/{len(ROUTE_SPAWN_TO_LOCKER_KEY)}")
            reached_waypoints = index
        except WaypointTimeout as timeout:
            print(f"  {timeout}")
            break

    state = walker.state()
    print(f"got {reached_waypoints}/{len(ROUTE_SPAWN_TO_LOCKER_KEY)} waypoints in {time.monotonic() - started_at:.0f}s")
    print(f"standing at tile {state.x_tl},{state.y_tl}")

    if reached_waypoints == len(ROUTE_SPAWN_TO_LOCKER_KEY):
        since = walker.mark()
        for attempt in range(8):
            walker.press(CONFIRM_KEY)
            if walker.wait_for("received item: locker_key", 1.5, since):
                print(f"  took the locker key after {attempt + 1} press(es)")
                break
        else:
            print("  standing at the key but it did not come along")

    print(f"dialogue confirmations: {walker._confirmations_sent}, damage taken: {walker._damage_taken}")
