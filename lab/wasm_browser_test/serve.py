"""
Serve build_wasm/ with the COOP/COEP headers required for SharedArrayBuffer.

pthreads in WASM requires SharedArrayBuffer, which browsers only expose when
the page is cross-origin isolated. Both headers below must be present.

Usage:
    python serve.py          # serves on http://localhost:9080/deceptus.html
    uv run python serve.py
"""

import http.server
import os

BUILD_DIR = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", "..", "build_wasm"))
PORT = 9080


class CoopCoepHandler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        super().end_headers()


if __name__ == "__main__":
    os.chdir(BUILD_DIR)
    print(f"Serving {BUILD_DIR}")
    print(f"Open http://localhost:{PORT}/deceptus.html")
    with http.server.HTTPServer(("", PORT), CoopCoepHandler) as httpd:
        httpd.serve_forever()
