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
