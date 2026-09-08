"""Plays the catacombs and graveyard progression through the debug console and checks the log.

    uv run --project tests pytest tests/desktop/test_level_progression.py -s

The chain is the one a player walks: take the locker key, unlock the locker with it, take the
handle out of it, insert the handle into the cell lever, open the desk drawer for the solar seal,
unlock the chest with the seal, take the head torch the chest spawns, cross to the graveyard, take
the owl's eyes, come back, insert them into the shrine and take the sword that releases.

Walking between those places would make the test a platforming exercise, so the run teleports with
`tpp <x> <y>` (tile coordinates) and switches levels with `level load <name>`. Every step is
verified against a line the engine or the level script writes to stdout, so the test fails where
the chain breaks rather than at the end.

Needs a Windows desktop build with DEVELOPMENT_MODE - the console is compiled out otherwise.
`build_rel` is used unless DECEPTUS_DESKTOP_BUILD_DIR says otherwise. Input is delivered with
PostMessage, so the game does not need focus, but it does own a window while the test runs, and
slot 0 of the save state is replaced for the duration of the run and restored afterwards.
"""

import json
import os
import queue
import shutil
import subprocess
import threading
import time
from pathlib import Path

import pytest
import win32api
import win32con
import win32gui

REPO_ROOT = Path(__file__).resolve().parents[2]
BUILD_DIRECTORY = REPO_ROOT / os.environ.get("DECEPTUS_DESKTOP_BUILD_DIR", "build_rel")
GAME_EXECUTABLE = BUILD_DIRECTORY / "deceptus.exe"

SETTINGS_DIRECTORY = Path(os.environ["APPDATA"]) / "deceptus" / "settings"
SAVE_STATE_PATH = SETTINGS_DIRECTORY / "savestate.json"
SAVE_STATE_BACKUP_PATH = SETTINGS_DIRECTORY / "savestate.json.progression_backup"

GAME_TITLE_PREFIX = "deceptus"

LEVEL_LOADING_STARTED = "parsing tmx"
LEVEL_LOADED = "level loading finished"

WINDOW_TIMEOUT_S = 20
LEVEL_LOAD_TIMEOUT_S = 60
MENU_CONFIRM_ATTEMPTS = 8
MENU_CONFIRM_INTERVAL_S = 2.5

# how long a single interaction attempt is given before the key is pressed again
INTERACTION_TIMEOUT_S = 1.5
INTERACTION_ATTEMPTS = 8

# a console command is repeated when the engine did not log it, see Game.console
CONSOLE_TIMEOUT_S = 3.0
CONSOLE_ATTEMPTS = 4
# Player::keyboardKeyPressed dispatches the two inventory slots on LControl and LAlt, and SFML
# derives left from right for those two out of VK_CONTROL / VK_MENU plus the extended flag in
# lParam (WindowImplWin32::virtualKeyCodeToSF). A posted VK_LCONTROL translates to Key::Unknown,
# so the generic virtual key with a clear extended bit is what reaches the game as the left one.
SLOT_1_KEY = win32con.VK_CONTROL
SLOT_2_KEY = win32con.VK_MENU

# an extra that is collected on touch may need more than one try, see Game.collect
COLLECT_TIMEOUT_S = 2.5
COLLECT_ATTEMPTS = 5

# the player body is placed at the center of the given tile, so these are tiles the player ends up
# standing in, picked so the body overlaps the mechanism it has to reach
LOCKER_KEY_TILE = (54, 91)
LOCKER_TILE = (139, 88)  # the locker sensor, and the handle inside it once it is open
CELL_LEVER_TILE = (81, 110)
DRAWER_TILE = (291, 88)
CHEST_TILE = (145, 102)
HEAD_TORCH_TILE = (145, 99)  # the chest spawns the extra 24 right and 56 up of itself
CATACOMBS_SHRINE_TILE = (125, 128)
SWORD_TILE = (125, 122)
GRAVEYARD_SHRINE_TILE = (230, 61)


def find_game_window() -> int | None:
    found = []

    def visitor(handle, _):
        if win32gui.IsWindowVisible(handle) and win32gui.GetWindowText(handle).startswith(GAME_TITLE_PREFIX):
            found.append(handle)

    win32gui.EnumWindows(visitor, None)
    return found[0] if found else None


def level_name(needle: str) -> str:
    """Returns the levels.json entry that contains the given name, so the console can load it."""
    levels = json.loads((REPO_ROOT / "data" / "config" / "levels.json").read_text())
    matches = [entry["levelname"] for entry in levels if needle in entry["levelname"]]
    assert matches, f'no level in levels.json contains "{needle}"'
    return needle


class Game:
    """A running game, its stdout and the keys and console commands sent to it."""

    def __init__(self):
        self._process = None
        self._handle = None
        self._lines: list[str] = []
        self._incoming: queue.Queue = queue.Queue()

    def start(self) -> None:
        self._process = subprocess.Popen(
            [str(GAME_EXECUTABLE)],
            cwd=str(REPO_ROOT),
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            errors="replace",
            bufsize=1,
        )
        threading.Thread(target=self._drain_stdout, daemon=True).start()

        deadline = time.monotonic() + WINDOW_TIMEOUT_S
        while time.monotonic() < deadline and not self._handle:
            self._handle = find_game_window()
            time.sleep(0.2)
        assert self._handle, f"no game window appeared within {WINDOW_TIMEOUT_S}s"

    def stop(self) -> None:
        if self._process:
            self._process.terminate()
            self._process.wait()

    def _drain_stdout(self) -> None:
        for line in self._process.stdout:
            stripped = line.rstrip()
            self._lines.append(stripped)
            self._incoming.put(stripped)

    def mark(self) -> int:
        """Returns a position in the log, so a step only looks at what happens after it."""
        return len(self._lines)

    def wait_for(self, marker: str, timeout_s: float, since: int = 0) -> bool:
        deadline = time.monotonic() + timeout_s
        while time.monotonic() < deadline:
            if any(marker in line for line in self._lines[since:]):
                return True
            time.sleep(0.05)
        return False

    def tail(self, count: int = 30) -> str:
        """The end of the game log, for the message of a step that failed."""
        lines = [f"    | {line}" for line in self._lines[-count:]]
        return os.linesep.join(lines)

    def press(self, virtual_key: int, hold_s: float = 0.06) -> None:
        win32api.PostMessage(self._handle, win32con.WM_KEYDOWN, virtual_key, 0)
        time.sleep(hold_s)
        win32api.PostMessage(self._handle, win32con.WM_KEYUP, virtual_key, 0)

    def dismiss_dialogue(self) -> None:
        """Closes an open message box.

        MessageBox::keyboardKeyPressed swallows every key while a box is up, F12 included, so a
        dialogue that is still on screen makes the console unreachable. Enter closes it.
        """
        self.press(win32con.VK_RETURN)
        time.sleep(0.6)

    def console(self, command: str) -> None:
        """Runs a console command, and checks the engine logged it."""
        for attempt in range(CONSOLE_ATTEMPTS):
            since = self.mark()
            self.press(win32con.VK_F12)
            time.sleep(0.3)
            for character in command:
                win32api.PostMessage(self._handle, win32con.WM_CHAR, ord(character), 0)
                time.sleep(0.02)
            time.sleep(0.2)
            self.press(win32con.VK_RETURN)
            if self.wait_for(f"process command: {command}", CONSOLE_TIMEOUT_S, since):
                self.press(win32con.VK_F12)
                time.sleep(0.4)
                return
            # nothing was executed, so the keys went somewhere else: most likely an open dialogue
            self.dismiss_dialogue()
        raise AssertionError(f"the console never ran '{command}'{os.linesep}{self.tail()}")

    def start_level_from_menu(self) -> None:
        """Sends Return until the level starts loading, then waits for it to finish."""
        time.sleep(4.0)
        for attempt in range(MENU_CONFIRM_ATTEMPTS):
            since = self.mark()
            self.press(win32con.VK_RETURN)
            if self.wait_for(LEVEL_LOADING_STARTED, MENU_CONFIRM_INTERVAL_S, since):
                assert self.wait_for(LEVEL_LOADED, LEVEL_LOAD_TIMEOUT_S, since), "the level never finished loading"
                print(f"level reached after {attempt + 1} menu confirmation(s)")
                return
        raise AssertionError(f"the level did not start loading after {MENU_CONFIRM_ATTEMPTS} confirmations")

    def load_level(self, needle: str) -> None:
        since = self.mark()
        self.console(f"level load {needle}")
        assert self.wait_for(LEVEL_LOADING_STARTED, 20.0, since), f'"level load {needle}" did not start loading'
        assert self.wait_for(LEVEL_LOADED, LEVEL_LOAD_TIMEOUT_S, since), f'"{needle}" never finished loading'
        time.sleep(3.0)

    def teleport(self, tile: tuple[int, int]) -> None:
        self.console(f"tpp {tile[0]} {tile[1]}")
        # a teleport can cross a room border, and the mechanisms there only run once the transition
        # is through
        time.sleep(2.0)

    def collect(self, tile: tuple[int, int], marker: str, what: str = "") -> None:
        """Teleports onto an extra until it has been picked up.

        An extra without requires_button_press is collected on touch, so the player has to be at it
        in the frame it becomes collectible. A chest keeps the extra it spawns disabled until its
        spawn effect is through, roughly three seconds, and gravity pulls the player off the spot in
        the meantime. Teleporting again is cheaper than guessing the delay.
        """
        since = self.mark()
        for attempt in range(COLLECT_ATTEMPTS):
            self.teleport(tile)
            if self.wait_for(marker, COLLECT_TIMEOUT_S, since):
                print(f"  '{marker}' after {attempt + 1} teleport(s)")
                return
        raise AssertionError(f"{what or marker}: '{marker}' never showed up in the log{os.linesep}{self.tail()}")

    def interact(self, marker: str, virtual_key: int = win32con.VK_RETURN, what: str = "") -> None:
        """Presses a key until the log shows the step happened."""
        since = self.mark()
        for attempt in range(INTERACTION_ATTEMPTS):
            self.press(virtual_key)
            if self.wait_for(marker, INTERACTION_TIMEOUT_S, since):
                print(f'  "{marker}" after {attempt + 1} press(es)')
                return
        raise AssertionError(f"{what or marker}: '{marker}' never showed up in the log{os.linesep}{self.tail()}")

    def use_item(self, item: str, marker: str) -> None:
        """Uses an item from one of the two slots.

        Which slot an item ends up in depends on what was picked up and consumed before it, so both
        are tried and the log decides which one held the item.
        """
        since = self.mark()
        for attempt in range(INTERACTION_ATTEMPTS):
            self.press(SLOT_1_KEY)
            if self.wait_for(marker, INTERACTION_TIMEOUT_S, since):
                print(f'  "{marker}" from slot 1 after {attempt + 1} attempt(s)')
                return
            self.press(SLOT_2_KEY)
            if self.wait_for(marker, INTERACTION_TIMEOUT_S, since):
                print(f'  "{marker}" from slot 2 after {attempt + 1} attempt(s)')
                return
        raise AssertionError(f"using '{item}' never produced '{marker}'{os.linesep}{self.tail()}")


@pytest.fixture
def game():
    assert GAME_EXECUTABLE.exists(), f"{GAME_EXECUTABLE} not found, build the desktop target first"

    # a fresh slot 0: the run has to start without the key, the seal, the eyes or the treasures that
    # remember what was already inserted. levelstate must stay null, an empty object makes
    # Level::loadSaveState index a missing key
    SETTINGS_DIRECTORY.mkdir(parents=True, exist_ok=True)
    if SAVE_STATE_PATH.exists() and not SAVE_STATE_BACKUP_PATH.exists():
        shutil.copy2(SAVE_STATE_PATH, SAVE_STATE_BACKUP_PATH)
    slots = json.loads(SAVE_STATE_PATH.read_text()) if SAVE_STATE_PATH.exists() else [{}, {}, {}]
    slots[0] = {"levelindex": 0, "checkpoints": {}, "levelstate": None, "playerinfo": {}}
    SAVE_STATE_PATH.write_text(json.dumps(slots, indent=4))

    running_game = Game()
    running_game.start()
    try:
        yield running_game
    finally:
        running_game.stop()
        if SAVE_STATE_BACKUP_PATH.exists():
            shutil.move(str(SAVE_STATE_BACKUP_PATH), str(SAVE_STATE_PATH))


def test_level_progression(game):
    game.start_level_from_menu()

    print("locker key")
    game.teleport(LOCKER_KEY_TILE)
    game.interact("received item: locker_key", what="taking the locker key")

    print("locker")
    game.teleport(LOCKER_TILE)
    game.use_item("locker_key", "open locker")

    print("handle")
    game.interact("received item: handle", what="taking the handle out of the locker")

    print("cell lever")
    game.teleport(CELL_LEVER_TILE)
    game.use_item("handle", "handle inserted into lever_cell")

    print("desk drawer")
    game.teleport(DRAWER_TILE)
    game.interact("open drawer", what="opening the desk drawer")
    assert game.wait_for("received item: solar_seal", 5.0), "the drawer did not hand over the solar seal"

    print("chest")
    game.teleport(CHEST_TILE)
    game.interact("locked box opened", what="unlocking the chest with the solar seal")

    print("head torch")
    game.collect(HEAD_TORCH_TILE, "received item: headtorch", what="picking up the head torch the chest spawned")

    print("graveyard")
    game.load_level(level_name("graveyard"))
    game.teleport(GRAVEYARD_SHRINE_TILE)
    game.interact("owl eyes taken", what="taking the owl's eyes")

    print("back to the catacombs")
    game.load_level(level_name("catacombs"))
    game.teleport(CATACOMBS_SHRINE_TILE)
    game.interact("owl eyes inserted", what="inserting the owl's eyes into the shrine")

    print("sword")
    game.collect(SWORD_TILE, "sword acquired", what="picking up the sword the shrine released")
