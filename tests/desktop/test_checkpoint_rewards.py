"""Check that a loaded checkpoint retains uncollected progression rewards.

Run with pytest tests/desktop/test_checkpoint_rewards.py -s against a development build.
The shared game fixture isolates settings and saves in a temporary directory.
"""

import pytest

from test_level_progression import HEAD_TORCH_TILE, LOCKER_TILE, game  # noqa: F401


@pytest.mark.parametrize(
    "game",
    [{
        "playerinfo": {"name": "checkpoint regression"},
        "checkpoints": {"data/level-catacombs/level.json": 2},
        "levelstate": {
            "data/level-catacombs/catacombs.tmx": {
                "extras": {
                    "locker_key": {"active": False},
                    "handle": {"active": True},
                    "headtorch": {"active": True},
                },
                "treasure_chests": {"locked_box": {"open": True}},
            }
        },
    }],
    indirect=True,
)
def test_uncollected_checkpoint_rewards(game):
    game.start_level_from_menu()
    game.teleport(LOCKER_TILE)
    game.interact("received item: handle", what="taking the saved unlocked locker's handle")
    game.collect(HEAD_TORCH_TILE, "received item: headtorch", what="taking the saved open chest's head torch")
