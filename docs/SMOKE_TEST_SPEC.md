# Smoke Test Spec (v0.1)

This file defines deterministic smoke tests and pass/fail regex patterns.
It is intentionally simple so it can be executed manually today and automated later (HIL runner).

## Serial boot banner
**Goal:** confirm firmware boots and starts services.

**Pass regex (any one is acceptable):**
- `^BOOT: OK`
- `^ESP32-MEDIA: READY`
- `^WIFI\s+AP\s+STARTED`

**Fail regex (any match fails):**
- `Guru Meditation Error`
- `abort\(\)`
- `Brownout detector`
- `CORRUPT HEAP`
- `WDT`

## SD mount + read
**Pass regex:**
- `^SD: MOUNTED`
- `^SD: LIST OK`
- `^SD: READ OK`

**Fail regex:**
- `^SD: MOUNT FAILED`
- `^SD: TIMEOUT`
- `^SD: CRC ERROR`

Manual browser diagnostics check when SD is unavailable:
- Open `http://192.168.4.1/files`.
- Expect a user-friendly SD status and next-step guidance (not only a generic unavailable message).
- Open `http://192.168.4.1/sd-diagnostics`.
- Expect stage-specific status + actionable troubleshooting guidance.

## RTC read
**Pass regex:**
- `^RTC: READ OK`
- `^RTC: TIME OK`

**Fail regex:**
- `^RTC: NOT FOUND`
- `^RTC: I2C ERROR`

## Streaming (HTTP Range)
Manual verification (phone VLC or curl from laptop):
- Request: `Range: bytes=0-1023`
- Expect: HTTP 206 and header `Content-Range: bytes 0-1023/<total>`

**Pass regex (server log):**
- `^HTTP: RANGE 0-1023 -> 206`
- `^HTTP: ACCEPT-RANGES BYTES`

**Fail regex:**
- `^HTTP: RANGE .* -> 200`
- `^HTTP: RANGE ERROR`

## Optional: play 2s clip (future local playback)
This is a placeholder until we add local audio-out. For now, streaming playback on phone is the test.

## OTA web update (manual)
Manual verification:
- Open `http://192.168.4.1/ota`.
- Upload a valid `firmware.bin` for `esp32-s3-devkitc-1-n32r16v`.
- Expect browser upload progress updates plus explicit OTA stage text (upload, upload_complete, validate/apply, final success/failure).
- Expect OTA diagnostics to include build env and running/target partition details for failure triage.
- Success path: status indicates success and device reboots.
- Failure path (interrupt upload/network or invalid image): browser status indicates failure with actionable next steps and app remains reachable after retry.
