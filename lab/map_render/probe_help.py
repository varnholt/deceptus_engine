"""Opens the debug console in a running game and screenshots the output of a command.

    uv run python probe_help.py [command]

Defaults to 'help'. Used to check that newly registered console commands show up.
"""

import subprocess
import sys
import time

import drive_desktop as driver


def main() -> int:
    command = sys.argv[1] if len(sys.argv) > 1 else "help"

    driver.install_clean_save_state()
    process = None
    try:
        driver.OUTPUT_DIRECTORY.mkdir(exist_ok=True)
        log_file = (driver.OUTPUT_DIRECTORY / "probe.log").open("w", encoding="utf-8", errors="replace")
        process = subprocess.Popen(
            [str(driver.REPO_ROOT / "build_rel" / "deceptus.exe")],
            cwd=str(driver.REPO_ROOT),
            stdout=log_file,
            stderr=subprocess.STDOUT,
        )

        for _ in range(60):
            time.sleep(1.0)
            if driver.find_window():
                break

        time.sleep(3.0)
        driver.send_key(0, driver.VK_RETURN)
        time.sleep(1.5)
        driver.send_key(0, driver.VK_RETURN)
        time.sleep(10.0)

        driver.send_key(0, driver.VK_F12)
        time.sleep(0.5)
        driver.send_text(0, command)
        time.sleep(0.3)
        driver.send_key(0, driver.VK_RETURN)
        time.sleep(1.0)

        safe_name = command.replace(" ", "_")
        driver.grab_window(0, driver.OUTPUT_DIRECTORY / f"70_console_{safe_name}.png")
        return 0
    finally:
        if process:
            process.terminate()
        subprocess.run(["taskkill", "/F", "/IM", "deceptus.exe"], capture_output=True)
        driver.restore_save_state()


if __name__ == "__main__":
    raise SystemExit(main())
