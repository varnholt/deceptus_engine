"""Drives the Switch build inside Ryujinx: launches it, sends controller input and captures
the window at each step.

run_ryujinx.ps1 only launches and screenshots once, which is enough to see whether the game
starts but not whether it is playable. This drives it the way the desktop harness in
lab/map_render/drive_desktop.py drives the desktop build, and the capture and focus code is
lifted from there for the same reasons: real keyboard events rather than posted messages,
because the emulator reads key state rather than window messages, and PrintWindow with
PW_RENDERFULLCONTENT, because the emulator's surface is hardware accelerated and a plain
screen grab of an obscured window comes back blank.

Ryujinx maps the keyboard to Player 1 as a Pro Controller. Note that the Nintendo layout
puts A where a standard pad puts B: the button the game treats as confirm is SDL's
positional A, which is Nintendo's B, which Ryujinx binds to the X key.

    uv run --with pywin32 --with pillow python lab/switch_smoke/drive_ryujinx.py
"""

import ctypes
import subprocess
import sys
import time
from pathlib import Path

import win32api
import win32con
import win32gui
import win32ui
from PIL import Image

# without this the GDI capture is DPI virtualized and only returns the top left crop
ctypes.windll.shcore.SetProcessDpiAwareness(2)

REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
OUTPUT_DIRECTORY = Path(__file__).resolve().parent / "out"
RYUJINX_PATH = Path(r"D:\games\ryujinx-1.3.2-win_x64\publish\Ryujinx.exe")
NRO_PATH = REPOSITORY_ROOT / "build_switch_engine" / "deceptus.nro"

# Ryujinx' default keyboard bindings for Player 1
VK_X = 0x58  # Nintendo B  -> SDL button A, the game's confirm
VK_Z = 0x5A  # Nintendo A  -> SDL button B, the game's cancel
VK_C = 0x43  # Nintendo X
VK_V = 0x56  # Nintendo Y
VK_D = 0x44  # left stick right
VK_A = 0x41  # left stick left
VK_W = 0x57  # left stick up
VK_S = 0x53  # left stick down
VK_UP = 0x26
VK_DOWN = 0x28
VK_LEFT = 0x25
VK_RIGHT = 0x27


def find_window() -> int | None:
    """Finds the emulator window. Its title carries the running application's name."""
    result: list[int] = []

    def callback(handle: int, _) -> bool:
        title = win32gui.GetWindowText(handle)
        if win32gui.IsWindowVisible(handle) and "Ryujinx" in title:
            result.append(handle)
        return True

    win32gui.EnumWindows(callback, None)
    return result[0] if result else None


def focus_window() -> int | None:
    """Brings the emulator to the foreground and verifies it got there.

    Windows refuses plain SetForegroundWindow calls from a background process, so the input
    queues are attached first.
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
        print("warning: could not bring the emulator window to the foreground")

    return handle


def send_key(virtual_key: int, hold_s: float = 0.12, settle_s: float = 0.4) -> None:
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
        print(f"no emulator window while grabbing {name}")
        return

    OUTPUT_DIRECTORY.mkdir(exist_ok=True)
    path = OUTPUT_DIRECTORY / f"{name}.png"
    image.save(path)
    print(f"saved {path}")


def main() -> int:
    if not RYUJINX_PATH.exists():
        print(f"emulator not found at {RYUJINX_PATH}")
        return 1

    if not NRO_PATH.exists():
        print(f"nro not found at {NRO_PATH} - run build_switch.bat . build_switch_engine first")
        return 1

    guest_log_path = OUTPUT_DIRECTORY / "ryujinx_stdout.txt"
    OUTPUT_DIRECTORY.mkdir(exist_ok=True)

    print(f"launching {RYUJINX_PATH.name} with {NRO_PATH.name}")
    with guest_log_path.open("w", encoding="utf-8", errors="replace") as log_file:
        process = subprocess.Popen(
            [str(RYUJINX_PATH), str(NRO_PATH)],
            stdout=log_file,
            stderr=subprocess.STDOUT,
        )

        try:
            # the emulator needs a while to boot the guest, and the guest then loads 100 MB of romfs
            print("waiting for the main menu")
            time.sleep(45)
            grab("switch_01_menu")

            # "Continue" on the main menu opens File Select
            print("pressing confirm on the main menu")
            send_key(VK_X, settle_s=2.0)
            grab("switch_02_file_select")

            # confirm again on the highlighted save slot to actually load it
            print("pressing confirm on the save slot")
            send_key(VK_X, settle_s=2.0)
            grab("switch_03_after_load_confirm")

            print("waiting for the level to load")
            time.sleep(45)
            grab("switch_04_level")

            # walking proves the level is simulating rather than just drawn once
            print("walking right")
            hold_key(VK_D, True)
            time.sleep(2.0)
            hold_key(VK_D, False)
            time.sleep(0.5)
            grab("switch_05_walked")

            print("jumping")
            send_key(VK_X, settle_s=0.3)
            grab("switch_06_jumped")

            time.sleep(3)
            grab("switch_07_settled")
        finally:
            process.kill()
            process.wait(timeout=10)

    guest_lines = [
        line.split("OutputDebugString: ", 1)[-1].rstrip()
        for line in guest_log_path.read_text(encoding="utf-8", errors="replace").splitlines()
        if "OutputDebugString" in line
    ]

    print(f"\n--- guest log ({len(guest_lines)} lines, 'egl: make current' filtered) ---")
    for line in guest_lines:
        if "egl: make current" in line:
            continue
        print(line)

    return 0


if __name__ == "__main__":
    sys.exit(main())
