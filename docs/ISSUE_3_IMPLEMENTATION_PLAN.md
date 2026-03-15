# Issue #3 Implementation Plan

Issue: https://github.com/i-schuyler/esp32-s3-media-player/issues/3

## Scope
- Rename user-facing software naming to **ESP32-S3 Media Server** in docs/UI where safe.
- Add a web OTA update page and OTA POST flow with progress and clear status reporting.
- Align SD SPI pin mapping with the required wiring documentation.
- Add `CHANGELOG.md` with prepend-order release template.
- Update docs/process so merged PRs must update changelog and create a release tag.

## Mapping to ACCEPTANCE.md
- `ACCEPTANCE.md` #7-10: preserve existing HTTP range streaming behavior (`Accept-Ranges`, `206`, `Content-Range`, `no-store`).
- `ACCEPTANCE.md` #13: keep CI green and policy-gate compatible PR metadata.

Issue-specific acceptance beyond `ACCEPTANCE.md`:
- OTA web update UI + upload flow with progress/status and graceful failure handling.
- Wiring block is included verbatim in README, and SD code pinning matches docs.
- Changelog template + release hygiene process docs are added.

## Mapping to docs/SMOKE_TEST_SPEC.md
- Preserve boot markers (`BOOT: OK`, `WIFI AP STARTED`, `ESP32-MEDIA: READY`) for smoke regex compatibility.
- Preserve SD mount/read/list markers.
- Preserve streaming smoke behavior and logs for Range requests.
- Add OTA manual verification notes aligned with the issue test plan and existing smoke process.

## Verification Plan
- Run `./ci.sh` (policy files present + PlatformIO compile for `esp32-s3-devkitc-1-n32r16v`).
- Manual OTA check:
  - Open `/ota`, upload valid `firmware.bin`, observe progress + success message + reboot.
  - Interrupt upload/network to verify clear failure status and continued app availability.
- Manual stream check:
  - `Range: bytes=0-1023` => `206` + `Content-Range`; VLC Android playback remains functional.
