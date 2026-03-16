# Issue #18 Implementation Plan

Issue: https://github.com/i-schuyler/esp32-s3-media-player/issues/18

## Goal
Fix OTA update UX and diagnostics so upload completion is not mistaken for overall success, and provide actionable browser-visible recovery guidance when OTA apply/validation fails (including Flash Read Failed scenarios).

## Scope (minimal)
- Keep OTA route shape unchanged (`/ota`, `/api/ota/status`).
- Add explicit OTA stage reporting for browser UI: upload, validate/apply, success/failure.
- Add actionable user guidance tied to OTA failure type.
- Preserve existing SD browser, streaming, and non-OTA behavior.

## Acceptance Mapping
- `ACCEPTANCE.md` #7/#8/#9/#10: keep HTTP Range behavior unchanged (`Accept-Ranges`, `206`, `Content-Range`, `no-store`).
- `ACCEPTANCE.md` #13: CI remains green and PR policy metadata remains compliant.

Issue-specific acceptance beyond `ACCEPTANCE.md`:
- OTA progress reaching 100% is no longer presented as final success.
- OTA status clearly distinguishes upload vs validation/apply vs final state.
- Browser-visible failures include practical next steps for user-fixable conditions.

## Smoke Test Mapping
- `docs/SMOKE_TEST_SPEC.md` OTA manual checks remain the base path.
- OTA manual verification additionally checks visible stage transitions and actionable guidance on induced failures.

## Implementation Steps
1. Extend OTA status payload in `src/main.cpp` with stage + guidance fields.
2. Update OTA page UI script to display:
   - upload percentage
   - OTA stage label
   - actionable next-step guidance
3. Improve OTA failure mapping using `Update.getError()` to convert opaque failures into user-actionable messages.
4. Keep existing OTA success + reboot flow intact.
5. Validate with `./ci.sh` build and policy checks.

## Verification
- CI/build: `./ci.sh` (includes PlatformIO build for `esp32-s3-devkitc-1-n32r16v`).
- Manual OTA checks (issue + smoke spec):
  - Upload known-good `firmware.bin`: stage and status progress are accurate through reboot.
  - Induce failure (bad/wrong/corrupt image or interrupted upload): failure stays browser-visible with clear next steps.
  - Regression guard: SD browser and streaming behavior unchanged.
