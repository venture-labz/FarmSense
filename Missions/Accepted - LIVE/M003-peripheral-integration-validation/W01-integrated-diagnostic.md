# W01 — Run integrated peripheral diagnostic

- **Mission:** M003
- **Status:** Verified
- **Owner:** Codex + user
- **Dependencies:** XIAO ESP32-C3; powered BMP280 at `0x76`; MCP23017 at `0x20`; powered RYLR998 connected to D6/D7
- **Affected files/components:** `firmware/PeripheralValidation/`
- **Hardware impact:** Approval required to flash the XIAO. Volatile BMP280 measurement-mode setting; non-persistent RYLR `AT` command; no MCP GPIO writes.

## Problem

Give a repeatable, visible test of all attached local peripherals before application behavior is introduced.

## Scope

**In:** BMP280 compensated data, MCP presence probe, RYLR UART health/receive listener, and D1 activity feedback.

**Out:** RYLR transmit/configuration commands; MCP register writes; BME280 humidity; any cloud, Wi-Fi, or web functionality.

## Acceptance criteria

- [x] Build succeeds.
- [x] Serial output contains live temperature and pressure values from chip ID `0x58`.
- [x] Serial output confirms the MCP23017 and RYLR `+OK` health response.
- [x] Indicator behavior is visibly correct.

## Verification

| Level | Method / command / fixture | Expected result | Result |
| --- | --- | --- | --- |
| Static | Review I2C/UART operations | No MCP GPIO writes or stored RYLR configuration writes | Passed: no MCP register-write call; only `AT` is sent to the RYLR UART. PWM frequency type was corrected to the known-good 32-bit 5,000 Hz value. |
| Build | Explicit XIAO FQBN, core 3.3.12 | Successful compile | Passed: 322,576 bytes (24%) flash; 14,256 bytes (4%) RAM. |
| Hardware | Approved upload; USB monitor at 115200 | Repeated local peripheral results | Passed for BMP280/MCP23017/RYLR UART: BMP280 ID `0x58`; 24.32–24.36 °C; 984.52–984.60 hPa; MCP23017 detected; RYLR `+OK`. D1 visual check pending. |

## Decisions and risks

- A second LoRa node is not needed to validate the BMP280, MCP23017, or RYLR UART link; it is required to satisfy W02's over-the-air receive criterion.

## Evidence

- The first upload attempt did not open the busy serial port and did not alter the board. A stale Arduino serial-monitor process was identified and closed before the successful retry.
- The successful upload was verified by the flasher. The monitor was explicitly closed after collection; the port is available for later upload/monitor work.
- User could not see D1 under the integrated diagnostic. Review found `kLedFrequencyHz` was mistakenly typed as an 8-bit value, truncating 5,000 Hz. The known-good baseline uses a 32-bit frequency; this is being corrected before retest.
- The corrected firmware was compiled, uploaded, and flash-verified. User confirmed D1 remains visible and functional on 2026-10-14.

## Next action

Complete. The mission owner may now review the combined evidence for acceptance.
