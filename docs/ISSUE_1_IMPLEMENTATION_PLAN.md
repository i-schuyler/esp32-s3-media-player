# Issue #1 Implementation Plan

Issue: https://github.com/i-schuyler/esp32-s3-media-player/issues/1

## Scope
- Add PlatformIO board/build plumbing for `esp32-s3-devkitc-1-n32r16v` with 32MB flash + OPI PSRAM settings.
- Add minimal AP + SD file browser + upload/download + HTTP Range media streaming firmware paths.
- Add CI compile step for the target board env.

## Mapping To ACCEPTANCE.md
- `ACCEPTANCE.md` #1-3 (Wi-Fi AP + landing + file browser): implemented via AP boot and `/` + `/files`.
- `ACCEPTANCE.md` #4-6 (SD mount/read/upload/download): implemented via SD mount logs, upload endpoint (<=20MB), and download endpoint.
- `ACCEPTANCE.md` #7-10 (Range streaming + cache header): implemented via `/stream` with `Accept-Ranges: bytes`, `206` + `Content-Range`, and `Cache-Control: no-store`.
- `ACCEPTANCE.md` #13 (CI green): compile now runs in `ci.sh` when `platformio.ini` exists.

Note: `ACCEPTANCE.md` #11-12 (RTC persistence) are not requested by Issue #1 and are out of scope for this slice.

## Mapping To docs/SMOKE_TEST_SPEC.md
- Serial boot regexes: emits `BOOT: OK`, `WIFI AP STARTED`, `ESP32-MEDIA: READY`.
- SD smoke regexes: emits `SD: MOUNTED`, `SD: LIST OK`, `SD: READ OK`.
- Streaming smoke regexes: emits `HTTP: ACCEPT-RANGES BYTES` and `HTTP: RANGE <start>-<end> -> 206`.
- Manual VLC path: `/stream?path=/...` endpoint is provided for Android VLC verification.

## Verification Plan
- CI: run `./ci.sh` to compile `esp32-s3-devkitc-1-n32r16v`.
- Manual:
  - Join AP and open `http://192.168.4.1/`.
  - Upload/download file through `/files`.
  - Request `/stream?path=/file.mp3` with `Range: bytes=0-1023`; expect `206` and `Content-Range`.
