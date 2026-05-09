"""
Launches the game, waits for the "level loading finished" line on stdout,
then captures the window via ffmpeg gdigrab and saves a GIF.

Run from anywhere:
    uv run --project lab/record_gameplay pytest lab/record_gameplay -s

Paths are read from lab/record_gameplay/config.json.
"""

import json
import queue
import subprocess
import threading
import time
from pathlib import Path

import win32api
import win32con
import win32gui

CONFIG_FILE = Path(__file__).parent / "config.json"

_config = json.loads(CONFIG_FILE.read_text())
GAME_EXECUTABLE = Path(_config["game_executable"])
GAME_WORKING_DIR = Path(_config["working_directory"])
_teleport_x: int | None = _config.get("teleport_x")
_teleport_y: int | None = _config.get("teleport_y")
TELEPORT_COMMAND: str | None = f"tpp {_teleport_x} {_teleport_y}" if _teleport_x is not None and _teleport_y is not None else None

GAME_TITLE_PREFIX = "deceptus"

LEVEL_LOADED_MARKER = "level loading finished"
LOAD_TIMEOUT_SECONDS = 30

CAPTURE_FPS = 60
CAPTURE_DURATION_SECONDS: int = _config.get("capture_duration_seconds", 8)
GIF_FPS: int = _config.get("gif_fps", 25)

OUTPUT_DIR = Path(__file__).parent / "output"
OUTPUT_GIF = OUTPUT_DIR / "gameplay.gif"


def find_game_hwnd() -> int | None:
    found = []

    def visitor(hwnd, _):
        if win32gui.IsWindowVisible(hwnd) and win32gui.GetWindowText(hwnd).startswith(GAME_TITLE_PREFIX):
            found.append(hwnd)

    win32gui.EnumWindows(visitor, None)
    return found[0] if found else None


def wait_for_level_loaded(stdout_queue: queue.Queue) -> bool:
    deadline = time.monotonic() + LOAD_TIMEOUT_SECONDS
    while time.monotonic() < deadline:
        try:
            line = stdout_queue.get(timeout=0.1)
            if LEVEL_LOADED_MARKER in line:
                return True
        except queue.Empty:
            pass
    return False


def post_key(hwnd: int, vk_code: int) -> None:
    win32api.PostMessage(hwnd, win32con.WM_KEYDOWN, vk_code, 0)
    time.sleep(0.05)
    win32api.PostMessage(hwnd, win32con.WM_KEYUP, vk_code, 0)


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


def capture_to_gif(hwnd: int, duration_seconds: float, fps: int, gif_fps: int, output_path: Path) -> None:
    output_path.parent.mkdir(exist_ok=True)
    raw_video = output_path.parent / "raw_capture.mp4"

    rect = win32gui.GetWindowRect(hwnd)
    window_width = rect[2] - rect[0]
    window_height = rect[3] - rect[1]

    capture_command = [
        "ffmpeg", "-y",
        "-f", "gdigrab",
        "-framerate", str(fps),
        "-offset_x", str(rect[0]),
        "-offset_y", str(rect[1]),
        "-video_size", f"{window_width}x{window_height}",
        "-i", "desktop",
        "-t", str(duration_seconds),
        "-c:v", "libx264",
        "-preset", "ultrafast",
        "-crf", "18",
        str(raw_video),
    ]
    capture_result = subprocess.run(capture_command, capture_output=True, text=True)
    assert capture_result.returncode == 0, f"ffmpeg capture failed:\n{capture_result.stderr}"

    gif_filter = (
        f"fps={gif_fps},"
        "split[s0][s1];[s0]palettegen=max_colors=256[p];[s1][p]paletteuse=dither=bayer"
    )
    gif_command = [
        "ffmpeg", "-y",
        "-i", str(raw_video),
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
        post_key(hwnd, win32con.VK_RETURN)
        time.sleep(1.0)
        post_key(hwnd, win32con.VK_RETURN)

        loaded = wait_for_level_loaded(stdout_queue)
        assert loaded, f'"{LEVEL_LOADED_MARKER}" not seen on stdout within {LOAD_TIMEOUT_SECONDS}s'

        if TELEPORT_COMMAND:
            console_teleport(hwnd, TELEPORT_COMMAND)
            time.sleep(2.0)

        capture_to_gif(hwnd, CAPTURE_DURATION_SECONDS, CAPTURE_FPS, GIF_FPS, OUTPUT_GIF)
        assert OUTPUT_GIF.exists()
        print(f"\nGIF written to {OUTPUT_GIF}")

    finally:
        process.terminate()
        process.wait()
