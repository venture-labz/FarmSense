# W04 — Authenticated sensor list, detail, and editing

- **Mission:** M004
- **Status:** Accepted
- **Owner:** Codex + user
- **Dependencies:** W03 Verified
- **Affected files/components:** `firmware/HomeHub/src/HomeHubWeb.cpp`; HomeHub sensor registry APIs
- **Hardware impact:** None for implementation/build; no upload

## Problem

Authenticated customers can create an account and the firmware can collect sensor records, but the page does not yet let customers manage or inspect their network.

## Scope

**In:** Responsive list and focused sensor detail; current received values and last-seen/awaiting states; search, sort, and grouping by customer title or location; editing name, alarm enablement, grouping, location, and map coordinates; manual ID wait; claim, ignore, restore; bounded input validation and save feedback.

**Out:** Historical charts, cloud access, map base-layer rendering/download, or configuring unsupported sensor-specific calculations.

## Acceptance criteria

- [x] Authenticated page shows owned, unclaimed, ignored, and waiting sensor states; unauthenticated APIs are rejected.
- [x] Manual ID entry creates a waiting record; reports show descriptor and current data; claim is explicit.
- [x] List supports search, name/ID/last-seen sorting, and grouping by location or customer title.
- [x] Sensor detail shows available data, description, setup and last-seen state, with editable properties.
- [x] Name, alarms, grouping, location, and coordinates are bounded/validated and persisted with response feedback.
- [x] Empty inventory, no reading, full registry, and storage-unavailable messages exist. Stale is represented by last-report age (no threshold policy chosen).
- [x] UI uses no external CDN; sensor text is inserted using textContent and JSON strings are escaped.
- [x] HomeHub profile compiles within OTA slot; browser and power-cycle checks are pending hardware/page access.

## Verification

| Level | Method / command / fixture | Expected result | Result |
| --- | --- | --- | --- |
| Static | Review route authentication, JSON encoding, browser input/output, and bounded fields | No unescaped untrusted text or unprotected mutations | Passed source review; protected edit route and JSON field encoding present |
| Build | Project-scoped Arduino CLI HomeHub build | Successful pinned-profile compile within OTA slot | Passed: latest UX build 1,044,589 bytes (79% app slot); 46,100 bytes RAM (14%) |
| Logic | Inventory/action/edit input vectors and empty/stale/storage-full states | Valid changes persist; invalid changes fail without partial mutation | Source review only; no runtime harness currently exists |
| Browser | Phone/laptop on Home Hub Wi-Fi, account fixture, test sensor records | List/detail/edit flows render and interact correctly on a small screen | Existing flashed build inspected at `http://192.168.4.1`: signed-in state incorrectly showed sign-in; list/map toggles responded. Source fix compiled but still requires reflash and retest. No records were added. |

## Decisions and risks

- Support a bounded maximum of 32 sensor records; show a clear full-registry error.
- Only current values and last received uptime are shown; no historical retention is added.
- Generic alarms are stored as enabled/disabled until a sensor descriptor defines the meaning and range of its readings.
- Sign-in currently uses HTTP on an open SoftAP; this is not suitable for production until network/credential bootstrap security is resolved.

## Evidence

- **Acceptance, 2026-10-26:** accepted as part of M004 after the user confirmed live operation. Earlier pending checks and map work below are historical; final scope, user-reported validation, deferred items, firmware identity and rollback are recorded in Mission.md.

- Authenticated routes cover inventory, manual ID watch, claim/ignore/restore, and editing. Sensor properties and coordinates are saved to the versioned registry.
- User/radio content is rendered as text, and JSON output escapes control/quote characters. API responses set no-store headers.
- The browser check covers the current flashed page's signed-in visibility and view toggles only; sensor edit flows and persistence remain untested end to end.
- Follow-up browser inspection of the flashed M004 build reproduced the redundant sign-in form. Source now hides it for authenticated sessions, wires the Wi-Fi form submit, prevents credentials entering a query-string URL, and improves label associations, focus states, and mobile touch targets. The corrected build has not yet been flashed.
- Follow-up UX pass adds an upper-right hamburger menu, moves sign-out/navigation into that menu, places Add Sensor at the inventory heading and opens an ID-entry dialog, and gives Wi-Fi setup a dedicated screen using plain-language network-name/password prompts, show-password control, and connection/IP status. The Wi-Fi form sends POST data, not query-string credentials.
- Research basis: [W3C form-label guidance](https://www.w3.org/WAI/tutorials/forms/labels/), [W3C focus-visible guidance](https://www.w3.org/WAI/WCAG21/Understanding/focus-visible.html), [W3C status-message guidance](https://www.w3.org/WAI/WCAG22/Understanding/status-messages.html), and [Nielsen Norman Group's guidance on persistent visible labels](https://www.nngroup.com/articles/form-design-placeholders/).
- Final UI-layout build passed: 1,044,589 bytes (79% app slot), 46,100 bytes RAM (14%). New menu/dialog/Wi-Fi flows still need device/browser testing after an approved upload.

## Next action

Accepted with M004. No further implementation for this version; see Mission.md for release closeout and explicitly deferred work.
