"""
Serve the mobile touch-control shell for the WASM build.

This is fully self-contained: it serves the emscripten artifacts
(deceptus.js/.wasm/.data) straight out of build_wasm/ WITHOUT modifying them,
and injects this folder's mobile.html at "/" and "/mobile.html". Nothing in the
game source or build output is touched.

The COOP/COEP headers are required for SharedArrayBuffer (the build uses pthreads).

Usage:
    uv run python serve.py           # http://localhost:9090/  (phone: http://<pc-ip>:9090/)
    python serve.py
"""

import http.server
import os

HERE = os.path.dirname(os.path.abspath(__file__))
BUILD_DIR = os.path.normpath(os.path.join(HERE, "..", "..", "build_wasm"))
MOBILE_HTML = os.path.join(HERE, "mobile.html")
PORT = 9090


class MobileShellHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        # game assets are resolved relative to build_wasm/
        super().__init__(*args, directory=BUILD_DIR, **kwargs)

    def end_headers(self):
        # cross-origin isolation, required for SharedArrayBuffer / pthreads
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        self.send_header("Cache-Control", "no-store")
        super().end_headers()

    def do_GET(self):
        if self.path.split("?")[0] in ("/", "/index.html", "/mobile.html"):
            self._serve_mobile_shell()
            return
        super().do_GET()

    def _serve_mobile_shell(self):
        with open(MOBILE_HTML, "rb") as file:
            body = file.read()
        self.send_response(200)
        self.send_header("Content-Type", "text/html; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)


if __name__ == "__main__":
    if not os.path.isdir(BUILD_DIR):
        raise SystemExit(f"build_wasm not found at {BUILD_DIR} — build the WASM target first")
    print(f"Serving mobile shell on http://localhost:{PORT}/")
    print(f"Game assets from: {BUILD_DIR}")
    with http.server.ThreadingHTTPServer(("", PORT), MobileShellHandler) as httpd:
        httpd.serve_forever()
