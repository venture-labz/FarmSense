#!/usr/bin/env python3
"""Serve the embedded HomeHub UI locally with isolated, in-memory demo APIs."""

import argparse
import ast
import json
import re
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import parse_qs


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "firmware/HomeHub/src/HomeHubUi.h"
STARTED = time.monotonic()


def current_page():
    source = SOURCE.read_text(encoding="utf-8")
    match = re.search(r'R"HTML\((.*?)\)HTML";', source, re.S)
    if not match:
        raise RuntimeError("Could not find the embedded HomeHub page")
    page = match.group(1)
    banner = ('<aside style="padding:12px;text-align:center;background:#fff0c5">'
              'Local preview · sample data only · Wi-Fi setup disabled'
              '<label for="previewNetwork">Simulate Wi-Fi status</label>'
              '<select id="previewNetwork">' +
              ''.join('<option>' + state + '</option>' for state in
                      ('disconnected', 'connecting', 'connected', 'not_found', 'failed', 'timeout')) +
              '</select><button id="previewReport">Simulate incoming sensor report</button></aside>')
    guard = """<script>document.getElementById('wifi').addEventListener('submit',e=>{
      e.preventDefault();e.stopImmediatePropagation();
      document.getElementById('wifiMessage').textContent='Preview only. No Wi-Fi details were sent or saved.';
      e.target.reset();
    },true);
    document.getElementById('previewNetwork').onchange=async e=>{
      await fetch('/__preview/network/'+e.target.value);refresh();
    };
    document.getElementById('previewReport').onclick=async()=>{
      await fetch('/__preview/report');refresh();
    };</script>"""
    return page.replace("<body>", "<body>" + banner, 1).replace("</body>", guard + "</body>", 1)


SENSORS = [
    {
        "id": "DEMO-SOIL-03", "type": "Soil sensor", "name": "South plot",
        "description": "Demo claimed sensor · setup complete", "grouping": "Farm 1",
        "location": "", "primary_label": "Soil moisture", "primary_value": "42%",
        "latitude_e6": 0, "longitude_e6": 0, "last_seen_ms": 0, "flags": 21,
    },
    {
        "id": "DEMO-GATE-01", "type": "Soil sensor", "name": "North field",
        "description": "Demo sensor · setup complete", "grouping": "Farm 1",
        "location": "North field", "primary_label": "Soil moisture",
        "primary_value": "31%", "latitude_e6": 43000000, "longitude_e6": -93000000,
        "last_seen_ms": 0, "flags": 85,
    },
    {
        "id": "DEMO-AIR-02", "type": "Weather sensor", "name": "Barn weather",
        "description": "Demo sensor · available to claim", "grouping": "Farm 1",
        "location": "Barn", "primary_label": "Temperature",
        "primary_value": "22.4 °C", "latitude_e6": 0, "longitude_e6": 0,
        "last_seen_ms": 0, "flags": 16,
    },
    {
        "id": "DEMO-PENDING", "type": "Sensor", "name": "DEMO-PENDING",
        "description": "Waiting for this sensor to report", "grouping": "",
        "location": "", "primary_label": "", "primary_value": "",
        "latitude_e6": 0, "longitude_e6": 0, "last_seen_ms": 0, "flags": 8,
    },
]
SIGNED_IN = True
EVENTS = []
LAST_EVENT = -1
NETWORK_STATE = "disconnected"


class Handler(BaseHTTPRequestHandler):
    server_version = "HomeHubLocalPreview/1.0"

    def log_message(self, _format, *_args):
        # Avoid logging request bodies or other user-entered values.
        pass

    def respond(self, payload, status=200, content_type="application/json"):
        body = payload.encode("utf-8") if isinstance(payload, str) else json.dumps(payload).encode()
        self.send_response(status)
        self.send_header("Content-Type", content_type + ("; charset=utf-8" if content_type.startswith("text/") else ""))
        self.send_header("Cache-Control", "no-store")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        global LAST_EVENT, NETWORK_STATE
        if self.path == "/" or self.path == "/index.html":
            self.respond(current_page(), content_type="text/html")
        elif self.path == "/api/auth/state":
            self.respond({"account": True, "signed_in": SIGNED_IN, "storage_ready": True})
        elif self.path == "/api/network/state":
            self.respond({"state": NETWORK_STATE, "connected": NETWORK_STATE == "connected",
                          "connecting": NETWORK_STATE == "connecting",
                          "ip": "192.0.2.1" if NETWORK_STATE == "connected" else ""})
        elif self.path.startswith("/__preview/network/"):
            state = self.path.rsplit("/", 1)[-1]
            if state in ("disconnected", "connecting", "connected", "not_found", "failed", "timeout"):
                NETWORK_STATE = state
            self.respond({"ok": True})
        elif self.path == "/__preview/report":
            for sensor in SENSORS:
                if sensor["flags"] & 1:
                    sensor["flags"] &= ~8
                    sensor["last_seen_ms"] = int((time.monotonic() - STARTED) * 1000)
                    if not sensor["primary_value"]:
                        sensor["primary_label"] = "Gate state"
                        sensor["primary_value"] = "OPEN"
            self.respond({"ok": True})
        elif self.path == "/api/sensors":
            elapsed = int((time.monotonic() - STARTED) * 1000)
            self.respond({"sensors": [{**sensor, "last_seen_ms": max(0, elapsed - 8000)} if sensor["flags"] & 16 else sensor for sensor in SENSORS],
                          "home_hub": {"ready": True, "temperature_c": 24.3,
                                       "pressure_hpa": 984.6, "last_seen_ms": elapsed},
                          "uptime_ms": elapsed})
        elif self.path == "/api/events":
            if not SIGNED_IN:
                self.respond({"error": "Sign in required"}, 401)
                return
            elapsed = int((time.monotonic() - STARTED) * 1000)
            sequence = elapsed // 2500
            if sequence != LAST_EVENT:
                LAST_EVENT = sequence
                sensor = SENSORS[sequence % 3]
                EVENTS.append({"sequence": sequence, "received_ms": elapsed,
                               "sensor_id": sensor["id"],
                               "message": sensor["primary_label"] + ": " + sensor["primary_value"]})
                del EVENTS[:-32]
            self.respond({"events": EVENTS, "uptime_ms": elapsed})
        else:
            self.respond({"error": "Not found"}, 404)

    def do_POST(self):
        global SIGNED_IN
        length = min(int(self.headers.get("Content-Length", "0")), 8192)
        path = self.path
        if path in ("/api/account", "/api/login", "/api/logout"):
            SIGNED_IN = path != "/api/logout"
            self.respond({"ok": True})
        elif path == "/api/network/wifi":
            # Do not read or parse Wi-Fi form data in the local preview.
            self.respond({"error": "Wi-Fi setup is disabled in the local preview"}, 400)
        else:
            fields = {key: values[-1] for key, values in parse_qs(self.rfile.read(length).decode("utf-8", "replace")).items()}
            self.handle_demo_post(path, fields)

    def handle_demo_post(self, path, fields):
        if path == "/api/sensor/wait":
            sensor_id = fields.get("id", "")[:24]
            if not re.fullmatch(r"[A-Za-z0-9_-]+", sensor_id):
                self.respond({"error": "Enter a valid sensor ID"}, 400)
                return
            existing = next((sensor for sensor in SENSORS if sensor["id"] == sensor_id), None)
            if existing and existing["flags"] & 1:
                self.respond({"error": "Sensor is already claimed"}, 409)
                return
            if existing:
                existing["flags"] = (existing["flags"] | 1) & ~(2 | 4)
            else:
                SENSORS.append({"id": sensor_id, "type": "Sensor", "name": sensor_id,
                                "description": "Demo · waiting for a report", "grouping": "",
                                "location": "", "primary_label": "", "primary_value": "",
                                "latitude_e6": 0, "longitude_e6": 0, "last_seen_ms": 0, "flags": 9})
            self.respond({"ok": True}, 202)
        elif path == "/api/sensor/action":
            sensor = next((item for item in SENSORS if item["id"] == fields.get("id")), None)
            if sensor:
                action = fields.get("action")
                sensor["flags"] = {"claim": (sensor["flags"] | 1) & ~(2 | 4), "ignore": sensor["flags"] | 2, "restore": sensor["flags"] & ~2}.get(action, sensor["flags"])
            self.respond({"ok": bool(sensor)})
        elif path == "/api/sensor/edit":
            sensor = next((item for item in SENSORS if item["id"] == fields.get("id")), None)
            if sensor:
                for key in ("name", "grouping", "location"):
                    if key in fields:
                        sensor[key] = fields[key]
                if fields.get("latitude") and fields.get("longitude"):
                    sensor["latitude_e6"] = int(float(fields["latitude"]) * 1_000_000)
                    sensor["longitude_e6"] = int(float(fields["longitude"]) * 1_000_000)
                    sensor["flags"] |= 64
                else:
                    sensor["latitude_e6"] = 0
                    sensor["longitude_e6"] = 0
                    sensor["flags"] &= ~64
                sensor["flags"] = sensor["flags"] | 32 if fields.get("alarm_enabled") == "on" else sensor["flags"] & ~32
            self.respond({"ok": bool(sensor)})
        else:
            self.respond({"error": "Not found"}, 404)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", type=int, default=8765)
    args = parser.parse_args()
    server = ThreadingHTTPServer(("127.0.0.1", args.port), Handler)
    print(f"HomeHub local preview: http://127.0.0.1:{args.port}/ (mock data; loopback only)", flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
