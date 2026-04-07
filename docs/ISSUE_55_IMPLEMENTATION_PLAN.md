# Issue #55 Implementation Plan

Issue: https://github.com/i-schuyler/esp32-s3-media-player/issues/55

## Goal
Add a minimal alternate SD pin profile experiment path so persistent CMD0 timeout behavior can be compared between the existing default wiring profile and a second firmware pin mapping, with clear browser-visible profile/pin reporting.

## Scope (minimal)
- Touch only SD bring-up/diagnostics in `src/main.cpp` plus release/pin docs.
- Preserve current default/documented SD pin profile as first/primary profile.
- Add one alternate test SD pin profile and include it in bring-up attempt order.
- Expose active and attempted pin profiles in browser-visible diagnostics and rolling logs.
- Add a new top `CHANGELOG.md` entry for this testable slice.

## Mapping to ACCEPTANCE.md
- `ACCEPTANCE.md` #4/#5/#6: SD mount/list/read behavior remains the same on successful bring-up; this slice only adds profile-selection attempts and reporting.
- `ACCEPTANCE.md` #7/#8/#9/#10: streaming/range/cache behavior remains unchanged by scope.
- `ACCEPTANCE.md` #13: keep CI/policy gates green.
- Issue acceptance #1/#2/#3/#5: default+alternate profile support and profile/pin visibility are implemented in API/page/log outputs for side-by-side comparison.
- Issue acceptance #6/#7/#8/#9: OTA and streaming paths remain unchanged outside SD pin-profile scope.
- Issue acceptance #10/#11: top changelog entry added and CI compile/policy gate run.

## Mapping to docs/SMOKE_TEST_SPEC.md
- Preserve SD smoke pass/fail semantics (`SD: MOUNTED`, `SD: READ OK`, `SD: LIST OK`, `SD: MOUNT FAILED`).
- Preserve browser diagnostics flow (`/sd-diagnostics`) while expanding it with pin profile visibility.
- Preserve streaming smoke expectations (`Range: bytes=0-1023` -> `206` + `Content-Range`, `Accept-Ranges: bytes`).
- Preserve OTA smoke baseline and behavior outside this SD-focused slice.

## Implementation Steps
1. Introduce two SD pin profiles in firmware: existing default profile and one alternate profile.
2. Attempt SD bring-up across pin profiles (default first), then existing host/speed retries.
3. Report active profile and attempted profile list in `/api/sd/status`, `/sd-diagnostics`, init trace, and rolling debug log.
4. Update wiring/pin policy docs to keep firmware and published pin guidance aligned.
5. Add a new top changelog release entry and validate with `./ci.sh`.

## Verification
- `./ci.sh` passes locally (policy checks + `esp32-s3-devkitc-1-n32r16v` build).
- `/api/sd/status` includes `pin_profile`, `pin_map`, and `attempted_pin_profiles`.
- `/sd-diagnostics` shows active profile, pin map, and attempted profile list.
- Debug log includes active pin profile and attempted profile set during SD bring-up.
