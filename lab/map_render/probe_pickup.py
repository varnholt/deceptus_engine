"""Verifies the extra -> mechanism event -> map reveal path.

Teleports onto the sword extra (tile 125,122) which has a temporary
pickup_event=reveal_map property, then opens the map.
"""

import subprocess
import time

import drive_desktop as driver


def main() -> int:
    driver.install_clean_save_state()
    process = None
    try:
        driver.OUTPUT_DIRECTORY.mkdir(exist_ok=True)
        log_file = (driver.OUTPUT_DIRECTORY / "probe_pickup.log").open("w", encoding="utf-8", errors="replace")
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

        # map before the pickup: only the spawn room
        if driver.open_map_page(0):
            driver.grab_window(0, driver.OUTPUT_DIRECTORY / "80_before_pickup.png")
        driver.send_key(0, driver.VK_TAB)
        time.sleep(1.5)

        # the sword extra sits at world 3000,2928 -> tile 125,122
        driver.run_console_command(0, "tpp 125, 122")
        time.sleep(2.0)

        if driver.open_map_page(0):
            driver.grab_window(0, driver.OUTPUT_DIRECTORY / "81_after_pickup.png")

        return 0
    finally:
        if process:
            process.terminate()
        subprocess.run(["taskkill", "/F", "/IM", "deceptus.exe"], capture_output=True)
        driver.restore_save_state()


if __name__ == "__main__":
    raise SystemExit(main())
