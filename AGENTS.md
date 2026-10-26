# Farm Sense agent guide

## Purpose and boundaries

Develop Farm Sense firmware, supporting tools, and documentation for Arduino-compatible ESP32-C3 sensor devices. Work locally and keep the project understandable to a hardware technician.

- `Home_hub/` is requirements reference material only. Do not edit, move, rename, or format its files unless the user explicitly requests it.
- Treat `Home_hub/Home Hub - FAQ.md` as the current product and hardware baseline; flag conflicts rather than silently changing requirements.
- Never read, print, copy, commit, or disclose credentials, `.env` values, Wi-Fi passwords, API keys, or device identifiers that are not already public.
- Do not flash, erase, reset, partition, OTA-update, change fuses, or operate attached hardware without the user's explicit approval for the named device and port. Compilation and non-invasive port discovery are safe. Serial upload normally requires a separate, scoped unsandboxed-command approval because device ports are outside the workspace sandbox; request it with the exact board, port, and effect (reset/flash) immediately before upload. Do not assume a prior session's approval persists.
- Do not install packages, board cores, or libraries without approval. Pin all build dependencies before relying on them.

## How work is organized

`Missions/Work Protocol.md` is the operating process. One active mission is allowed at a time. Before implementing, read the active mission and its next work file. Use the templates in `Missions/_templates/`; keep mission files concise and update status, evidence, and decisions as work progresses.

## Firmware conventions

- Prefer Arduino CLI as the reproducible, agent-facing build interface. Arduino IDE may be used for human editing, board setup, and serial inspection, but it is not the source of build truth.
- Use `tools/serial-dashboard/` for local browser-based serial observation when it is running. It is intentionally read-only and owns the selected port; stop it before an upload or another serial monitor, then restart it with an explicit port afterward.
- Use one sketch folder per firmware target. Its primary `.ino` must match its folder name; put reusable C++ in `src/`.
- Commit a `sketch.yaml` profile with the ESP32 FQBN, exact ESP32 core version, direct library versions, and no machine-specific port by default.
- Separate hardware I/O from application logic. Name GPIOs and I2C addresses centrally; never guess a pin, voltage level, sensor variant, LoRa command, or radio parameter.
- Keep the device safe by default: bounded retries/timeouts, watchdog-friendly loops, no blocking delays in operational paths, explicit degraded/offline behavior, and conservative power use for battery nodes.

## Verification and handoff

- Run the smallest relevant checks first, then compile the exact declared profile. Record commands and outcomes in the work file.
- Hardware validation requires the board, port, wiring/fixture, firmware version, expected observation, and result. A successful build is not a hardware pass.
- For a permitted serial upload, use the installed IDE-bundled Arduino CLI with project-scoped `.arduino/` directories and the declared FQBN. Record the upload verifier result, but never record a device MAC, serial number, or secrets.
- Keep changes focused. Do not reformat unrelated files. Report modified files, test evidence, known gaps, and any hardware action requiring approval.

## Version control and publication

Local Git is the development record; the public GitHub repository (`https://github.com/venture-labz/FarmSense`) is the accepted, releasable source of record. Work on local `mission/M###-short-name` branches and make small, imperative commits after passing work steps. Never push work in progress, secrets, device logs containing sensitive data, build artifacts, or unapproved documentation to GitHub.

After explicit user acceptance, audit the accepted commit, push it as `release/M###-short-name`, open/review a pull request, then merge it into protected `main` only with explicit approval. Do not push, merge, tag, release, deploy, or operate hardware without explicit approval.
