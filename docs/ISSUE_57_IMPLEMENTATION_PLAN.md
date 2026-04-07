# Issue #57 Implementation Plan

Issue: https://github.com/i-schuyler/esp32-s3-media-player/issues/57

## Goal
Add explicit SD pin-profile selection in the web UI, rerun SD diagnostics using only the selected profile, and add a software reset button on the home page.

## Scope (minimal)
- Touch only `src/main.cpp` SD diagnostics/profile-selection/home-page flow and restart endpoint.
- Keep existing OTA, streaming, and build/config diagnostics behavior unchanged.
- Add one issue plan doc and a new top changelog entry for release automation.

## Mapping to ACCEPTANCE.md
- `ACCEPTANCE.md` #3/#4: SD browser/listing and mount behavior remain, but SD bring-up now uses only the user-selected profile per run.
- `ACCEPTANCE.md` #7/#8/#9/#10: streaming range/caching behavior remains unchanged.
- `ACCEPTANCE.md` #13: keep CI/policy gates green and PR metadata compliant.
- Issue acceptance #1/#2/#3/#4/#5: UI exposes explicit profile selection, reruns SD bring-up on selection change, reports selected/active profile and pin map, and adds software reset action.
- Issue acceptance #6/#7/#8/#9: OTA/rolling log/streaming preserved outside this scope.
- Issue acceptance #10/#11: add top changelog entry and keep compile target unchanged (`esp32-s3-devkitc-1-n32r16v`).

## Mapping to docs/SMOKE_TEST_SPEC.md
- Preserve SD smoke pass/fail regex expectations (`SD: MOUNTED`, `SD: READ OK`, `SD: LIST OK`, `SD: MOUNT FAILED`).
- Preserve streaming regression checks (`Range: bytes=0-1023` -> `206` + `Content-Range`, `Accept-Ranges: bytes`).
- Preserve OTA smoke baseline behavior and diagnostics.
- Add issue-specific manual checks: change SD profile in UI and confirm rerun uses only selected profile; verify home-page software reset reboots and reconnects.

## Implementation Steps
1. Add selected SD profile state + API endpoints to read/apply profile selection.
2. Change SD bring-up flow to attempt only the selected profile (no automatic profile fallback mixing).
3. Update home page with SD profile selector (apply + rerun) and software reset button.
4. Expand SD diagnostics/status payload with selected profile visibility.
5. Add new top `CHANGELOG.md` entry and run CI script.

## Verification
- `./ci.sh` passes locally (policy checks + `esp32-s3-devkitc-1-n32r16v` compile).
- `GET /api/sd/profile` returns selected profile, active profile, and profile pin maps.
- `POST /api/sd/profile` reruns SD diagnostics/mount and attempted profiles show only the selected profile.
- Home page reset action calls restart endpoint and device reboots.
