"""loads a local html file in headless chrome, dumps console errors and screenshots it"""

import pathlib
import sys
import time

from selenium import webdriver
from selenium.webdriver.chrome.options import Options


def main() -> int:
    page_path = pathlib.Path(sys.argv[1]).resolve()
    screenshot_path = pathlib.Path(sys.argv[2]).resolve()

    options = Options()
    options.add_argument("--headless=new")
    options.add_argument("--window-size=1280,2400")
    options.add_argument("--allow-file-access-from-files")
    # headless has no gpu, swiftshader gives us a real webgl2 context
    options.add_argument("--enable-unsafe-swiftshader")
    options.add_argument("--use-gl=angle")
    options.set_capability("goog:loggingPrefs", {"browser": "ALL"})

    driver = webdriver.Chrome(options=options)
    try:
        driver.get(page_path.as_uri())
        time.sleep(3.0)

        print("=== console ===")
        for entry in driver.get_log("browser"):
            print(f"{entry['level']}: {entry['message'][:400]}")

        probe = driver.execute_script(
            """
            var ring = document.getElementById('ring');
            var scene = document.getElementById('scene');
            var context = ring ? ring.getContext('webgl2') : null;
            function nonBlack(canvas) {
              var copy = document.createElement('canvas');
              copy.width = canvas.width; copy.height = canvas.height;
              var ctx = copy.getContext('2d');
              ctx.drawImage(canvas, 0, 0);
              var data = ctx.getImageData(0, 0, copy.width, copy.height).data;
              var lit = 0;
              for (var i = 0; i < data.length; i += 4) {
                if (data[i] + data[i+1] + data[i+2] > 24 && data[i+3] > 8) { lit++; }
              }
              return lit;
            }
            return {
              bodyHeight: document.body.scrollHeight,
              h1: (document.querySelector('h1') || {}).textContent,
              beatName: (document.getElementById('beat-name') || {}).textContent,
              beatTime: (document.getElementById('beat-time') || {}).textContent,
              ringSize: ring ? [ring.width, ring.height] : null,
              sceneSize: scene ? [scene.width, scene.height] : null,
              glLost: context ? context.isContextLost() : 'no context',
              litRingPixels: ring ? nonBlack(ring) : -1,
              litScenePixels: scene ? nonBlack(scene) : -1,
              stripCells: document.querySelectorAll('#strip canvas').length,
              sheetRows: document.querySelectorAll('#sheet tr').length
            };
            """
        )
        print("=== probe ===")
        for key, value in probe.items():
            print(f"{key}: {value}")

        driver.save_screenshot(str(screenshot_path))
        print(f"=== screenshot: {screenshot_path}")
    finally:
        driver.quit()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
