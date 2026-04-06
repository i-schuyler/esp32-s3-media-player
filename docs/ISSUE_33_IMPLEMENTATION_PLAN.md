# ISSUE #33 Implementation Plan

Issue: https://github.com/i-schuyler/esp32-s3-media-player/issues/33

## Goal
Prepare the next release by adding a new top `CHANGELOG.md` entry that reflects the latest merged fixes currently on `main`, with a docs-only scope and no runtime behavior changes.

## Scope (minimal)
- Add one new release entry in `CHANGELOG.md` in newest-first order.
- Summarize latest merged memory/build parity diagnostics and release-flow alignment work present on `main`.
- Keep version formatting consistent with release automation (`vX.Y.Z` + dated changelog header).

## Mapping to ACCEPTANCE.md
- `ACCEPTANCE.md` #7/#8/#9/#10: streaming behavior remains unchanged in this docs-only slice.
- `ACCEPTANCE.md` #13: CI remains green; PR metadata fields remain policy-compliant.
- Issue acceptance #1/#2/#3/#4: changelog top entry exists, reflects latest merged fixes, keeps release versioning consistent, and does not alter firmware behavior.
- Issue acceptance #5: compile target remains `esp32-s3-devkitc-1-n32r16v` in CI.

## Mapping to docs/SMOKE_TEST_SPEC.md
- Keep `docs/SMOKE_TEST_SPEC.md` streaming and OTA manual checks as regression baseline.
- No smoke spec changes are required; this issue is release-prep docs only.
- Manual verification from issue: confirm the new top changelog entry accurately describes latest merged work and is suitable for release automation.

## Implementation steps
1. Prepend a new `CHANGELOG.md` release header for the next version with current date.
2. Capture latest merged fixes on `main` (memory/build parity diagnostics and release alignment) in Added/Changed/Fixed sections.
3. Run CI entrypoint locally (`ci.sh`) to validate policy gates + compile target parity.
