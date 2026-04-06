# Issue #49 Implementation Plan

Issue: https://github.com/i-schuyler/esp32-s3-media-player/issues/49

## Goal
Add a more conservative staged SD power-up/settle sequence and expose SPI host/mode plus deeper raw CMD0 response diagnostics in browser-visible SD diagnostics and rolling logs, without changing OTA/streaming behavior.

## Scope (minimal)
- Touch only SD init/diagnostics paths in `src/main.cpp` plus release-note/process docs.
- Add per-attempt staged power-up settling immediately before SD init attempts.
- Surface SPI host/instance and SPI mode in `/api/sd/status`, `/sd-diagnostics`, init trace, and rolling debug log.
- Add bounded raw CMD0 poll-byte detail to existing diagnostics (no verbose/unbounded dumping).
- Add a new top `CHANGELOG.md` entry for this testable slice.

## Mapping to ACCEPTANCE.md
- `ACCEPTANCE.md` #4/#5/#6: SD mount/list/read flow remains intact; this change narrows SD init root cause with stronger pre-init settling and diagnostics.
- `ACCEPTANCE.md` #7/#8/#9/#10: streaming/range behavior unchanged by scope.
- `ACCEPTANCE.md` #13: keep CI/policy gates green.
- Issue acceptance #1/#2/#3/#4/#6/#11/#12: addressed via staged settle sequence, host/mode visibility, deeper CMD0 raw detail, rolling-log surfacing, changelog entry, and compile gate.
- Issue acceptance #7/#8/#9/#10: OTA and streaming remain unchanged by implementation scope.

## Mapping to docs/SMOKE_TEST_SPEC.md
- Preserve SD unavailable diagnostics flow (`/files` + `/sd-diagnostics`) with stage-specific status/guidance.
- Preserve SD serial pass/fail semantics (`SD: MOUNTED`, `SD: READ OK`, `SD: LIST OK`, `SD: MOUNT FAILED`).
- Preserve streaming smoke expectation (`Range: bytes=0-1023` => `206` + `Content-Range`, `Accept-Ranges: bytes`).
- OTA smoke procedure remains unchanged for this SD-only slice.

## Implementation Steps
1. Add staged per-attempt SD power-up settle timing before each `SD.begin` attempt.
2. Record/report SPI host and SPI mode in bounded SD diagnostics outputs and rolling log.
3. Add bounded CMD0 polling first-byte diagnostics to failed/ambiguous init traces/details.
4. Add `v0.1.14` changelog entry documenting this SD timing/host/raw-response diagnostics slice.
5. Run `./ci.sh` (policy checks + compile for `esp32-s3-devkitc-1-n32r16v`).

## Verification
- `./ci.sh` passes locally.
- `/api/sd/status` and `/sd-diagnostics` include SPI host/mode plus existing pin map/speeds/init trace.
- CMD0 raw response diagnostics include bounded first poll bytes and remain readable.
- `/debug-log` includes key new SD power-up/host/raw-response lines.
- OTA and streaming paths are unchanged by diff.
