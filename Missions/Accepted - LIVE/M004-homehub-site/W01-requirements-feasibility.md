# W01 — Requirements baseline and device feasibility

- **Mission:** M004
- **Status:** Accepted
- **Owner:** Codex + user
- **Dependencies:** M003 accepted
- **Affected files/components:** `Home_hub/Home Hub - FAQ.md`; M004 mission records; `firmware/PeripheralValidation/`
- **Hardware impact:** None; source and build-resource inspection only

## Problem

The current FAQ and status-page prototype do not fully define the customer sensor-management site, while radio protocol and on-device storage limits affect its implementation.

## Scope

**In:** Add the agreed site behavior to the FAQ; inspect current network, sensor-message, persistence, and build setup; document feasibility constraints and unresolved choices for subsequent work.

**Out:** Changing LoRa protocol, implementing customer UI or authentication, allocating flash partitions, connecting to a customer router, downloading map data, or operating hardware.

## Acceptance criteria

- [x] FAQ covers local sign-in, sensor discovery and ID matching, claiming, setup completion, sensor list/detail/edit behavior, and list/map organization.
- [x] FAQ states Home Hub Wi-Fi remains available, router connection is optional, map locations require download over customer Wi-Fi, and downloaded locations work offline.
- [x] Record current software interfaces, protocol gaps, build size, persistence options, AP/STA constraints, and map-cache feasibility for W02 onward.
- [x] Review FAQ wording against the earlier customer requirements and existing Home Hub network baseline.

## Verification

| Level | Method / command / fixture | Expected result | Result |
| --- | --- | --- | --- |
| Static | Manual review of FAQ diff and firmware routes/state | Requirements represented without changing hardware facts or protocol assumptions | Passed; FAQ now covers local sign-in, sensor discovery/claim/setup, list/detail/edit, optional router join, and downloaded maps available offline. Map source is constrained to one that permits offline downloads and must show attribution. |
| Build | `/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli --config-file .arduino/arduino-cli.yaml compile --fqbn esp32:esp32:XIAO_ESP32C3 --build-path .build/M004-W01 firmware/PeripheralValidation` | Baseline compiles and reports flash/RAM use for sizing | Passed with ESP32 core 3.3.12: 1,040,787 bytes (79% of 1,310,720-byte app slot); 38,556 bytes static RAM (11% of 327,680 bytes). |

## Decisions and risks

- The user's earlier interface requirements are supplemental input because they were not yet present in the checked-in FAQ.
- Existing prototype runs `WIFI_AP` only; optional router connectivity while preserving SoftAP requires AP+STA validation.
- Existing preferences store one Gate Sensor configuration, not a sensor registry.
- No filesystem partition or generic setup descriptor is currently defined.
- The declared XIAO profile uses the ESP32 core's `default` partition: two 1.25 MiB OTA app slots and 1.5 MiB SPIFFS. The existing sketch is already 79% of the app-slot limit. Keep UI assets compact and map cache bounded; validate final app size against the pinned OTA profile.
- `WIFI_AP_STA` exists in the pinned core headers, but simultaneous Home Hub SoftAP + router operation still needs hardware validation.
- The public OpenStreetMap tile server does not permit prefetching/offline area downloads ([policy](https://operations.osmfoundation.org/policies/tiles/)). Offline use needs a separately approved map source; current MapTiler documentation indicates API bulk downloads require a Custom plan and its on-prem free package is restricted to non-commercial/evaluation use. Provider decision remains open.

## Evidence

- **Acceptance, 2026-10-26:** accepted as part of M004 after the user confirmed live operation. Earlier pending checks and map work below are historical; final scope, user-reported validation, deferred items, firmware identity and rollback are recorded in Mission.md.

- FAQ was updated in the customer setup, sensor discovery/setup, and sensor organization sections, including the requirement that the chosen open map source permit offline downloads and display attribution.
- Initial source review found `/api/status`, diagnostic Gate Sensor settings routes, `Preferences` storage, and `WiFi.mode(WIFI_AP)` in `PeripheralValidation`.

## Next action

Accepted with M004. No further implementation for this version; see Mission.md for release closeout and explicitly deferred work.
