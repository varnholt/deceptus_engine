"""
Diagnose crash when loading the first level.
Navigates: main menu → file select → name select → game start (3x Return).
Captures all console output around the crash.
"""

import time
import io
import os
import sys

from selenium import webdriver
from selenium.webdriver.chrome.options import Options
from selenium.webdriver.common.keys import Keys
from selenium.webdriver.common.action_chains import ActionChains

WASM_URL = "http://localhost:8080/deceptus.html"
INIT_WAIT = 22     # seconds to let the WASM module boot and show the main menu
STEP_WAIT = 3      # seconds between each Return keypress to let the menu transition
CRASH_WAIT = 15    # seconds after the last keypress to let the level load (or crash)

def make_driver():
    options = Options()
    options.add_argument("--headless=new")
    options.add_argument("--no-sandbox")
    options.add_argument("--disable-dev-shm-usage")
    options.add_argument("--enable-webgl")
    options.add_argument("--use-gl=angle")
    options.add_argument("--use-angle=swiftshader")
    options.set_capability("goog:loggingPrefs", {"browser": "ALL"})
    driver = webdriver.Chrome(options=options)
    driver.set_page_load_timeout(60)
    driver.set_window_size(1400, 900)
    return driver


def save_screenshot(driver, tag):
    path = os.path.join(os.path.dirname(__file__), f"diag_{tag}.png")
    driver.save_screenshot(path)
    print(f"[screenshot] saved {path}")


def dump_logs(driver, label):
    entries = driver.get_log("browser")
    if not entries:
        print(f"[{label}] (no console messages)")
        return entries
    print(f"[{label}] {len(entries)} messages:")
    for entry in entries:
        print(f"  [{entry['level']}] {entry['message']}")
    return entries


def press_return(driver):
    canvas = driver.find_element("tag name", "canvas")
    canvas.click()
    ActionChains(driver).send_keys(Keys.RETURN).perform()


def main():
    print(f"Opening {WASM_URL}")
    driver = make_driver()
    try:
        driver.get(WASM_URL)

        print(f"Waiting {INIT_WAIT}s for WASM init...")
        time.sleep(INIT_WAIT)
        dump_logs(driver, "after-init")
        save_screenshot(driver, "01_main_menu")

        print("Pressing Return #1 (main menu → file select)...")
        press_return(driver)
        time.sleep(STEP_WAIT)
        dump_logs(driver, "after-return-1")
        save_screenshot(driver, "02_file_select")

        print("Pressing Return #2 (file select → name select)...")
        press_return(driver)
        time.sleep(STEP_WAIT)
        dump_logs(driver, "after-return-2")
        save_screenshot(driver, "03_name_select")

        print("Pressing Return #3 (name select → level load)...")
        press_return(driver)
        print(f"Waiting {CRASH_WAIT}s for level load / crash...")
        time.sleep(CRASH_WAIT)
        dump_logs(driver, "after-level-load")
        save_screenshot(driver, "04_level_or_crash")

        title = driver.title
        print(f"\n[title] {title}")

    finally:
        driver.quit()


if __name__ == "__main__":
    main()
