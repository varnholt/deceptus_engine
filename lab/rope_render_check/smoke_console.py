"""Short smoke test for the console input path, so a broken one is not discovered 30 teleports in.

    uv run --with pywin32 --with pillow python smoke_console.py [build_dir]

It turns on 'pwatch', which logs the player tile position to stdout, and then teleports once. That
makes both steps verifiable from the game's own log instead of from pixels.
"""

import sys
import time
from pathlib import Path

import drive_rope_check as harness

TEST_TILE_X = 208
TEST_TILE_Y = 118


def log_tail(log_path, count=8, needle="player position:"):
    try:
        lines = log_path.read_text(encoding="utf-8", errors="replace").split("\n")
    except OSError:
        return []
    return [line for line in lines if needle in line][-count:]


def run():
    build_directory = sys.argv[1] if len(sys.argv) > 1 else "build_rel"
    executable = harness.REPO_ROOT / build_directory / "deceptus.exe"
    if not executable.exists():
        print(f"{executable} not found")
        return 1

    harness.OUTPUT_DIRECTORY.mkdir(exist_ok=True)
    harness.install_clean_save_state()
    harness.install_windowed_mode()

    import subprocess

    log_path = harness.OUTPUT_DIRECTORY / "smoke.log"
    log_file = log_path.open("w", encoding="utf-8", errors="replace")
    process = subprocess.Popen(
        [str(executable)], cwd=str(harness.REPO_ROOT), stdout=log_file, stderr=subprocess.STDOUT
    )
    print(f"started pid {process.pid}")

    try:
        for _ in range(60):
            time.sleep(1.0)
            if harness.find_window():
                break
        else:
            print("no window")
            return 1

        time.sleep(3.0)
        if not harness.wait_for_level_loaded(log_path, harness.VK_RETURN):
            print("level never finished loading")
            harness.capture().save(harness.OUTPUT_DIRECTORY / "smoke_no_level.png")
            return 1
        time.sleep(3.0)
        print("level loaded")

        # step 1: is the console reachable at all
        opened = harness.open_console()
        print(f"console opened: {opened}")
        if not opened:
            harness.capture().save(harness.OUTPUT_DIRECTORY / "smoke_no_console.png")
            return 1

        # step 2: does typed text reach it. the help panel filters as the command grows, and the
        # screenshot shows the input line, so this is checkable both ways
        harness.send_text("pwatch 300")
        time.sleep(0.5)
        image = harness.capture()
        if image is not None:
            image.save(harness.OUTPUT_DIRECTORY / "smoke_after_typing.png")
            print(f"green pixels after typing: {harness.console_green_pixels(image)}")

        harness.send_key(harness.VK_RETURN, settle_s=0.6)
        harness.close_console()
        time.sleep(2.5)

        watch_lines = log_tail(log_path)
        print(f"pwatch lines in log: {len(watch_lines)}")
        for line in watch_lines[-2:]:
            print("   " + line.strip()[:140])
        if not watch_lines:
            print("FAIL: typed text never reached the console")
            return 1

        # step 3: does a teleport land where it was asked to
        harness.open_console()
        harness.send_text(f"tpp {TEST_TILE_X}, {TEST_TILE_Y}")
        time.sleep(0.4)
        image = harness.capture()
        if image is not None:
            image.save(harness.OUTPUT_DIRECTORY / "smoke_before_enter.png")
        harness.send_key(harness.VK_RETURN, settle_s=0.6)
        harness.close_console()
        time.sleep(2.0)

        after = log_tail(log_path, count=3)
        print("after teleport:")
        for line in after:
            print("   " + line.strip()[:140])

        image = harness.capture()
        if image is not None:
            image.save(harness.OUTPUT_DIRECTORY / "smoke_after_teleport.png")

        return 0

    finally:
        process.terminate()
        try:
            process.wait(timeout=10)
        except subprocess.TimeoutExpired:
            process.kill()
        log_file.close()
        harness.restore_save_state()
        harness.restore_game_settings()


if __name__ == "__main__":
    sys.exit(run())
