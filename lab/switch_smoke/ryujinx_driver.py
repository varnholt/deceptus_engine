"""Drives the Switch build inside Ryujinx: launch, controller input, window capture.

Extracted from drive_ryujinx.py so the pytest suite can reuse it. Nothing here is
Deceptus-specific beyond the default paths.

Two details are load-bearing and were both learned the hard way:

- Input goes through real keybd_event rather than posted window messages, because the
  emulator reads key state rather than its message queue.
- Capture goes through PrintWindow with PW_RENDERFULLCONTENT, because a plain screen grab of
  a hardware-accelerated surface comes back blank or cropped by whatever overlaps it.

Ryujinx maps the keyboard to Player 1 as a Pro Controller. Mind the layout: Nintendo puts A
where a standard pad puts B, so the button the game treats as confirm is SDL's positional A,
which is Nintendo's B, which Ryujinx binds to the X key.
"""

import ctypes
import subprocess
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

# The emulator draws its own chrome around the guest surface: a menu bar on top and a status
# bar at the bottom carrying a frame counter that changes every capture. Both have to come off
# before two frames can be compared, or the frame counter alone fails every comparison.
CHROME_TOP_PX = 40
CHROME_BOTTOM_PX = 30


def find_window() -> int | None:
    result: list[int] = []

    def callback(handle: int, _) -> bool:
        if win32gui.IsWindowVisible(handle) and "Ryujinx" in win32gui.GetWindowText(handle):
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

    return win32gui.GetForegroundWindow() == handle and handle or handle


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
    win32api.keybd_event(virtual_key, scan_code, 0 if down else win32con.KEYEVENTF_KEYUP, 0)


def capture() -> Image.Image | None:
    """Grabs the emulator window, chrome included. Use capture_guest_surface() to compare."""
    handle = find_window()
    if not handle:
        return None

    left, top, right, bottom = win32gui.GetClientRect(handle)
    width, height = right - left, bottom - top

    window_dc = win32gui.GetWindowDC(handle)
    source_dc = win32ui.CreateDCFromHandle(window_dc)
    memory_dc = source_dc.CreateCompatibleDC()

    bitmap = win32ui.CreateBitmap()
    bitmap.CreateCompatibleBitmap(source_dc, width, height)
    memory_dc.SelectObject(bitmap)

    # 3 = PW_RENDERFULLCONTENT, needed for hardware accelerated windows
    ctypes.windll.user32.PrintWindow(handle, memory_dc.GetSafeHdc(), 3)

    info = bitmap.GetInfo()
    image = Image.frombuffer("RGB", (info["bmWidth"], info["bmHeight"]), bitmap.GetBitmapBits(True), "raw", "BGRX", 0, 1)

    memory_dc.DeleteDC()
    source_dc.DeleteDC()
    win32gui.ReleaseDC(handle, window_dc)

    return image


def capture_guest_surface() -> Image.Image | None:
    """The game's own output, with the emulator's menu and status bars cropped away."""
    image = capture()
    if image is None:
        return None

    width, height = image.size
    return image.crop((0, CHROME_TOP_PX, width, height - CHROME_BOTTOM_PX))


class RyujinxSession:
    """Launches the emulator for the duration of a with-block and kills it afterwards."""

    def __init__(self, nro_path: Path = NRO_PATH, log_path: Path | None = None):
        self._nro_path = nro_path
        self._log_path = log_path
        self._process: subprocess.Popen | None = None
        self._log_file = None

    def __enter__(self) -> "RyujinxSession":
        if not RYUJINX_PATH.exists():
            raise FileNotFoundError(f"emulator not found at {RYUJINX_PATH}")
        if not self._nro_path.exists():
            raise FileNotFoundError(f"nro not found at {self._nro_path} - build it first")

        stdout = subprocess.DEVNULL
        if self._log_path is not None:
            self._log_path.parent.mkdir(parents=True, exist_ok=True)
            self._log_file = self._log_path.open("w", encoding="utf-8", errors="replace")
            stdout = self._log_file

        self._process = subprocess.Popen(
            [str(RYUJINX_PATH), str(self._nro_path)],
            stdout=stdout,
            stderr=subprocess.STDOUT,
        )
        return self

    def __exit__(self, *_) -> None:
        if self._process is not None:
            self._process.kill()
            self._process.wait(timeout=10)
        if self._log_file is not None:
            self._log_file.close()

    def wait_for_window(self, timeout_s: float = 90.0) -> bool:
        """Waits for the guest to actually be drawing, not merely for a window to exist.

        The emulator window appears long before the game does, so this waits for the captured
        surface to stop being uniformly dark.
        """
        deadline = time.time() + timeout_s
        while time.time() < deadline:
            surface = capture_guest_surface()
            if surface is not None:
                thumbnail = surface.resize((32, 18))
                pixels = list(thumbnail.getdata())
                brightest = max(sum(pixel) for pixel in pixels)
                if brightest > 90:
                    return True
            time.sleep(2.0)
        return False
