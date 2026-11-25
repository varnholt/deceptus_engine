"""In-game frames of a level: loads it, teleports to each spot and keeps the last recorded frame there.

Drives the game through the engine's desktop test harness (tests/desktop/test_level_progression.py),
so run it inside that project's environment:
  uv run --project <engine>/tests python -m pixelart.capture <out_dir> <level> <x,y> [<x,y> ...]
GAME_DATA_ROOT points the game at the checkout whose data/ holds the new art, GAME_EXE at the
build to run (default: the harness's build_rel).

The dev build keeps its save in AppData/<profile>/settings/savestate.json; it is backed up before
the run and put back afterwards, so captures never leave a checkpoint in the player's slot.
"""
import glob
import json
import os
import shutil
import sys
import time
from pathlib import Path

# lab/pixelart/pixelart/capture.py: the repository root is three folders up
ENGINE = Path(os.environ.get("ENGINE_ROOT", Path(__file__).resolve().parents[3]))
sys.path.insert(0, str(ENGINE / "tests" / "desktop"))

import test_level_progression as tlp  # noqa: E402
from test_level_progression import Game, level_name  # noqa: E402

PROFILE = os.environ.get("SAVE_PROFILE", "deceptus-dev")
STARTUP_TIMEOUT_S = 90
SHUTDOWN_TIMEOUT_S = 10


def close_game(game):
    """End the game whatever state it is in: ask it to stop, then kill it if it is still there."""
    process = game._process
    if process is None:
        return
    if process.poll() is None:
        process.terminate()
        try:
            process.wait(SHUTDOWN_TIMEOUT_S)
        except Exception:
            process.kill()
            process.wait()


def recorded_frames(root):
    return set(glob.glob(str(root / "[0-9][0-9][0-9][0-9][0-9].bmp")))


def capture(out_dir, level, spots):
    import win32gui
    from PIL import Image

    if os.environ.get("GAME_DATA_ROOT"):
        tlp.REPO_ROOT = Path(os.environ["GAME_DATA_ROOT"])
    if os.environ.get("GAME_EXE"):
        tlp.GAME_EXECUTABLE = Path(os.environ["GAME_EXE"])
    root = tlp.REPO_ROOT
    os.makedirs(out_dir, exist_ok=True)
    save = Path(os.environ["APPDATA"]) / PROFILE / "settings" / "savestate.json"
    backup = save.with_name("savestate.json.capture_backup")
    if save.exists() and not backup.exists():
        shutil.copy2(save, backup)
    slots = json.loads(save.read_text()) if save.exists() else [{}, {}, {}]
    slots[0] = {"levelindex": 0, "checkpoints": {}, "levelstate": None, "playerinfo": {}}
    save.parent.mkdir(parents=True, exist_ok=True)
    save.write_text(json.dumps(slots, indent=4))

    game = Game()
    try:
        game.start()
        # the game only reads keys once its main loop runs, which the fps counter in the title shows
        deadline = time.monotonic() + STARTUP_TIMEOUT_S
        while time.monotonic() < deadline and "fps" not in win32gui.GetWindowText(game._handle):
            time.sleep(0.5)
        time.sleep(3)
        game.load_level(level_name(level))
        for x, y in spots:
            game.teleport((x, y))
            time.sleep(2.5)
            before = recorded_frames(root)
            game.press(ord("M"))
            time.sleep(0.5)
            game.press(ord("M"))
            time.sleep(2.5)
            new = sorted(recorded_frames(root) - before)
            print(f"spot {x},{y}: {len(new)} frames")
            if new:
                Image.open(new[-1]).save(f"{out_dir}/spot_{x}_{y}.png")
            for f in new:
                os.remove(f)
    except Exception:
        print(game.tail(60))
        raise
    finally:
        close_game(game)
        if backup.exists():
            shutil.move(str(backup), str(save))
        for f in recorded_frames(root):
            os.remove(f)


if __name__ == "__main__":
    capture(sys.argv[1], sys.argv[2], [tuple(int(v) for v in s.split(",")) for s in sys.argv[3:]])
