# Automation policy

## Merge policy (automatable)
A PR may be auto-merged only when ALL are true:
- All required CI checks pass.
- PR body includes `RISK: low` (exact token) and `BREAKING: no`.
- PR scope matches a linked issue (Issue URL present).
- No forbidden paths touched (see `.github/workflows/ci.yml`).

Anything else must be labeled `needs-human`.

## Risk rubric
- low: docs, CI, tests, small isolated change with clear tests/evidence
- med: touches SD, HTTP streaming, RTC drivers, config
- high: partitions, boot, Wi-Fi stack changes, major refactor, timing-critical changes

## Hard-stops
Automation MUST stop and ask for human input when:
- risk=high
- breaking=yes
- unknown hardware assumptions
- flaky CI or non-deterministic test failures

## Release hygiene (required after merge)
- Every merged PR must update `CHANGELOG.md` (prepend order; newest entry first).
- After a PR is merged to `main`, `.github/workflows/release.yml` reads the newest changelog release entry (`## [vX.Y.Z] - YYYY-MM-DD`) and uses that version.
- The release workflow builds `esp32-s3-devkitc-1-n32r16v`, tags the merge commit, creates the GitHub release, and uploads `firmware.bin`.
- If changelog parsing, tag creation, or firmware artifact checks fail, the release job stops with explicit errors and does not publish a release.
