"""Boots the desktop build straight into the harpoon test level and drives a swing.

    uv run --with pywin32 --with pillow python drive_harpoon.py [build_dir]

Gives the player the harpoon via the debug console, then fires it with the regular fire button
(slot 1 / LControl), swings while holding a direction, releases it again and captures the whole
sequence frame by frame so the rope and the momentum after release can be inspected.

See lab/map_render/drive_desktop.py for the input/capture details this reuses.
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
SAVE_STATE_BACKUP_PATH = SETTINGS_DIRECTORY / "savestate.json.drive_harpoon_backup"
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


def grab(name: str) -> None:
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

    time.sleep(3.0)

    # main menu: Continue -> file select -> confirm slot
    send_key(VK_RETURN)
    time.sleep(1.5)
    send_key(VK_RETURN)
    time.sleep(8.0)

    run_console_command("weapon add harpoon")

    grab("10_level.png")

    # scenario 1: cross the first gap. teleporting to the platform edge makes the shot reproducible,
    # the anchor then lands in the ceiling above open space instead of above the platform
    run_console_command("tpp 13, 10")
    time.sleep(1.0)
    grab("20_at_edge.png")

    hold_key(VK_RIGHT, True)
    time.sleep(0.45)
    send_key(VK_LCONTROL, hold_s=0.05, settle_s=0.0)
    capture_burst("21_swing_out", 2.5)
    capture_burst("22_swing_on", 2.0)
    send_key(VK_LCONTROL, hold_s=0.05, settle_s=0.0)
    capture_burst("23_released", 2.0)
    hold_key(VK_RIGHT, False)
    time.sleep(1.0)
    grab("24_landed.png")

    # scenario 2: straight up into the ceiling from the base floor, in the middle of the gap right
    # below the stalactite, then swing to the right
    run_console_command("tpp 17, 14")
    time.sleep(1.0)
    hold_key(VK_UP, True)
    send_key(VK_LCONTROL, hold_s=0.05, settle_s=0.0)
    hold_key(VK_UP, False)
    capture_burst("30_hooked_up", 1.5)
    hold_key(VK_RIGHT, True)
    capture_burst("31_pump_right", 2.5)
    hold_key(VK_RIGHT, False)
    capture_burst("32_swing_free", 2.5)
    send_key(VK_LCONTROL, hold_s=0.05, settle_s=0.0)
    capture_burst("33_released", 1.5)
    grab("34_after.png")

    # scenario 2b: reel the rope in with up and back out with down while hanging, sampled densely
    # so the smoothness of the length change can be measured rather than eyeballed
    run_console_command("tpp 17, 14")
    time.sleep(1.0)
    hold_key(VK_UP, True)
    send_key(VK_LCONTROL, hold_s=0.05, settle_s=0.0)
    hold_key(VK_UP, False)
    time.sleep(1.2)
    grab("35_reel_before.png")
    hold_key(VK_UP, True)
    capture_burst("36_reel_in", 1.6)
    hold_key(VK_UP, False)
    time.sleep(0.4)
    grab("37_reel_in_done.png")
    hold_key(VK_DOWN, True)
    capture_burst("38_reel_out", 2.0)
    hold_key(VK_DOWN, False)
    time.sleep(0.4)
    grab("39_reel_out_done.png")
    send_key(VK_LCONTROL, hold_s=0.05, settle_s=0.0)
    time.sleep(0.5)

    # scenario 3: the pit near the right end is the one place the base floor cannot be walked across.
    # hook the ceiling straight up from the bottom and swing out of it
    run_console_command("tpp 110, 17")
    time.sleep(1.0)
    grab("40_in_pit.png")

    hold_key(VK_UP, True)
    send_key(VK_LCONTROL, hold_s=0.05, settle_s=0.0)
    hold_key(VK_UP, False)
    capture_burst("41_pit_hooked", 1.5)
    hold_key(VK_RIGHT, True)
    capture_burst("42_pit_swing", 2.5)
    send_key(VK_LCONTROL, hold_s=0.05, settle_s=0.0)
    capture_burst("43_pit_out", 2.0)
    hold_key(VK_RIGHT, False)
    time.sleep(1.0)
    grab("44_pit_after.png")

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
