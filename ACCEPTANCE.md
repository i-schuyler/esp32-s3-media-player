# v0.1 Acceptance (binary checks)

These are the objective gates that define “done” for v0.1.

## Device / platform assumptions
- Target module: ESP32‑S3 WROOM‑2 (32MB flash, 16MB PSRAM).
- Storage: microSD (start with 32GB FAT32).
- RTC: I2C RTC module.

## Wi‑Fi AP + Web UI
1. AP boots and is discoverable with SSID `ESP32-MEDIA` (or configured value).
2. Captive page or landing page loads from `http://192.168.4.1/` within 5 seconds after join.
3. File browser shows SD root listing.

## SD card (SPI) + filesystem
4. SD mounts read-only or read-write (explicitly shown) and lists files/folders.
5. Upload a file (<= 20MB) from phone → appears on SD with correct size.
6. Download the same file → checksum matches (MD5/SHA256 shown in UI or via API).

## Streaming (no phone storage accumulation)
7. Server responds with `Accept-Ranges: bytes` for media types.
8. `Range: bytes=0-1023` returns **206 Partial Content** with valid `Content-Range`.
9. Streaming an MP3 from the phone works without requiring full download (phone may buffer; server supports range).
10. A “no-store” caching header is sent for media endpoints:
    - `Cache-Control: no-store` (or equivalent) on streaming responses.

## RTC
11. RTC time can be read via UI/API and shows a plausible timestamp.
12. Setting time via UI/API persists across reboot (RTC-backed).

## Regression guard
13. CI must be green for any merge, and PR must include `RISK:` field (see PR template).
