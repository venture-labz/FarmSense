# Farm Sense — Mini Home Hub

M004 is an accepted ESP32-C3 prototype for local sensor monitoring. The Mini
receives LoRa gate reports, hosts a signed-in customer interface, and shows its
built-in BMP280 temperature and pressure together on a Home Hub card.

## Included

- Sensor discovery, claiming, manual ID addition, details and property editing.
- Persistent local account, sensor settings and optional router Wi-Fi settings.
- Home Hub Wi-Fi access point plus optional customer-router connection.
- A bounded Live data feed and clear Wi-Fi connection feedback.
- Built-in BMP280 readings, a synthetic local UI preview, and host regression tests.

Cloud forwarding and maps are not implemented in this version. Gate calibration,
unique production sensor IDs, additional mobile/UI polish, and expanded fault
testing are deferred. The existing diagnostic gate ID is accepted for this build.

## Prototype security

The local site uses HTTP and an open access point. Local sign-in does not encrypt
traffic. This source is published for development and evaluation, not as a
production-secure deployment. Do not expose the site directly to the internet.

## Targets and verification

The primary target is `firmware/HomeHub/`. Its `sketch.yaml` pins
`esp32:esp32:XIAO_ESP32C3` with Espressif Arduino core **3.3.12**. No third-party
sensor library is required. The validated BMP280 wiring is SDA=D5, SCL=D4,
address `0x76`, chip ID `0x58`; do not substitute a different sensor silently.

With Arduino CLI 1.5.1 and the pinned core already installed:

```sh
arduino-cli compile --fqbn esp32:esp32:XIAO_ESP32C3 --build-path .build/HomeHub firmware/HomeHub
python3 tools/test-homehub-environment.py
python3 tools/test-homehub-events.py
python3 tools/homehub-preview.py --port 8768
```

Host tests were validated on macOS with Python 3 and the system C++ compiler.
The preview runs at `http://127.0.0.1:8768/` using synthetic data; it does not
operate hardware or submit Wi-Fi credentials. Device uploads require explicit
approval for the board and port, and reset the board. Do not install the
service-only `HomeHubAccountRecovery` target as normal firmware.

## Accepted version

See [M004 acceptance and release record](Missions/Accepted%20-%20LIVE/M004-homehub-site/Mission.md)
for exact build evidence, firmware checksum, user-confirmed operation, deferred
work and rollback guidance. The full development history remains local; public
publication snapshots exclude unrelated cloud-document pointers, Finder
metadata, credentials, logs and generated binaries.
