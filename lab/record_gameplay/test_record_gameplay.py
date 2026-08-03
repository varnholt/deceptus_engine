"""
Launches the game, waits for the "level loading finished" line on stdout,
then captures the window via ffmpeg gdigrab and saves a lossless master plus a GIF.

Run from anywhere:
    uv run --project lab/record_gameplay pytest lab/record_gameplay -s

Paths are read from lab/record_gameplay/config.json.
"""

import ctypes
import json
import queue
import subprocess
import threading
import time
from pathlib import Path

import win32api
import win32con
import win32gui


def enable_dpi_awareness() -> None:
    """Ask Windows for real pixel coordinates from GetWindowRect.

    A DPI unaware process gets virtualised coordinates: on a 125% display a 1296 pixel wide
    window is reported as 1038. gdigrab captures physical desktop pixels, so feeding it
    virtualised coordinates records the wrong region of the screen entirely.
    """
    try:
        ctypes.windll.shcore.SetProcessDpiAwareness(2)  # PROCESS_PER_MONITOR_DPI_AWARE
    except (AttributeError, OSError):
        ctypes.windll.user32.SetProcessDPIAware()


enable_dpi_awareness()

CONFIG_FILE = Path(__file__).parent / "config.json"

_config = json.loads(CONFIG_FILE.read_text())
GAME_EXECUTABLE = Path(_config["game_executable"])
GAME_WORKING_DIR = Path(_config["working_directory"])
_teleport_x: int | None = _config.get("teleport_x")
_teleport_y: int | None = _config.get("teleport_y")
TELEPORT_COMMAND: str | None = f"tpp {_teleport_x} {_teleport_y}" if _teleport_x is not None and _teleport_y is not None else None

GAME_TITLE_PREFIX = "deceptus"

LEVEL_LOADED_MARKER = "level loading finished"
LEVEL_LOADING_STARTED_MARKER = "parsing tmx"
LOAD_TIMEOUT_SECONDS = 30

# How many times Return is sent to walk from the title screen into the level. The number of
# screens in between changes whenever the menu changes, so keep pressing until the level starts
# loading rather than assuming a fixed count.
MENU_CONFIRM_ATTEMPTS = 8
MENU_CONFIRM_INTERVAL_SECONDS = 2.5

CAPTURE_FPS = 60
CAPTURE_DURATION_SECONDS: int = _config.get("capture_duration_seconds", 8)
GIF_FPS: int = _config.get("gif_fps", 25)

OUTPUT_DIR = Path(__file__).parent / "output"
OUTPUT_GIF = OUTPUT_DIR / "gameplay.gif"

# The capture is kept as a lossless RGB master. Anything derived from it (the GIF here, the
# README media in lab/media_assets) is a re-encode of untouched game output, so no
# quantisation or dither noise is baked in upstream of it.
OUTPUT_MASTER = OUTPUT_DIR / "master.mkv"


def find_game_hwnd() -> int | None:
    found = []

    def visitor(hwnd, _):
        if win32gui.IsWindowVisible(hwnd) and win32gui.GetWindowText(hwnd).startswith(GAME_TITLE_PREFIX):
            found.append(hwnd)

    win32gui.EnumWindows(visitor, None)
    return found[0] if found else None


def wait_for_marker(stdout_queue: queue.Queue, marker: str, timeout_seconds: float) -> bool:
    deadline = time.monotonic() + timeout_seconds
    while time.monotonic() < deadline:
        try:
            line = stdout_queue.get(timeout=0.1)
            if marker in line:
                return True
        except queue.Empty:
            pass
    return False


def post_key(hwnd: int, vk_code: int) -> None:
    win32api.PostMessage(hwnd, win32con.WM_KEYDOWN, vk_code, 0)
    time.sleep(0.05)
    win32api.PostMessage(hwnd, win32con.WM_KEYUP, vk_code, 0)


def start_level_from_menu(hwnd: int, stdout_queue: queue.Queue) -> bool:
    """Send Return until the level starts loading, then wait for it to finish."""
    for attempt in range(MENU_CONFIRM_ATTEMPTS):
        post_key(hwnd, win32con.VK_RETURN)
        if wait_for_marker(stdout_queue, LEVEL_LOADING_STARTED_MARKER, MENU_CONFIRM_INTERVAL_SECONDS):
            print(f"level started loading after {attempt + 1} confirmation(s)")
            return wait_for_marker(stdout_queue, LEVEL_LOADED_MARKER, LOAD_TIMEOUT_SECONDS)
    return False


def console_teleport(hwnd: int, command: str) -> None:
    post_key(hwnd, win32con.VK_F12)
    time.sleep(0.15)
    for character in command:
        win32api.PostMessage(hwnd, win32con.WM_CHAR, ord(character), 0)
        time.sleep(0.02)
    time.sleep(0.1)
    post_key(hwnd, win32con.VK_RETURN)
    time.sleep(0.15)
    post_key(hwnd, win32con.VK_F12)
    time.sleep(0.3)


def set_window_topmost(hwnd: int, topmost: bool) -> None:
    """Lift the game window above everything else for the duration of the capture.

    Windows refuses SetForegroundWindow to a process that does not own the foreground, so a
    covered game window cannot be raised that way. SetWindowPos to HWND_TOPMOST needs no such
    right, and keyboard focus is not required here because input is delivered with PostMessage.
    """
    win32gui.SetWindowPos(
        hwnd,
        win32con.HWND_TOPMOST if topmost else win32con.HWND_NOTOPMOST,
        0,
        0,
        0,
        0,
        win32con.SWP_NOMOVE | win32con.SWP_NOSIZE | win32con.SWP_NOACTIVATE,
    )
    time.sleep(0.5)


def focus_window(hwnd: int) -> None:
    win32gui.ShowWindow(hwnd, win32con.SW_RESTORE)
    set_window_topmost(hwnd, True)


def get_client_rect_on_screen(hwnd: int) -> tuple[int, int, int, int]:
    """Return the client area in screen coordinates as (left, top, right, bottom).

    Capturing the client area instead of the whole window keeps the title bar and border out of
    the recording, so the master needs no cropping downstream and no hardcoded border sizes that
    change with the Windows theme or the display scaling.
    """
    _, _, client_width, client_height = win32gui.GetClientRect(hwnd)
    left, top = win32gui.ClientToScreen(hwnd, (0, 0))
    return left, top, left + client_width, top + client_height


def assert_window_is_unobscured(hwnd: int, rect: tuple[int, int, int, int]) -> None:
    """Fail loudly when something else covers the capture region.

    gdigrab records a screen rectangle, not a window, so whatever sits on top ends up in the
    recording. Without this check a covered game window yields a capture of unrelated windows.
    """
    centre = ((rect[0] + rect[2]) // 2, (rect[1] + rect[3]) // 2)
    window_at_centre = win32gui.GetAncestor(win32gui.WindowFromPoint(centre), win32con.GA_ROOT)
    assert window_at_centre == hwnd, (
        f"the game window is not on top at {centre}: found window "
        f'"{win32gui.GetWindowText(window_at_centre)}" instead. Keep the game window visible '
        f"and unobscured during recording."
    )


def capture_to_gif(hwnd: int, duration_seconds: float, fps: int, gif_fps: int, output_path: Path) -> None:
    output_path.parent.mkdir(exist_ok=True)

    focus_window(hwnd)
    rect = get_client_rect_on_screen(hwnd)
    assert_window_is_unobscured(hwnd, rect)
    window_width = rect[2] - rect[0]
    window_height = rect[3] - rect[1]
    print(f"capturing client area {window_width}x{window_height} at {rect[0]},{rect[1]}")

    # libx264rgb with -qp 0 is mathematically lossless and stays in RGB, so the capture keeps the
    # exact pixels the engine drew. Plain libx264 would convert to yuv420p and throw away three
    # quarters of the chroma, which is very visible on 1 pixel wide UI details.
    capture_command = [
        "ffmpeg", "-y",
        "-f", "gdigrab",
        "-framerate", str(fps),
        "-offset_x", str(rect[0]),
        "-offset_y", str(rect[1]),
        "-video_size", f"{window_width}x{window_height}",
        "-i", "desktop",
        "-t", str(duration_seconds),
        "-c:v", "libx264rgb",
        "-preset", "ultrafast",
        "-qp", "0",
        str(OUTPUT_MASTER),
    ]
    capture_result = subprocess.run(capture_command, capture_output=True, text=True)
    set_window_topmost(hwnd, False)
    assert capture_result.returncode == 0, f"ffmpeg capture failed:\n{capture_result.stderr}"

    # dither=none keeps the GIF free of ordered dither noise. Dither hides banding but it changes
    # every pixel of every frame, which defeats GIF compression and any downstream re-encode.
    gif_filter = (
        f"fps={gif_fps},"
        "split[s0][s1];[s0]palettegen=max_colors=256[p];[s1][p]paletteuse=dither=none"
    )
    gif_command = [
        "ffmpeg", "-y",
        "-i", str(OUTPUT_MASTER),
        "-vf", gif_filter,
        "-loop", "0",
        str(output_path),
    ]
    gif_result = subprocess.run(gif_command, capture_output=True, text=True)
    assert gif_result.returncode == 0, f"ffmpeg gif failed:\n{gif_result.stderr}"


def test_record_gameplay():
    stdout_queue: queue.Queue = queue.Queue()

    process = subprocess.Popen(
        [str(GAME_EXECUTABLE)],
        cwd=str(GAME_WORKING_DIR),
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1,
    )

    def drain_stdout():
        for line in process.stdout:
            stdout_queue.put(line.rstrip())

    threading.Thread(target=drain_stdout, daemon=True).start()

    try:
        hwnd = None
        deadline = time.monotonic() + 10
        while time.monotonic() < deadline:
            hwnd = find_game_hwnd()
            if hwnd:
                break
            time.sleep(0.1)
        assert hwnd is not None, "game window did not appear within 10 seconds"

        time.sleep(5.0)
        loaded = start_level_from_menu(hwnd, stdout_queue)
        assert loaded, (
            f'"{LEVEL_LOADED_MARKER}" not seen on stdout after {MENU_CONFIRM_ATTEMPTS} '
            f"menu confirmations"
        )

        if TELEPORT_COMMAND:
            console_teleport(hwnd, TELEPORT_COMMAND)
            time.sleep(2.0)

        capture_to_gif(hwnd, CAPTURE_DURATION_SECONDS, CAPTURE_FPS, GIF_FPS, OUTPUT_GIF)
        assert OUTPUT_MASTER.exists()
        assert OUTPUT_GIF.exists()
        print(f"\nlossless master written to {OUTPUT_MASTER}")
        print(f"GIF written to {OUTPUT_GIF}")

    finally:
        process.terminate()
        process.wait()
