#!/usr/bin/env python3
"""Local screensaver host.

Serves this folder and, when the Ubuntu laptop exposes an IIO accelerometer,
publishes it at /api/imu. Binds to loopback only.
"""

from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import json
import math
import time

ROOT = Path(__file__).resolve().parent
PORT = 8765

TYPES = {
    ".html": "text/html; charset=utf-8",
    ".css": "text/css; charset=utf-8",
    ".js": "text/javascript; charset=utf-8",
    ".mjs": "text/javascript; charset=utf-8",
    ".json": "application/json; charset=utf-8",
    ".png": "image/png",
    ".svg": "image/svg+xml",
    ".glb": "model/gltf-binary",
    ".gltf": "model/gltf+json",
}


def read_accel():
    base = Path("/sys/bus/iio/devices")
    if not base.is_dir():
        return None
    for dev in sorted(base.glob("iio:device*")):
        if not (dev / "in_accel_x_raw").exists():
            continue

        def axis(name, device=dev):
            raw = float((device / f"in_accel_{name}_raw").read_text())
            scale_path = device / f"in_accel_{name}_scale"
            if not scale_path.exists():
                scale_path = device / "in_accel_scale"
            scale = float(scale_path.read_text()) if scale_path.exists() else 1.0
            return raw * scale

        try:
            ax, ay, az = axis("x"), axis("y"), axis("z")
        except (OSError, ValueError):
            continue
        mag = math.hypot(ax, ay, az)
        if 0.4 < mag < 2.5:
            ax, ay, az = ax * 9.81, ay * 9.81, az * 9.81
        return {"ax": ax, "ay": ay, "az": az, "device": dev.name}
    return None


class Handler(BaseHTTPRequestHandler):
    def log_message(self, fmt, *args):
        if self.path.startswith("/api/imu"):
            return
        super().log_message(fmt, *args)

    def do_GET(self):
        path = self.path.split("?", 1)[0]
        if path == "/api/imu":
            sample = read_accel()
            if sample is None:
                body = {"available": False, "t": time.time()}
            else:
                body = {"available": True, "t": time.time(), **sample}
            data = json.dumps(body).encode()
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Cache-Control", "no-store")
            self.send_header("Content-Length", str(len(data)))
            self.end_headers()
            self.wfile.write(data)
            return

        rel = "index.html" if path in ("", "/") else path.lstrip("/")
        file = (ROOT / rel).resolve()
        if not file.is_relative_to(ROOT) or not file.is_file():
            self.send_error(404)
            return
        data = file.read_bytes()
        self.send_response(200)
        self.send_header("Content-Type", TYPES.get(file.suffix, "application/octet-stream"))
        self.send_header("Cache-Control", "no-store")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)


def main():
    server = ThreadingHTTPServer(("127.0.0.1", PORT), Handler)
    print(f"screensaver http://127.0.0.1:{PORT}", flush=True)
    server.serve_forever()


if __name__ == "__main__":
    main()
