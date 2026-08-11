"""Drives the harpoon and a grab rope against each other in the harpoon test level.

    uv run --with pywin32 --with pillow python drive_two_owners.py [build_dir]

Both features own up and down while they carry the player, and both read those keys past the claim.
This checks that only one of them can ever be the owner:

  * hanging on a rope, holding fire must not start an aim - no indicator, and up still climbs
  * aiming, jumping into a rope must not grab it
  * letting go of the rope frees the keys again, so the harpoon can aim afterwards

Reuses the input and capture plumbing of drive_harpoon.py.
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
OUTPUT_DIRECTORY = Path(__file__).resolve().parent / "out_two_owners"
SETTINGS_DIRECTORY = Path(os.environ["APPDATA"]) / "deceptus" / "settings"
SAVE_STATE_PATH = SETTINGS_DIRECTORY / "savestate.json"
SAVE_STATE_BACKUP_PATH = SETTINGS_DIRECTORY / "savestate.json.drive_grab_rope_backup"
GAME_CONFIG_PATH = SETTINGS_DIRECTORY / "game.json"

_restore_fullscreen: bool | None = None

# index of data/level-harpoon_test/level.json in data/config/levels.json
HARPOON_TEST_LEVEL_INDEX = 3

VK_RETURN = 0x0D
VK_SPACE = 0x20
VK_LEFT = 0x25
VK_UP = 0x26
VK_RIGHT = 0x27
VK_DOWN = 0x28
VK_F12 = 0x7B
VK_LCONTROL = 0xA2  # slot 1, the fire button the harpoon is fired with

WM_CHAR = 0x0102


def install_save_state() -> None:
    """Points slot 0 at the harpoon test level so the run does not resume somewhere else."""
    if SAVE_STATE_PATH.exists() and not SAVE_STATE_BACKUP_PATH.exists():
        shutil.copy2(SAVE_STATE_PATH, SAVE_STATE_BACKUP_PATH)
        print(f"backed up save state to {SAVE_STATE_BACKUP_PATH.name}")

    slots = json.loads(SAVE_STATE_PATH.read_text()) if SAVE_STATE_PATH.exists() else [{}, {}, {}]

    # levelstate must stay null, an empty object makes Level::loadSaveState index a missing key
    slots[0] = {
        "levelindex": HARPOON_TEST_LEVEL_INDEX,
        "checkpoints": {},
        "levelstate": None,
        "playerinfo": slots[0].get("playerinfo", {}) if slots else {},
    }
    SAVE_STATE_PATH.write_text(json.dumps(slots, indent=4))
    print(f"pointed save slot 0 at level index {HARPOON_TEST_LEVEL_INDEX}")


def force_windowed_mode() -> None:
    """PrintWindow only ever returns a blank frame while the game runs fullscreen."""
    global _restore_fullscreen

    if not GAME_CONFIG_PATH.exists():
        return

    config = json.loads(GAME_CONFIG_PATH.read_text())
    if not config["GameConfiguration"].get("fullscreen"):
        return

    _restore_fullscreen = True
    config["GameConfiguration"]["fullscreen"] = False
    GAME_CONFIG_PATH.write_text(json.dumps(config, indent=4))
    print("temporarily switched the game to windowed mode so captures are not blank")


def restore_fullscreen_mode() -> None:
    if _restore_fullscreen is None:
        return

    config = json.loads(GAME_CONFIG_PATH.read_text())
    config["GameConfiguration"]["fullscreen"] = _restore_fullscreen
    GAME_CONFIG_PATH.write_text(json.dumps(config, indent=4))
    print("restored the fullscreen setting")


def restore_save_state() -> None:
    if SAVE_STATE_BACKUP_PATH.exists():
        shutil.move(str(SAVE_STATE_BACKUP_PATH), str(SAVE_STATE_PATH))
        print("restored original save state")


def find_window() -> int | None:
    result: list[int] = []

    def callback(handle: int, _) -> bool:
        title = win32gui.GetWindowText(handle).lower()
        if win32gui.IsWindowVisible(handle) and title.startswith("deceptus -") and "fps" in title:
            result.append(handle)
        return True

    win32gui.EnumWindows(callback, None)
    return result[0] if result else None


def focus_window() -> int | None:
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

    return handle


def send_key(virtual_key: int, hold_s: float = 0.08, settle_s: float = 0.15) -> None:
    if not focus_window():
        return

    scan_code = ctypes.windll.user32.MapVirtualKeyW(virtual_key, 0)
    win32api.keybd_event(virtual_key, scan_code, 0, 0)
    time.sleep(hold_s)
    win32api.keybd_event(virtual_key, scan_code, win32con.KEYEVENTF_KEYUP, 0)
    time.sleep(settle_s)


def hold_key(virtual_key: int, down: bool) -> None:
    if not focus_window():
        return

    scan_code = ctypes.windll.user32.MapVirtualKeyW(virtual_key, 0)
    flags = 0 if down else win32con.KEYEVENTF_KEYUP
    win32api.keybd_event(virtual_key, scan_code, flags, 0)


def send_text(text: str) -> None:
    handle = focus_window()
    if not handle:
        return
    for character in text:
        win32gui.PostMessage(handle, WM_CHAR, ord(character), 0)
        time.sleep(0.02)


def run_console_command(command: str) -> None:
    """Opens the debug console with F12, types a command, executes it and closes the console."""
    send_key(VK_F12)
    time.sleep(0.4)
    send_text(command)
    time.sleep(0.3)
    send_key(VK_RETURN)
    time.sleep(1.0)
    send_key(VK_F12)
    time.sleep(1.0)


def capture() -> Image.Image | None:
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


def grab_frame(name: str) -> None:
    image = capture()
    if image is None:
        print(f"no game window while grabbing {name}")
        return

    OUTPUT_DIRECTORY.mkdir(exist_ok=True)
    image.save(OUTPUT_DIRECTORY / name)
    print(f"saved {name}")


def capture_burst(prefix: str, duration_s: float) -> None:
    frames = []
    start = time.perf_counter()
    while time.perf_counter() - start < duration_s:
        frames.append((time.perf_counter() - start, capture()))

    OUTPUT_DIRECTORY.mkdir(exist_ok=True)
    for index, (elapsed, frame) in enumerate(frames):
        if frame is not None:
            frame.save(OUTPUT_DIRECTORY / f"{prefix}_{index:02d}.png")
    print(f"{prefix}: {len(frames)} frames over {frames[-1][0]:.2f}s")


def run() -> int:
    build_directory = sys.argv[1] if len(sys.argv) > 1 else "build/Release"
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

    time.sleep(3.0)

    # main menu: Continue -> file select -> confirm slot
    send_key(VK_RETURN)
    time.sleep(1.5)
    send_key(VK_RETURN)
    time.sleep(8.0)

    run_console_command("weapon add harpoon")

    # scenario 1: hang on the rope, then hold fire. the harpoon must stay out of it entirely
    run_console_command("tpp 17, 14")
    time.sleep(1.5)
    hold_key(VK_RIGHT, True)
    send_key(VK_SPACE, hold_s=0.25, settle_s=0.0)
    capture_burst("10_jump_into_rope", 2.0)
    hold_key(VK_RIGHT, False)
    time.sleep(0.6)
    grab_frame("11_hanging.png")

    hold_key(VK_LCONTROL, True)
    time.sleep(0.8)
    grab_frame("20_fire_held_while_hanging.png")

    # up must still climb rather than sweep an aim angle
    hold_key(VK_UP, True)
    capture_burst("21_climb_while_fire_held", 1.6)
    hold_key(VK_UP, False)
    time.sleep(0.4)
    grab_frame("22_climbed_with_fire_held.png")
    hold_key(VK_LCONTROL, False)
    time.sleep(0.5)

    # scenario 2: let go, then aim. the keys have to be free again
    send_key(VK_SPACE, hold_s=0.1, settle_s=0.0)
    time.sleep(1.5)
    run_console_command("tpp 30, 10")
    time.sleep(1.2)
    hold_key(VK_LCONTROL, True)
    time.sleep(0.6)
    grab_frame("30_aiming_after_release.png")
    hold_key(VK_DOWN, True)
    time.sleep(1.0)
    grab_frame("31_aim_swept_down.png")
    hold_key(VK_DOWN, False)
    hold_key(VK_LCONTROL, False)
    time.sleep(1.0)

    print("--- game log tail ---")
    log_file.flush()
    log_text = log_path.read_text(encoding="utf-8", errors="replace")
    print("\n".join(log_text.splitlines()[-25:]))

    process.terminate()
    return 0


def main() -> int:
    install_save_state()
    force_windowed_mode()
    try:
        return run()
    finally:
        subprocess.run(["taskkill", "/F", "/IM", "deceptus.exe"], capture_output=True)
        restore_save_state()
        restore_fullscreen_mode()


if __name__ == "__main__":
    raise SystemExit(main())
