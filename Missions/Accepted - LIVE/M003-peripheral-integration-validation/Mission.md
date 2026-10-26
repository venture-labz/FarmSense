# M003 — Validate integrated local peripherals

- **Status:** Accepted
- **Owner:** Codex + user
- **Created:** 2026-10-14
- **Product references:** `Home_hub/Home Hub - FAQ.md` (pin map, BMP280, MCP23017, RYLR998)

## Problem

Validate live environmental sensing, the I/O-expander bus presence, and the Home Hub LoRa receive path with a visible transaction indicator.

## Scope

**In:** BMP280 temperature/pressure reads at `0x76`; MCP23017 acknowledgement at `0x20`; RYLR998 UART `AT` health check and passive `+RCV` packet listener; D1 50%-idle/80%-activity indication; a digital Hall-effect sensor input; and an ESP32-hosted local status page that does not depend on USB serial data.

**Out:** persistent RYLR998 radio configuration changes, MCP23017 GPIO writes,
cloud/application networking, and edits to `Home_hub/`.

## Acceptance criteria

- [x] The firmware builds with the pinned XIAO ESP32-C3 profile.
- [x] BMP280 chip identity and repeated compensated temperature/pressure readings are visible over USB serial.
- [x] MCP23017 continues to acknowledge at `0x20`.
- [x] RYLR998 answers the non-persistent `AT` health command and forwards an actual received `+RCV` packet from a peer.
- [x] D1 is visibly 50% at idle and briefly reaches 80% for successful transactions.
- [x] The installed digital Hall-effect sensor produces stable, documented active/inactive input events.
- [x] The Home Hub serves current local status over Wi-Fi without requiring a USB data connection.

## Work plan

| ID | Outcome | Status | Evidence |
| --- | --- | --- | --- |
| W01 | Compile and run integrated sensor/UART listener | Verified | Build, BMP280, MCP23017, RYLR UART, and corrected D1 observation passed |
| W02 | Receive live LoRa packets from a matching peer | Verified | Initial receive accepted; later numbered 60-second beacon received |
| W03 | Local browser serial dashboard | Accepted | Local live stream verified; user accepted |
| W04 | Validate digital Hall-effect sensor input | Accepted | Magnet transitions, fixed polarity, LoRa event transport, and Home Hub display validated; production sleep and latency characterization deferred |
| W05 | ESP32-hosted local status page | Verified | SoftAP page loaded from phone; live status requests observed |

## Decisions and risks

- The RYLR998 listener leaves its stored settings untouched. It uses the documented 115200-baud default and only sends `AT`, which may wake a module that was temporarily sleeping but does not persist a configuration change.
- A valid `+RCV` line proves a packet was received with matching on-air parameters; it cannot be simulated by a local UART response.
- The BMP280's normal-mode setting is volatile and is required for fresh measurements. The driver uses Bosch's documented calibration/compensation flow.
- The Gate Sensor uses the confirmed DRV5032FALPGM at 3.3 V with OUT on D1/GPIO3.
  It is a push-pull digital device; magnetic thresholds are fixed in hardware.
- Gate configuration is application-level, versioned LoRa traffic. It does not alter
  persistent RYLR998 settings. While the Home Hub SoftAP is intentionally open,
  settings controls are test-only and unauthenticated.
- The Home Hub is continuously powered and must never enter ESP sleep. Its SoftAP
  Wi-Fi radio sleep is disabled so the local page and LoRa receive loop remain
  continuously available. Battery-saving behavior belongs only on remote sensors.
- The ESP-hosted page must use an explicit Wi-Fi access model. An access-point page avoids Wi-Fi credentials; joining a home network requires user-provided untracked credentials.
- The user accepted M003 on 2026-10-20 after setup and diagnostic testing. Production low-power wake behavior and end-to-end latency characterization remain deferred follow-up work; they are not represented as a production performance pass.

## Evidence

- `PeripheralValidation` compiled with `esp32:esp32:XIAO_ESP32C3` and core 3.3.12: 322,576 bytes (24%) flash; 14,256 bytes (4%) RAM.
- An approved upload to the connected XIAO ESP32-C3 completed and verified successfully.
- USB serial at 115200 reported BMP280 chip ID `0x58`, followed by repeated compensated readings between 24.32–24.36 °C and 984.52–984.60 hPa.
- The same run repeatedly reported `MCP23017 at 0x20: DETECTED` and reported `RYLR998: +OK` followed by `UART health check passed`.
- No `+RCV` packet was observed during this local test. The serial monitor was closed after capture to free the port.
- A second compatible peer later sent one `FARMSENSE-LORA-TEST` broadcast using its existing settings. The user confirmed the Home Hub dashboard received it as `+RCV`.
- User confirmed the corrected D1 indicator remains functional. All M003 acceptance criteria are now met.
- M003 was reopened on 2026-10-14 to add the Hall-sensor input and ESP-hosted status-page scope.
- Gate Sensor diagnostic and low-power firmware compiled successfully with the
  pinned XIAO ESP32-C3 core. Hardware observations remain pending.
- The diagnostic Gate Sensor and updated Home Hub firmware were uploaded and
  flash-verified on 2026-10-15; only the controlled magnet/configuration tests
  remain for W04.
- The user confirmed repeated magnet-present and magnet-absent raw transitions.
  The confirmed fixed mapping is magnet detected/LOW (`0`) = CLOSED and magnet
  absent/HIGH (`1`) = OPEN. State changes were transmitted as LoRa events and
  displayed by the local Home Hub page during diagnostics.
- Timestamp experiments identified that independent device uptime clocks cannot
  directly measure one-way transport without synchronization. A diagnostic sync
  exchange was added for future latency investigation; no production latency
  claim is made by this mission.

## Next action

Mission accepted by the user. Archive as LIVE local record; do not publish until
the user separately authorizes a release review and push.
