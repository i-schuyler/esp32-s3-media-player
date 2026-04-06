# Issue #37 Implementation Plan

Issue: https://github.com/i-schuyler/esp32-s3-media-player/issues/37

## Goal
Fix firmware-side SD SPI bring-up reliability for a known-good FAT32 card on `esp32-s3-devkitc-1-n32r16v`, and make browser-visible SD diagnostics more actionable for true root-cause triage.

## Scope (minimal)
- Harden SD bring-up with a lightweight SPI preflight response check and bounded retry-at-speed attempts.
- Ensure SD failure-stage reporting clearly distinguishes:
  - card absent / no communication
  - init/select failure
  - mount/filesystem failure
  - root-open/read failure
- Keep OTA, streaming, and non-SD diagnostics behavior unchanged.

## Mapping to ACCEPTANCE.md
- `ACCEPTANCE.md` #3/#4: SD browser root listing remains the success-path behavior after mount.
- `ACCEPTANCE.md` #7/#8/#9/#10: existing streaming Range + cache semantics remain unchanged.
- `ACCEPTANCE.md` #13: CI remains green with policy-compliant PR metadata.

Issue-specific acceptance beyond `ACCEPTANCE.md`:
- Firmware-side SD bring-up no longer defaults known-good card failures into only `card_comm_failure`; stage detail now differentiates init/select vs mount/filesystem where possible.
- Browser-visible SD diagnostics stay accurate and actionable without expanding unrelated feature surface.

## Mapping to docs/SMOKE_TEST_SPEC.md
- Preserve SD mount/read/list serial markers (`SD: MOUNTED`, `SD: READ OK`, `SD: LIST OK`) on success path.
- Preserve browser diagnostics flow for SD-unavailable state via `/files` and `/sd-diagnostics`.
- Preserve streaming smoke regression checks (`Range: bytes=0-1023` -> `206` + `Content-Range`).

## Implementation Steps
1. Add SD SPI CMD0 preflight probe in `src/main.cpp` and use it in mount attempts.
2. Reclassify SD init failures so stage codes map to the issue-required categories.
3. Add bounded per-speed retry attempts to improve known-good card bring-up reliability while keeping current pin mapping and target env unchanged.
4. Validate with `./ci.sh` (policy + compile for `esp32-s3-devkitc-1-n32r16v`).

## Verification
- CI/build: run `./ci.sh` locally (includes PlatformIO build for `esp32-s3-devkitc-1-n32r16v`).
- Manual (issue + smoke-spec aligned):
  - Insert known-good FAT32 card with documented wiring and verify SD no longer reports `card_comm_failure` for firmware-side init-select failures.
  - Confirm `/files` loads and root listing works on success.
  - Confirm `/sd-diagnostics` stage/detail/guidance map to observed failure class when inducing faults.
  - Re-run streaming spot check (`Range: bytes=0-1023`) and OTA page sanity to confirm no regression outside SD scope.
