# Issue #39 Implementation Plan

Issue: https://github.com/i-schuyler/esp32-s3-media-player/issues/39

## Goal
Prepare the next release by adding a new top changelog entry that accurately reflects the latest merged SD bring-up work on `main`, while keeping this slice docs-only and non-breaking.

## Scope (minimal)
- Add one new newest-first release entry to `CHANGELOG.md` for the next version.
- Summarize the latest merged SD bring-up-related changes currently present on `main` (Issue #37 / PR #38).
- Do not modify firmware behavior, board env selection, or streaming logic.

## Mapping to ACCEPTANCE.md
- `ACCEPTANCE.md` #7/#8/#9/#10: preserve existing streaming Range and cache semantics (no runtime changes in this docs-only slice).
- `ACCEPTANCE.md` #13: CI must stay green and PR metadata must remain policy-compliant.
- Issue acceptance #1/#2/#3/#4/#5: top changelog entry exists, is newest-first, accurately reflects latest merged SD bring-up work, and is suitable for release automation input without behavior changes.
- Issue acceptance #6: compile target remains `esp32-s3-devkitc-1-n32r16v` in CI.

## Mapping to docs/SMOKE_TEST_SPEC.md
- Preserve `docs/SMOKE_TEST_SPEC.md` SD mount/read diagnostics and streaming regression checks as the baseline guardrail.
- Manual verification for this issue remains docs-focused: confirm the new top changelog entry accurately matches latest merged SD bring-up scope on `main`.

## Implementation Steps
1. Add `v0.1.10` as the new top release entry in `CHANGELOG.md` using repo-consistent `vX.Y.Z` formatting and date.
2. Capture latest merged SD bring-up-related work from `main` in concise Added/Changed/Fixed notes.
3. Run `./ci.sh` to validate policy files and compile for `esp32-s3-devkitc-1-n32r16v`.

## Verification
- `./ci.sh` passes locally.
- New changelog top entry remains parseable by release workflow (`## [vX.Y.Z] - YYYY-MM-DD`).
- No firmware code/path behavior changes outside release-prep docs updates.
