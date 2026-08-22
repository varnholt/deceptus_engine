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
    """Brings the game to the foreground and verifies it got there.

    Posted messages are not enough: the game clears its key state on FocusLost, and the inventory
    opens from that key state rather than from the event itself. Windows also refuses plain
    SetForegroundWindow calls from a background process, so the input queues are attached first.
    """
    handle = find_window()
    if not handle:
        return None

    for _ in range(5):
        if win32gui.GetForegroundWindow() == handle:
            return handle

        win32gui.ShowWindow(handle, win32con.SW_RESTORE)

        foreground_thread = ctypes.windll.user32.GetWindowThreadProcessId(win32gui.GetForegroundWindow(), None)
        own_thread = ctypes.windll.kernel32.GetCurrentThreadId()
        attached = ctypes.windll.user32.AttachThreadInput(foreground_thread, own_thread, True)
        try:
            win32gui.BringWindowToTop(handle)
            win32gui.SetForegroundWindow(handle)
        except Exception:
            pass
        finally:
            if attached:
                ctypes.windll.user32.AttachThreadInput(foreground_thread, own_thread, False)

        time.sleep(0.3)

    if win32gui.GetForegroundWindow() != handle:
        print("warning: could not bring the game window to the foreground")

    return handle


def send_key(_stale_handle: int, virtual_key: int, hold_s: float = 0.12, settle_s: float = 0.25) -> None:
    if not focus_window():
        return

    scan_code = ctypes.windll.user32.MapVirtualKeyW(virtual_key, 0)
    win32api.keybd_event(virtual_key, scan_code, 0, 0)
    time.sleep(hold_s)
    win32api.keybd_event(virtual_key, scan_code, win32con.KEYEVENTF_KEYUP, 0)
    time.sleep(settle_s)


def hold_key(_stale_handle: int, virtual_key: int, down: bool) -> None:
    """Presses or releases a key without waiting, for testing held-down behaviour."""
    if not focus_window():
        return

    scan_code = ctypes.windll.user32.MapVirtualKeyW(virtual_key, 0)
    flags = 0 if down else win32con.KEYEVENTF_KEYUP
    win32api.keybd_event(virtual_key, scan_code, flags, 0)


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


def _selected_tab_center_x() -> float | None:
    """Locates the magenta pill behind the selected submenu tab in the header.

    The band starts below the hud health bar, whose red pixels would otherwise match as well.
    """
    image = capture()
    if image is None:
        return None

    band = image.crop((0, 30, image.width, 80))
    columns = []
    for index, (red, green, blue) in enumerate(band.getdata()):
        if red > 150 and green < 80 and 70 < blue < 170:
            columns.append(index % band.width)

    if len(columns) < 500:
        return None

    return sum(columns) / len(columns)


def is_ingame_menu_open() -> bool:
    return _selected_tab_center_x() is not None


def selected_submenu() -> str | None:
    """Returns 'map', 'inventory' or 'archives' based on which tab is highlighted."""
    center_x = _selected_tab_center_x()
    if center_x is None:
        return None
    if center_x < 560:
        return "map"
    if center_x < 720:
        return "inventory"
    return "archives"


def go_to_map_page(handle: int) -> bool:
    """Rotates the submenus until the map page is selected."""
    for _ in range(4):
        current = selected_submenu()
        if current == "map":
            return True
        if current is None:
            return False
        send_key(handle, VK_LSHIFT)
        time.sleep(2.0)
    return False


def run_console_command(handle: int, command: str) -> None:
    """Opens the debug console with F12, types a command, executes it and closes the console."""
    send_key(handle, VK_F12)
    time.sleep(0.4)
    send_text(handle, command)
    time.sleep(0.3)
    send_key(handle, VK_RETURN)
    time.sleep(1.5)
    send_key(handle, VK_F12)
    time.sleep(1.5)


def open_map_page(handle: int) -> bool:
    """Opens the ingame menu if needed and rotates to the map page."""
    for _ in range(6):
        if is_ingame_menu_open():
            break
        send_key(handle, VK_TAB)
        time.sleep(2.0)
    else:
        print("could not open the ingame menu")
        return False

    return go_to_map_page(handle)


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
    # room so the map has something to show around the player. checkpoint 3 is kept back: touching
    # a checkpoint for the first time is what writes the save state, so it is used later to check
    # that the reveal flag persists.
    for command in ["tpc 1", "tpc 2", "tpc 5", "tpc 4"]:
        run_console_command(handle, command)

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
    go_to_map_page(handle)
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

    # zoom back in, at the coarsest level the whole level fits the viewport and panning is
    # correctly clamped to centered, which would make the pan measurement below meaningless
    for _ in range(3):
        send_key(handle, VK_A)
    time.sleep(0.8)

    # hold left and sample the view while it accelerates, then keep sampling after release so the
    # deceleration shows up as well
    hold_key(handle, VK_LEFT, True)
    frames = []
    start = time.perf_counter()
    released = False
    while time.perf_counter() - start < 1.6:
        elapsed = time.perf_counter() - start
        if not released and elapsed > 0.9:
            hold_key(handle, VK_LEFT, False)
            released = True
        frames.append((elapsed, capture()))
    if not released:
        hold_key(handle, VK_LEFT, False)

    for index, (elapsed, frame) in enumerate(frames):
        if frame is not None:
            frame.save(OUTPUT_DIRECTORY / f"25_pan_{index:02d}.png")
    print(f"captured {len(frames)} pan frames over {frames[-1][0]:.2f}s")

    grab_window(handle, OUTPUT_DIRECTORY / "26_map_panned.png")

    # close the menu, reveal the whole map from the console, then touch the checkpoint that was
    # kept back so the save state is written while the map is revealed
    send_key(handle, VK_TAB)
    time.sleep(1.5)
    run_console_command(handle, "map reveal")
    run_console_command(handle, "tpc 3")

    if not open_map_page(handle):
        print("could not get back to the map page")
    grab_window(handle, OUTPUT_DIRECTORY / "27_map_revealed.png")

    for _ in range(2):
        send_key(handle, VK_S)
        time.sleep(0.8)
    grab_window(handle, OUTPUT_DIRECTORY / "28_map_revealed_zoomed_out.png")

    visited, revealed = read_map_state()
    print("visited rooms in save state:", visited)
    print("map revealed in save state:", revealed)

    # map clear resets the whole thing back to unexplored
    send_key(handle, VK_TAB)
    time.sleep(1.5)
    run_console_command(handle, "map clear")

    if not open_map_page(handle):
        print("could not get back to the map page")
    grab_window(handle, OUTPUT_DIRECTORY / "29_map_cleared.png")

    process.terminate()
    return 0


def read_map_state() -> tuple[list[str], bool | None]:
    """Reads back what the running game persisted, checkpoints trigger a save."""
    if not SAVE_STATE_PATH.exists():
        return [], None

    slots = json.loads(SAVE_STATE_PATH.read_text())
    level_state = slots[0].get("levelstate") or {}
    for level_data in level_state.values():
        if "__visited_rooms" in level_data:
            return level_data["__visited_rooms"], level_data.get("__map_revealed")
    return [], None


def main() -> int:
    install_clean_save_state()
    try:
        return run()
    finally:
        subprocess.run(["taskkill", "/F", "/IM", "deceptus.exe"], capture_output=True)
        restore_save_state()


if __name__ == "__main__":
    raise SystemExit(main())
