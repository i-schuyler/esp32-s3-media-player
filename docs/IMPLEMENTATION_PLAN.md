# Issue #1 Implementation Plan

Issue URL: https://github.com/i-schuyler/esp32-s3-media-player/issues/1

## Scope
- Add a minimal PlatformIO firmware project targeting `esp32-s3-devkitc-1-n32r16v`.
- Implement Wi-Fi AP + SD file browser + upload/download + HTTP Range streaming.
- Enforce target-board compilation in CI policy gates.

## Mapping to ACCEPTANCE.md
- Board/build plumbing aligns to platform requirements and CI regression guard in item 13.
- Wi-Fi AP + landing page + root listing aligns to items 1-3.
- SD mount/list + upload/download align to items 4-5.
- Range + 206 + no-store aligns to items 7-10.
- Checksum display/API from item 6 is deferred in this minimal slice.
- RTC (items 11-12) is explicitly out of scope for this issue slice.

## Mapping to docs/SMOKE_TEST_SPEC.md
- Boot/AP log lines align to Serial boot banner pass regexes.
- SD mount/list log lines align to SD mount + read pass regexes.
- Range handling emits expected stream logs and HTTP behavior for `Range: bytes=0-1023`.
- VLC Android playback remains a manual smoke step on hardware.

## Test plan
- CI: run `pio run -e esp32-s3-devkitc-1-n32r16v` in `ci.sh`.
- Local/manual: AP join, browse root, upload <=20MB, download, `curl` Range check, VLC stream playback.
