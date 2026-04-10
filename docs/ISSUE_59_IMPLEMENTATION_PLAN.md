# Issue #59 Implementation Plan

Issue: https://github.com/i-schuyler/esp32-s3-media-player/issues/59

## Goal
Make SD pin-profile selection on the home page reliably apply to SD diagnostics (including `alt-io4-5-6-7`), avoid silent fallback to default profile, and document deterministic reboot behavior.

## Scope (minimal)
- Touch only SD profile apply/parsing and home/diagnostics UI text in `src/main.cpp`.
- Keep existing SD mount/diagnostics flow, OTA behavior, streaming behavior, and compile target unchanged.
- Add one issue plan doc and a new top changelog entry for release automation.

## Mapping to ACCEPTANCE.md
- `ACCEPTANCE.md` #3/#4: SD browser/listing and mount behavior remain while profile selection/apply path is hardened.
- `ACCEPTANCE.md` #7/#8/#9/#10: streaming/range/cache behavior remains unchanged by scope.
- `ACCEPTANCE.md` #13: keep CI/policy gates green and PR metadata compliant.
- Issue acceptance #1/#2/#3/#4/#5: home page profile control supports required options and reliably applies selected profile for rerun diagnostics with selected/active/pin-map visibility.
- Issue acceptance #6: reboot behavior is deterministic and now explicitly documented in UI/diagnostics text.
- Issue acceptance #7/#8: changelog top entry added and target build env remains `esp32-s3-devkitc-1-n32r16v`.

## Mapping to docs/SMOKE_TEST_SPEC.md
- Preserve SD smoke pass/fail expectations (`SD: MOUNTED`, `SD: READ OK`, `SD: LIST OK`, `SD: MOUNT FAILED`) and SD diagnostics page behavior.
- Preserve streaming regression checks (`Range: bytes=0-1023` -> `206`, `Content-Range`, `Accept-Ranges`).
- Preserve OTA smoke baseline behavior.
- Add issue-specific manual check: switch UI profile to `alt-io4-5-6-7`, apply/rerun diagnostics, and confirm selected/in-use profile + pin map align.

## Implementation Steps
1. Harden SD profile POST parsing to accept explicit profile names and strictly-validated index values.
2. Update home-page profile form submit path to send explicit profile value (name) and keep existing apply/rerun behavior.
3. Keep profile/status payloads aligned for UI rendering and fix related JSON field formatting.
4. Add deterministic reboot behavior note in home page and SD diagnostics page.
5. Add a new top `CHANGELOG.md` entry and run CI script.

## Verification
- `./ci.sh` passes locally (policy checks + `esp32-s3-devkitc-1-n32r16v` compile).
- `GET /api/sd/profile` exposes both required options and selected/active profile with pin map.
- `POST /api/sd/profile` with `profile=alt-io4-5-6-7` applies selection and reruns diagnostics using alternate pin map.
- `POST /api/sd/profile` rejects invalid non-numeric/non-profile values instead of silently defaulting.
