# W02 — Persistent registry and local sign-in

- **Mission:** M004
- **Status:** Accepted
- **Owner:** Codex + user
- **Dependencies:** W01 Verified
- **Affected files/components:** New `firmware/HomeHub/` target; local storage and web session service
- **Hardware impact:** None for implementation/build; no upload

## Problem

The prototype stores only one gate sensor's settings and exposes test controls without authentication. The customer site needs a restart-safe sensor registry and local sign-in that does not depend on cloud access.

## Scope

**In:** Establish a dedicated HomeHub firmware target; implement versioned persistent records for sensors and local account credentials; provide first-run account setup, sign-in/out, and protected site/API access; keep session secrets in RAM and credential material out of logs and source.

**Out:** Router Wi-Fi provisioning, production sensor setup-packet ingestion, cloud accounts, password recovery via cloud, map storage, TLS, or hardware upload.

## Acceptance criteria

- [x] HomeHub sketch has a folder-matched `.ino`, pinned ESP32 FQBN/core, and reusable code under `src/`.
- [x] First run creates a local account; subsequent access requires sign-in; logout and restart invalidate the browser session.
- [x] Protected sensor-list route rejects missing/invalid sessions; account setup validates username and password bounds.
- [x] Account and sensor registry use local NVS storage and a versioned registry record; corrupt registry state is surfaced as unavailable rather than empty.
- [x] Passwords are stored as salted PBKDF2-HMAC-SHA256 verifiers, never plaintext; no credentials or session tokens are committed.
- [x] Exact declared profile compiles within the default OTA app slot.

## Verification

| Level | Method / command / fixture | Expected result | Result |
| --- | --- | --- | --- |
| Static | Inspect credential flow, auth guards, storage versioning, error paths | No plaintext password persistence/logging; protected operations consistently gated | Passed by source review; device-level account and NVS power-cycle validation pending |
| Build | `/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli --config-file .arduino/arduino-cli.yaml compile --fqbn esp32:esp32:XIAO_ESP32C3 --build-path .build/M004-W02 firmware/HomeHub` | Successful build within selected OTA app limit | Passed: 994,351 bytes (75% of 1,310,720 bytes); 45,788 bytes RAM (13%). |
| Logic | Source review of account bounds, PBKDF verifier comparison, session expiry, registry version/count/field validation | Invalid data rejected; valid records remain versioned | Passed source inspection; runtime vectors deferred to hardware/manual validation |

## Decisions and risks

- Local account works without a cloud account. The first-run bootstrap is on the local Home Hub interface; its physical proximity/security properties must be documented and reviewed.
- Current SoftAP and HTTP do not encrypt login or session traffic. Authentication is functional route protection, not production network security; do not release until provisioning or transport security is resolved.
- Keep direct Home Hub Wi-Fi behavior available; AP+STA configuration is W05.
- Default OTA app limit is 1,310,720 bytes and current baseline uses 1,040,787 bytes; keep assets compact and measure every step.
- Use no additional libraries unless separately approved and pinned.

## Evidence

- **Acceptance, 2026-10-26:** accepted as part of M004 after the user confirmed live operation. Earlier pending checks and map work below are historical; final scope, user-reported validation, deferred items, firmware identity and rollback are recorded in Mission.md.

- `firmware/HomeHub/` provides the pinned standalone target, local account setup/sign-in, expiring in-RAM session cookie, and versioned 32-record persistent registry foundation.
- New target compiles successfully with the pinned XIAO ESP32-C3 / ESP32 core 3.3.12 profile.

## Next action

Accepted with M004. No further implementation for this version; see Mission.md for release closeout and explicitly deferred work.
