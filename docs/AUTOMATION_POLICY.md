# Automation policy

## Merge policy (automatable)
A PR may be auto-merged only when ALL are true:
- All required CI checks pass.
- PR body includes `RISK: low` (exact token) and `BREAKING: no`.
- PR body includes `NEEDS_HIL: no` (until HIL runner exists).
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
- After merging, create and push a release tag that matches the changelog entry.
- Release/version metadata and pin/docs alignment rules are defined in `docs/RELEASE_AND_PIN_POLICY.md`.
- After a PR is merged to `main`, `.github/workflows/release.yml` reads the newest changelog release entry (`## [vX.Y.Z] - YYYY-MM-DD`) and uses that version.
- The release workflow builds `esp32-s3-devkitc-1-n32r16v`, tags the merge commit, creates the GitHub release, and uploads `media_server_vX_X_X.bin`.
- Release and manual-attach workflows must verify PlatformIO env/board/partition parity (`esp32-s3-devkitc-1-n32r16v`) before building firmware artifacts.
- If changelog parsing, tag creation, or firmware artifact checks fail, the release job stops with explicit errors and does not publish a release.
- For an already-existing tag/release missing `media_server_vX_X_X.bin`, run `.github/workflows/manual-release-attach.yml` via `workflow_dispatch` and provide the existing tag (for example `v0.1.3`).
