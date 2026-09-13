"""Verifies the solar seal chain: locked box -> drawer -> seal -> box opens.

Teleports to the locked box first to capture the locked hint, then to the library
drawer to receive the solar seal, then back to the box, which should now open.
"""

import json
import shutil
import subprocess
import time
from pathlib import Path

import drive_desktop as driver

LOCALE_PATH = driver.REPO_ROOT / "data" / "locale" / "en.json"
LOCALE_BACKUP_PATH = driver.REPO_ROOT / "data" / "locale" / "en.json.probe_seal_backup"

LOCKED_BOX_TILE = "145, 102"
DRAWER_TILE = "290, 88"


def clear_inventory() -> None:
    """Empties slot 0's inventory so the run does not start with the old key already in it."""
    slots = json.loads(driver.SAVE_STATE_PATH.read_text())
    player_info = slots[0].setdefault("playerinfo", {})
    player_info["inventory"] = {"items": [], "slots": []}
    driver.SAVE_STATE_PATH.write_text(json.dumps(slots, indent=4))
    print("cleared inventory in slot 0")


def wait_for_log_line(log_path: Path, needle: str, timeout_s: float) -> bool:
    """Blocks until the game log contains needle, which is more reliable than a fixed sleep."""
    deadline = time.time() + timeout_s
    while time.time() < deadline:
        if log_path.exists() and needle in log_path.read_text(encoding="utf-8", errors="replace"):
            return True
        time.sleep(0.5)
    print(f"timed out waiting for '{needle}'")
    return False


def dismiss_dialogues(handle: int) -> None:
    """Pages through any open dialogue, otherwise it swallows the next console keystrokes."""
    for _ in range(4):
        driver.send_key(handle, driver.VK_RETURN)
        time.sleep(1.5)


def open_inventory_page(handle: int) -> bool:
    for _ in range(6):
        if driver.is_ingame_menu_open():
            break
        driver.send_key(handle, driver.VK_TAB)
        time.sleep(2.0)
    else:
        print("could not open the ingame menu")
        return False

    for _ in range(4):
        if driver.selected_submenu() == "inventory":
            return True
        driver.send_key(handle, driver.VK_LSHIFT)
        time.sleep(2.0)
    return False


def main() -> int:
    shutil.copy2(LOCALE_PATH, LOCALE_BACKUP_PATH)
    driver.install_clean_save_state()
    clear_inventory()
    process = None
    try:
        driver.OUTPUT_DIRECTORY.mkdir(exist_ok=True)
        log_path = driver.OUTPUT_DIRECTORY / "probe_seal.log"
        log_file = log_path.open("w", encoding="utf-8", errors="replace")
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
        wait_for_log_line(log_path, "level loading finished", 180.0)
        time.sleep(5.0)

        # 1. the box is still locked, pressing action must show the sun seal hint
        driver.run_console_command(0, f"tpp {LOCKED_BOX_TILE}")
        time.sleep(2.5)
        driver.send_key(0, driver.VK_RETURN)
        time.sleep(1.5)
        driver.grab_window(0, driver.OUTPUT_DIRECTORY / "90_box_locked.png")
        dismiss_dialogues(0)

        # 2. the library drawer hands out the solar seal
        driver.run_console_command(0, f"tpp {DRAWER_TILE}")
        time.sleep(2.5)
        driver.send_key(0, driver.VK_RETURN)
        time.sleep(2.0)
        driver.grab_window(0, driver.OUTPUT_DIRECTORY / "91_drawer_seal.png")
        dismiss_dialogues(0)

        # 3. the seal shows up in the inventory with its icon
        if open_inventory_page(0):
            driver.grab_window(0, driver.OUTPUT_DIRECTORY / "92_inventory.png")
        driver.send_key(0, driver.VK_TAB)
        time.sleep(1.5)

        # 4. back at the box, it now opens and spawns the head torch
        driver.run_console_command(0, f"tpp {LOCKED_BOX_TILE}")
        time.sleep(2.5)
        driver.send_key(0, driver.VK_RETURN)
        time.sleep(4.0)
        driver.grab_window(0, driver.OUTPUT_DIRECTORY / "93_box_open.png")

        return 0
    finally:
        if process:
            process.terminate()
        subprocess.run(["taskkill", "/F", "/IM", "deceptus.exe"], capture_output=True)
        driver.restore_save_state()
        time.sleep(1.0)
        shutil.move(str(LOCALE_BACKUP_PATH), str(LOCALE_PATH))


if __name__ == "__main__":
    raise SystemExit(main())
