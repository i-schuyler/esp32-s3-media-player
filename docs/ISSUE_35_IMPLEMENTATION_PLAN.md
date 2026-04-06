# ISSUE #35 Implementation Plan

Issue: https://github.com/i-schuyler/esp32-s3-media-player/issues/35

## Goal
Add lightweight OTA regression protection in repo docs/process so OTA-sensitive changes are clearly flagged for retest and the known-good OTA smoke flow remains repeatable.

## Scope (minimal)
- Update `docs/SMOKE_TEST_SPEC.md` OTA section with explicit regression smoke steps based on the known-good `v0.1.8` release asset flow.
- Update `docs/AUTOMATION_POLICY.md` with explicit pre-merge OTA retest triggers for OTA-sensitive change classes.
- Keep this slice docs/process only (no firmware/runtime behavior changes).

## Mapping to ACCEPTANCE.md
- `ACCEPTANCE.md` #7/#8/#9/#10: streaming behavior remains unchanged in this docs-only scope.
- `ACCEPTANCE.md` #13: CI remains green and PR metadata fields remain policy-compliant.
- Issue acceptance #5/#6/#7/#8: no runtime change in OTA/SD/streaming/diagnostics paths; compile target remains `esp32-s3-devkitc-1-n32r16v`; streaming acceptance is preserved as an explicit regression guard.

## Mapping to docs/SMOKE_TEST_SPEC.md
- Strengthen OTA manual smoke procedure to include:
  1. Upload known-good release asset.
  2. Verify OTA success status.
  3. Verify reboot + reconnect.
  4. Verify displayed firmware version changed as expected.
  5. Verify running/target partition diagnostics when exposed.
  6. Verify core app pages remain functional after reboot.
- Keep SD and streaming smoke checks unchanged as baseline regression coverage.

## Implementation steps
1. Add this issue plan doc mapped to acceptance/smoke criteria.
2. Expand OTA smoke section in `docs/SMOKE_TEST_SPEC.md` with deterministic regression checklist tied to known-good `v0.1.8` flow.
3. Add concise OTA retest trigger list in `docs/AUTOMATION_POLICY.md` to define when OTA smoke is required before merge.
4. Add changelog entry for traceability.
5. Run `ci.sh` to verify policy files/build still pass.
