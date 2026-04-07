# Issue #51 Implementation Plan

Issue: https://github.com/i-schuyler/esp32-s3-media-player/issues/51

## Goal
Add a minimal SD SPI host fallback path (current host to alternate host) so firmware can test whether host selection is the root cause of persistent CMD0 timeout behavior, while preserving existing OTA/streaming behavior and diagnostics.

## Scope (minimal)
- Touch only SD init/diagnostics logic in `src/main.cpp` plus release-note/process docs.
- Keep the existing SD pin map unchanged; vary only SPI host selection order.
- Attempt SD bring-up on primary host first, then alternate host as fallback if needed.
- Ensure browser-visible diagnostics and rolling log show host selection and host used per attempt.
- Add a new top `CHANGELOG.md` entry for this test slice.

## Mapping to ACCEPTANCE.md
- `ACCEPTANCE.md` #4/#5/#6: SD mount/list/read behavior remains the same on success path; this slice only adds host fallback in bring-up attempts.
- `ACCEPTANCE.md` #7/#8/#9/#10: streaming/range behavior is unchanged by scope.
- `ACCEPTANCE.md` #13: keep CI/policy gates green.
- Issue acceptance #1/#2/#4/#5/#11: addressed via alternate-host fallback attempts, host-visible diagnostics/logging, tightly scoped implementation, changelog top entry, and CI compile gate.
- Issue acceptance #6/#7/#8/#9: OTA and streaming behavior are preserved by keeping changes isolated to SD init/diagnostics.

## Mapping to docs/SMOKE_TEST_SPEC.md
- Preserve SD mount/read/list pass/fail semantics (`SD: MOUNTED`, `SD: READ OK`, `SD: LIST OK`, `SD: MOUNT FAILED`).
- Preserve browser SD diagnostics flow (`/files` + `/sd-diagnostics`) with actionable status while unavailable.
- Preserve streaming regression expectation (`Range: bytes=0-1023` -> `206` + `Content-Range`, `Accept-Ranges: bytes`).
- OTA smoke flow remains unchanged for this SD-scoped slice.

## Implementation Steps
1. Add primary+alternate SPI host candidates and a bounded fallback attempt order while preserving the documented SD pin map.
2. Include host label in each SD bring-up attempt trace/log line and expose attempted-host summary in browser diagnostics/status JSON.
3. Keep existing SD failure-stage classification and CMD0 diagnostics behavior intact, only augmenting host context.
4. Add `v0.1.15` changelog entry for this host-fallback diagnostics slice.
5. Run `./ci.sh` to satisfy policy checks and compile for `esp32-s3-devkitc-1-n32r16v`.

## Verification
- `./ci.sh` passes locally.
- `/api/sd/status` and `/sd-diagnostics` show active host plus attempted host list.
- SD init trace/debug log lines include host per attempt (FSPI vs alternate host) for side-by-side behavior comparison.
- OTA and streaming paths are unchanged by diff.
