# ESP32-S3 Media Server

ESP32-S3 Media Server is firmware for `esp32-s3-devkitc-1-n32r16v` that:
- starts a local Wi-Fi AP and serves a browser UI,
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
- SD **GND** → Row 1 Right (GND)
- SD **VCC** → Row 21 Right (3V3)
- SD **CS** → Row 4 Right (**IO13**)
- SD **SCK** → Row 5 Right (**IO12**)
- SD **MISO** → Row 6 Right (**IO11**)
- SD **MOSI** → Row 7 Right (**IO10**)

## OTA Web Update
- Open `http://192.168.4.1/ota`.
- Upload a board-matching `firmware.bin`.
- The page reports upload progress and OTA status.
- On success, the device reports completion and reboots.
- On interruption/error, OTA failure is reported and normal app flow continues.

## Validation references
- `ACCEPTANCE.md` for v0.1 binary checks
- `docs/SMOKE_TEST_SPEC.md` for smoke verification details

## Changelog and releases
- Update `CHANGELOG.md` in prepend order (newest entry first) for each merged PR.
- Tag a release that matches the changelog update after merge.
- Follow `docs/RELEASE_AND_PIN_POLICY.md` for version bump and pin/docs alignment guardrails.

## License
MIT (see `LICENSE`).
