# Farm Sense local serial dashboard

This local-only browser page owns one serial port and displays the device's USB output. It never sends serial commands and listens only on `127.0.0.1`.

## Setup

```sh
python3 -m venv tools/serial-dashboard/.venv
tools/serial-dashboard/.venv/bin/pip install -r tools/serial-dashboard/requirements.txt
```

## Run

Close Arduino IDE Serial Monitor and any other monitor first. Then run:

```sh
tools/serial-dashboard/.venv/bin/python tools/serial-dashboard/serial_dashboard.py \
  --port /dev/cu.usbmodem11401
```

Open `http://127.0.0.1:8765` in a browser. Stop the dashboard with `Ctrl-C` before uploading firmware or using another serial tool.

The port is intentionally supplied at launch rather than committed as a default because serial-port names are machine-specific.
