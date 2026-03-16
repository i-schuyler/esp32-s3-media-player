# Changelog

## Release Entry Template (prepend newest entries directly below this template)
## [vX.Y.Z] - YYYY-MM-DD
### Added
- ...
### Changed
- ...
### Fixed
- ...
### Notes
- PR: <url>
- Issue: <url>

## [v0.1.2] - 2026-03-16

### Added

* Quick start documentation covering AP access, web UI usage, upload/browse/stream flow, and basic OTA usage.
* Issue #12 implementation plan mapping scope to `ACCEPTANCE.md` and `docs/SMOKE_TEST_SPEC.md`.
* Release and pin alignment policy doc with docs-first pin-change rule.

### Changed

* PR template now includes `VERSION_BUMP` and `RELEASE_VERSION` metadata fields.
* CI PR policy-gate checks now validate `NEEDS_HIL: no`, version metadata fields, and changelog header presence for declared releases.
* README/docs now link to quick start and release/pin policy guidance.

### Fixed

* Repository policy discipline to reduce changelog/tag/docs drift and make pin/docs alignment expectations explicit.

### Notes

* PR:
* Issue: https://github.com/i-schuyler/esp32-s3-media-player/issues/12

## [v0.1.1] - 2026-03-15

### Added

* User-friendly SD diagnostics and mount troubleshooting visibility in the web UI.

### Changed

* Improved browser-visible guidance for SD initialization / mount failure states.

### Fixed

* Codex issue-to-PR workflow reliability so issue automation can start, create PRs, and recover more predictably.

### Notes

* PR:
* Issue:

## [v0.1.0] - 2026-03-14
### Added
- Initial ESP32-S3 Media Server firmware baseline with AP UI, SD browser, upload/download, and HTTP Range streaming.
