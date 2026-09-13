"""
Verify the head torch helmet sprite renders on the player's head in the WASM build.

The helmet used to be drawn without the level view in RenderStates, so on VRSFML it
landed outside the visible camera rect. Drives input through the Chrome DevTools
Protocol so Emscripten receives real key/char events.
"""

import time
import os

from selenium import webdriver
from selenium.webdriver.chrome.options import Options

WASM_URL = "http://localhost:9080/deceptus.html"
INIT_WAIT = 25
LOAD_WAIT = 13


def make_driver():
    options = Options()
    options.add_argument("--headless=new")
    options.add_argument("--no-sandbox")
    options.add_argument("--disable-dev-shm-usage")
    options.add_argument("--enable-webgl")
    options.add_argument("--use-gl=angle")
    options.add_argument("--use-angle=swiftshader")
    driver = webdriver.Chrome(options=options)
    driver.set_page_load_timeout(60)
    driver.set_window_size(1400, 900)
    return driver


def key(driver, code, vk, key_name):
    """Dispatch a full keyDown/keyUp for a control key (Enter, F12)."""
    base = {"windowsVirtualKeyCode": vk, "nativeVirtualKeyCode": vk, "key": key_name, "code": code}
    driver.execute_cdp_cmd("Input.dispatchKeyEvent", {"type": "rawKeyDown", **base})
    driver.execute_cdp_cmd("Input.dispatchKeyEvent", {"type": "keyUp", **base})


def type_text(driver, text):
    """Type printable text as char events (triggers keypress -> TextEntered in Emscripten)."""
    for character in text:
        driver.execute_cdp_cmd(
            "Input.dispatchKeyEvent",
            {"type": "keyDown", "text": character, "key": character, "unmodifiedText": character},
        )
        driver.execute_cdp_cmd("Input.dispatchKeyEvent", {"type": "keyUp", "key": character})
        time.sleep(0.03)


def console_command(driver, command):
    key(driver, "F12", 123, "F12")
    time.sleep(0.6)
    type_text(driver, command)
    time.sleep(0.3)
    key(driver, "Enter", 13, "Enter")
    time.sleep(0.8)
    key(driver, "F12", 123, "F12")
    time.sleep(1.5)


def main():
    driver = make_driver()
    try:
        driver.get(WASM_URL)
        print(f"init wait {INIT_WAIT}s")
        time.sleep(INIT_WAIT)

        canvas = driver.find_element("tag name", "canvas")
        canvas.click()

        # main menu Continue -> File Select -> confirm catacombs slot -> load
        key(driver, "Enter", 13, "Enter")
        time.sleep(2.5)
        key(driver, "Enter", 13, "Enter")
        print(f"level load wait {LOAD_WAIT}s")
        time.sleep(LOAD_WAIT)

        # equip the head torch: clear the slots first, since adding an item only
        # auto-populates a slot when one is free
        console_command(driver, "item clear")
        console_command(driver, "item add headtorch")
        time.sleep(2.0)

        path = os.path.join(os.path.dirname(__file__), "verify_headtorch_helmet_wasm.png")
        driver.save_screenshot(path)
        print(f"[screenshot] {path}")

        # zoom in on the player so the helmet sprite is easy to inspect
        from PIL import Image

        image = Image.open(path)
        center_x, center_y = 684, 440
        crop = image.crop((center_x - 150, center_y - 110, center_x + 150, center_y + 110))
        crop = crop.resize((crop.width * 3, crop.height * 3), Image.NEAREST)
        zoom_path = os.path.join(os.path.dirname(__file__), "verify_headtorch_helmet_wasm_zoom.png")
        crop.save(zoom_path)
        print(f"[zoom] {zoom_path}")
    finally:
        driver.quit()


if __name__ == "__main__":
    main()
