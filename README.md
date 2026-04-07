# ESP32-S3 Media Server

ESP32-S3 Media Server is firmware for `esp32-s3-devkitc-1-n32r16v` that:
- starts a local Wi-Fi AP and serves a browser UI,
- shows the running firmware version on the main and OTA UI pages,
- mounts and browses microSD media files,
- supports upload/download + HTTP Range streaming for mobile players (VLC),
- provides a web OTA firmware update flow with progress/status reporting.

## Quick start
- See `docs/QUICK_START.md` for first-time setup, AP credentials, web UI paths, upload/browse/stream usage, and basic OTA flow.

## Build and Upload (`esp32-s3-devkitc-1-n32r16v`)
1. Install PlatformIO Core (CLI) and Python 3.
2. Build:
```bash
pio run -e esp32-s3-devkitc-1-n32r16v
```
3. Upload firmware:
```bash
pio run -e esp32-s3-devkitc-1-n32r16v -t upload
```
4. Optional serial monitor:
```bash
pio device monitor -b 115200
```

## Wiring
### DS3231 RTC (I2C)
- RTC **GND** → Row 1 Right (GND)
- RTC **VCC** → Row 21 Right (3V3)
- RTC **SCL** → Row 8 Right (**IO9**)
- RTC **SDA** → Row 11 Right (**IO8**)
### MicroSD (SPI)
Default SD pin profile (`default-io13-12-11-10`):
- SD **GND** → Row 1 Right (GND)
- SD **VCC** → Row 21 Right (3V3)
- SD **CS** → Row 4 Right (**IO13**)
- SD **SCK** → Row 5 Right (**IO12**)
- SD **MISO** → Row 6 Right (**IO11**)
- SD **MOSI** → Row 7 Right (**IO10**)

Alternate SD pin profile (`alt-io4-5-6-7`) for bring-up experiments:
- SD **CS** → **IO4**
- SD **SCK** → **IO5**
- SD **MISO** → **IO6**
- SD **MOSI** → **IO7**

## OTA Web Update
- Open `http://192.168.4.1/ota`.
- Upload a board-matching `firmware.bin`.
- The page reports upload progress, OTA status, and a build/config diagnostics block (env, board, flash size, flash mode, memory type, PSRAM type, partition layout, running/target partition).
- On success, the device reports completion and reboots.
- On interruption/error, OTA failure is reported and normal app flow continues.

## Validation references
- `ACCEPTANCE.md` for v0.1 binary checks
- `docs/SMOKE_TEST_SPEC.md` for smoke verification details

## Changelog and releases
- Update `CHANGELOG.md` in prepend order (newest entry first) for each merged PR.
- Tag a release that matches the changelog update after merge.
- Follow `docs/RELEASE_AND_PIN_POLICY.md` for version bump and pin/docs alignment guardrails.
- Before merging to `main`, prepend a changelog release entry using `## [vX.Y.Z] - YYYY-MM-DD`.
- After merge, GitHub Actions (`.github/workflows/release.yml`) automatically tags that version, creates a GitHub release, and uploads `media_server_vX_X_X.bin` built for `esp32-s3-devkitc-1-n32r16v`.
- If an existing release is missing `media_server_vX_X_X.bin`, run `.github/workflows/manual-release-attach.yml` manually with the existing release tag (for example `v0.1.3`).
- Release automation verifies env/board/flash-size/flash-mode/memory-type/PSRAM-type/partition parity with the same `pio run -e esp32-s3-devkitc-1-n32r16v` USB build path before publishing artifacts.

## License
MIT (see `LICENSE`).
