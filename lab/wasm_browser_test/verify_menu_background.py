"""
Verify the 3D menu background (starmap) renders in the WASM build.

Boots the WASM module, waits for the main menu, captures a screenshot, and
scans the browser console for GLSL compile/link errors from the raw-GL scene.
"""

import time
import os

from selenium import webdriver
from selenium.webdriver.chrome.options import Options

WASM_URL = "http://localhost:9080/deceptus.html"
INIT_WAIT = 25  # seconds to let the WASM module boot and show the main menu


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


def main():
    print(f"Opening {WASM_URL}")
    driver = make_driver()
    try:
        driver.get(WASM_URL)
        print(f"Waiting {INIT_WAIT}s for WASM init...")
        time.sleep(INIT_WAIT)

        entries = driver.get_log("browser")
        shader_problems = []
        for entry in entries:
            message = entry["message"]
            lowered = message.lower()
            if any(
                token in lowered
                for token in ("shader", "glsl", "compil", "link failed", "webgl")
            ):
                shader_problems.append(f"  [{entry['level']}] {message}")

        print(f"\n[console] {len(entries)} total messages")
        if shader_problems:
            print("[console] shader/webgl-related messages:")
            for line in shader_problems:
                print(line)
        else:
            print("[console] no shader/webgl-related messages")

        path = os.path.join(os.path.dirname(__file__), "verify_menu_background.png")
        driver.save_screenshot(path)
        print(f"[screenshot] saved {path}")
        print(f"[title] {driver.title}")
    finally:
        driver.quit()


if __name__ == "__main__":
    main()
