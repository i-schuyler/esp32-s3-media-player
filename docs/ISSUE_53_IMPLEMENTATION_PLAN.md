# Issue #53 Implementation Plan

Issue: https://github.com/i-schuyler/esp32-s3-media-player/issues/53

## Goal
Add a tightly scoped diagnostic software-SPI SD bring-up fallback path so persistent hardware-SPI CMD0 timeout behavior can be compared against a software-SPI path using the same wiring/card, without changing OTA or streaming behavior.

## Scope (minimal)
- Touch only SD bring-up diagnostics in `src/main.cpp` plus release/process docs.
- Preserve current hardware SPI bring-up sequence and host fallback behavior.
- Add a post-hardware-failure software-SPI CMD0 diagnostic probe (bit-banged on the same pins) for root-cause discrimination.
- Ensure browser-visible SD diagnostics and rolling log clearly indicate hardware-vs-software SD bus path attempts.
- Add a new top `CHANGELOG.md` entry for this testable diagnostics slice.

## Mapping to ACCEPTANCE.md
- `ACCEPTANCE.md` #4/#5/#6: SD mount/list/read behavior remains unchanged on success path; added software-SPI probe is diagnostic-only after hardware attempts fail.
- `ACCEPTANCE.md` #7/#8/#9/#10: streaming/range/cache behavior remains unchanged by scope.
- `ACCEPTANCE.md` #13: keep CI/policy gates green.
- Issue acceptance #1/#2/#3/#4: software-SPI diagnostic path is present and exercised after hardware failures; diagnostics/log clearly show which bus path was used and whether software SPI behaved differently.
- Issue acceptance #5/#6/#7/#8: OTA and streaming paths are untouched and preserved outside SD diagnostics scope.
- Issue acceptance #9/#10: new top changelog entry added and compile gate validated via `ci.sh`.

## Mapping to docs/SMOKE_TEST_SPEC.md
- Preserve SD mount/read/list pass/fail semantics and browser diagnostics checks.
- Preserve streaming smoke expectations (`Range: bytes=0-1023` -> `206` + `Content-Range`, `Accept-Ranges: bytes`).
- Preserve OTA smoke baseline and behavior outside this SD diagnostics slice.
- Add issue-specific manual check: compare hardware-SPI attempt trace lines vs software-SPI diagnostic trace line on same card/wiring.

## Implementation Steps
1. Keep existing hardware SPI host/speed retry behavior unchanged.
2. Add a bounded software-SPI CMD0 diagnostic probe executed only after hardware retries are exhausted.
3. Surface attempted SD bus paths (`hardware-spi` vs `software-spi`) in status JSON, diagnostics page, and rolling debug log.
4. Ensure failure detail captures software-SPI CMD0 outcome so hardware-vs-software differences are explicit.
5. Add new top changelog entry and validate with `./ci.sh`.

## Verification
- `./ci.sh` passes locally.
- `/api/sd/status` includes attempted SD bus paths and existing host/speed diagnostics.
- `/sd-diagnostics` shows attempted SD bus paths and trace lines that include `bus=hardware-spi` and `bus=software-spi`.
- Rolling debug log includes the same bus-specific trace context on SD bring-up attempts.
