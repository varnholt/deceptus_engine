"""
Serve the mobile touch-control shell for the WASM build.

This is fully self-contained: it serves the emscripten artifacts
(deceptus.js/.wasm/.data) straight out of build_wasm/ WITHOUT modifying them,
and injects this folder's mobile.html at "/" and "/mobile.html". Nothing in the
game source or build output is touched.

The COOP/COEP headers are required for SharedArrayBuffer (the build uses pthreads).

SharedArrayBuffer is ALSO gated behind a secure context: it is only exposed over
https:// or http://localhost. A phone reaching this server over a plain-HTTP LAN
IP (http://<pc-ip>:9090) is NOT a secure context, so the game will refuse to run
there. To play on a phone, either:
  * run with --https (serves a self-signed cert; accept the browser warning), or
  * put a real-HTTPS tunnel in front of the HTTP server, e.g.
        cloudflared tunnel --url http://localhost:9090
        ngrok http 9090
    and open the https URL the tunnel prints on the phone.

Usage:
    uv run python serve.py                    # http (laptop via http://localhost:9090/)
    uv run --extra https python serve.py --https   # https (phone via https://<pc-ip>:9090/)
    python serve.py
"""

import http.server
import os
import ssl
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
BUILD_DIR = os.path.normpath(os.path.join(HERE, "..", "..", "build_wasm"))
MOBILE_HTML = os.path.join(HERE, "mobile.html")
CERT_FILE = os.path.join(HERE, "cert.pem")
KEY_FILE = os.path.join(HERE, "key.pem")
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


def _ensure_self_signed_cert():
    """Generate a self-signed cert/key pair next to serve.py if none exists yet."""
    if os.path.isfile(CERT_FILE) and os.path.isfile(KEY_FILE):
        return
    try:
        import datetime
        from cryptography import x509
        from cryptography.hazmat.primitives import hashes, serialization
        from cryptography.hazmat.primitives.asymmetric import rsa
        from cryptography.x509.oid import NameOID
    except ImportError:
        raise SystemExit(
            "--https needs the 'cryptography' package for cert generation.\n"
            "Run:  uv run --extra https python serve.py --https\n"
            "(or drop your own cert.pem/key.pem next to serve.py)"
        )

    private_key = rsa.generate_private_key(public_exponent=65537, key_size=2048)
    subject = issuer = x509.Name([x509.NameAttribute(NameOID.COMMON_NAME, "deceptus-mobile")])
    now = datetime.datetime.now(datetime.timezone.utc)
    certificate = (
        x509.CertificateBuilder()
        .subject_name(subject)
        .issuer_name(issuer)
        .public_key(private_key.public_key())
        .serial_number(x509.random_serial_number())
        .not_valid_before(now - datetime.timedelta(days=1))
        .not_valid_after(now + datetime.timedelta(days=825))
        .add_extension(x509.SubjectAlternativeName([x509.DNSName("localhost")]), critical=False)
        .sign(private_key, hashes.SHA256())
    )
    with open(KEY_FILE, "wb") as key_output:
        key_output.write(
            private_key.private_bytes(
                encoding=serialization.Encoding.PEM,
                format=serialization.PrivateFormat.TraditionalOpenSSL,
                encryption_algorithm=serialization.NoEncryption(),
            )
        )
    with open(CERT_FILE, "wb") as cert_output:
        cert_output.write(certificate.public_bytes(serialization.Encoding.PEM))
    print(f"Generated self-signed cert: {CERT_FILE}")


if __name__ == "__main__":
    if not os.path.isdir(BUILD_DIR):
        raise SystemExit(f"build_wasm not found at {BUILD_DIR} — build the WASM target first")

    use_https = "--https" in sys.argv[1:]
    scheme = "https" if use_https else "http"
    print(f"Serving mobile shell on {scheme}://localhost:{PORT}/")
    print(f"Game assets from: {BUILD_DIR}")

    httpd = http.server.ThreadingHTTPServer(("", PORT), MobileShellHandler)
    if use_https:
        _ensure_self_signed_cert()
        ssl_context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        ssl_context.load_cert_chain(certfile=CERT_FILE, keyfile=KEY_FILE)
        httpd.socket = ssl_context.wrap_socket(httpd.socket, server_side=True)
        print(f"Phone: open https://<your-pc-ip>:{PORT}/ and accept the self-signed cert warning.")
    with httpd:
        httpd.serve_forever()
