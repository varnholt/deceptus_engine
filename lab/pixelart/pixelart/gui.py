"""Local web GUI for tuning how a piece is stylized: options, palette, preview, pipeline steps.

usage: python -m pixelart.gui [asset.json] [--port N] [--no-browser]

Serves the page on localhost and opens the browser. Stylizing runs in the request's worker thread
(a newer request supersedes a waiting one); render, integrate, capture and compare run one after
another in a background job thread whose state the page polls.
"""
import argparse
import itertools
import json
import threading
import time
import traceback
import webbrowser
from http import HTTPStatus
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import parse_qs, urlparse

from . import workbench
from .__main__ import load

PAGE = Path(__file__).with_name("gui_page.html")
RECENT_FILE = Path.home() / ".pixelart" / "recent_assets.json"
RECENT_LIMIT = 12
DEFAULT_PORT = 8765
HOST = "127.0.0.1"
# the page only ever shows images; anything else stays off the wire
IMAGE_TYPES = {".png": "image/png", ".bmp": "image/bmp", ".jpg": "image/jpeg", ".jpeg": "image/jpeg", ".gif": "image/gif"}
JOB_HISTORY = 20


def recent_assets():
    try:
        return json.loads(RECENT_FILE.read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return []


def remember_asset(path):
    entries = [path] + [p for p in recent_assets() if p != path]
    RECENT_FILE.parent.mkdir(parents=True, exist_ok=True)
    RECENT_FILE.write_text(json.dumps(entries[:RECENT_LIMIT], indent=2), encoding="utf-8")


def resolve_source(source):
    """The page names what it works on either as {asset, piece} or as {passes: {beauty, ids, normals}}."""
    if "passes" in source:
        passes = source["passes"]
        return workbench.target_for_passes(passes["beauty"], passes["ids"], passes["normals"]), None
    cfg = load(source["asset"])
    return workbench.target_for_piece(cfg, source["piece"]), cfg


class LatestStylizer:
    """Runs one stylize at a time; a request still waiting when a newer one arrives is dropped,
    so dragging a slider only costs the first and the last stylize."""

    def __init__(self):
        self.newest = 0
        self.counter_lock = threading.Lock()
        self.work_lock = threading.Lock()

    def run(self, target, options, ramps, box):
        with self.counter_lock:
            self.newest += 1
            ticket = self.newest
        with self.work_lock:
            if ticket != self.newest:
                return {"superseded": True}
            preview = workbench.stylize_to(target, options, ramps, target.preview_path("preview"))
            return {
                "preview": str(preview),
                "whole": workbench.image_metrics(preview),
                "visible": workbench.image_metrics(preview, box) if box else None,
            }


class JobQueue:
    """Long pipeline steps, one at a time (Blender and the game should not run twice at once)."""

    def __init__(self):
        self.jobs = {}
        self.ids = itertools.count(1)
        self.lock = threading.Lock()
        self.wake = threading.Condition(self.lock)
        threading.Thread(target=self._work, daemon=True).start()

    def submit(self, kind, title, step):
        """step() returns (result dict, log text) or raises."""
        with self.lock:
            job_id = next(self.ids)
            self.jobs[job_id] = {"id": job_id, "kind": kind, "title": title, "state": "queued", "log": "",
                                 "result": None, "error": None, "started": None, "finished": None, "step": step}
            self._forget_old_jobs()
            self.wake.notify()
        return job_id

    def _forget_old_jobs(self):
        finished = [j for j in self.jobs.values() if j["state"] in ("done", "failed")]
        for job in finished[:max(0, len(self.jobs) - JOB_HISTORY)]:
            del self.jobs[job["id"]]

    def snapshot(self):
        now = time.time()
        with self.lock:
            return [{**{k: v for k, v in job.items() if k != "step"},
                     "elapsed": round((job["finished"] or now) - job["started"]) if job["started"] else 0}
                    for job in self.jobs.values()]

    def _next_queued(self):
        with self.lock:
            while True:
                queued = [j for j in self.jobs.values() if j["state"] == "queued"]
                if queued:
                    job = queued[0]
                    job.update(state="running", started=time.time())
                    return job
                self.wake.wait()

    def _work(self):
        while True:
            job = self._next_queued()
            try:
                result, log = job["step"]()
                job.update(state="done", result=result, log=log)
            except workbench.StepFailed as failure:
                job.update(state="failed", error=str(failure), log=failure.log)
            except Exception as failure:
                job.update(state="failed", error=f"{type(failure).__name__}: {failure}", log=traceback.format_exc())
            job["finished"] = time.time()


class Api:
    """The page's endpoints; each takes the request's JSON body and returns a JSON-able dict."""

    def __init__(self, initial_asset):
        self.initial_asset = initial_asset
        self.stylizer = LatestStylizer()
        self.jobs = JobQueue()

    def info(self, _):
        return {
            "specs": workbench.option_specs(),
            "recent": recent_assets(),
            "initial_asset": self.initial_asset,
        }

    def open_asset(self, body):
        cfg = load(body["path"])
        remember_asset(cfg["_path"])
        return {
            "asset": cfg["_path"],
            "pieces": [workbench.piece_summary(cfg, piece) for piece in cfg["pieces"]],
            "references": [{"index": i, "label": ref.get("label", Path(ref["path"]).name)}
                           for i, ref in enumerate(cfg.get("references", []))],
        }

    def open_passes(self, body):
        target = workbench.target_for_passes(body["beauty"], body["ids"], body["normals"])
        return {"label": target.label, "missing": workbench.missing_passes(target), "written_art": str(target.written_art),
                "options": workbench.default_options(), "palette": workbench.default_palette()}

    def stylize(self, body):
        target, _ = resolve_source(body["source"])
        return self.stylizer.run(target, body["options"], body["palette"], body.get("box"))

    def metrics(self, body):
        return workbench.image_metrics(body["path"], body.get("box"))

    def keep_before(self, body):
        target, _ = resolve_source(body["source"])
        before = workbench.keep_as_before(target, target.preview_path("preview"))
        return {"before": str(before), "whole": workbench.image_metrics(before)}

    def save(self, body):
        source = body["source"]
        saved = workbench.save_piece_settings(source["asset"], source["piece"], body["options"],
                                              body.get("palette"), body.get("palette_scope", "piece"))
        return {"saved_options": saved, **self.keep_before(body)}

    def write(self, body):
        target, _ = resolve_source(body["source"])
        return {"written": str(workbench.write_art(target, body["options"], body["palette"]))}

    def read_palette(self, body):
        return workbench.read_palette(body["path"], body.get("box"))

    def job(self, body):
        kind = body["kind"]
        target, cfg = resolve_source(body["source"])
        if cfg is None:
            raise ValueError(f"{kind} needs an asset config, not loose passes")
        piece = target.label
        if kind == "render":
            step = lambda: ({"passes": [str(target.beauty), str(target.ids), str(target.normals)]},
                            workbench.render(cfg, piece))
        elif kind == "integrate":
            step = lambda: ({}, workbench.integrate(cfg, piece))
        elif kind == "capture":
            pieces = body.get("pieces") or [piece]
            label = time.strftime("%Y%m%d-%H%M%S")
            step = lambda: self._capture_result(cfg, pieces, label)
        elif kind == "compare":
            step = lambda: self._compare_result(cfg, piece, target)
        else:
            raise ValueError(f"unknown job '{kind}'")
        return {"id": self.jobs.submit(kind, f"{kind} {piece}", step)}

    @staticmethod
    def _capture_result(cfg, pieces, label):
        frames, log = workbench.capture(cfg, pieces, label)
        return {"images": frames}, log

    @staticmethod
    def _compare_result(cfg, piece, target):
        """The sheet shows the preview being tuned, so it lands beside the preview, not over compare_<piece>.png."""
        sheet, log = workbench.compare_sheet(cfg, piece, target.preview_path("preview"), target.preview_path("compare"))
        if sheet is None:
            raise ValueError(f"piece '{piece}' has no compare_boxes")
        return {"images": [str(sheet)]}, log

    def job_list(self, _):
        return {"jobs": self.jobs.snapshot()}


POST_ROUTES = {"open_asset", "open_passes", "stylize", "metrics", "keep_before", "save", "write", "read_palette", "job"}


class Handler(BaseHTTPRequestHandler):
    api = None  # set by serve()

    def log_message(self, *_):
        # the page polls the job list every second; logging each request would bury real output
        pass

    def _send(self, status, content_type, data):
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(data)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(data)

    def _send_json(self, payload, status=HTTPStatus.OK):
        self._send(status, "application/json", json.dumps(payload).encode("utf-8"))

    def _send_error_json(self, failure):
        self._send_json({"error": f"{type(failure).__name__}: {failure}"}, HTTPStatus.BAD_REQUEST)

    def do_GET(self):
        url = urlparse(self.path)
        query = {k: v[0] for k, v in parse_qs(url.query).items()}
        try:
            if url.path == "/":
                self._send(HTTPStatus.OK, "text/html; charset=utf-8", PAGE.read_bytes())
            elif url.path == "/api/info":
                self._send_json(self.api.info(query))
            elif url.path == "/api/jobs":
                self._send_json(self.api.job_list(query))
            elif url.path == "/api/reference":
                ref = load(query["asset"])["references"][int(query["index"])]
                self._send(HTTPStatus.OK, "image/png", workbench.reference_png(ref))
            elif url.path == "/image":
                self._send_image(Path(query["path"]))
            else:
                self._send_json({"error": f"no page {url.path}"}, HTTPStatus.NOT_FOUND)
        except Exception as failure:
            self._send_error_json(failure)

    def _send_image(self, path):
        content_type = IMAGE_TYPES.get(path.suffix.lower())
        if content_type is None:
            raise ValueError(f"not an image: {path}")
        self._send(HTTPStatus.OK, content_type, path.read_bytes())

    def do_POST(self):
        name = urlparse(self.path).path.removeprefix("/api/")
        if name not in POST_ROUTES:
            self._send_json({"error": f"no endpoint {self.path}"}, HTTPStatus.NOT_FOUND)
            return
        try:
            body = json.loads(self.rfile.read(int(self.headers.get("Content-Length", 0))) or b"{}")
            self._send_json(getattr(self.api, name)(body))
        except Exception as failure:
            self._send_error_json(failure)


def serve(initial_asset, port, open_browser):
    Handler.api = Api(initial_asset)
    try:
        server = ThreadingHTTPServer((HOST, port), Handler)
    except OSError:
        # the usual port is taken (another GUI still open); any free port will do
        server = ThreadingHTTPServer((HOST, 0), Handler)
    url = f"http://{HOST}:{server.server_address[1]}/"
    print(f"pixelart GUI on {url} (Ctrl+C stops it)", flush=True)
    if open_browser:
        webbrowser.open(url)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


def main(argv=None):
    parser = argparse.ArgumentParser(prog="pixelart.gui", description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("asset", nargs="?", help="asset config to open at start")
    parser.add_argument("--port", type=int, default=DEFAULT_PORT)
    parser.add_argument("--no-browser", action="store_true", help="only serve, do not open a browser")
    args = parser.parse_args(argv)
    initial_asset = str(Path(args.asset).resolve()) if args.asset else None
    serve(initial_asset, args.port, not args.no_browser)


if __name__ == "__main__":
    main()
