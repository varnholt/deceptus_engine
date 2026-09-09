"""Measures what each movement pattern actually achieves, and writes it to movetable.json.

    uv run --project tests pytest tests/desktop/test_calibrate_moves.py -s

The route planner and the walk have to agree on what the player can do. Guessing "a jump reaches
three tiles" and planning at that limit produces routes that only work when every take off is
perfect. So instead each pattern is run on the flat floor of the starting cell, its trajectory is
sampled from the position the game logs, and what comes out - how high it rose, how far across it
was at the apex, where it landed - is what the planner is allowed to use.

Every pattern is a fixed sequence of held keys, so the walk can replay exactly what was measured.
"""

import json
import time
from pathlib import Path

import win32con

from test_level_playthrough import (
    JUMP_KEY,
    WALK_KEY_LEFT,
    WALK_KEY_RIGHT,
    walker,  # noqa: F401  - the fixture that starts a game with a fresh slot 0
)

MOVE_TABLE_PATH = Path(__file__).with_name("movetable.json")

# a long flat stretch of the starting cell floor, with room to run in both directions
CALIBRATION_TILE = (60, 110)

# how often the trajectory is sampled while a pattern plays out
SAMPLE_INTERVAL_S = 0.03

# a pattern is over once the player has been back on the ground for this long
LANDED_QUIET_S = 0.35
PATTERN_TIMEOUT_S = 3.0

# each pattern: how long the direction is held before the jump, how long the jump key is held, and
# how long the direction is kept afterwards. this is the whole vocabulary the walk has
MOVE_PATTERNS = [
    {"name": "hop", "run_up_s": 0.0, "jump_hold_s": 0.2, "carry_s": 0.4, "with_direction": False},
    {"name": "step_jump", "run_up_s": 0.0, "jump_hold_s": 0.2, "carry_s": 0.8, "with_direction": True},
    {"name": "run_jump", "run_up_s": 0.35, "jump_hold_s": 0.2, "carry_s": 0.9, "with_direction": True},
    {"name": "long_run_jump", "run_up_s": 0.7, "jump_hold_s": 0.2, "carry_s": 1.1, "with_direction": True},
    {"name": "low_hop", "run_up_s": 0.0, "jump_hold_s": 0.08, "carry_s": 0.5, "with_direction": True},
    {"name": "run_low_hop", "run_up_s": 0.35, "jump_hold_s": 0.08, "carry_s": 0.7, "with_direction": True},
]


def run_pattern(walker, pattern: dict, direction_key: int) -> dict:
    """Plays one pattern and returns what it achieved, in tiles, relative to the take off."""
    start = walker.state()
    samples = []

    if pattern["with_direction"] and pattern["run_up_s"] > 0.0:
        walker.hold(direction_key)
        time.sleep(pattern["run_up_s"])
    elif pattern["with_direction"]:
        walker.hold(direction_key)

    take_off = walker.state()
    walker.hold(JUMP_KEY)
    time.sleep(pattern["jump_hold_s"])
    walker.release(JUMP_KEY)

    started_at = time.monotonic()
    back_on_ground_at = None
    while time.monotonic() - started_at < PATTERN_TIMEOUT_S:
        state = walker.state()
        samples.append(state)
        if time.monotonic() - started_at > pattern["carry_s"]:
            walker.release(direction_key)
        # the player is down again once it is no longer above the take off height
        if state.y_px >= take_off.y_px - 1.0:
            if back_on_ground_at is None:
                back_on_ground_at = time.monotonic()
            elif time.monotonic() - back_on_ground_at > LANDED_QUIET_S:
                break
        else:
            back_on_ground_at = None
        time.sleep(SAMPLE_INTERVAL_S)

    walker.release_all()
    time.sleep(0.4)
    landing = walker.state()

    highest = min(samples, key=lambda sample: sample.y_px) if samples else take_off
    rise_tl = round((take_off.y_px - highest.y_px) / 24.0, 2)
    across_at_apex_tl = round(abs(highest.x_px - take_off.x_px) / 24.0, 2)
    across_at_landing_tl = round(abs(landing.x_px - take_off.x_px) / 24.0, 2)
    drop_tl = round((landing.y_px - take_off.y_px) / 24.0, 2)

    return {
        "rise_tl": rise_tl,
        "across_at_apex_tl": across_at_apex_tl,
        "across_at_landing_tl": across_at_landing_tl,
        "drop_tl": drop_tl,
        "run_up_tl": round(abs(take_off.x_px - start.x_px) / 24.0, 2),
    }


def test_calibrate_moves(walker):
    walker.start_level_from_menu()
    walker.watch_state()

    walker.walk_to(CALIBRATION_TILE, timeout_s=30.0, what="the calibration stretch")

    measured = {}
    for pattern in MOVE_PATTERNS:
        # both directions, then keep the smaller reach of the two, so a plan never counts on the
        # better side of a difference the walk cannot choose
        results = []
        for direction_key, direction_name in ((WALK_KEY_RIGHT, "right"), (WALK_KEY_LEFT, "left")):
            walker.walk_to(CALIBRATION_TILE, timeout_s=20.0, what="back to the calibration stretch")
            result = run_pattern(walker, pattern, direction_key)
            print(f"{pattern['name']:<14} {direction_name:<6} {result}")
            results.append(result)

        measured[pattern["name"]] = {
            "run_up_s": pattern["run_up_s"],
            "jump_hold_s": pattern["jump_hold_s"],
            "carry_s": pattern["carry_s"],
            "with_direction": pattern["with_direction"],
            "rise_tl": min(result["rise_tl"] for result in results),
            "across_at_apex_tl": min(result["across_at_apex_tl"] for result in results),
            "across_at_landing_tl": min(result["across_at_landing_tl"] for result in results),
        }

    MOVE_TABLE_PATH.write_text(json.dumps(measured, indent=4, sort_keys=True))
    print(f"\nwrote {MOVE_TABLE_PATH}")
    for name, entry in sorted(measured.items()):
        print(
            f"  {name:<14} rises {entry['rise_tl']:.1f} tiles, "
            f"{entry['across_at_apex_tl']:.1f} across at the apex, "
            f"{entry['across_at_landing_tl']:.1f} across on landing"
        )
