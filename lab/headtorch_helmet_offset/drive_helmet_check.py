"""Drives the desktop build with the head torch and the sword equipped and grabs the player.

    uv run --with pywin32 --with pillow python drive_helmet_check.py [build_dir]

Each capture is cropped around the middle of the window and upscaled, because the player is only
24x48 view pixels and a helmet that sits two pixels off is invisible in a full frame grab.

Console commands are typed as real key events, so a mistimed F12 leaks the letters into the global
hotkeys - 'm' in "item" starts the frame recorder, which dumps a numbered bmp per frame into the
repo root. Those get cleaned up at the end of the run.
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

ctypes.windll.shcore.SetProcessDpiAwareness(2)

REPO_ROOT = Path(__file__).resolve().parents[2]
OUTPUT_DIRECTORY = Path(__file__).resolve().parent / "out"
SETTINGS_DIRECTORY = Path(os.environ["APPDATA"]) / "deceptus" / "settings"
SAVE_STATE_PATH = SETTINGS_DIRECTORY / "savestate.json"
SAVE_STATE_BACKUP_PATH = SETTINGS_DIRECTORY / "savestate.json.helmet_check_backup"
GAME_SETTINGS_PATH = SETTINGS_DIRECTORY / "game.json"
GAME_SETTINGS_BACKUP_PATH = SETTINGS_DIRECTORY / "game.json.helmet_check_backup"

WM_CHAR = 0x0102

VK_RETURN = 0x0D
VK_SPACE = 0x20
VK_F12 = 0x7B
VK_LEFT = 0x25
VK_RIGHT = 0x27
VK_DOWN = 0x28
VK_LCONTROL = 0xA2
VK_LMENU = 0xA4
VK_X = 0x58
VK_Z = 0x5A
VK_R = 0x52

CROP_WIDTH = 420
CROP_HEIGHT = 220
CROP_SCALE = 3

# set from the command line: --only <substring> restricts which strips are captured, --full-frames
# saves the whole window instead of a crop, for when the crop is what is hiding something
ONLY_FILTER = ""
FULL_FRAMES = False

# where the player ends up on screen once teleported to checkpoint 1. The room clamp keeps the
# camera off centre there, so the crop is offset to match rather than sitting in the middle
CROP_CENTER_OFFSET_X = -160
CROP_CENTER_OFFSET_Y = 100


def install_clean_save_state():
    if SAVE_STATE_PATH.exists() and not SAVE_STATE_BACKUP_PATH.exists():
        shutil.copy2(SAVE_STATE_PATH, SAVE_STATE_BACKUP_PATH)
        print(f"backed up save state to {SAVE_STATE_BACKUP_PATH.name}")

    slots = json.loads(SAVE_STATE_PATH.read_text()) if SAVE_STATE_PATH.exists() else [{}, {}, {}]
    slots[0] = {
        "levelindex": 0,
        "checkpoints": {},
        "levelstate": None,
        "playerinfo": slots[0].get("playerinfo", {}) if slots else {},
    }
    SAVE_STATE_PATH.write_text(json.dumps(slots, indent=4))
    print("installed clean save state in slot 0")


def restore_save_state():
    if SAVE_STATE_BACKUP_PATH.exists():
        shutil.move(str(SAVE_STATE_BACKUP_PATH), str(SAVE_STATE_PATH))
        print("restored original save state")


def install_windowed_mode():
    """PrintWindow only ever returns black frames while the game runs fullscreen."""
    if not GAME_SETTINGS_PATH.exists():
        return
    if not GAME_SETTINGS_BACKUP_PATH.exists():
        shutil.copy2(GAME_SETTINGS_PATH, GAME_SETTINGS_BACKUP_PATH)
    settings = json.loads(GAME_SETTINGS_PATH.read_text())
    settings["GameConfiguration"]["fullscreen"] = False
    GAME_SETTINGS_PATH.write_text(json.dumps(settings, indent=3))
    print("switched the game to windowed mode for the run")


def restore_game_settings():
    if GAME_SETTINGS_BACKUP_PATH.exists():
        shutil.move(str(GAME_SETTINGS_BACKUP_PATH), str(GAME_SETTINGS_PATH))
        print("restored original game settings")


def find_window():
    result = []

    def callback(handle, _):
        title = win32gui.GetWindowText(handle).lower()
        if win32gui.IsWindowVisible(handle) and title.startswith("deceptus -") and "fps" in title:
            result.append(handle)
        return True

    win32gui.EnumWindows(callback, None)
    return result[0] if result else None


def focus_window():
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


def send_key(virtual_key, hold_s=0.12, settle_s=0.25):
    if not focus_window():
        return
    scan_code = ctypes.windll.user32.MapVirtualKeyW(virtual_key, 0)
    win32api.keybd_event(virtual_key, scan_code, 0, 0)
    time.sleep(hold_s)
    win32api.keybd_event(virtual_key, scan_code, win32con.KEYEVENTF_KEYUP, 0)
    time.sleep(settle_s)


def hold_key(virtual_key, down):
    if not focus_window():
        return
    scan_code = ctypes.windll.user32.MapVirtualKeyW(virtual_key, 0)
    win32api.keybd_event(virtual_key, scan_code, 0 if down else win32con.KEYEVENTF_KEYUP, 0)


def send_text(text):
    handle = focus_window()
    if not handle:
        return
    for character in text:
        virtual_key_and_shift = ctypes.windll.user32.VkKeyScanA(ord(character))
        virtual_key = virtual_key_and_shift & 0xFF
        needs_shift = bool(virtual_key_and_shift & 0x100)
        scan_code = ctypes.windll.user32.MapVirtualKeyW(virtual_key, 0)
        if needs_shift:
            win32api.keybd_event(win32con.VK_SHIFT, 0, 0, 0)
        win32api.keybd_event(virtual_key, scan_code, 0, 0)
        time.sleep(0.02)
        win32api.keybd_event(virtual_key, scan_code, win32con.KEYEVENTF_KEYUP, 0)
        if needs_shift:
            win32api.keybd_event(win32con.VK_SHIFT, 0, win32con.KEYEVENTF_KEYUP, 0)
        time.sleep(0.03)


def capture():
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
    ctypes.windll.user32.PrintWindow(handle, memory_dc.GetSafeHdc(), 3)

    bitmap_info = bitmap.GetInfo()
    bitmap_bits = bitmap.GetBitmapBits(True)
    image = Image.frombuffer("RGB", (bitmap_info["bmWidth"], bitmap_info["bmHeight"]), bitmap_bits, "raw", "BGRX", 0, 1)

    memory_dc.DeleteDC()
    source_dc.DeleteDC()
    win32gui.ReleaseDC(handle, window_dc)
    return image


def crop_player(image):
    center_x = image.width // 2 + CROP_CENTER_OFFSET_X
    center_y = image.height // 2 + CROP_CENTER_OFFSET_Y
    box = (
        max(0, center_x - CROP_WIDTH // 2),
        max(0, center_y - CROP_HEIGHT // 2),
        min(image.width, center_x + CROP_WIDTH // 2),
        min(image.height, center_y + CROP_HEIGHT // 2),
    )
    crop = image.crop(box)
    return crop.resize((crop.width * CROP_SCALE, crop.height * CROP_SCALE), Image.NEAREST)


def grab_strip(name, frame_count, interval_s):
    """Grabs a burst of frames and lays them out side by side."""
    if ONLY_FILTER and ONLY_FILTER not in name:
        return

    frames = []
    for _ in range(frame_count):
        image = capture()
        if image is not None:
            frames.append(image if FULL_FRAMES else crop_player(image))
        time.sleep(interval_s)

    if not frames:
        print(f"no frames for {name}")
        return

    strip = Image.new("RGB", (sum(frame.width + 4 for frame in frames), frames[0].height), (12, 12, 16))
    offset_x = 0
    for frame in frames:
        strip.paste(frame, (offset_x, 0))
        offset_x += frame.width + 4

    OUTPUT_DIRECTORY.mkdir(exist_ok=True)
    strip.save(OUTPUT_DIRECTORY / f"{name}.png")
    print(f"saved {name}.png ({len(frames)} frames)")


def run_console_command(command):
    send_key(VK_F12)
    time.sleep(0.4)
    send_text(command)
    time.sleep(0.3)
    send_key(VK_RETURN)
    time.sleep(0.8)
    send_key(VK_F12)
    time.sleep(0.8)


def run():
    positional = [argument for argument in sys.argv[1:] if not argument.startswith("--")]
    if ONLY_FILTER in positional:
        positional.remove(ONLY_FILTER)
    build_directory = positional[0] if positional else "build_rel"
    executable = REPO_ROOT / build_directory / "deceptus.exe"
    if not executable.exists():
        print(f"{executable} not found")
        return 1

    OUTPUT_DIRECTORY.mkdir(exist_ok=True)
    log_path = OUTPUT_DIRECTORY / "game.log"
    log_file = log_path.open("w", encoding="utf-8", errors="replace")
    process = subprocess.Popen([str(executable)], cwd=str(REPO_ROOT), stdout=log_file, stderr=subprocess.STDOUT)
    print(f"started {executable} (pid {process.pid})")

    handle = None
    for _ in range(60):
        time.sleep(1.0)
        handle = find_window()
        if handle:
            break
    if not handle:
        process.terminate()
        print("game window not found")
        return 1

    time.sleep(3.0)
    send_key(VK_RETURN)
    time.sleep(1.5)
    send_key(VK_RETURN)
    time.sleep(9.0)

    # slot 0 takes the head torch, slot 1 the sword, so the attack button is the slot 2 key
    # the fresh save state starts in the intro level, so the catacombs are loaded explicitly and a
    # checkpoint is used to get onto flat ground with room to run
    for command in ["level load 0"]:
        run_console_command(command)
    time.sleep(6.0)

    # the save state already fills both inventory slots, so they are cleared before the head torch
    # and the sword go in - the attack button only fires while the sword sits in a slot
    for command in ["iddqd", "extra all", "weapon add sword", "item clear", "item add headtorch", "item add sword", "tpc 1"]:
        run_console_command(command)

    def reset_position():
        """Teleports back to the checkpoint so every capture starts from the same spot."""
        run_console_command("tpc 1")
        time.sleep(2.0)

    def face(direction_key):
        send_key(direction_key, hold_s=0.15, settle_s=0.4)

    def sweep(prefix):
        """Captures every cycle reachable from flat ground, in both directions."""
        for direction_name, direction_key in (("right", VK_RIGHT), ("left", VK_LEFT)):
            reset_position()
            face(direction_key)
            grab_strip(f"{prefix}_00_idle_{direction_name}", 4, 0.2)

            reset_position()
            face(direction_key)
            hold_key(direction_key, True)
            time.sleep(0.3)
            grab_strip(f"{prefix}_01_run_{direction_name}", 6, 0.06)
            hold_key(direction_key, False)

            reset_position()
            face(direction_key)
            hold_key(VK_DOWN, True)
            grab_strip(f"{prefix}_02_bend_down_{direction_name}", 8, 0.06)
            hold_key(VK_DOWN, False)
            grab_strip(f"{prefix}_03_bend_up_{direction_name}", 6, 0.06)

            reset_position()
            face(direction_key)
            hold_key(VK_SPACE, True)
            grab_strip(f"{prefix}_04_jump_{direction_name}", 12, 0.05)
            hold_key(VK_SPACE, False)

            reset_position()
            face(direction_key)
            send_key(VK_SPACE, hold_s=0.2, settle_s=0.15)
            send_key(VK_SPACE, hold_s=0.2, settle_s=0.0)
            grab_strip(f"{prefix}_05_double_jump_{direction_name}", 10, 0.05)

            reset_position()
            face(direction_key)
            send_key(VK_Z if direction_key == VK_LEFT else VK_X, hold_s=0.08, settle_s=0.0)
            grab_strip(f"{prefix}_06_dash_{direction_name}", 6, 0.05)

            reset_position()
            face(direction_key)
            hold_key(VK_DOWN, True)
            time.sleep(0.6)
            hold_key(VK_LMENU, True)
            grab_strip(f"{prefix}_07_crouch_attack_{direction_name}", 10, 0.045)
            hold_key(VK_LMENU, False)
            hold_key(VK_DOWN, False)

            reset_position()
            face(direction_key)
            hold_key(VK_LMENU, True)
            grab_strip(f"{prefix}_08_standing_attack_{direction_name}", 12, 0.045)
            hold_key(VK_LMENU, False)

            reset_position()
            face(direction_key)
            hold_key(VK_SPACE, True)
            time.sleep(0.05)
            hold_key(VK_LMENU, True)
            grab_strip(f"{prefix}_09_jump_attack_{direction_name}", 12, 0.045)
            hold_key(VK_LMENU, False)
            hold_key(VK_SPACE, False)

    sweep("sword")

    # the appear cycle only plays on a respawn, and it is one of the two non sword cycles that does
    # not use the default origin
    run_console_command("weapon clear")
    send_key(VK_R, hold_s=0.12, settle_s=0.2)
    grab_strip("plain_10_appear", 14, 0.08)

    sweep("plain")

    process.terminate()
    return 0


def remove_recorded_frames():
    """Drops what leaked keypresses dumped into the repo root: numbered bmps from the frame recorder
    that 'm' toggles, and the render target set that 's' writes."""
    dumped = sorted(REPO_ROOT.glob("[0-9][0-9][0-9][0-9][0-9].bmp"))
    dumped += sorted(REPO_ROOT.glob("texture_map_*.png"))
    dumped += sorted(REPO_ROOT.glob("texture_atmosphere_*.png"))
    for path in dumped:
        path.unlink()
    if dumped:
        print(f"removed {len(dumped)} leaked dumps from the repo root")


def main():
    global ONLY_FILTER, FULL_FRAMES
    if "--only" in sys.argv:
        ONLY_FILTER = sys.argv[sys.argv.index("--only") + 1]
    FULL_FRAMES = "--full-frames" in sys.argv

    install_clean_save_state()
    install_windowed_mode()
    try:
        return run()
    finally:
        restore_save_state()
        restore_game_settings()
        remove_recorded_frames()


if __name__ == "__main__":
    sys.exit(main())
