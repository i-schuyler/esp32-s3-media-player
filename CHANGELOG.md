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

## [v0.1.7] - 2026-03-24

### Added

* Issue #23 root-cause implementation details for release-build parity verification and OTA/SD diagnostics.
* Shared release parity guard script (`scripts/verify_release_build_parity.sh`) used by release and manual release-attach workflows.

### Changed

* OTA status API/UI now exposes build env, running partition, target OTA partition, and normalized OTA error code/name.
* Release workflows now enforce env/board/partition parity (`esp32-s3-devkitc-1-n32r16v`) before building assets.

### Fixed

* SD mount path now retries initialization at lower SPI clocks and reports concrete attempt-level diagnostics for card-comm/init/root failures.
* OTA begin/write/end failure paths now report concrete update errors with clearer stage/action guidance for wrong artifact vs apply failure triage.

### Notes

* PR:
* Issue: https://github.com/i-schuyler/esp32-s3-media-player/issues/23

## [v0.1.6] - 2026-03-22

### Added

* Issue #23 implementation notes covering OTA apply reliability, SD mount reliability, and release asset naming.

### Changed

* Release workflows now publish firmware assets as `media_server_vX_X_X.bin`.
* OTA page/stage flow now distinguishes upload completion (`upload_complete`) from validation/apply and final result.

### Fixed

* OTA now initializes update with the expected upload size when available and enforces size-consistent finalization to avoid false validation/apply failures.
* SD bring-up no longer treats `SD.totalBytes()==0` alone as mount failure when root access is valid, improving known-good FAT32 card handling.

### Notes

* PR:
* Issue: https://github.com/i-schuyler/esp32-s3-media-player/issues/23

## [v0.1.5] - 2026-03-16

### Added

* Issue #21 implementation plan mapping SD format scope to acceptance/smoke checks.
* `/files` now includes a destructive SD format action with explicit confirmation requirements and browser-visible status/guidance.
* SD format status/trigger APIs for stage reporting and diagnostics.

### Changed

* Firmware now attempts SD reformat-to-usable FAT/FAT32 flow with remount and post-format write/read validation.

### Fixed

* SD format flow now reports actionable failure guidance instead of opaque errors for common card/precheck/remount/validation failures.

### Notes

* PR:
* Issue: https://github.com/i-schuyler/esp32-s3-media-player/issues/21

## [v0.1.4] - 2026-03-16

### Added

* Issue #18 implementation plan mapping OTA diagnostics scope to acceptance/smoke checks.
* OTA browser-visible stage field and guidance field in status API.

### Changed

* OTA page now distinguishes upload progress from validate/apply and final result stages.
* OTA UI now shows actionable next steps for recoverable failure conditions.

### Fixed

* Reduced misleading OTA UX where 100% upload could appear complete before validation/apply failed.
* Added explicit handling guidance for `Flash Read Failed` style OTA outcomes.

### Notes

* PR:
* Issue: https://github.com/i-schuyler/esp32-s3-media-player/issues/18

## [v0.1.3] - 2026-03-16

### Added

* User-friendly SD diagnostics page and raw JSON SD status endpoint.
* Browser-visible troubleshooting guidance for SD init, mount, and root access failures.

### Changed

* File browser and SD-unavailable states now point users to clearer next-step diagnostics.

### Fixed

* Current main now includes the recovered SD diagnostics work that was previously stranded on a remote branch.

### Notes

* PR: <fill in PR URL if desired>
* Issue: https://github.com/i-schuyler/esp32-s3-media-player/issues/8

## [v0.1.2] - 2026-03-16

### Added

* Quick start documentation covering AP access, web UI usage, upload/browse/stream flow, and basic OTA usage.
* Issue #12 implementation plan mapping scope to `ACCEPTANCE.md` and `docs/SMOKE_TEST_SPEC.md`.
* Release and pin alignment policy doc with docs-first pin-change rule.

### Changed

* PR template now includes `VERSION_BUMP` and `RELEASE_VERSION` metadata fields.
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
