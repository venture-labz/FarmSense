# Farm Sense work protocol

## Purpose

This is the lightweight control plane for agent-assisted firmware work. It makes requirements, decisions, verification, and release state visible without duplicating source code. Product and hardware requirements remain in [`Home_hub/Home Hub - FAQ.md`](../Home_hub/Home%20Hub%20-%20FAQ.md), which is read-only unless the user says otherwise.

## Mission structure

Only one mission may be active. Create it in `Accepted - Dev/` only after the user authorizes it. Use the templates in `_templates/`.

```
Missions/
  Work Protocol.md                 # this process
  _templates/
    Mission.md                     # mission brief and evidence
    Work.md                        # independently testable step
  Accepted - Dev/
    M###-short-name/
      Mission.md
      W01-short-name.md
      W02-short-name.md
  Accepted - LIVE/                 # accepted release records, immutable except corrections
  Rejected/                        # declined/cancelled mission records and rationale
```

`Mission.md` owns the outcome; each `W##` file owns a small, testable increment. Do not create a work item whose result cannot be inspected, compiled, tested, or otherwise evidenced.

### Required fields

Every Mission and Work file includes: **ID, title, status, problem, scope, acceptance criteria, evidence, decisions/risks, and next action**. Work files additionally list dependencies, affected files, verification, and hardware impact.

Statuses are intentionally small:

`Proposed → Ready → In progress → Blocked | Verified → Accepted | Rejected`

Keep only `Ready` or `In progress` work items within the active mission. `Blocked` must name the missing input or hardware. Move the mission to `Accepted - LIVE` only after user acceptance of its release evidence; move it to `Rejected` if cancelled or declined. Never delete the record.

## Delivery loop

1. Read the active mission, its next work item, and only the requirements needed from the Home Hub FAQ.
2. State assumptions and turn unknown board/sensor facts into explicit verification work.
3. Make the smallest cohesive change; update the work record rather than writing duplicate narrative elsewhere.
4. Verify proportionally, record the exact command/fixture and outcome, then obtain review or user acceptance before changing mission state.

Do not begin a second mission before the first is Accepted or Rejected.

## Development and release flow

Local Git is the development record. The public remote is `https://github.com/venture-labz/FarmSense`. Publication uses audited accepted snapshots when local history includes unrelated files that must not be published; retain the full development history locally. The first public baseline requires explicit user authorization.

After Git is initialized:

```
local main → local mission/M###-short-name → verified local commits → user acceptance
                                                               → release/M###-short-name pushed to GitHub
                                                               → reviewed PR → protected GitHub main
                                                               → user-approved device/release action
```

- Local Git keeps development history and test evidence. Public GitHub `main` is the accepted, releasable source of record; unaccepted work remains local.
- One mission branch per mission; work steps are commits, not long-lived branches. Create a remote `release/M###-short-name` branch only after acceptance, so the pull request is an audit/review record of already accepted work.
- Before any public push, check the staged diff for credentials, Wi-Fi data, private endpoints, personal data, raw serial logs, compiled binaries, and large/generated files. Add appropriate ignore rules before the first commit.
- Rebase/merge, remote push, tagging, publishing, deployment, and hardware flashing each need explicit approval.
- Protect GitHub `main`: disallow force pushes and deletion; require pull requests and passing status checks once CI exists. Use the protected branch as the final enforcement point, while local acceptance remains the gate for publication.
- A production (LIVE) record must identify the immutable source revision, firmware version, board profile, build artifact/checksum where applicable, validation evidence, and rollback path.

## Arduino ESP32-C3 toolchain policy

Use the installed Arduino IDE as the human-facing GUI for setup, visual serial monitoring, and troubleshooting. Use **Arduino CLI** as the agent-facing interface for board discovery, reproducible builds, uploads only when approved, and scripts/CI. Arduino CLI is the shared build contract because it also underpins the official IDE.

Before firmware work, create and pin `sketch.yaml` profiles. Each profile declares the ESP32 FQBN, exact `esp32` core version, direct library versions, and (only when deliberately device-specific) port/monitor settings. Put local secrets in untracked configuration, never in sketches or mission records.

Arduino CLI 1.5.1 is bundled with the installed Arduino IDE at `/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli`; it is not on the shell `PATH`. Agent commands must use that executable and project-scoped Arduino data/download/user directories under ignored `.arduino/`, rather than write into a user-wide Arduino configuration. The project-local Espressif Arduino core `esp32:esp32` 3.3.12 is installed, and the validated board profile is `esp32:esp32:XIAO_ESP32C3`. Use an explicit FQBN and build directory for agent builds because profile-based dependency resolution may try to refresh package indexes, which is not reliable in this environment. Do not assume every ESP32-C3 board uses the same USB bridge, port, or boot sequence.

## QA evidence standard

Use the cheapest meaningful evidence first; omit inapplicable levels and say why.

| Level | Required when applicable | Evidence to record |
| --- | --- | --- |
| Static | every source/config change | formatter/linter or documented manual review |
| Build | every firmware change | exact profile and successful compile output |
| Logic | parsers, calculations, state, protocol code | deterministic host/unit test vectors including bad input |
| Hardware | pins, buses, sensors, radio, power, LEDs | board + port/fixture, firmware version, expected vs observed result |
| Regression | shared behavior or release | prior critical flows exercised and result |

Build success never substitutes for hardware validation. A failed or skipped test is evidence, not something to hide. Hardware operations are opt-in per named device/port; agents may prepare commands and check non-invasive port metadata, but must not flash or reset a device without approval.

### Serial-upload preflight

1. Compile first and identify the exact FQBN, port, and build artifact.
2. Ask for explicit user authorization to flash that named port; state that flashing resets the board.
3. Request a scoped unsandboxed-command approval immediately before the upload. New sessions must request it again; do not rely on a previous agent's permission.
4. Record upload success/failure without storing device identifiers such as MAC addresses or serial numbers.

## Completion checklist

- Acceptance criteria met or explicitly waived by the user.
- Scope changes, risks, and unresolved requirements recorded.
- Relevant QA evidence recorded, including failures/skips.
- Firmware work has a pinned, repeatable build profile.
- Hardware work has an approved, recorded fixture result.
- User has accepted before LIVE archival or any release action.

