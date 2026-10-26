# W02 — Receive a live LoRa packet

- **Mission:** M003
- **Status:** Verified
- **Owner:** Codex + user
- **Dependencies:** W01 verified; a powered compatible peer with antenna
- **Affected files/components:** `firmware/PeripheralValidation/`, `firmware/LoraPeerTest/`
- **Hardware impact:** Radio reception only on the Home Hub; one approved broadcast transmission from the peer, using its existing stored radio configuration.

## Problem

Prove the Home Hub receives an actual over-the-air RYLR message rather than merely communicating with the RYLR module over UART.

## Scope

**In:** capture and record a valid RYLR `+RCV` message from a known peer after its UART link responds to `AT`; a test-only peer beacon sends a numbered broadcast once per 60 seconds.

**Out:** discovering arbitrary radio traffic, changing persistent radio settings, and transmitting from the Home Hub.

## Acceptance criteria

- [x] A valid `+RCV` line is received and displayed by the Home Hub.
- [x] The sender/receiver configuration basis and observed payload are recorded without credentials or device identifiers.

## Verification

| Level | Method / command / fixture | Expected result | Result |
| --- | --- | --- | --- |
| Hardware | Approved peer transmission; Home Hub local status page/diagnostic monitor | Valid `+RCV` result | Passed: user confirmed initial receipt; later diagnostic capture recorded `+RCV=0,22,FARMSENSE-LORA-TEST-14,...` from the 60-second beacon. |

## Decisions and risks

- The peer sends a single destination-`0` broadcast using its existing stored settings. It does not change radio frequency, network, address, power, parameters, encryption, or other persistent state.
- The peer beacon was extended at the user's request to broadcast once at startup and then every 60 seconds, still without changing stored radio configuration.
- Reception still requires compatible existing on-air configuration and applicable RF compliance.

## Evidence

- The second compatible XIAO ESP32-C3 was discovered on a separate USB serial port and flashed with `LoraPeerTest` after scoped approval.
- The earlier `LoraPeerTest` sent one broadcast packet, `FARMSENSE-LORA-TEST`, without changing any stored RYLR998 configuration.
- The user confirmed the Home Hub local serial dashboard displayed the received `+RCV` line on 2026-10-14.
- During the SoftAP status-page validation, the Home Hub recorded a later numbered peer beacon (`FARMSENSE-LORA-TEST-14`), verifying ongoing periodic reception.

## Next action

Verified. Restore or replace the peer's test-beacon firmware before using the peer for another purpose.
