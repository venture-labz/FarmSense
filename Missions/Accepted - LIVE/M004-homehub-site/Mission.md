# M004 — Deliver the customer-facing Home Hub site

- **Status:** Accepted
- **Owner:** Codex + user
- **Created:** 2026-10-22
- **Product references:** `Home_hub/Home Hub - FAQ.md`; M003 local status-page evidence

## Problem

The accepted M003 prototype serves a local status page, but customers need a manageable sensor network. The revised baseline positions the XIAO ESP32-C3 as the Mini Home Hub: a lightweight LoRa-to-cloud gateway with an authenticated local sensor interface. Limited firmware headroom makes the expanded local map feature set unsuitable for this product direction; maps now belong to the cloud version.

## Scope

**In:** Revised Mini requirements; local account setup and sign-in; persistent sensor inventory and properties; sensor setup/claim state; sensor tiles and focused views; customer-router Wi-Fi while Mini Wi-Fi remains available; readable bounded Live data feed; removal of local map functionality; cloud relay requirements and explicit future integration design; integrated verification on the pinned ESP32-C3 profile.

**Out:** Cloud application/maps implementation, cloud transport implementation before its contract is designed, remote access to the local site from the internet, repeater/coverage extension, local maps/downloads, long-term local history/charts, outage replay guarantees, production release or hardware upload without separate approval.

## Acceptance criteria

- [x] FAQ describes the Mini local interface, cloud-only maps, cloud relay direction, and disconnected behavior.
- [x] Customer can sign in locally without cloud access and manage an owned sensor registry that survives restart.
- [x] Discovery/claiming and setup state are implemented and preview-tested; user confirms gate visibility and accurate OPEN/CLOSED. Further handshake validation is deferred for this version.
- [x] Sensor list, focused detail, search, sorting, grouping, and editing show available received data and retain customer properties.
- [x] Mini navigation and assets contain no local map, tile-source settings, or map download feature; editable sensor locations remain.
- [x] Signed-in Live data shows bounded readable readings/setup/status events, sensor names, receipt or elapsed time, filtering, and pause/resume without stopping reception.
- [x] AP+STA and distinct Wi-Fi/cloud status implemented. User believes Wi-Fi is working and accepts it for this version; full router failure/coexistence testing is deferred.
- [x] Local operation has no cloud dependency; user reports loss-of-signal feedback, re-login and sensor recovery. Full outage testing deferred for this version; no cloud replay promised.
- [x] Exact declared firmware profile builds; all non-hardware flows have repeatable evidence; hardware validation is recorded only after separately approved device/port use.

- [x] Built-in temperature and pressure appear together on one Home Hub card and are confirmed on the device.

## Work plan

| ID | Outcome | Status | Evidence |
| --- | --- | --- | --- |
| W01 | Requirements baseline and device feasibility | Accepted | FAQ update, baseline build, network and storage constraints recorded |
| W02 | Persistent sensor registry and local sign-in | Accepted | HomeHub target builds; account/session routes and versioned registry foundation are present |
| W03 | Sensor discovery, manual ID matching, and setup handshake | Accepted | HomeHub and GateSensor compile; live radio handshake awaits approved upload |
| W04 | Authenticated list, detail, and sensor editing | Accepted | Authenticated routes and UI implemented; pinned HomeHub build passed |
| W05 | Mini network access, local scope transition, and Live data | Accepted | User accepts current gate/settings behavior and defers remaining UI/identity/calibration work; built-in Home Hub card uploaded and accepted |

## Decisions and risks

- The existing M003 prototype uses an open SoftAP and has unauthenticated test controls. M004 requires local sign-in; first-run credential bootstrap and recovery must be designed so an open network does not silently become production security.
- Gate firmware includes the versioned FS2 setup descriptor and acknowledgement. It is a minimal gate contract, not a universal sensor instruction schema. User accepts the existing gate ID and defers calibration/identity expansion for this version.
- The default XIAO ESP32-C3 partition provides two 1.25 MiB OTA app slots and 1.5 MiB SPIFFS. The current build with restored BMP280 support uses 81% of its app slot. This supports a limited-headroom finding, not a claim that hosting a website is impossible.
- The user superseded the local map/cache direction on 2026-10-22: maps move to the cloud; Mini keeps tiles/details, sensor management, local sign-in, Wi-Fi setup, and gains Live data. Local map code has now been removed; evidence below is historical, not current acceptance criteria.
- Cloud forwarding is a core product requirement, not an implemented capability. Transport, authentication, and provisioning need a future design before implementation. Local access remains independent of a cloud account. No queue or replay of readings missed during an outage is promised.
- Live data is a bounded readable event feed, not permanent history or raw packets. The feed is bounded to 32 events in RAM. Repeater operation remains a possible future hardware role only.
- The local account is an application-level gate over HTTP. The current open SoftAP and lack of TLS mean credentials and session cookies are not protected from a nearby network attacker; production provisioning/network security remains a release risk.
- Keep Home Hub SoftAP available while optionally joining the customer router (AP+STA behavior); do not implement Wi-Fi repeating.
- Existing worktree includes an unrelated untracked `HUMAN.md.md`; preserve it.

## Evidence

- 2026-10-26: user confirmed everything looks good and functions as expected, then explicitly accepted M004 as complete and authorized GitHub publication. This supersedes pending final-card/user-acceptance notes below; earlier evidence remains historical.

- 2026-10-26: user-approved upload of built-in temperature/pressure card (`acffcb0`) to Home Hub `/dev/cu.usbmodem11401` succeeded with four verified sections and reset. Post-reset HTTP check timed out; live card/readings and final acceptance remain pending user confirmation.

- User confirms settings retained after Mini restart, visible loss-of-signal feedback and re-login recovery, and accurate gate OPEN/CLOSED readings. User believes Wi-Fi works; treats further UI/phone-layout work, calibration, ID replacement and remaining validation as deferred for this version. This is user-reported evidence, not an instrumented router or handshake test.
- Restored verified BMP280 support (D5 SDA, D4 SCL, 0x76/chip 0x58) and added one built-in Home Hub card with temperature °C and pressure hPa. Current build: 1,074,699 bytes flash (81%), 50,564 bytes static RAM (15%). Driver/regression tests and synthetic card check pass; device confirmation pending.

- Approved upload of sensor/Wi-Fi fixes `ed0f509` to Home Hub `/dev/cu.usbmodem11401` passed hash verification and reset; website returned HTTP 200 afterward. Live sensor-claim and router outcome validation remain pending.

- Fixed the reported disappearing manually added sensor by persisting ownership independently of report-wait state; added prominent discovery and Claim controls. Added explicit Wi-Fi outcomes with a 30-second connection window. Host regression tests, synthetic browser checks, and pinned compile passed (1,046,167 bytes flash; 49,828 bytes static RAM). That build was subsequently uploaded successfully; W05 records evidence.

- User-approved Mini upload of `2b52a1c` to XIAO ESP32-C3 on `/dev/cu.usbmodem11401` passed all upload hash checks and reset. Live local website returned HTTP 200; existing account and storage-ready flags confirmed. Gate sensor and authenticated live-reading tests remain pending; no gate-sensor upload performed.

- Mini implementation: unified embedded UI with labeled forms, native modal focus handling, responsive layout, loading/error feedback, separate auth states, restore filter, and edit-preserving refresh. Map header and preview tile machinery removed. `/api/events` is session-protected and returns at most 32 chronological RAM events; the feed is not durable or a cloud outage queue.
- Current pinned build passes: 1,043,851 bytes flash (79%), 49,828 bytes static RAM (15%). Host event-buffer test and synthetic browser flows pass; detailed evidence is in W05. Cloud integration and hardware acceptance are not claimed.

- 2026-10-22 requirements revision: updated FAQ, mission, and W05 for Mini/cloud separation, readable Live data, and local-only outage operation. Documentation only; existing firmware and prior build evidence are unchanged.
- Earlier map-specific evidence below is retained as superseded development history. The initial public OSM rejection test predates the later online-OSM mode.
- M003 was accepted at `3551c42`; its status page runs on the XIAO ESP32-C3 SoftAP and exposes status plus diagnostic gate settings.
- Initial M004 work is on local branch `mission/M004-homehub-site`.
- W01 baseline compile used the installed Arduino CLI 1.5.1 with project `.arduino/` directories: `arduino-cli --config-file .arduino/arduino-cli.yaml compile --fqbn esp32:esp32:XIAO_ESP32C3 --build-path .build/M004-W01 firmware/PeripheralValidation` — passed, 1,040,787 bytes flash (79% of 1,310,720-byte app slot), 38,556 bytes static RAM (11% of 327,680 bytes).
- ESP32 core 3.3.12 exposes `WIFI_AP_STA`; the feasibility result is compile/API availability only, not verified coexistence on the device.
- W02 HomeHub build: 994,351 bytes (75% of the 1,310,720-byte OTA app slot), 45,788 bytes static RAM (13%). Build command: `/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli --config-file .arduino/arduino-cli.yaml compile --fqbn esp32:esp32:XIAO_ESP32C3 --build-path .build/M004-W02 firmware/HomeHub`.
- W02 introduced a 32-record versioned registry foundation and local sign-in with salted PBKDF2-HMAC-SHA256 verification. Device-level account and NVS power-cycle validation remain pending; account bootstrap uses the open SoftAP.
- Offline tiles from the public OSM tile server are prohibited by its [Tile Usage Policy](https://operations.osmfoundation.org/policies/tiles/). The user selected a self-hosted OSM-derived source, but did not provide its endpoint or attribution/download terms.
- W04 HomeHub build passed: 1,028,679 bytes (78% app slot), 46,100 bytes static RAM (14%). Authenticated site routes and list/detail/editor are implemented; browser and power-cycle validation are pending.
- W05 HomeHub build passed after online-map support: 1,073,147 bytes (81% app slot), 46,100 bytes static RAM (14%). AP+STA startup and protected router settings are implemented; coexistence has not been tested on hardware. Synthetic local preview testing confirmed map rendering, placement, 1 km download (62 tiles/25.4 KiB), offline cache readback at zoom 16 (16/16 visible tiles loaded), and internet-required messaging; real tiles remain pending.
- Router credentials are stored in local NVS and submitted over HTTP; the prototype remains unsuitable for production credential provisioning until transport/bootstrap security is addressed.

## Accepted release record

- **Version:** M004 Mini Home Hub prototype, accepted 2026-10-26.
- **Firmware source:** local immutable commit `acffcb0`; publication snapshot contains identical firmware content. Later commits record upload/acceptance only.
- **Target:** Seeed XIAO ESP32-C3; `esp32:esp32:XIAO_ESP32C3`, core 3.3.12.
- **Artifact:** `.build/M004-homehub-environment/HomeHub.ino.bin` (local, not published).
- **SHA-256:** `bf1388d575f57ea01d07defc3be816f796b46766aaf6987515c54decc7562407`.
- **Build:** 1,074,699 bytes flash; 50,564 bytes static RAM. Driver and ownership/network/event tests pass; upload verified; user confirms live operation, settings retention, gate state, and built-in readings.
- **Rollback:** with fresh device/port approval, restore prior verified `.build/M004-W05-sensor-wifi` HomeHub artifact from local source `ed0f509`; preserves current NVS layout but omits built-in readings. Do not use the account-recovery sketch for rollback. Retain local artifacts; rebuild with pinned core if needed.
- **Deferred:** cloud forwarding/maps, production provisioning/transport security, unique IDs, calibration, phone-layout review, minor UX refinements and expanded router/protocol fault testing. Existing gate ID accepted for this version. Published source is a prototype, not production certification.

## Next action

Publish the audited release branch and review its pull request. Merge into protected main only with explicit approval; no further M004 feature work remains.
