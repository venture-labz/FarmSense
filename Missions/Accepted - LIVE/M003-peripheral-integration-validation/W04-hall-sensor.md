# W04 — Validate digital Hall-effect sensor input

- **Mission:** M003
- **Status:** Accepted
- **Owner:** Codex + user
- **Dependencies:** Approved uploads to the Home Hub and Gate Sensor boards; controlled magnet test
- **Affected files/components:** `firmware/GateSensor/`, `firmware/PeripheralValidation/`
- **Hardware impact:** Reads one explicitly identified GPIO; no output drive is permitted on that GPIO.

## Problem

Prove the installed DRV5032FALPGM Hall-effect sensor reports stable gate transitions on the Home Hub.

## Scope

**In:** digital GPIO sampling, three-sample / 150 ms debounce, LoRa gate packets,
Home Hub raw-state display, CLOSED calibration, alarm enablement, and configurable
ALIVE reporting.

**Out:** analog field-strength measurement, software-adjustable magnetic thresholds,
physical alarm LEDs, cloud delivery, and persistent RYLR998 radio configuration.

## Acceptance criteria

- [x] Sensor model, supply voltage, output type, GPIO, and active polarity are recorded.
- [x] Repeated magnet-present and magnet-absent transitions are correctly observed.
- [x] The selected input pin is configured only as an input.
- [ ] Home Hub configuration delivery is deferred from this diagnostic acceptance.
- [ ] Low-power timer and Hall-level wakeups are deferred from this diagnostic acceptance.

## Verification

| Level | Method / command / fixture | Expected result | Result |
| --- | --- | --- | --- |
| Build | Pinned XIAO ESP32-C3 profile | Successful compile | Passed: Gate Sensor 299,982 bytes (22%); Home Hub binary produced |
| Hardware | Magnet installed/removed while diagnostic firmware runs | Stable raw transitions on serial and Home Hub page | Passed: user confirmed both physical states and Home Hub event display. |
| Configuration | Fixed LOW=CLOSED mapping and diagnostic ALIVE setting | Confirm behavior before production use | Deferred. |
| Low power | Production mode plus timer and Hall movement | ALIVE and changed-state wake reports | Deferred; diagnostic mode intentionally remains active for this mission closeout. |

## Decisions and risks

- The installed DRV5032FALPGM is a 20 Hz, omnipolar, push-pull digital Hall switch.
  It runs from 1.65–5.5 V, but is supplied at 3.3 V here so OUT is safe for the
  ESP32-C3. It cannot provide a magnetic-strength value or a user-adjustable
  threshold; calibration maps observed raw HIGH/LOW to CLOSED/OPEN.
- Sensor OUT is D1/GPIO3 on the sensor node. VCC is 3V3 and GND is common.
  Firmware configures D1 only as `INPUT`.
- The Home Hub remains open-SoftAP test infrastructure. Its new write controls
  are unauthenticated and must not be treated as production-safe access control.

## Evidence

- `firmware/GateSensor/` defaults to diagnostic mode and sends a stable raw
  transition after three matching 50 ms samples.
- `firmware/PeripheralValidation/` recognizes the `FS1` Gate Sensor packet,
  displays raw state/history, retains settings, and queues a configuration reply.
- Both compiled sketches were uploaded and flash-verified to their intended XIAO
  ESP32-C3 boards on 2026-10-15. No RYLR998 persistent settings were changed.
- The user accepted the diagnostic Hall-sensor validation on 2026-10-20. The
  production power profile and exact latency budget require a dedicated follow-up
  mission.

## Next action

Complete. Follow up with a production-power and radio-latency mission before
field deployment.
