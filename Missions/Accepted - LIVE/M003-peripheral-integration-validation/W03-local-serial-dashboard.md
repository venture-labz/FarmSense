# W03 — Local browser serial dashboard

- **Mission:** M003
- **Status:** Accepted
- **Owner:** Codex + user
- **Dependencies:** Python 3; pinned `pyserial`; explicit user-selected serial port
- **Affected files/components:** `tools/serial-dashboard/`
- **Hardware impact:** Opens the selected USB serial port read-only in application behavior; it must be stopped before firmware uploads.

## Problem

Provide an IDE-independent, local view of firmware serial output without competing serial monitors.

## Scope

**In:** a loopback-only page, live serial text stream, connection status, and clear/pause display controls.

**Out:** sending serial commands, remote access, data persistence, authentication, or firmware control.

## Acceptance criteria

- [x] Dashboard starts with an explicit port and is reachable only at `127.0.0.1`.
- [x] Live device lines appear in the browser event stream.
- [x] The dashboard sends no serial bytes.
- [x] Its ownership of the serial port is explicit; stop it with Ctrl-C before uploads.

## Verification

| Level | Method / command / fixture | Expected result | Result |
| --- | --- | --- | --- |
| Static | Review server/bus calls | No serial write operation; loopback-only bind | Passed: the code has no `write` call on the serial device and defaults to `127.0.0.1`. |
| Dependency | Isolated virtual environment | Exact `pyserial` version installed | Passed: `pyserial==3.5` installed in ignored `tools/serial-dashboard/.venv/`. |
| Local | Browser at `127.0.0.1:8765` with board connected | Live validation output visible | Passed: health/status endpoints returned 200 and live BMP280/MCP23017 events were received. |

## Decisions and risks

- `pyserial==3.5` is the sole dependency. The built-in Python HTTP server avoids a web framework and package-lock surface.
- A serial port cannot be shared safely; this dashboard deliberately owns it until stopped.

## Evidence

- `http://127.0.0.1:8765/healthz` returned `{"ok": true}`; status confirmed the selected port was connected at 115200 baud.
- The server-sent event stream delivered repeated BMP280 measurements and `MCP23017 at 0x20: DETECTED` lines from the running firmware.
- The local dashboard page was opened after validation. It remains running for the user and owns the selected serial port until stopped.
- User accepted this completed local dashboard work on 2026-10-14.

## Next action

Complete. Use the dashboard for local observation; stop it with Ctrl-C before any firmware upload, then use the explicit-port launch command in the README to restart it afterward.
