# Quick Start (v0.1)

This guide covers first boot through basic streaming and OTA.

## 1) Flash firmware
Build and upload for `esp32-s3-devkitc-1-n32r16v`:

```bash
pio run -e esp32-s3-devkitc-1-n32r16v
pio run -e esp32-s3-devkitc-1-n32r16v -t upload
```

## 2) Connect to device AP
- SSID: `ESP32-MEDIA` (unless overridden by `DEFAULT_AP_SSID` build flag)
- Password: `12345678`
- Device UI IP: `http://192.168.4.1/`

## 3) Open the web UI
- Home page: `http://192.168.4.1/`
- File browser: `http://192.168.4.1/files`
- OTA page: `http://192.168.4.1/ota`

## 4) Upload files
1. Open `/files`.
2. Use the upload form (`Choose file` -> `Upload`).
3. Wait for `upload complete`.

## 5) Browse files
- `/files` shows SD root entries.
- Each file row includes:
  - `download` link (`/download?path=...`)
  - `stream` link (`/stream?path=...`)

## 6) Stream music
1. Tap `stream` in `/files`, or open VLC network stream:
   - `http://192.168.4.1/stream?path=/your-file.mp3`
2. Firmware supports HTTP byte ranges (`Accept-Ranges: bytes`).
3. Smoke check example:
   - Request `Range: bytes=0-1023`
   - Expect `206 Partial Content` with `Content-Range`.

## 7) Basic OTA update
1. Open `/ota`.
2. Upload a board-matching `firmware.bin`.
3. Watch upload progress and status text.
4. Success path: status reports success and device reboots.
5. Failure/interrupt path: status reports failure; retry upload.
