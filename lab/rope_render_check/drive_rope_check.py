"""Visits every rope in the catacombs and grabs the frame, so a sprite sheet change can be checked.

    uv run --with pywin32 --with pillow python drive_rope_check.py [build_dir]

The rope positions are read straight out of catacombs.tmx, so the run covers whatever is in the
level rather than a hand picked sample. Each rope is teleported to by tile coordinate and captured,
and the crops are laid out as one contact sheet at the end.

Input has to be real key events. PostMessage is ignored by this build entirely - posted keys do not
even move the main menu selection - so the game is pulled to the foreground for the run.

Every step is verified rather than timed, because all the failure modes here are silent:
  - 'tpp' needs the space after the comma or its handler drops the command without a word.
  - while the console is open the level is paused, so a capture taken then is a frozen frame.
  - if the console is NOT open, the letters land on the global hotkeys instead, where the two p's
    of 'tpp' toggle the pause menu.
  - the camera pans to a teleport instead of snapping to it, so an immediate grab photographs the
    room the player just left.
The verification reads the game's own log rather than the screen: 'pwatch' logs the player tile
only while the level updates, which makes it both a console state probe and a teleport check.
"""

import ctypes
import json
import os
import re
import shutil
import subprocess
import sys
import time
from pathlib import Path

import win32api
import win32con
import win32gui
import win32ui
from PIL import Image, ImageDraw

# without this the GDI capture is DPI virtualized and only returns the top left crop of the window
ctypes.windll.shcore.SetProcessDpiAwareness(2)

REPO_ROOT = Path(__file__).resolve().parents[2]
OUTPUT_DIRECTORY = Path(__file__).resolve().parent / "out"
CATACOMBS_TMX_PATH = REPO_ROOT / "data" / "level-catacombs" / "catacombs.tmx"
SETTINGS_DIRECTORY = Path(os.environ["APPDATA"]) / "deceptus" / "settings"
SAVE_STATE_PATH = SETTINGS_DIRECTORY / "savestate.json"
SAVE_STATE_BACKUP_PATH = SETTINGS_DIRECTORY / "savestate.json.rope_check_backup"
GAME_SETTINGS_PATH = SETTINGS_DIRECTORY / "game.json"
GAME_SETTINGS_BACKUP_PATH = SETTINGS_DIRECTORY / "game.json.rope_check_backup"

VK_RETURN = 0x0D
VK_BACK = 0x08
VK_F12 = 0x7B

TILE_SIZE_PX = 24

CROP_WIDTH = 360
CROP_HEIGHT = 300
CROP_SCALE = 2

# the console draws its category headers in saturated green. counting those pixels is only ever used
# as a second opinion alongside the paused level, never on its own: the catacombs have green lit
# rooms of their own, so a clean frame there counts as high as an open console does. the count is
# sampled, so the threshold is a sampled one, and an open console lands around 70
CONSOLE_GREEN_THRESHOLD = 12


def read_rope_objects():
    """Reads the ropes_with_light object group out of the catacombs map.

    The anchor is derived the way Rope::setup derives it: the polyline's SECOND y offset is added
    to the object position, and it runs upwards, so the anchor is that far above the object.

    The first point is not always 0,0 - object 893 starts at 0,15.04 - so it cannot be matched as a
    literal. Doing that silently read the rope as zero length and aimed the camera at the wrong end.
    """
    lines = CATACOMBS_TMX_PATH.read_text(encoding="utf-8").split("\n")

    start_index = None
    end_index = None
    for index, line in enumerate(lines):
        if 'name="ropes_with_light"' in line:
            start_index = index
        elif start_index is not None and line.strip() == "</objectgroup>":
            end_index = index
            break

    ropes = []
    current = None
    for line in lines[start_index : end_index + 1]:
        object_match = re.search(r'<object id="(\d+)" x="([-\d.]+)" y="([-\d.]+)"', line)
        if object_match:
            current = {
                "id": int(object_match.group(1)),
                "x_px": float(object_match.group(2)),
                "y_px": float(object_match.group(3)),
                "lamp_sprite": 1,
                "length_px": 0.0,
            }
            ropes.append(current)
            continue
        if current is None:
            continue
        lamp_match = re.search(r'name="lamp_sprite" type="int" value="(\d+)"', line)
        if lamp_match:
            current["lamp_sprite"] = int(lamp_match.group(1))
        polyline_match = re.search(r'<polyline points="[-\d.]+,[-\d.]+ ([-\d.]+),([-\d.]+)"', line)
        if polyline_match:
            current["length_px"] = abs(float(polyline_match.group(2)))

    return ropes


def install_clean_save_state():
    """Points slot 0 at the catacombs so the run does not resume wherever the player left off."""
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
    """The title looks like 'deceptus - 61fps [Release]'; 'deceptus' alone also hits the IDE."""
    result = []

    def callback(handle, _):
        title = win32gui.GetWindowText(handle).lower()
        if win32gui.IsWindowVisible(handle) and title.startswith("deceptus -") and "fps" in title:
            result.append(handle)
        return True

    win32gui.EnumWindows(callback, None)
    return result[0] if result else None


def focus_window():
    """Brings the game to the foreground. Posted messages are ignored by this build, so every key
    has to be real input, and real input only lands on the foreground window."""
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


def send_key(virtual_key, settle_s=0.12):
    if not focus_window():
        return
    scan_code = ctypes.windll.user32.MapVirtualKeyW(virtual_key, 0)
    win32api.keybd_event(virtual_key, scan_code, 0, 0)
    time.sleep(0.12)
    win32api.keybd_event(virtual_key, scan_code, win32con.KEYEVENTF_KEYUP, 0)
    time.sleep(settle_s)


def send_text(text):
    """Types a command. The characters have to become WM_CHAR, which only happens for real input."""
    if not focus_window():
        return
    for character in text:
        virtual_key_and_shift = ctypes.windll.user32.VkKeyScanA(ord(character))
        if virtual_key_and_shift == -1:
            continue
        virtual_key = virtual_key_and_shift & 0xFF
        needs_shift = bool(virtual_key_and_shift & 0x100)
        scan_code = ctypes.windll.user32.MapVirtualKeyW(virtual_key, 0)
        if needs_shift:
            win32api.keybd_event(win32con.VK_SHIFT, 0, 0, 0)
        win32api.keybd_event(virtual_key, scan_code, 0, 0)
        time.sleep(0.03)
        win32api.keybd_event(virtual_key, scan_code, win32con.KEYEVENTF_KEYUP, 0)
        if needs_shift:
            win32api.keybd_event(win32con.VK_SHIFT, 0, win32con.KEYEVENTF_KEYUP, 0)
        time.sleep(0.04)


def capture():
    # the game recreates its window on resolution changes, so the handle is looked up every time
    handle = find_window()
    if not handle:
        return None

    left, top, right, bottom = win32gui.GetClientRect(handle)
    width = right - left
    height = bottom - top
    if width <= 0 or height <= 0:
        return None

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


def console_green_pixels(image):
    if image is None:
        return 0
    # sampled rather than counted in full, this runs between every console step
    pixels = list(image.getdata())[::13]
    return sum(1 for red, green, blue in pixels if green > 140 and red < 120 and blue < 120)


def count_watch_lines(log_path):
    try:
        return log_path.read_text(encoding="utf-8", errors="replace").count("player position:")
    except OSError:
        return 0


def level_is_running(log_path, window_s=0.7):
    """Tells a running level from a paused one by whether the player watch is still logging.

    This replaced a pixel probe. Counting the console's green help headers looked reasonable but
    reports nonsense both ways: the catacombs have green lit rooms of their own, and the help panel
    filters down to a single topic as soon as a command is typed, so the green can vanish with the
    console still wide open.
    """
    before = count_watch_lines(log_path)
    time.sleep(window_s)
    return count_watch_lines(log_path) > before


def open_console(log_path):
    """Opens the console and proves it, so no letters can ever leak out to the global hotkeys.

    Both signals have to agree. A paused level on its own is not enough, a room transition pauses
    it too, and typing into a level that only looks paused is what turns 'tpp' into two presses of
    the pause hotkey.
    """
    for _ in range(5):
        if not level_is_running(log_path) and console_green_pixels(capture()) > CONSOLE_GREEN_THRESHOLD:
            return True
        send_key(VK_F12, settle_s=0.4)
    return False


def close_console(log_path):
    for _ in range(6):
        if level_is_running(log_path):
            return True
        send_key(VK_F12, settle_s=0.4)
    return False


def wait_for_level_loaded(log_path, confirm_key, attempts=20):
    """Presses the confirm key until the game logs that the level is up."""
    for _ in range(attempts):
        try:
            if "level loading finished" in log_path.read_text(encoding="utf-8", errors="replace"):
                return True
        except OSError:
            pass
        send_key(confirm_key, settle_s=1.5)
    return False


def read_player_tile(log_path):
    """Returns the last player tile position the game logged, which 'pwatch' keeps up to date."""
    try:
        content = log_path.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return None
    matches = re.findall(r"player position: tile (-?\d+) (-?\d+)", content)
    if not matches:
        return None
    return int(matches[-1][0]), int(matches[-1][1])


def run_console_command(log_path, command, burst_frames=0):
    """Types one command into the console and closes it again, verifying each step.

    With burst_frames set, frames are grabbed in a tight loop the instant the closing F12 goes out,
    before the close is verified. That ordering matters: verifying first costs about 0.7s, and the
    player falls some 200px in that time, which drags the rope out of the frame.
    """
    if not open_console(log_path):
        print("  console would not open")
        return False, []

    # clear anything a previous attempt left on the input line
    for _ in range(24):
        send_key(VK_BACK, settle_s=0.0)
    time.sleep(0.2)

    send_text(command)
    time.sleep(0.3)
    send_key(VK_RETURN, settle_s=0.4)

    # a reference of the console still being up. the level is paused behind it, so this frame is
    # static, and the first burst frame that differs from it is the first one after it closed
    reference = capture() if burst_frames else None
    send_key(VK_F12, settle_s=0.0)
    frames = [capture() for _ in range(burst_frames)]

    if not close_console(log_path):
        print("  console would not close")
        return False, (reference, frames)
    return True, (reference, frames)


def first_frame_without_console(burst, floor_difference=4.0):
    """Picks the earliest frame of a burst that the console is no longer drawn on.

    Compared against the reference rather than inspected on its own. Looking for the console's green
    help headers in the frame itself cannot work: the catacombs have green lit rooms, and in those
    a perfectly clean frame still reads as 'console up'.

    The bar is set from the burst instead of fixed. How much the frame changes when the overlay goes
    depends on how bright the room behind it is, and in the darkest rooms a fixed bar is never met.
    """
    reference, frames = burst
    if reference is None:
        return next((image for image in frames if image is not None), None)

    reference_samples = list(reference.convert("L").getdata())[::311]
    differences = []
    for image in frames:
        if image is None or image.size != reference.size:
            differences.append((None, 0.0))
            continue
        samples = list(image.convert("L").getdata())[::311]
        differences.append((image, sum(abs(a - b) for a, b in zip(samples, reference_samples)) / len(samples)))

    largest = max(difference for _, difference in differences)
    if largest < floor_difference:
        return None

    bar = max(floor_difference, largest * 0.4)
    for image, difference in differences:
        if image is not None and difference >= bar:
            return image
    return None


def crop_around_center(image):
    """Crops around the player, who the camera keeps near the middle of the window."""
    center_x = image.width // 2
    center_y = image.height // 2
    box = (
        max(0, center_x - CROP_WIDTH // 2),
        max(0, center_y - CROP_HEIGHT // 2),
        min(image.width, center_x + CROP_WIDTH // 2),
        min(image.height, center_y + CROP_HEIGHT // 2),
    )
    crop = image.crop(box)
    return crop.resize((crop.width * CROP_SCALE, crop.height * CROP_SCALE), Image.NEAREST)


def build_contact_sheet(entries, columns, path):
    if not entries:
        print("nothing captured, no contact sheet")
        return

    cell_width = max(image.width for _, image in entries)
    cell_height = max(image.height for _, image in entries)
    label_height = 18
    rows = (len(entries) + columns - 1) // columns

    sheet = Image.new(
        "RGB",
        (columns * (cell_width + 6) + 6, rows * (cell_height + label_height + 6) + 6),
        (14, 14, 18),
    )
    draw = ImageDraw.Draw(sheet)

    for index, (label, image) in enumerate(entries):
        column = index % columns
        row = index // columns
        x = 6 + column * (cell_width + 6)
        y = 6 + row * (cell_height + label_height + 6)
        sheet.paste(image, (x, y))
        draw.text((x + 2, y + cell_height + 3), label, fill=(215, 215, 225))

    sheet.save(path)
    print(f"saved {path} ({len(entries)} cells)")


def parse_arguments(arguments):
    """Returns the build directory and the rope ids to re-shoot, from '--only 893 1154'."""
    build_directory = "build_rel"
    only_ids = set()
    collecting_ids = False
    for argument in arguments:
        if argument == "--only":
            collecting_ids = True
        elif collecting_ids and argument.isdigit():
            only_ids.add(int(argument))
        else:
            collecting_ids = False
            if not argument.startswith("--"):
                build_directory = argument
    return build_directory, only_ids


def run():
    build_directory, only_ids = parse_arguments(sys.argv[1:])
    executable = REPO_ROOT / build_directory / "deceptus.exe"
    if not executable.exists():
        print(f"{executable} not found")
        return 1

    ropes = read_rope_objects()
    if only_ids:
        ropes = [rope for rope in ropes if rope["id"] in only_ids]
        print(f"re-shooting {len(ropes)} rope(s): {sorted(only_ids)}")
    else:
        print(f"{len(ropes)} ropes in the catacombs")

    OUTPUT_DIRECTORY.mkdir(exist_ok=True)
    install_clean_save_state()
    install_windowed_mode()

    log_path = OUTPUT_DIRECTORY / "game.log"
    log_file = log_path.open("w", encoding="utf-8", errors="replace")
    process = subprocess.Popen([str(executable)], cwd=str(REPO_ROOT), stdout=log_file, stderr=subprocess.STDOUT)
    print(f"started {executable} (pid {process.pid}), log: {log_path}")

    entries = []
    failures = []
    try:
        handle = None
        for _ in range(60):
            time.sleep(1.0)
            handle = find_window()
            if handle:
                break
        if not handle:
            print("game window not found")
            return 1

        time.sleep(3.0)

        # walk the menus by pressing Enter until the game says it loaded the level. the menu
        # background is as bright as the catacombs are dark, so a luma probe cannot tell the two
        # apart, and a fixed sleep just leaves the run sitting on the title screen
        if not wait_for_level_loaded(log_path, VK_RETURN):
            print("level never finished loading")
            return 1
        time.sleep(3.0)

        image = capture()
        if image is not None:
            image.save(OUTPUT_DIRECTORY / "00_after_load.png")

        # the game logging the player tile is what every teleport below is checked against
        # the watch has to go on before anything else: every console step below is verified by
        # whether it is still logging, and every teleport by what it logs
        watch_on, _ = run_console_command(log_path, "pwatch 200")
        if not watch_on:
            print("could not turn the player watch on")
            return 1

        # bypass the deferred lighting pass. the catacombs are lit so dimly that several ropes sit
        # in rooms that read as solid black, which proves nothing either way about the sprites
        run_console_command(log_path, "lighting disable")

        for rope in sorted(ropes, key=lambda item: item["id"]):
            # aim at the anchor, the upper end of the rope, so the rope hangs into the frame below
            # the player instead of above him once he drops
            target_x_tile = int(round(rope["x_px"] / TILE_SIZE_PX))
            target_y_tile = int(round((rope["y_px"] - rope["length_px"]) / TILE_SIZE_PX))

            # teleported twice on purpose. the first move lets the camera pan across the level and
            # settle while the console is being driven for the second one, which then re-seats the
            # player on the anchor with the camera already there, so the grab needs no long wait
            command = f"tpp {target_x_tile}, {target_y_tile}"
            moved, _ = run_console_command(log_path, command)
            if not moved:
                failures.append(rope["id"])
                print(f"rope {rope['id']}: could not drive the console, skipping")
                continue

            moved, burst = run_console_command(log_path, command, burst_frames=14)
            if not moved:
                failures.append(rope["id"])
                print(f"rope {rope['id']}: could not drive the console, skipping")
                continue

            image = first_frame_without_console(burst)
            if image is None:
                print(f"rope {rope['id']}: no clean frame in the burst")
                continue

            # the y is not checked: the player falls as soon as the level resumes
            player_tile = read_player_tile(log_path)
            if player_tile is None or abs(player_tile[0] - target_x_tile) > 2:
                failures.append(rope["id"])
                print(f"rope {rope['id']}: wanted tile x {target_x_tile}, player is at {player_tile}, skipping")
                image.save(OUTPUT_DIRECTORY / f"FAILED_rope_{rope['id']:04d}.png")
                continue

            label = f"id {rope['id']}  lamp {rope['lamp_sprite']}  len {int(rope['length_px'])}px"
            entries.append((label, crop_around_center(image)))
            image.save(OUTPUT_DIRECTORY / f"rope_{rope['id']:04d}_full.png")
            print(f"captured rope {rope['id']} at tile {target_x_tile},{target_y_tile}")

        # a burst on the last rope visited, so the lamp flicker and the swing show up as motion
        flicker_frames = []
        for _ in range(6 if not only_ids else 0):
            image = capture()
            if image is not None:
                flicker_frames.append(crop_around_center(image))
            time.sleep(0.12)
        if flicker_frames:
            strip = Image.new(
                "RGB",
                (sum(frame.width + 4 for frame in flicker_frames), flicker_frames[0].height),
                (14, 14, 18),
            )
            offset_x = 0
            for frame in flicker_frames:
                strip.paste(frame, (offset_x, 0))
                offset_x += frame.width + 4
            strip.save(OUTPUT_DIRECTORY / "lamp_flicker_strip.png")
            print("saved lamp_flicker_strip.png")

    finally:
        # a re-shoot of single ropes must not replace the sheet with a one cell version. montage.py
        # rebuilds it from every frame in out/, including the ones this run just refreshed
        if not only_ids:
            build_contact_sheet(entries, 5, OUTPUT_DIRECTORY / "catacombs_ropes_contact_sheet.png")
        process.terminate()
        try:
            process.wait(timeout=10)
        except subprocess.TimeoutExpired:
            process.kill()
        log_file.close()
        restore_save_state()
        restore_game_settings()

    print(f"captured {len(entries)} of {len(ropes)} ropes, failures: {failures or 'none'}")
    return 0 if len(entries) == len(ropes) else 1


if __name__ == "__main__":
    sys.exit(run())
