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

## [v0.1.13] - 2026-04-06

### Added

* Issue #47 implementation plan mapping SD SPI ownership/init-flow scope to `ACCEPTANCE.md` and `docs/SMOKE_TEST_SPEC.md`.
* SD init trace now explicitly records an authoritative flow (`sd_begin_then_cmd0_diag_on_failure`) with retained pin map/speed/outcome diagnostics.

### Changed

* SD bring-up now uses a dedicated SPI instance for SD ownership isolation instead of the global shared SPI object.
* Per-attempt init sequence is simplified to `prime bus -> SD.begin`, with raw CMD0 diagnostics captured only on failed/ambiguous attempts to reduce probe/init overlap.

### Fixed

* Reduced ambiguity from overlapping raw-probe and `SD.begin` behavior by making `SD.begin` the authoritative bring-up step while keeping browser-visible root-cause diagnostics.

### Notes

* PR:
* Issue: https://github.com/i-schuyler/esp32-s3-media-player/issues/47

## [v0.1.12] - 2026-04-06

### Added

* Issue #44 implementation plan mapping SD SPI-mode hardening scope to `ACCEPTANCE.md` and `docs/SMOKE_TEST_SPEC.md`.
* Browser-visible SD init trace now includes raw CMD0 response detail (`R1` byte + poll count), explicit dummy-clock issuance, and CS transition-state diagnostics.

### Changed

* SD bring-up attempts now use one clearer authoritative pre-mount sequence per attempt: bus prime, SPI-mode preclock/CMD0 probe, then `SD.begin`.
* Rolling debug log mirrors the deeper bounded SD trace so CMD0/CS failures are diagnosable from the browser.

### Fixed

* Reduced ambiguity in no-card-response triage by exposing raw CMD0/CS probe evidence without requiring USB serial.

### Notes

* PR:
* Issue: https://github.com/i-schuyler/esp32-s3-media-player/issues/44

## [v0.1.11] - 2026-04-06

### Added

* Issue #41 implementation plan mapping deeper SD init diagnostics scope to `ACCEPTANCE.md` and `docs/SMOKE_TEST_SPEC.md`.
* Browser-visible SD diagnostics now include actual SD pin map in use (CS/SCK/MISO/MOSI), every attempted SPI init speed, and bounded attempt-level init trace details.

### Changed

* Existing SD bring-up path now records attempt-level CMD0 outcome and card-init stage progression into the existing rolling debug log for browser-only troubleshooting.
* `/api/sd/status` and `/sd-diagnostics` now expose deeper SD init visibility while keeping diagnostics bounded/readable.

### Fixed

* Reduced blind spots when SD bring-up fails before mount by surfacing lower-level init progress/failure detail without requiring USB serial logs.

### Notes

* PR:
* Issue: https://github.com/i-schuyler/esp32-s3-media-player/issues/41

## [v0.1.10] - 2026-04-06

### Added

* Issue #39 implementation plan mapping this release-prep/docs slice to `ACCEPTANCE.md` and `docs/SMOKE_TEST_SPEC.md`.

### Changed

* Prepared next release automation input by adding a new newest-first changelog release entry sourced from current `main`.
* SD bring-up diagnostics now clearly separate card-communication preflight failures from init/select and mount/filesystem failures for known-good FAT32 card triage (latest merged SD bring-up work on `main`, via Issue #37 / PR #38).

### Fixed

* Release-prep gap where the latest merged SD bring-up reliability work was not yet represented in the top changelog entry used by automated tag/release asset creation.

### Notes

* PR:
* Issue: https://github.com/i-schuyler/esp32-s3-media-player/issues/39

## [v0.1.9] - 2026-04-06

### Added

* Issue #35 implementation plan mapping OTA regression-protection scope to `ACCEPTANCE.md` and `docs/SMOKE_TEST_SPEC.md`.
* Explicit OTA retest trigger list in `docs/AUTOMATION_POLICY.md` for OTA-sensitive change classes.

### Changed

* OTA smoke spec now defines a lightweight regression procedure anchored to the known-good `v0.1.8` release asset flow (success status, reboot/reconnect, version check, partition diagnostics, and post-reboot core-page checks).

### Fixed

* Documentation/process gap where OTA-sensitive changes were not clearly required to run OTA smoke retest before merge.

### Notes

* PR:
* Issue: https://github.com/i-schuyler/esp32-s3-media-player/issues/35

## [v0.1.8] - 2026-04-06

### Added

* Issue #33 implementation plan mapping release-prep scope to `ACCEPTANCE.md` and `docs/SMOKE_TEST_SPEC.md`.

### Changed

* Release-prep changelog now includes a newest-first entry for the next automated tag/release cycle.
* Version source-of-truth remains `CHANGELOG.md` with `vX.Y.Z` formatting aligned to release automation parsing.

### Fixed

* ESP32-S3 N32R16V memory/build parity release notes are now represented at the top of the changelog for current-main release prep, including explicit flash/memory/PSRAM/partition parity diagnostics in release checks.

### Notes

* PR:
* Issue: https://github.com/i-schuyler/esp32-s3-media-player/issues/33

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
