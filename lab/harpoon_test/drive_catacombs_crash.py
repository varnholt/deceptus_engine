"""Reproduces the crash the user hit: catacombs start, crumbling stones above the blob enemy,
player hits the solid block left of the crumbling one while hanging on the rope.

    uv run --with pywin32 --with pillow python drive_catacombs_crash.py [build_dir]

Run it against build_deb so the dump carries symbols.
"""

import subprocess
import sys
import time

import drive_harpoon as harness

harness.HARPOON_TEST_LEVEL_INDEX = 0  # catacombs


def run() -> int:
    build_directory = sys.argv[1] if len(sys.argv) > 1 else "build_deb"
    executable = harness.REPO_ROOT / build_directory / "deceptus.exe"

    harness.OUTPUT_DIRECTORY.mkdir(exist_ok=True)
    log_file = (harness.OUTPUT_DIRECTORY / "catacombs_crash.log").open("w", encoding="utf-8", errors="replace")
    process = subprocess.Popen([str(executable)], cwd=str(harness.REPO_ROOT), stdout=log_file, stderr=subprocess.STDOUT)

    for _ in range(90):
        time.sleep(1.0)
        if harness.find_window():
            break

    time.sleep(4.0)
    harness.send_key(harness.VK_RETURN)
    time.sleep(1.5)
    harness.send_key(harness.VK_RETURN)
    time.sleep(14.0)

    # the intro dialogue locks the controls, the console does not care
    harness.run_console_command("weapon add harpoon")
    harness.run_console_command("iddqd")
    harness.run_console_command("tpc 1")
    time.sleep(2.0)
    harness.grab("70_start.png")

    def alive() -> bool:
        return process.poll() is None and harness.find_window() is not None

    # sweep the area: hook in every direction, then bash left and right into whatever is there
    directions = [
        ("up", harness.VK_UP),
        ("forward", None),
        ("horizontal", harness.VK_DOWN),
    ]

    for attempt in range(6):
        for label, modifier in directions:
            if not alive():
                print(f"CRASHED before attempt {attempt} {label}")
                return 0

            if modifier:
                harness.hold_key(modifier, True)
            harness.send_key(harness.VK_LCONTROL, hold_s=0.05, settle_s=0.0)
            if modifier:
                harness.hold_key(modifier, False)
            time.sleep(0.8)

            for walk_key in (harness.VK_LEFT, harness.VK_RIGHT):
                harness.hold_key(walk_key, True)
                time.sleep(1.6)
                harness.hold_key(walk_key, False)
                time.sleep(0.4)
                if not alive():
                    print(f"CRASHED: attempt {attempt}, shot {label}, walking {'left' if walk_key == harness.VK_LEFT else 'right'}")
                    harness.OUTPUT_DIRECTORY.joinpath("crash_context.txt").write_text(
                        f"attempt {attempt} shot {label} walk {'left' if walk_key == harness.VK_LEFT else 'right'}\n"
                    )
                    return 0

            # drop the rope for the next shot
            harness.send_key(harness.VK_LCONTROL, hold_s=0.05, settle_s=0.0)
            time.sleep(0.5)

        harness.grab(f"71_attempt_{attempt}.png")

    print("no crash reproduced")
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
