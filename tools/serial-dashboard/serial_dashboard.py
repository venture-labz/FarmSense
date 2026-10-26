#!/usr/bin/env python3
"""Local-only, read-only browser viewer for Farm Sense device serial output."""

import argparse
import json
import queue
import threading
import time
from http import HTTPStatus
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

import serial
from serial import SerialException


PAGE = """<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Farm Sense serial dashboard</title>
<style>
body{margin:0;background:#101714;color:#e6f2eb;font:15px ui-monospace,SFMono-Regular,Menlo,monospace}
main{max-width:1000px;margin:0 auto;padding:24px}h1{font:600 24px system-ui;margin:0 0 8px}
#status{color:#9ee2bd;margin:0 0 18px}.bar{display:flex;gap:10px;margin-bottom:10px}button{background:#263b31;color:#e6f2eb;border:1px solid #51715e;border-radius:5px;padding:7px 11px;cursor:pointer}
#log{height:70vh;overflow:auto;white-space:pre-wrap;background:#070b09;border:1px solid #294335;border-radius:7px;padding:14px;line-height:1.45}.rcv{color:#92d8ff}.err{color:#ffb5a9}.hint{color:#aabbb0;font-family:system-ui}
</style></head><body><main><h1>Farm Sense · serial dashboard</h1><p id="status">Connecting…</p>
<div class="bar"><button id="clear">Clear</button><button id="pause">Pause</button></div>
<div id="log" role="log" aria-live="polite"></div><p class="hint">Read-only: this page never sends serial commands. Close this dashboard before using another serial monitor or uploading firmware.</p>
<script>
const log=document.querySelector('#log'), status=document.querySelector('#status'); let paused=false;
function add(entry){if(paused)return; const line=document.createElement('div'); line.textContent=`${entry.time}  ${entry.text}`; if(entry.text.startsWith('+RCV')||entry.text.includes('received LoRa'))line.className='rcv'; if(entry.text.includes('failed')||entry.text.includes('NOT DETECTED'))line.className='err'; log.append(line); log.scrollTop=log.scrollHeight;}
document.querySelector('#clear').onclick=()=>log.replaceChildren(); document.querySelector('#pause').onclick=e=>{paused=!paused;e.target.textContent=paused?'Resume':'Pause'};
fetch('/api/status').then(r=>r.json()).then(s=>status.textContent=s.connected?`Connected · ${s.port} · ${s.baud} baud`:`Waiting for ${s.port}: ${s.error||'not connected'}`);
const stream=new EventSource('/events'); stream.onmessage=e=>add(JSON.parse(e.data)); stream.onerror=()=>status.textContent='Serial stream reconnecting…';
</script></main></body></html>"""


class SerialReader:
    def __init__(self, port: str, baud: int):
        self.port = port
        self.baud = baud
        self.events = queue.Queue(maxsize=500)
        self.connected = False
        self.error = "starting"
        self._stop = threading.Event()

    def publish(self, text: str) -> None:
        event = {"time": time.strftime("%H:%M:%S"), "text": text}
        try:
            self.events.put_nowait(event)
        except queue.Full:
            self.events.get_nowait()
            self.events.put_nowait(event)

    def run(self) -> None:
        while not self._stop.is_set():
            try:
                with serial.Serial(self.port, self.baud, timeout=0.25) as device:
                    self.connected = True
                    self.error = ""
                    self.publish(f"Connected to {self.port} at {self.baud} baud")
                    while not self._stop.is_set():
                        raw = device.readline()
                        if not raw:
                            continue
                        text = raw.decode("utf-8", errors="replace").strip()
                        if text:
                            self.publish(text)
            except (SerialException, OSError) as error:
                self.connected = False
                self.error = str(error)
                self.publish(f"Serial unavailable: {self.error}")
                self._stop.wait(1)

    def stop(self) -> None:
        self._stop.set()


def make_handler(reader: SerialReader):
    class Handler(BaseHTTPRequestHandler):
        def log_message(self, *_args):
            return

        def send_json(self, value):
            body = json.dumps(value).encode()
            self.send_response(HTTPStatus.OK)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)

        def do_GET(self):
            if self.path == "/":
                body = PAGE.encode()
                self.send_response(HTTPStatus.OK)
                self.send_header("Content-Type", "text/html; charset=utf-8")
                self.send_header("Content-Length", str(len(body)))
                self.end_headers()
                self.wfile.write(body)
            elif self.path == "/api/status":
                self.send_json({"connected": reader.connected, "error": reader.error,
                                "port": reader.port, "baud": reader.baud})
            elif self.path == "/healthz":
                self.send_json({"ok": True})
            elif self.path == "/events":
                self.send_response(HTTPStatus.OK)
                self.send_header("Content-Type", "text/event-stream")
                self.send_header("Cache-Control", "no-cache")
                self.send_header("Connection", "keep-alive")
                self.end_headers()
                try:
                    while True:
                        try:
                            event = reader.events.get(timeout=15)
                            payload = json.dumps(event)
                            self.wfile.write(f"data: {payload}\n\n".encode())
                        except queue.Empty:
                            self.wfile.write(b": keepalive\n\n")
                        self.wfile.flush()
                except (BrokenPipeError, ConnectionResetError):
                    return
            else:
                self.send_error(HTTPStatus.NOT_FOUND)

    return Handler


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True, help="USB serial port, for example /dev/cu.usbmodem11401")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--listen", default="127.0.0.1", help="loopback host (default: 127.0.0.1)")
    parser.add_argument("--http-port", type=int, default=8765)
    args = parser.parse_args()

    reader = SerialReader(args.port, args.baud)
    thread = threading.Thread(target=reader.run, daemon=True)
    thread.start()
    server = ThreadingHTTPServer((args.listen, args.http_port), make_handler(reader))
    print(f"Farm Sense serial dashboard: http://{args.listen}:{args.http_port}")
    print("The dashboard owns the serial port. Press Ctrl-C before uploading firmware.")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        reader.stop()
        server.server_close()


if __name__ == "__main__":
    main()
