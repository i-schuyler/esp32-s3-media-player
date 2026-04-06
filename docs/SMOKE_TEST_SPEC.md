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
Purpose: preserve known-good OTA behavior with a lightweight repeatable regression check.

When to run:
- Required before merge for OTA-sensitive changes (see `docs/AUTOMATION_POLICY.md`).
- Optional for non-OTA-sensitive docs-only changes.

Known-good baseline asset:
- Source: GitHub Releases
- Version: `v0.1.8`
- Artifact: release `firmware.bin` built for `esp32-s3-devkitc-1-n32r16v`

Manual regression procedure:
1. Open `http://192.168.4.1/ota`.
2. Upload the known-good `v0.1.8` release `firmware.bin`.
3. Verify OTA stage/status reaches explicit success (after upload/validate/apply stages) with no terminal failure.
4. Verify device reboots and reconnects to AP/UI successfully.
5. Verify displayed firmware version after reboot changed to the uploaded version as expected.
6. Verify OTA diagnostics (if present) show running/target partition details consistent with a successful apply/switch.
7. Verify core app pages still load after reboot:
   - `http://192.168.4.1/`
   - `http://192.168.4.1/files`
   - `http://192.168.4.1/ota`
8. Re-run streaming spot-check (`Range: bytes=0-1023`) to confirm no OTA-adjacent regression in existing media flow.

Failure triage notes:
- If upload/network is interrupted or image is invalid, expect explicit failure status with actionable guidance.
- After a failed OTA attempt, app should remain reachable for retry/recovery.
