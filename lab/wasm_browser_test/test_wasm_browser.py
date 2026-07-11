"""
WASM browser smoke test — connects to the local dev server and captures
any SEVERE/WARNING console messages from the browser.

Usage:
    uv run pytest test_wasm_browser.py -v -s

Requirements:
    - The HTTP server must be running:
          python -m http.server 9080 --directory ../../build_wasm
      (or the run.bat in this directory handles it automatically)
    - Google Chrome must be installed.

The test waits up to WAIT_SECONDS for the WASM module to initialize, then
dumps every console message so it can be pasted into the port status doc.
"""

import subprocess
import time
import socket
import os
import sys
import io
import http.server
import threading
import pytest
from selenium import webdriver
from selenium.webdriver.chrome.options import Options
from selenium.webdriver.support.ui import WebDriverWait

WASM_URL = "http://localhost:9080/deceptus.html"
BUILD_DIR = os.path.normpath(
    os.path.join(os.path.dirname(__file__), "..", "..", "build_wasm")
)
# Seconds to wait after page load before collecting logs.
# WASM startup is slow; increase if the game doesn't get far enough.
WAIT_SECONDS = 20


def _port_open(host: str, port: int) -> bool:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.settimeout(1)
        return sock.connect_ex((host, port)) == 0


class _CoopCoepHandler(http.server.SimpleHTTPRequestHandler):
    """SimpleHTTPRequestHandler that adds COOP/COEP headers required for SharedArrayBuffer.

    pthreads in WASM requires SharedArrayBuffer, which browsers only expose when
    the page is cross-origin isolated (COOP + COEP headers both present).
    """

    def end_headers(self):
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        super().end_headers()

    def log_message(self, *args):
        pass  # silence request logs


@pytest.fixture(scope="session")
def http_server():
    """Start an HTTP server with COOP/COEP headers for build_wasm/ if not already up."""
    already_running = _port_open("127.0.0.1", 9080)
    server_thread = None
    httpd = None

    if not already_running:
        if not os.path.isdir(BUILD_DIR):
            pytest.skip(f"build_wasm/ not found at {BUILD_DIR} — build first")
        print(f"\n[fixture] Starting COOP/COEP HTTP server on :9080 from {BUILD_DIR}")
        os.chdir(BUILD_DIR)
        httpd = http.server.HTTPServer(("", 9080), _CoopCoepHandler)
        server_thread = threading.Thread(target=httpd.serve_forever, daemon=True)
        server_thread.start()
        deadline = time.monotonic() + 5
        while not _port_open("127.0.0.1", 9080):
            if time.monotonic() > deadline:
                httpd.shutdown()
                pytest.fail("HTTP server did not start within 5 seconds")
            time.sleep(0.2)
        print("[fixture] HTTP server ready")
    else:
        print("\n[fixture] HTTP server already running on :9080")

    yield

    if httpd is not None:
        print("[fixture] Stopping HTTP server")
        httpd.shutdown()


@pytest.fixture(scope="session")
def browser():
    options = Options()
    options.add_argument("--headless=new")
    options.add_argument("--no-sandbox")
    options.add_argument("--disable-dev-shm-usage")
    # Enable GPU-based WebGL even in headless mode (required for WASM/WebGL)
    options.add_argument("--enable-webgl")
    options.add_argument("--use-gl=angle")
    options.add_argument("--use-angle=swiftshader")
    # Capture all browser console messages
    options.set_capability("goog:loggingPrefs", {"browser": "ALL"})

    driver = webdriver.Chrome(options=options)
    driver.set_page_load_timeout(60)
    # Large enough window to capture the 1280×720 canvas plus page chrome
    driver.set_window_size(1400, 900)
    yield driver
    driver.quit()


def _collect_logs(driver: webdriver.Chrome) -> list[dict]:
    """Return all browser console log entries."""
    return driver.get_log("browser")


def test_wasm_loads_without_severe_errors(http_server, browser):
    """
    Navigate to the WASM page and assert no SEVERE console errors appear
    within the startup window.
    """
    print(f"\nNavigating to {WASM_URL}")
    browser.get(WASM_URL)

    # Give the WASM module time to download, compile, and start running
    print(f"Waiting {WAIT_SECONDS}s for WASM initialization…")
    time.sleep(WAIT_SECONDS)

    logs = _collect_logs(browser)
    _print_all_logs(logs)

    severe = [
        entry for entry in logs
        if entry["level"] == "SEVERE"
        and "favicon.ico" not in entry["message"]
    ]
    assert not severe, _format_log_block("SEVERE errors", severe)


def test_wasm_no_gl_errors(http_server, browser):
    """Check that no WebGL-related errors appeared during startup."""
    logs = _collect_logs(browser)
    gl_errors = [
        entry for entry in logs
        if entry["level"] in ("SEVERE", "WARNING")
        and any(kw in entry["message"].lower() for kw in ("webgl", "gl_", "shader", "layout"))
    ]
    assert not gl_errors, _format_log_block("WebGL/shader errors", gl_errors)


def test_wasm_no_memory_errors(http_server, browser):
    """Check that no memory / heap / abort errors appeared during startup."""
    logs = _collect_logs(browser)
    mem_errors = [
        entry for entry in logs
        if entry["level"] == "SEVERE"
        and any(kw in entry["message"].lower() for kw in ("abort", "heap", "memory", "oom", "out of memory"))
    ]
    assert not mem_errors, _format_log_block("Memory/abort errors", mem_errors)


def test_wasm_canvas_not_black(http_server, browser):
    """
    Verify the game canvas is actually rendering something by taking a browser
    screenshot and checking that the center region contains non-black pixels.
    WebGL's readPixels is unreliable after swapBuffers (backbuffer cleared),
    so a DOM screenshot is used instead.
    """
    # pillow is needed to analyse the screenshot pixels
    try:
        from PIL import Image
    except ImportError:
        pytest.skip("pillow not installed — install it with: uv add pillow")

    # Wait a bit more in case we're still on the splash screen
    time.sleep(5)

    # Save screenshot for offline inspection
    scratchpad = os.path.join(os.path.dirname(__file__), "last_screenshot.png")
    browser.save_screenshot(scratchpad)
    print(f"\n[screenshot] saved to {scratchpad}")

    # Print DOM info to understand what's on the page
    dom_info = browser.execute_script("""
        const canvas = document.querySelector('canvas');
        const body = document.body;
        return {
            title: document.title,
            bodyBg: body ? window.getComputedStyle(body).backgroundColor : 'no body',
            canvasFound: !!canvas,
            canvasWidth: canvas ? canvas.width : 0,
            canvasHeight: canvas ? canvas.height : 0,
            canvasStyle: canvas ? canvas.getAttribute('style') : '',
            canvasCssWidth: canvas ? window.getComputedStyle(canvas).width : '',
            canvasCssHeight: canvas ? window.getComputedStyle(canvas).height : '',
        };
    """)
    print(f"[dom] {dom_info}")

    png_bytes = browser.get_screenshot_as_png()
    image = Image.open(io.BytesIO(png_bytes)).convert("RGB")

    width, height = image.size
    # Find canvas bounding box via JS to know where to sample
    canvas_rect = browser.execute_script("""
        const canvas = document.querySelector('canvas');
        if (!canvas) return null;
        const rect = canvas.getBoundingClientRect();
        return {left: rect.left, top: rect.top, width: rect.width, height: rect.height};
    """)
    print(f"\n[screenshot] size={width}x{height}, canvas_rect={canvas_rect}")

    if canvas_rect and canvas_rect['width'] > 0 and canvas_rect['height'] > 0:
        # Device pixel ratio may differ; scale JS coords to screenshot coords
        scale = width / browser.execute_script("return window.innerWidth;")
        left = int(canvas_rect['left'] * scale)
        top = int(canvas_rect['top'] * scale)
        right = min(int((canvas_rect['left'] + canvas_rect['width']) * scale), width)
        bottom = min(int((canvas_rect['top'] + canvas_rect['height']) * scale), height)
        canvas_region = image.crop((left, top, right, bottom))
        print(f"[screenshot] cropped canvas region: ({left},{top})-({right},{bottom})")
    else:
        canvas_region = image

    flat = list(canvas_region.getdata())  # list of (r, g, b) tuples
    non_black = [(r, g, b) for (r, g, b) in flat if r > 5 or g > 5 or b > 5]

    print(f"[screenshot] canvas pixels: {len(flat)}, non-black: {len(non_black)}")
    if non_black:
        print(f"[screenshot] sample non-black: {non_black[:5]}")

    assert non_black, (
        f"All {len(flat)} canvas pixels are black — "
        f"the game is rendering a blank frame. Screenshot saved to {scratchpad}"
    )


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _format_log_block(title: str, entries: list[dict]) -> str:
    lines = [f"\n{'=' * 60}", f"  {title} ({len(entries)} entries)", "=" * 60]
    for entry in entries:
        lines.append(f"[{entry['level']}] {entry['message']}")
    return "\n".join(lines)


def _print_all_logs(entries: list[dict]) -> None:
    if not entries:
        print("[console] (no messages captured)")
        return
    print(f"[console] {len(entries)} messages:")
    for entry in entries:
        print(f"  [{entry['level']}] {entry['message']}")
