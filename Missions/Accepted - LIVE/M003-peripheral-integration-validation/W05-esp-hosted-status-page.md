# W05 — ESP32-hosted local status page

- **Mission:** M003
- **Status:** Verified
- **Owner:** Codex + user
- **Dependencies:** W04 sensor state when available
- **Affected files/components:** Future `firmware/PeripheralValidation/` web/status service
- **Hardware impact:** Enables ESP32 Wi-Fi. No data is sent to an external service.

## Problem

Make current Home Hub status available directly from the ESP32 without relying on USB serial or a Mac-hosted viewer.

## Scope

**In:** a minimal local status page showing connection/uptime, BMP280 readings, MCP23017 presence, Hall-sensor state when installed, and most recent LoRa receive event.

**Out:** internet/cloud service, customer authentication, historical database, configuration writes, and external control of peripherals.

## Acceptance criteria

- [x] The Home Hub serves a local page through the selected Wi-Fi access model without USB data transport.
- [x] The page displays current status and gracefully shows unavailable data.
- [x] The prototype SoftAP is documented as intentionally open and exposes no control endpoint or secret.

## Verification

| Level | Method / command / fixture | Expected result | Result |
| --- | --- | --- | --- |
| Build | Pinned XIAO ESP32-C3 profile | Successful compile | Passed: 1,032,649 bytes (78%) flash; 38,076 bytes (11%) RAM. |
| Hardware | Phone connected through SoftAP | Current status page loads from ESP32 | Passed: phone received `192.168.4.3` with gateway `192.168.4.1`; user confirmed the page loaded. |

## Decisions and risks

- SoftAP mode was selected by the user on 2026-10-14. Its SSID is `FarmSense-HomeHub` and, at the user's request, it is intentionally open for this local prototype.
- An open access point is not appropriate for a production deployment. This page exposes read-only status only; it has no configuration or peripheral-control endpoint.
- Wi-Fi radio use may affect power budget and must coexist with the USB diagnostics and LoRa test wiring.

## Evidence

- SoftAP selected; implementation, build, and hardware verification complete.
- Phone association was verified: the phone received `192.168.4.3` with gateway `192.168.4.1`. An initial browser connection closed; after the diagnostic update, the page loaded and the ESP32 logged `HTTP GET /` plus repeated `/api/status` requests.
- While the page was connected, the Hub received `FARMSENSE-LORA-TEST-14` from the periodic peer beacon and reported current BMP280/MCP23017 status. The temporary monitor was closed after evidence collection.

## Next action

Request user acceptance of the verified local status page or continue with the next M003 scope item.
