# W05 — Mini Home Hub network access and Live data

- **Mission:** M004
- **Status:** Accepted
- **Owner:** Codex + user
- **Dependencies:** W04 source/build verified
- **Affected files/components:** M004 records, `firmware/HomeHub/`, `tools/homehub-preview.py`, `tools/test-homehub-events.py`
- **Hardware impact:** Prior Mini uploads recorded below. Built-in BMP280 integration needs a newly approved upload; no hardware action performed for this increment.

## Problem

The Mini must remain reachable locally while joining a customer router for future cloud forwarding. The user has moved maps to the cloud because of limited firmware headroom. The local site must retain sensor management and Wi-Fi setup, remove map functionality, and provide a readable stream of incoming data. The Mini implementation now removes local maps and adds a bounded Live data feed; hardware validation remains.

## Scope

**In:** AP+STA mode; authenticated customer-friendly router setup; separate Wi-Fi/cloud status; retained sign-in, sensor tiles/details, discovery/claiming, manual IDs, setup acknowledgement, property editing, ignore/restore, search/sort/grouping; removal of local map UI/assets/settings/downloads; bounded signed-in Live data with sensor filtering and display pause/resume; offline local operation; cloud relay requirements.

**Out:** Local maps or tile services; cloud application/maps implementation; cloud transport implementation before its contract is designed; repeater behavior; raw packet console; permanent event history; outage queue/replay guarantees; exposing credentials in logs/UI responses.

## Acceptance criteria

- [x] Home Hub starts in AP+STA mode and retains its AP when configured to join a router.
- [x] Authenticated settings accept bounded SSID/password and store them locally without echoing the password.
- [x] Connection feedback implemented and preview-tested; user believes Wi-Fi works and defers further router checks for this version.
- [x] FAQ and mission reflect the Mini/cloud split and preserve historical map evidence as superseded work.
- [x] Local map navigation, assets, tile settings, and downloads are removed; editable sensor location information remains.
- [x] Local authentication UI and sensor management flows pass synthetic preview checks; real authentication/NVS persistence remains a hardware check.
- [x] Live data shows incoming readings and setup/status events with sensor names and receipt time, or elapsed time if the clock is unavailable.
- [x] Feed retention is bounded and verified against memory limits; automatic updates, sensor filtering, and display pause/resume work without stopping reception/forwarding.
- [x] Local monitoring has no cloud dependency; user confirms signal-loss feedback and recovery after login. Further outage checks deferred for this version.
- [x] Cloud requirements resume forwarding new readings on reconnection without promising missed-reading recovery; transport/authentication/provisioning design remains explicitly future work.
- [x] Exact pinned profile builds within the OTA app slot.

- [x] Live Home Hub temperature and pressure confirmed together on one built-in card after upload.

## Verification

Documentation revision: reviewed FAQ, mission, and W05 for conflicting map/offline/identical-screen requirements; `git diff --check` passed. Firmware checks below are prior evidence. Map-specific results are superseded and do not block Mini acceptance. Current Mini build and preview evidence follow; hardware validation remains pending.

| Level | Method / command / fixture | Expected result | Result |
| --- | --- | --- | --- |
| Static | Review AP+STA setup, authenticated route, bounds, and credential handling | AP remains configured; credentials are never returned/logged | Source review pending |
| Build | Project-scoped pinned HomeHub compile | Successful build within OTA app slot | Passed after online-map support: 1,073,147 bytes (81%); 46,100 bytes RAM (14%) |
| Logic | SSID/password validation and persisted settings | Empty/oversize inputs rejected; valid settings retained | Source review only |
| Hardware | Named HomeHub device, router fixture, and browser | AP remains accessible during STA connection and router address is reachable | Not run; upload/device approval required |
| Map preview | Local synthetic XYZ tile server and browser | Map loads, demo sensor placement updates the mock API, bounded tiles can be downloaded and read offline | Passed: 1 km download saved 62 tiles (25.4 KiB); with the mock server offline, all 16 visible tiles at zoom 16 loaded from browser cache. Zoom 17 is outside the downloaded 12–16 levels and does not load, as expected. Public `tile.openstreetmap.org` source was rejected before tile requests. No browser console errors |
| Real map | Configured self-hosted tile endpoint; browser restart and offline readback | Real source attribution, terms, bounded download, and same-device offline readback pass | Pending endpoint/source terms; actual geographic tiles not tested |

## Decisions and risks

### Current direction — 2026-10-22

- User selected readable sensor events and local monitoring only during internet outages. No raw packet view or outage queue is required.
- Mini retains its local account and AP; cloud forwarding requires internet and cloud configuration. Wi-Fi association is not proof of cloud reachability.
- Latest build uses 81% of the app slot. Preserve firmware headroom for gateway duties; maps belong to cloud scope. Cloud contracts and Live data retention limits need implementation design and validation.
- Mini UI and event feed are implemented; map code is removed. No hardware operation was performed.

### Superseded map decisions (historical)

- User selected a self-hosted OpenStreetMap-derived tile source. No endpoint, attribution text, hosting terms, or operating capacity was provided.
- Standard `tile.openstreetmap.org` is now the default for interactive online viewing only. It is never used for user-area downloads or the app's IndexedDB cache; those controls are hidden for that source. The browser loads it as ordinary image requests so default HTTP caching applies. If `navigator.onLine` is false or tile requests fail, the map explains that internet is required.
- Do not prefetch from `tile.openstreetmap.org`; its [public tile policy](https://operations.osmfoundation.org/policies/tiles/) disallows offline use. Availability is best-effort; production usage should monitor policy and consider a suitable hosted/self-hosted provider. Any customer-downloadable source must explicitly permit it.
- The default 1.5 MiB SPIFFS is too small for a meaningful map cache. Per user decision, browser/device-local storage is the interim cache until Home Hub hardware is upgraded with more memory. Each phone/browser needs its own downloaded area. The UI stores up to 40 MiB of tiles per browser using IndexedDB/LRU and exposes clear controls; actual browser quota/restart behavior still needs validation at the Home Hub local address.
- The map engine is implemented without new firmware or JS dependencies. It uses a configurable XYZ raster endpoint and coordinate centering; address search/geocoding is not implemented because no geocoder endpoint was specified.
### Continuing release risk

- Router credentials are stored in local NVS and submitted over the current HTTP interface. This is a prototype, not secure provisioning; do not release until local network/transport security is addressed.

## Evidence

- **Acceptance, 2026-10-26:** accepted as part of M004 after the user confirmed live operation. Earlier pending checks and map work below are historical; final scope, user-reported validation, deferred items, firmware identity and rollback are recorded in Mission.md.

### Current Mini verification

- 2026-10-26 approved upload: combined Home Hub environment card, source `acffcb0`, artifact `.build/M004-homehub-environment`, Seeed XIAO ESP32-C3 on `/dev/cu.usbmodem11401`. IDE-bundled CLI with project config and declared FQBN exited 0; all four written sections passed hash verification and the board reset. No account reset/full-chip erase or gate-sensor operation. Post-reset HTTP check to `192.168.4.1` timed out; user must reconnect and confirm live temperature/pressure. Upload success is not a sensor hardware pass.

- 2026-10-26 continuation: BMP280 and existing ownership/network/event host tests passed again; `git diff --check` passed. Built artifact remains available. No USB device port is currently present; previous `/dev/cu.usbmodem11401` is absent. No upload attempted. Initial synthetic browser snapshot confirmed the combined card; a later optional console/mark-deliverable check was not executed because automatic approval review reached its usage limit.

- User acceptance guidance: no gate calibration needed; existing gate ID acceptable; settings retained across restart; loss of signal displayed and login restored sensor visibility; gate OPEN/CLOSED accurate. User believes Wi-Fi works, defers phone-layout validation and minor UIUX refinements. Treat as user-reported acceptance of these areas, not proof of all previously planned hardware cases.
- Added `HomeHubEnvironment` using the earlier hardware-validated BMP280 driver: D5 SDA/D4 SCL, 0x76, requires chip ID 0x58; 50 ms I2C timeout; reads every 2 seconds, retries initialization every 10 seconds after failure. Failed readings become unavailable/null rather than stale current values. No new dependency or persisted sensor record.
- `/api/sensors` now includes `home_hub: {ready, temperature_c, pressure_hpa, last_seen_ms}`. One Home Hub card displays both values/units in My sensors and All visible sensors, with unavailable fallback. Built-in card has no claim/ignore/edit control and consumes no registry slot.
- `python3 tools/test-homehub-environment.py` passed compensation fixture, timing, disconnect/recovery, invalid sample and wrong-chip checks with undefined-behavior sanitizer. Existing `python3 tools/test-homehub-events.py` passed. Embedded JS syntax and `git diff --check` passed. Browser synthetic preview at port 8768 shows one Home Hub card with both readings. Live device values remain pending.
- Pinned build command: `/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli --config-file .arduino/arduino-cli.yaml compile --fqbn esp32:esp32:XIAO_ESP32C3 --build-path .build/M004-homehub-environment firmware/HomeHub` passed: 1,074,699 bytes flash (81%), 50,564 bytes static RAM (15%). No upload in this increment.

- User approved flashing sensor/Wi-Fi fixes: source `ed0f509`, artifact `.build/M004-W05-sensor-wifi`, Seeed XIAO ESP32-C3 on `/dev/cu.usbmodem11401`. IDE-bundled CLI upload with project config and pinned FQBN exited 0; all four written sections passed hash verification and board reset. Post-reset website at `http://192.168.4.1/` returned HTTP 200. No account reset/full-chip erase or gate-sensor operation. Real claiming persistence and router success/failure scenarios still require customer testing.

- Sensor/Wi-Fi regression fix: explicit Add Sensor now saves ownership independently of the transient awaiting-report flag. Incoming reports retain ownership and metadata. Discovered sensors can be claimed before descriptors arrive; setup remains pending until descriptor receipt and acknowledgement. Setup completion cannot be accepted without both ownership and a descriptor. Unclaimed-sensor banner and direct Claim buttons provide a visible discovery path; claiming returns to My sensors and clears search.
- Wi-Fi state API now reports connected/connecting/not_found/failed/timeout/disconnected, with a 30-second attempt deadline, startup/retry tracking, and recovery when connection succeeds later. UI gives specific guidance and a router-side link without returning network names/passwords or claiming internet connectivity. HomeHub SoftAP remains enabled while station settings change.
- Regression evidence: `python3 tools/test-homehub-events.py` passed ownership retention after reports, discovered/ignored sensor restoration without metadata loss, Wi-Fi outcomes/deadline, and event-buffer tests. Python AST and embedded JavaScript syntax checks passed. Synthetic browser at port 8768 verified added sensor remains under My sensors with OPEN after incoming report, prominent unclaimed discovery/claim flow, network-not-found guidance and connected confirmation/router link.
- Pinned compile to `.build/M004-W05-sensor-wifi` passed: 1,046,167 bytes flash (79%), 49,828 bytes static RAM (15%). These fixes have not been uploaded; actual router outcomes and gate persistence require live validation after scoped flash approval.

- User requested local account reset. Added service-only `HomeHubAccountRecovery` with pinned profile: removes only username/salt/password-hash keys and clears the account flag; no registry or router-key changes or credential reads. Recovery compile passed at 285,696 bytes flash and 13,400 bytes static RAM.
- On the approved Home Hub `/dev/cu.usbmodem11401`, uploaded recovery image, allowed its setup to run, then restored the verified `.build/M004-W05-mini` image. Both uploads exited successfully with hash verification and reset. Gate sensor untouched. Post-restoration auth-state checks timed out; account-reset completion still needs confirmation after the user reconnects to Hub Wi-Fi. No full-chip erase or raw credential/device logs.

- Approved hardware upload: Seeed XIAO ESP32-C3, `/dev/cu.usbmodem11401`, source `2b52a1c`, compiled artifact `.build/M004-W05-mini`. IDE-bundled Arduino CLI with project config and declared FQBN uploaded successfully; all written sections passed hash verification and the uploader reset the board. No full-chip erase requested; gate sensor firmware untouched.
- Post-upload network check: `http://192.168.4.1/` returned HTTP 200; auth-state endpoint reported account present, signed out, and storage ready. This verifies web availability and existing account presence, not password verification or sensor-registry persistence. Initial sandbox request timed out; approved direct network check succeeded. Fixture includes a user-provided magnetic gate sensor; actual LoRa readings and magnet transitions remain unverified pending local sign-in. No credentials, sensor identifiers, or raw device logs recorded.

- Replaced incremental page transformations with one embedded `HomeHubUi.h`, served from flash. No framework, CDN, map requests, or new packages. Preserved location fields; removed the superseded map header and mock tile routes. Historical map decisions/evidence below remain available; the untracked map header is no longer in the worktree.
- Added authenticated `GET /api/events`: `{events: [{sequence, received_ms, sensor_id, message}], uptime_ms}`. Holds the most recent 32 events in RAM (3,712 bytes on this target), evicts oldest entries, and clears on reboot. Radio readings, setup descriptors and setup acknowledgements create readable events. Polling is 2.5 seconds with an 8-second request timeout; pause freezes the display only. No disk history or cloud queue.
- `python3 tools/test-homehub-events.py` passed against the actual store header: capacity, ordering, eviction, receipt time, and text truncation. JavaScript `node --check` and Python AST parsing passed. `git diff --check` passed. Source search found no map implementation references in firmware/preview.
- `/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli --config-file .arduino/arduino-cli.yaml compile --fqbn esp32:esp32:XIAO_ESP32C3 --build-path .build/M004-W05-mini firmware/HomeHub` passed: 1,043,851 bytes flash (79% of 1,310,720), 49,828 bytes static RAM (15%). No installation or upload.
- Browser at http://127.0.0.1:8767/ with synthetic data: dashboard, navigation, event feed, pause/filter, Add Sensor, claim, edit/save, ignore/restore, Wi-Fi status and sign-out passed; draft edits survived background refresh; no console errors. Wi-Fi submission is disabled in preview. Real account creation/password verification, radio handshake, offline access from hardware, and NVS restart were not tested. Mobile CSS is implemented but device viewport/visual QA remains pending.
- Cloud status explicitly says unavailable; no invented endpoint or credentials. Non-finite sensor coordinates are rejected. Current sensor protocol exposes a primary reading; broader multi-reading sensor descriptors remain future protocol work.

### Earlier evidence

- Requirements-only revision updated FAQ, M004, and W05. Earlier map evidence below is retained for traceability; public OSM rejection testing predates the subsequent online-mode change. No new firmware build or hardware pass is claimed.
- `HomeHub.ino` starts in `WIFI_AP_STA`, configures the existing SoftAP, and reconnects using saved settings. Router secrets are not printed or returned by APIs.
- The protected `/api/network/wifi` route validates SSID/password lengths, stores them in NVS, and starts a STA connection without disabling the AP.
- The local preview injects the firmware's current embedded UI and serves synthetic XYZ tiles plus in-memory demo APIs on loopback. Preview-only controls can take the mock tile server offline/online. It does not contact a real map source or Home Hub. Browser testing confirmed map rendering, a sensor placement update, a 1 km cache download (62 tile requests, 25.4 KiB), offline cache readback at zoom 16 (16/16 visible tiles loaded), and rejection of the public OSM tile URL. No browser console errors.
- Added default online OSM display mode; its download controls are disabled/hidden and requests bypass the app's tile database. The browser preview verified the no-internet status and hidden offline-download controls without sending requests to OSM. Real OSM connectivity was not tested.
- Final pinned HomeHub compile command: `/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli --config-file .arduino/arduino-cli.yaml compile --fqbn esp32:esp32:XIAO_ESP32C3 --build-path .build/M004-W05-map firmware/HomeHub` — passed, 1,073,147 bytes flash (81% of 1,310,720-byte OTA slot), 46,100 bytes static RAM (14%).
- No router/hardware or real tile source was tested. Same-browser offline readback passed; a browser restart/quota lifecycle test remains pending.

## Next action

Accepted with M004. No further implementation for this version; see Mission.md for release closeout and explicitly deferred work.
