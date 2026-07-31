"""Launches the desktop build, walks through the menus, opens the map page and grabs the window.

    uv run --with pywin32 --with pillow python drive_desktop.py [build_dir]

The game is event based, so PostMessage reaches it without focus. The window content is grabbed
with PrintWindow, which also works while the window is partially covered.
"""

import ctypes
import json
import os
import shutil
import subprocess
import sys
import time
from pathlib import Path

import win32api
import win32con
import win32gui
import win32ui
from PIL import Image

# without this the GDI capture is DPI virtualized and only returns the top left crop of the window
ctypes.windll.shcore.SetProcessDpiAwareness(2)

REPO_ROOT = Path(__file__).resolve().parents[2]
OUTPUT_DIRECTORY = Path(__file__).resolve().parent / "out"
SETTINGS_DIRECTORY = Path(os.environ["APPDATA"]) / "deceptus" / "settings"
SAVE_STATE_PATH = SETTINGS_DIRECTORY / "savestate.json"
SAVE_STATE_BACKUP_PATH = SETTINGS_DIRECTORY / "savestate.json.drive_desktop_backup"

WM_KEYDOWN = 0x0100
WM_KEYUP = 0x0101
WM_CHAR = 0x0102

VK_RETURN = 0x0D
VK_TAB = 0x09
VK_F12 = 0x7B
VK_LSHIFT = 0xA0  # rotates to the previous submenu while the ingame menu is open
VK_LEFT = 0x25
VK_UP = 0x26
VK_RIGHT = 0x27
VK_DOWN = 0x28
VK_A = 0x41  # zooms the map in while the ingame menu is open
VK_S = 0x53  # zooms the map out while the ingame menu is open


def install_clean_save_state() -> None:
    """Points slot 0 at the catacombs level start so the run does not resume in the graveyard."""
    if SAVE_STATE_PATH.exists() and not SAVE_STATE_BACKUP_PATH.exists():
        shutil.copy2(SAVE_STATE_PATH, SAVE_STATE_BACKUP_PATH)
        print(f"backed up save state to {SAVE_STATE_BACKUP_PATH.name}")

    slots = json.loads(SAVE_STATE_PATH.read_text()) if SAVE_STATE_PATH.exists() else [{}, {}, {}]

    # levelstate must stay null, an empty object makes Level::loadSaveState index a missing key
    slots[0] = {
        "levelindex": 0,
        "checkpoints": {},
        "levelstate": None,
        "playerinfo": slots[0].get("playerinfo", {}) if slots else {},
    }
    SAVE_STATE_PATH.write_text(json.dumps(slots, indent=4))
    print("installed clean catacombs save state in slot 0")


def restore_save_state() -> None:
    if SAVE_STATE_BACKUP_PATH.exists():
        shutil.move(str(SAVE_STATE_BACKUP_PATH), str(SAVE_STATE_PATH))
        print("restored original save state")


def find_window() -> int | None:
    """Finds the game window. The title looks like 'deceptus - 61fps [Release]'.

    Matching on 'deceptus' alone also hits the IDE, so the fps part is required.
    """
    result: list[int] = []

    def callback(handle: int, _) -> bool:
        title = win32gui.GetWindowText(handle).lower()
        if win32gui.IsWindowVisible(handle) and title.startswith("deceptus -") and "fps" in title:
            result.append(handle)
        return True

    win32gui.EnumWindows(callback, None)
    return result[0] if result else None


def focus_window() -> int | None:
    """Brings the game to the foreground.

    Posted messages are not enough: the game clears its key state on FocusLost, and the inventory
    opens from that key state rather than from the event itself.
    """
    handle = find_window()
    if not handle:
        return None

    if win32gui.GetForegroundWindow() != handle:
        win32gui.ShowWindow(handle, win32con.SW_RESTORE)
        try:
            win32gui.SetForegroundWindow(handle)
        except Exception:
            pass
        time.sleep(0.3)

    return handle


def send_key(_stale_handle: int, virtual_key: int, hold_s: float = 0.12, settle_s: float = 0.25) -> None:
    if not focus_window():
        return

    scan_code = ctypes.windll.user32.MapVirtualKeyW(virtual_key, 0)
    win32api.keybd_event(virtual_key, scan_code, 0, 0)
    time.sleep(hold_s)
    win32api.keybd_event(virtual_key, scan_code, win32con.KEYEVENTF_KEYUP, 0)
    time.sleep(settle_s)


def send_text(_stale_handle: int, text: str) -> None:
    handle = focus_window()
    if not handle:
        return
    for character in text:
        win32gui.PostMessage(handle, WM_CHAR, ord(character), 0)
        time.sleep(0.02)


def capture() -> Image.Image | None:
    # the game recreates its window (resolution changes), so the handle is looked up every time
    handle = find_window()
    if not handle:
        return None

    left, top, right, bottom = win32gui.GetClientRect(handle)
    width = right - left
    height = bottom - top

    window_dc = win32gui.GetWindowDC(handle)
    source_dc = win32ui.CreateDCFromHandle(window_dc)
    memory_dc = source_dc.CreateCompatibleDC()

    bitmap = win32ui.CreateBitmap()
    bitmap.CreateCompatibleBitmap(source_dc, width, height)
    memory_dc.SelectObject(bitmap)

    # 3 = PW_RENDERFULLCONTENT, needed for hardware accelerated windows
    ctypes.windll.user32.PrintWindow(handle, memory_dc.GetSafeHdc(), 3)

    bitmap_info = bitmap.GetInfo()
    bitmap_bits = bitmap.GetBitmapBits(True)
    image = Image.frombuffer("RGB", (bitmap_info["bmWidth"], bitmap_info["bmHeight"]), bitmap_bits, "raw", "BGRX", 0, 1)

    memory_dc.DeleteDC()
    source_dc.DeleteDC()
    win32gui.ReleaseDC(handle, window_dc)

    return image


def grab_window(_stale_handle: int, path: Path) -> None:
    image = capture()
    if image is None:
        print(f"no game window while grabbing {path.name}")
        return

    OUTPUT_DIRECTORY.mkdir(exist_ok=True)
    image.save(path)
    print(f"saved {path}")


def is_ingame_menu_open() -> bool:
    """Detects the menu by the magenta pill behind the selected submenu tab.

    The band starts below the hud health bar, whose red pixels would otherwise match as well.
    """
    image = capture()
    if image is None:
        return False

    header = image.crop((0, 30, image.width, 80)).getdata()
    pink_pixels = sum(1 for red, green, blue in header if red > 150 and green < 80 and 70 < blue < 170)
    return pink_pixels > 500


def run() -> int:
    build_directory = sys.argv[1] if len(sys.argv) > 1 else "build_rel"
    executable = REPO_ROOT / build_directory / "deceptus.exe"

    if not executable.exists():
        print(f"{executable} not found")
        return 1

    OUTPUT_DIRECTORY.mkdir(exist_ok=True)
    log_path = OUTPUT_DIRECTORY / "game.log"
    log_file = log_path.open("w", encoding="utf-8", errors="replace")
    process = subprocess.Popen([str(executable)], cwd=str(REPO_ROOT), stdout=log_file, stderr=subprocess.STDOUT)
    print(f"started {executable} (pid {process.pid}), log: {log_path}")

    handle = None
    for _ in range(60):
        time.sleep(1.0)
        handle = find_window()
        if handle:
            break

    if not handle:
        print("game window not found")
        process.terminate()
        return 1

    print(f"window handle {handle}")
    time.sleep(3.0)

    # main menu: Continue -> file select -> confirm slot
    send_key(handle, VK_RETURN)
    time.sleep(1.5)
    send_key(handle, VK_RETURN)
    time.sleep(8.0)

    grab_window(handle, OUTPUT_DIRECTORY / "20_ingame.png")

    # teleport through the checkpoints so more than the spawn room is discovered, ending inside a
    # room so the map has something to show around the player
    for command in ["tpc 1", "tpc 2", "tpc 5", "tpc 4", "tpc 3"]:
        send_key(handle, VK_F12)
        time.sleep(0.4)
        send_text(handle, command)
        time.sleep(0.3)
        send_key(handle, VK_RETURN)
        time.sleep(1.5)
        send_key(handle, VK_F12)
        time.sleep(1.5)

    grab_window(handle, OUTPUT_DIRECTORY / "21_after_teleports.png")

    # let room transitions and respawns settle, the menu refuses to open during a screen transition
    time.sleep(4.0)

    # tab opens the inventory, left shift rotates to the map page
    for attempt in range(6):
        if is_ingame_menu_open():
            break
        send_key(handle, VK_TAB)
        time.sleep(2.0)
    else:
        print("could not open the ingame menu")

    grab_window(handle, OUTPUT_DIRECTORY / "22_inventory.png")

    # capture the submenu transition as fast as possible, the slide takes only 0.5s
    send_key(handle, VK_LSHIFT, hold_s=0.03, settle_s=0.0)
    frames = []
    start = time.perf_counter()
    while time.perf_counter() - start < 0.6:
        frames.append((time.perf_counter() - start, capture()))
    for index, (elapsed, frame) in enumerate(frames):
        if frame is not None:
            frame.save(OUTPUT_DIRECTORY / f"23_map_fade_{index}.png")
    print(f"captured {len(frames)} transition frames, last at {frames[-1][0]:.2f}s")

    time.sleep(2.0)
    grab_window(handle, OUTPUT_DIRECTORY / "23_map.png")

    # zoom all the way in with a, then step back out with s
    for _ in range(4):
        send_key(handle, VK_A)
    time.sleep(0.8)
    grab_window(handle, OUTPUT_DIRECTORY / "24_map_zoom_0.png")

    for index in range(1, 4):
        send_key(handle, VK_S)
        time.sleep(0.8)
        grab_window(handle, OUTPUT_DIRECTORY / f"24_map_zoom_{index}.png")

    # pan the view with the navigate keys
    for _ in range(6):
        send_key(handle, VK_LEFT)
    grab_window(handle, OUTPUT_DIRECTORY / "25_map_panned_left.png")

    for _ in range(6):
        send_key(handle, VK_DOWN)
    grab_window(handle, OUTPUT_DIRECTORY / "26_map_panned_down.png")

    print("visited rooms in save state:", read_visited_sub_rooms())

    process.terminate()
    return 0


def read_visited_sub_rooms() -> list[str]:
    """Reads back what the running game persisted, checkpoints trigger a save."""
    if not SAVE_STATE_PATH.exists():
        return []

    slots = json.loads(SAVE_STATE_PATH.read_text())
    level_state = slots[0].get("levelstate") or {}
    for level_data in level_state.values():
        if "__visited_rooms" in level_data:
            return level_data["__visited_rooms"]
    return []


def main() -> int:
    install_clean_save_state()
    try:
        return run()
    finally:
        subprocess.run(["taskkill", "/F", "/IM", "deceptus.exe"], capture_output=True)
        restore_save_state()


if __name__ == "__main__":
    raise SystemExit(main())
