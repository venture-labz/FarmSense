# W03 — Sensor discovery and setup handshake

- **Mission:** M004
- **Status:** Accepted
- **Owner:** Codex + user
- **Dependencies:** W02 Verified
- **Affected files/components:** `firmware/HomeHub/src/`; `firmware/GateSensor/GateSensor.ino`
- **Hardware impact:** LoRa firmware changes; compile-only until separately approved board/port upload

## Problem

The current Home Hub prototype hard-codes one Gate Sensor and accepts only its live state, config acknowledgement, and event receipt. The site needs an inventory of unclaimed sensors, manual unique-ID matching, and setup-completion status understood by both Home Hub and sensor.

## Scope

**In:** Add a versioned minimal setup descriptor for the existing Gate Sensor example; parse LoRa setup and state packets into registry records; identify reporting unknown sensors as unclaimed; create manual ID watch entries; claim, ignore, restore, and report setup state; acknowledge pending/complete setup to the sensor; reject malformed or oversized messages safely.

**Out:** Declaring the Gate Sensor descriptor universal for future sensor types, changing radio baud/pins, writing persistent RYLR998 settings, configuring all sensor-specific alarms, or hardware upload without approval.

## Acceptance criteria

- [x] A reporting unknown sensor appears once as unclaimed; repeated setup/ALIVE packets update the same ID rather than creating duplicates.
- [x] Explicit Add Sensor saves ownership immediately and keeps the waiting record visible through matching reports; descriptor receipt and setup acknowledgement remain separate.
- [x] Claim/ignore/restore state persists and is returned only to authenticated users.
- [x] Home Hub sends explicit PENDING or COMPLETE setup status; Gate Sensor stores and reports whether setup is complete.
- [x] Live Gate Sensor values and last-seen state update without persisting every radio packet.
- [x] Malformed, truncated, unknown-version, duplicate, and over-capacity data is bounded and does not corrupt registry state.
- [x] HomeHub and GateSensor pinned profiles compile; hardware handshake validation remains pending.

## Verification

| Level | Method / command / fixture | Expected result | Result |
| --- | --- | --- | --- |
| Static | Review protocol parser lengths, IDs, validation, and duplicate handling | No unbounded field copies or malformed-packet state changes | Passed source review; runtime packet vectors and hardware validation remain outstanding. |
| Build | Pinned XIAO ESP32-C3 / core 3.3.12 profile for HomeHub and GateSensor | Both firmware targets compile | Passed: HomeHub 1,011,239 bytes (77% app; 46,100 bytes RAM); GateSensor 302,620 bytes (23% app; 13,464 bytes RAM). |
| Hardware | Matching XIAO ESP32-C3 Home Hub and Gate Sensor, existing LoRa wiring | Setup state moves pending→complete after claim and sensor acknowledges | Not run; requires separate named board/port approval. |

## Decisions and risks

- Define a minimal `FS2` setup message contract in this work item and record it here before relying on it. Its descriptor fields are for the Gate Sensor example and must not imply all sensor types share the same measurements.
- Current Gate Sensor ID `gate-01` is a diagnostic identifier and is not the production serial-ID scheme from the FAQ.
- Setup-completion acknowledgement confirms Home Hub has claimed/accepted the descriptor; it does not certify sensor calibration or alarm configuration.
- Do not log setup packet payloads containing future customer metadata by default.
- Protocol v1 used by this prototype: sensor advertises `FS2|SETUP|id|1|type|name|description|primary-label|allowed-values`; Home Hub replies `FS2|SETUP_STATUS|id|1|PENDING|COMPLETE`; sensor returns `FS2|SETUP_ACK|id|1|PENDING|COMPLETE`. Delimiters are literal and descriptor fields are restricted to printable ASCII without `|`.

## Evidence

- **Acceptance, 2026-10-26:** accepted as part of M004 after the user confirmed live operation. Earlier pending checks and map work below are historical; final scope, user-reported validation, deferred items, firmware identity and rollback are recorded in Mission.md.

- `HomeHubRadio` parses length-bounded `+RCV` lines, adds or updates registry records, preserves customer naming, updates live values in RAM, acknowledges events, and sends setup status to the reporting radio address.
- Gate Sensor advertises a Gate descriptor at boot and while incomplete, stores Home Hub completion status, and no longer prints full received radio lines.
- Both pinned targets compile successfully. Live RYLR998 handshake is not claimed without hardware evidence.

## Next action

Accepted with M004. No further implementation for this version; see Mission.md for release closeout and explicitly deferred work.
