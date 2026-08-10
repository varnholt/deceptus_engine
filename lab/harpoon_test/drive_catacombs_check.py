"""Sanity check in the real catacombs level: normal walking and jumping must be unaffected by the
harpoon changes, and the harpoon itself has to behave in level geometry it was not designed around.

    uv run --with pywin32 --with pillow python drive_catacombs_check.py [build_dir]
"""

import sys
import time

import drive_harpoon as harness

harness.HARPOON_TEST_LEVEL_INDEX = 0  # catacombs


def run() -> int:
    build_directory = sys.argv[1] if len(sys.argv) > 1 else "build_rel"
    executable = harness.REPO_ROOT / build_directory / "deceptus.exe"

    log_path = harness.OUTPUT_DIRECTORY / "catacombs.log"
    harness.OUTPUT_DIRECTORY.mkdir(exist_ok=True)
    log_file = log_path.open("w", encoding="utf-8", errors="replace")

    import subprocess

    process = subprocess.Popen([str(executable)], cwd=str(harness.REPO_ROOT), stdout=log_file, stderr=subprocess.STDOUT)

    for _ in range(60):
        time.sleep(1.0)
        if harness.find_window():
            break

    time.sleep(3.0)
    harness.send_key(harness.VK_RETURN)
    time.sleep(1.5)
    harness.send_key(harness.VK_RETURN)
    time.sleep(10.0)

    # the intro dialogue locks all player controls; teleporting to a checkpoint leaves it behind
    harness.run_console_command("tpc 1")
    time.sleep(2.0)

    harness.grab("50_catacombs.png")

    # plain walking and jumping, the paths the harpoon must not have touched
    harness.hold_key(harness.VK_RIGHT, True)
    time.sleep(1.2)
    harness.send_key(harness.VK_SPACE, hold_s=0.2, settle_s=0.3)
    harness.capture_burst("51_walk_jump", 1.5)
    harness.hold_key(harness.VK_RIGHT, False)
    time.sleep(1.5)
    harness.grab("52_after_walk.png")

    # and the harpoon in real level geometry
    harness.send_key(harness.VK_H, hold_s=0.05, settle_s=0.0)
    harness.capture_burst("53_harpoon", 2.0)
    harness.grab("54_harpoon_attached.png")
    harness.send_key(harness.VK_H, hold_s=0.05, settle_s=0.0)
    time.sleep(1.0)
    harness.grab("55_released.png")

    # walking has to work again after the rope is gone
    harness.hold_key(harness.VK_RIGHT, True)
    time.sleep(1.5)
    harness.hold_key(harness.VK_RIGHT, False)
    harness.grab("56_walking_after.png")

    log_file.flush()
    text = log_path.read_text(encoding="utf-8", errors="replace")
    errors = [line for line in text.splitlines() if "[e]" in line]
    print(f"errors in log: {len(errors)}")
    for line in errors[-12:]:
        print(" ", line[:180])

    process.terminate()
    return 0


def main() -> int:
    harness.install_save_state()
    try:
        return run()
    finally:
        import subprocess

        subprocess.run(["taskkill", "/F", "/IM", "deceptus.exe"], capture_output=True)
        harness.restore_save_state()


if __name__ == "__main__":
    raise SystemExit(main())
