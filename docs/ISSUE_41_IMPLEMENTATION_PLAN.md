# Issue #41 Implementation Plan

Issue: https://github.com/i-schuyler/esp32-s3-media-player/issues/41

## Goal
Add deeper browser-visible SD bring-up diagnostics for v0.1 so SD initialization failures can be triaged without USB serial.

## Scope (minimal)
- Extend existing SD diagnostics in `src/main.cpp` only.
- Surface actual SD pin map in use (CS/SCK/MISO/MOSI) in browser-visible SD diagnostics.
- Surface all attempted SPI init speeds used during SD bring-up.
- Surface bounded attempt-level init trace details (including CMD0 outcome and stage progression).
- Add a new top changelog entry for this testable diagnostics slice.
- Do not change OTA flow, streaming behavior, or board/env targeting.

## Mapping to ACCEPTANCE.md
- `ACCEPTANCE.md` #7/#8/#9/#10: preserve existing streaming range/caching behavior (no streaming path changes).
- `ACCEPTANCE.md` #13: keep CI/policy gates green with required PR metadata.
- Issue acceptance #1/#2/#3/#4/#5: implemented via browser-visible pin map + attempted SPI speed list + bounded init trace lines and rolling debug log integration.
- Issue acceptance #6/#7/#8/#9/#11: scope avoids OTA/stream regressions and retains compile target `esp32-s3-devkitc-1-n32r16v`.
- Issue acceptance #10: satisfied by new top changelog entry.

## Mapping to docs/SMOKE_TEST_SPEC.md
- Follow SD diagnostics checks under manual browser diagnostics (`/files`, `/sd-diagnostics`) and verify stage-specific/actionable output.
- Preserve streaming smoke behavior (`Range: bytes=0-1023` -> 206 + `Content-Range` and `Accept-Ranges: bytes`).
- Preserve OTA smoke baseline behavior from the OTA manual regression section.

## Implementation Steps
1. Add bounded SD init trace capture across mount attempts (attempt number, speed, CMD0 result, key stage result).
2. Record and expose pin map + every attempted SPI init speed in SD status API and SD diagnostics page.
3. Mirror important new SD init trace lines into rolling debug log output.
4. Add `v0.1.11` top changelog entry describing this diagnostics slice.
5. Run `./ci.sh` to ensure policy gates + compile target remain green.

## Verification
- `./ci.sh` passes locally, including PlatformIO build for `esp32-s3-devkitc-1-n32r16v`.
- `/api/sd/status` includes `pin_map`, `attempted_spi_speeds`, and bounded `init_trace`.
- `/sd-diagnostics` visibly shows pin map, attempted speeds, and bounded init trace.
- Debug log contains SD init trace lines without requiring USB serial.
