# Issue #47 Implementation Plan

Issue: https://github.com/i-schuyler/esp32-s3-media-player/issues/47

## Goal
Isolate SD SPI ownership and simplify SD bring-up so there is one clearer authoritative init flow, while keeping browser-visible diagnostics and existing OTA/streaming behavior intact.

## Scope (minimal)
- Touch only SD init/diagnostics logic in `src/main.cpp` plus release-note/process docs.
- Move SD operations to a dedicated SPI instance for clearer ownership/isolation.
- Make `SD.begin` the authoritative bring-up step per attempt; use raw CMD0 probe as failure-side diagnostics only.
- Preserve browser-visible SD diagnostics (`/sd-diagnostics`, `/api/sd/status`) and rolling log behavior.
- Add a new top `CHANGELOG.md` entry for this testable slice.

## Mapping to ACCEPTANCE.md
- `ACCEPTANCE.md` #4/#5/#6: SD mount/list/upload/download behavior is preserved; this slice targets SD init reliability/diagnostics only.
- `ACCEPTANCE.md` #7/#8/#9/#10: streaming range/caching path is unchanged.
- `ACCEPTANCE.md` #13: keep CI/policy gates green.
- Issue acceptance #1/#2/#3/#5/#10/#11: addressed by single authoritative SD init flow, dedicated SPI ownership, preserved browser diagnostics, changelog top entry, and compile gate.
- Issue acceptance #6/#7/#8/#9: OTA and streaming behavior remain unchanged by scope.

## Mapping to docs/SMOKE_TEST_SPEC.md
- Keep SD browser diagnostics workflow intact (`/files`, `/sd-diagnostics`) with stage-specific status/guidance.
- Preserve SD pass/fail serial semantics (`SD: MOUNTED`, `SD: LIST OK`, `SD: READ OK`; `SD: MOUNT FAILED`).
- Re-run streaming smoke expectation (`Range: bytes=0-1023` -> `206` + `Content-Range`, `Accept-Ranges: bytes`) as regression guard.
- OTA smoke flow remains unchanged for this SD-scoped slice.

## Implementation Steps
1. Add dedicated SD SPI instance and route SD bus prime/CMD0 diagnostics/`SD.begin` through that instance.
2. Simplify per-attempt bring-up sequence to `prime bus -> SD.begin`; only run CMD0 raw probe when an attempt fails or card type remains none.
3. Keep diagnostics fields (pin map, attempted speeds, init trace, rolling debug log) and ensure they still expose key init outcomes.
4. Add `v0.1.13` changelog entry describing this SD SPI ownership + init-flow slice.
5. Run `./ci.sh` (policy checks + `esp32-s3-devkitc-1-n32r16v` build).

## Verification
- `./ci.sh` passes locally.
- `/api/sd/status` and `/sd-diagnostics` continue showing pin map, attempted speeds, and bounded init outcomes.
- `/debug-log` still mirrors SD trace details.
- OTA + streaming behavior remain unchanged by code path.
