# Issue #61 Implementation Plan

Issue: https://github.com/i-schuyler/esp32-s3-media-player/issues/61

## Goal
Extend the post-failure software-SPI SD diagnostic path so it probes raw initialization commands beyond CMD0 (`CMD8`, `CMD55` + `ACMD41`, `CMD58`) on the currently selected pin profile, and reports bounded, command-level outcomes.

## Scope (minimal)
- Touch only SD diagnostics logic/output in `src/main.cpp`.
- Keep selected-profile-only behavior; no profile hopping and no new SPI library.
- Preserve target/env (`esp32-s3-devkitc-1-n32r16v`) and existing pages/endpoints.
- Add one issue-plan doc and one new top changelog entry for release automation.

## Mapping to ACCEPTANCE.md
- `ACCEPTANCE.md` #3/#4: SD diagnostics remain available while mount/list behavior is preserved.
- `ACCEPTANCE.md` #13: keep CI/policy gates green.
- Issue acceptance (raw diag): after mount failure, software-SPI now attempts `CMD0`, `CMD8`, `CMD55`, `ACMD41`, `CMD58` and records per-command attempt/result/bytes in bounded form.
- Issue acceptance (triage clarity): diagnostics expose summary and per-command outcomes that distinguish no wire-level response vs partial response during init.
- Issue acceptance (profile discipline): diagnostics continue using only selected profile and pin map.

## Mapping to docs/SMOKE_TEST_SPEC.md
- Preserve SD diagnostics-page availability and stage-specific troubleshooting expectations under SD mount failures.
- Preserve existing boot/SD/streaming/OTA smoke scope (no unrelated code paths modified).
- Add issue-specific manual check: on selected `alt-io4-5-6-7`, rerun diagnostics and verify software-SPI raw command outputs include `CMD0/CMD8/CMD55/ACMD41/CMD58` with response classification.

## Implementation Steps
1. Add a bounded raw software-SPI command probe helper for command send, R1 polling, and optional 4-byte response capture.
2. Run a post-failure software-SPI diagnostic sequence (`CMD0`, `CMD8`, `CMD55`, `ACMD41`, `CMD58`) after existing hardware mount retries fail.
3. Append command-level trace lines and concise detail-summary text showing attempted/result/response bytes.
4. Keep existing stage mapping, but classify card communication as partially responsive when any raw command returns an R1-family response.
5. Prepend changelog entry for this slice.

## Verification
- `./ci.sh` passes (includes PlatformIO build for `esp32-s3-devkitc-1-n32r16v`).
- SD failure path includes software-SPI raw trace lines for all required commands.
- `/sd-diagnostics` and `/api/sd/status` continue to expose selected/active profile and pin map with bounded init trace/detail text.
