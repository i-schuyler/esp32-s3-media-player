# Issue #8 Implementation Plan

Issue: https://github.com/i-schuyler/esp32-s3-media-player/issues/8

## Goal
Provide browser-first SD diagnostics so users see actionable, stage-specific guidance when SD storage is unavailable, while preserving existing successful SD browser, streaming, and OTA behavior.

## Scope (minimal)
- Add SD failure-stage state tracking in firmware.
- Replace generic browser message (`SD root unavailable`) with clearer user-facing status and guidance.
- Add a small diagnostics page and JSON endpoint for SD status.
- Keep existing serial diagnostics and success-path behavior intact.

## Acceptance Mapping
- `ACCEPTANCE.md` #3/#4: SD browser still lists root when SD is healthy.
- `ACCEPTANCE.md` #7: Compile target remains `esp32-s3-devkitc-1-n32r16v`.
- `ACCEPTANCE.md` #8/#9: HTTP Range streaming behavior remains unchanged.
- Regression/behavior guard: OTA/UI behavior outside SD diagnostics scope remains unchanged.

## Smoke Test Mapping
- Preserve existing pass/fail serial markers from `docs/SMOKE_TEST_SPEC.md` SD and streaming sections.
- Add manual browser validation to confirm failure stage + actionable guidance is visible without serial-only access.

## Implementation Steps
1. Add SD diagnostics state model in `src/main.cpp` with major failure stages:
   - init/pin bus failure
   - card communication/detect failure
   - mount/filesystem failure
   - read/root-open failure
2. Surface diagnostics in browser UI:
   - file browser unavailable message includes stage-specific guidance
   - add `/sd-diagnostics` page with status, stage code, and next-step action
   - add `/api/sd/status` JSON endpoint
3. Keep existing SD success path, `/stream` Range logic, and OTA endpoints unchanged.
4. Validate with `./ci.sh` build/policy checks.

## Verification
- CI/build: `pio run -e esp32-s3-devkitc-1-n32r16v` via `./ci.sh`.
- Manual (from issue + smoke spec):
  - Healthy SD: `/files` root listing still works.
  - Induced SD failure: `/files` and `/sd-diagnostics` show specific stage + actionable guidance.
  - Streaming regression check: `Range: bytes=0-1023` returns 206 with `Content-Range` and `Accept-Ranges: bytes`.
