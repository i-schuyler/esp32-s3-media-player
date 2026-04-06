# Issue #44 Implementation Plan

Issue: https://github.com/i-schuyler/esp32-s3-media-player/issues/44

## Goal
Harden SD SPI-mode entry/init sequencing and expose raw CMD0/CS diagnostics in browser-visible diagnostics/logs so no-card-response failures can be root-caused without USB serial.

## Scope (minimal)
- Touch only SD init/diagnostics logic in `src/main.cpp`.
- Keep one authoritative init flow per attempt: SPI bus prime -> pre-CMD0 dummy clocks -> raw CMD0 probe -> `SD.begin`.
- Extend bounded SD init trace and rolling debug log with CMD0 raw response and CS transition-state details.
- Keep existing OTA, streaming, and unrelated diagnostics behavior unchanged.
- Add a new top `CHANGELOG.md` entry for this testable slice.

## Mapping to ACCEPTANCE.md
- `ACCEPTANCE.md` #7/#8/#9/#10: streaming range/caching path unchanged.
- `ACCEPTANCE.md` #13: preserve policy/CI requirements.
- Issue acceptance #1/#2/#3/#4: satisfied via hardened single-flow SPI init and bounded browser-visible CMD0/CS diagnostics.
- Issue acceptance #5/#6/#7/#8/#10: scope intentionally excludes OTA/streaming behavior changes and preserves compile target `esp32-s3-devkitc-1-n32r16v`.
- Issue acceptance #9: satisfied by new top changelog release entry.

## Mapping to docs/SMOKE_TEST_SPEC.md
- Use manual SD diagnostics checks (`/files`, `/sd-diagnostics`) to confirm stage-specific and actionable output.
- Re-run streaming smoke expectation (`Range: bytes=0-1023` -> `206` with `Content-Range` and `Accept-Ranges: bytes`) to confirm no regression.
- OTA smoke flow remains unchanged; OTA retest is not triggered by this SD-only slice.

## Implementation Steps
1. Add raw CMD0 probe struct/results (R1 byte, poll count, dummy-clock issued flag, CS state transitions).
2. Consolidate attempt sequencing so pre-CMD0 clocks and CMD0 probe are in one authoritative step before `SD.begin`.
3. Emit bounded trace/debug lines with SPI speed, CMD0 result, raw R1, and CS diagnostics.
4. Add `v0.1.12` changelog entry for this SD init diagnostics hardening slice.
5. Run `./ci.sh` (policy checks + PlatformIO build for `esp32-s3-devkitc-1-n32r16v`).

## Verification
- `./ci.sh` passes locally.
- `/api/sd/status` and `/sd-diagnostics` show bounded init trace containing raw CMD0 and CS diagnostics.
- `/debug-log` includes mirrored SD trace lines without requiring USB serial.
- OTA and streaming endpoints remain behaviorally unchanged by code path.
