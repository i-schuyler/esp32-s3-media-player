# Issue #21 Implementation Plan

Issue: https://github.com/i-schuyler/esp32-s3-media-player/issues/21

## Goal
Add a minimal Web UI SD format flow that is clearly destructive, requires explicit confirmation, attempts FAT/FAT32 formatting, remounts, and reports browser-visible diagnostics with actionable guidance.

## Scope (minimal)
- Add SD format controls to `/files` with strong destructive warning and explicit confirmation.
- Add SD format status + trigger APIs for browser-visible stage/message/guidance.
- Implement firmware-side format attempt and post-format remount/validation.
- Preserve existing upload/browse/stream/OTA behavior outside this feature scope.

## Acceptance Mapping
- `ACCEPTANCE.md` #3/#4: SD browser behavior remains intact when SD is healthy.
- `ACCEPTANCE.md` #7/#8/#9/#10: keep existing streaming Range and cache behavior unchanged.
- `ACCEPTANCE.md` #13: CI remains green and PR policy metadata remains compliant.

Issue-specific acceptance beyond `ACCEPTANCE.md`:
- UI exposes a clearly destructive SD format action with explicit confirmation.
- Firmware attempts to create a usable FAT/FAT32 filesystem and only reports success after remount + usability validation.
- Failure states report actionable next steps (absent card, precheck failure, format/remount/validation failure).

## Smoke Test Mapping
- `docs/SMOKE_TEST_SPEC.md` SD mount/read and streaming checks remain required regression coverage.
- `docs/SMOKE_TEST_SPEC.md` OTA checks remain unchanged and must still pass outside this scope.
- Manual SD format checks (issue-specific):
  - Verify warning + explicit confirmation are required before formatting.
  - Verify stage/status messaging (precheck/format/remount/validate/final).
  - Verify success requires remount plus write/read probe.
  - Verify failure guidance is actionable when card is absent/unusable.

## Implementation Steps
1. Add issue plan doc mapped to acceptance/smoke criteria.
2. Extend `src/main.cpp` with SD format status model and JSON API.
3. Add `/files` UI controls and confirmation checks for destructive format action.
4. Implement SD format flow: precheck, destructive format attempt, remount, and validation probe.
5. Run `./ci.sh` to confirm policy gates + compile target `esp32-s3-devkitc-1-n32r16v`.

## Verification
- CI/build: `./ci.sh`.
- Manual (sacrificial card):
  - Trigger format via `/files` only after explicit confirmation.
  - Confirm browser stage transitions and actionable status/guidance.
  - Confirm post-format SD usability (browse/upload) and stream regression behavior unchanged.
