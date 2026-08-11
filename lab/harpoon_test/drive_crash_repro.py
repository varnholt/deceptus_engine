"""Tries to reproduce the crash reported when the player hits level geometry while on the rope.

    uv run --with pywin32 --with pillow python drive_crash_repro.py [build_dir]

Runs a series of increasingly violent collisions while the rope is attached and reports which one
killed the process. Each scenario starts by teleporting, so a crash can be attributed to one.
"""

import subprocess
import sys
import time

import drive_harpoon as harness

harness.HARPOON_TEST_LEVEL_INDEX = 3


def alive(process) -> bool:
    return process.poll() is None and harness.find_window() is not None


def run() -> int:
    build_directory = sys.argv[1] if len(sys.argv) > 1 else "build_rel"
    executable = harness.REPO_ROOT / build_directory / "deceptus.exe"

    harness.OUTPUT_DIRECTORY.mkdir(exist_ok=True)
    log_file = (harness.OUTPUT_DIRECTORY / "crash.log").open("w", encoding="utf-8", errors="replace")
    process = subprocess.Popen([str(executable)], cwd=str(harness.REPO_ROOT), stdout=log_file, stderr=subprocess.STDOUT)

    for _ in range(90):
        time.sleep(1.0)
        if harness.find_window():
            break

    time.sleep(4.0)
    harness.send_key(harness.VK_RETURN)
    time.sleep(1.5)
    harness.send_key(harness.VK_RETURN)
    time.sleep(12.0)

    harness.run_console_command("weapon add harpoon")
    harness.run_console_command("iddqd")

    def shoot(up: bool = False, down: bool = False) -> None:
        if up:
            harness.hold_key(harness.VK_UP, True)
        if down:
            harness.hold_key(harness.VK_DOWN, True)
        harness.send_key(harness.VK_LCONTROL, hold_s=0.05, settle_s=0.0)
        if up:
            harness.hold_key(harness.VK_UP, False)
        if down:
            harness.hold_key(harness.VK_DOWN, False)
        time.sleep(1.0)

    scenarios = []

    def scenario(name):
        def register(function):
            scenarios.append((name, function))
            return function

        return register

    @scenario("swing into the platform wall from the ledge")
    def _():
        harness.run_console_command("tpp 13, 10")
        time.sleep(1.0)
        harness.hold_key(harness.VK_RIGHT, True)
        time.sleep(0.45)
        shoot()
        time.sleep(4.0)
        harness.hold_key(harness.VK_RIGHT, False)

    @scenario("reel in until slamming into the ceiling")
    def _():
        harness.run_console_command("tpp 17, 14")
        time.sleep(1.0)
        shoot(up=True)
        harness.hold_key(harness.VK_UP, True)
        time.sleep(4.0)
        harness.hold_key(harness.VK_UP, False)

    @scenario("reel in while swinging sideways into the stalactite")
    def _():
        harness.run_console_command("tpp 24, 14")
        time.sleep(1.0)
        shoot(up=True)
        harness.hold_key(harness.VK_LEFT, True)
        harness.hold_key(harness.VK_UP, True)
        time.sleep(4.0)
        harness.hold_key(harness.VK_UP, False)
        harness.hold_key(harness.VK_LEFT, False)

    @scenario("hook a wall sideways and get dragged into it")
    def _():
        harness.run_console_command("tpp 25, 14")
        time.sleep(1.0)
        shoot(down=True)
        harness.hold_key(harness.VK_RIGHT, True)
        time.sleep(3.0)
        harness.hold_key(harness.VK_RIGHT, False)

    @scenario("swing into the pit wall at speed")
    def _():
        harness.run_console_command("tpp 104, 10")
        time.sleep(1.0)
        harness.hold_key(harness.VK_RIGHT, True)
        time.sleep(0.5)
        shoot()
        time.sleep(4.0)
        harness.hold_key(harness.VK_RIGHT, False)

    @scenario("long rope, drop from the ceiling into the floor")
    def _():
        harness.run_console_command("tpp 60, 7")
        time.sleep(1.0)
        shoot(up=True)
        harness.hold_key(harness.VK_DOWN, True)
        time.sleep(4.0)
        harness.hold_key(harness.VK_DOWN, False)
        harness.hold_key(harness.VK_RIGHT, True)
        time.sleep(2.0)
        harness.hold_key(harness.VK_RIGHT, False)

    for name, function in scenarios:
        if not alive(process):
            print(f"process already gone before: {name}")
            break

        print(f"--- {name}")
        function()
        time.sleep(1.0)

        if not alive(process):
            print(f"CRASHED during: {name}")
            break

        # drop the rope again for the next scenario
        harness.send_key(harness.VK_LCONTROL, hold_s=0.05, settle_s=0.0)
        time.sleep(1.0)
    else:
        print("no crash reproduced")

    log_file.flush()
    text = (harness.OUTPUT_DIRECTORY / "crash.log").read_text(encoding="utf-8", errors="replace")
    for line in [line for line in text.splitlines() if "assert" in line.lower() or "Assertion" in line][-5:]:
        print("assert:", line[:200])

    if process.poll() is None:
        process.terminate()
    return 0


def main() -> int:
    harness.install_save_state()
    harness.force_windowed_mode()
    try:
        return run()
    finally:
        subprocess.run(["taskkill", "/F", "/IM", "deceptus.exe"], capture_output=True)
        harness.restore_save_state()
        harness.restore_fullscreen_mode()


if __name__ == "__main__":
    raise SystemExit(main())
