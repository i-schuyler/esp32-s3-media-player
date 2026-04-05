# ISSUE #29 Implementation Plan

## Goal
Add a browser-visible, bounded, rolling debug log for OTA + SD troubleshooting so key runtime diagnostics are available without USB serial access.

## Scope (minimal)
- Add an in-memory bounded rolling log buffer in firmware.
- Mirror key OTA and SD diagnostic lines into that buffer.
- Expose browser-visible log via:
  - `GET /debug-log` (simple readable page)
  - `GET /api/debug-log` (JSON)
- Keep existing OTA, SD browser, streaming, and diagnostics behavior unchanged outside this logging slice.

## Acceptance mapping
- `ACCEPTANCE.md` #3/#4: SD browser/listing behavior must remain intact.
- `ACCEPTANCE.md` #7/#8/#9/#10: HTTP Range streaming behavior must remain unchanged (`Accept-Ranges`, `206`, `Content-Range`, `no-store`).
- Issue acceptance #1/#4/#6: browser-visible rolling log, bounded memory, and runtime availability across navigation.
- Issue acceptance #2/#3: include key OTA + SD events/stages/errors currently used for troubleshooting.
- Issue acceptance #5: no regressions outside logging scope.
- Issue acceptance #7: compile target remains `esp32-s3-devkitc-1-n32r16v`.

## Smoke/test mapping
- Keep `docs/SMOKE_TEST_SPEC.md` SD and OTA checks as baseline regression guard.
- Additional manual checks for this issue:
  1. Open `/debug-log` and `/api/debug-log` without USB serial; verify entries render.
  2. Trigger OTA failure path and verify OTA stage/error lines appear in rolling log.
  3. Trigger SD mount/init failure and verify SD stage/detail lines appear in rolling log.
  4. Force enough events to exceed capacity; verify old entries roll off (bounded RAM).
  5. Re-run streaming check (`Range: bytes=0-1023`) to confirm unchanged behavior.

## Implementation steps
1. Add bounded ring-buffer state + safe line sanitization in `src/main.cpp`.
2. Add helper logging functions that keep existing serial output while mirroring selected lines to the ring buffer.
3. Add `/api/debug-log` JSON endpoint and `/debug-log` HTML page with periodic refresh.
4. Link debug-log page from existing UI pages where OTA/SD troubleshooting occurs.
5. Validate with policy-gates/build (`ci.sh`) and keep changes minimal.
