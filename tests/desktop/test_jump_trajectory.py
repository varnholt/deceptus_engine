"""Prints the raw trajectory of one jump, to settle how high the player can actually get.

    uv run --project tests pytest tests/desktop/test_jump_trajectory.py -s

Two earlier measurements disagreed by a factor of two: a crude one taken right after the level load
said 3.3 tiles, and the pattern calibration said 1.6. The difference matters more than anything else
in the route planner, because it decides whether a two tile ledge is a step or a wall. So this one
stands still on flat ground first, waits for the position to stop changing, and then prints every
sample of a single jump.
"""

import time

from test_level_playthrough import (
    JUMP_KEY,
    walker,  # noqa: F401  - the fixture that starts a game with a fresh slot 0
)

CALIBRATION_TILE = (60, 110)
SAMPLE_INTERVAL_S = 0.03


def test_jump_trajectory(walker):
    walker.start_level_from_menu()
    walker.watch_state(50)

    walker.walk_to(CALIBRATION_TILE, timeout_s=30.0, what="the flat stretch")
    walker.release_all()

    # wait until the player is really standing still, so the take off height is not a falling one
    settled = None
    for attempt in range(40):
        state = walker.state()
        if settled is not None and abs(state.y_px - settled) < 0.01:
            break
        settled = state.y_px
        time.sleep(0.1)

    take_off = walker.state()
    print(f"standing at tile {take_off.x_tl},{take_off.y_tl}, y {take_off.y_px:.1f} px")

    walker.hold(JUMP_KEY)
    time.sleep(0.2)
    walker.release(JUMP_KEY)

    samples = []
    started_at = time.monotonic()
    while time.monotonic() - started_at < 2.0:
        state = walker.state()
        samples.append((round(time.monotonic() - started_at, 2), state.y_px))
        time.sleep(SAMPLE_INTERVAL_S)

    walker.release_all()

    unique = []
    for elapsed, y_px in samples:
        if not unique or abs(unique[-1][1] - y_px) > 0.01:
            unique.append((elapsed, y_px))

    print("trajectory, as height above the take off in tiles:")
    for elapsed, y_px in unique:
        rise_tl = (take_off.y_px - y_px) / 24.0
        print(f"  {elapsed:>5.2f}s  {y_px:>8.1f} px  {rise_tl:+.2f} tiles  {'#' * max(0, int(rise_tl * 8))}")

    peak = min(y_px for _, y_px in samples)
    print(f"peak rise: {(take_off.y_px - peak) / 24.0:.2f} tiles ({take_off.y_px - peak:.1f} px)")
