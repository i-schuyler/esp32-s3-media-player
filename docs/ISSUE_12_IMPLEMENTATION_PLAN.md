# Issue #12 Implementation Plan

Issue: https://github.com/i-schuyler/esp32-s3-media-player/issues/12

## Scope
- Add process guardrails for deterministic version bumping across PR metadata, changelog entries, and release tagging.
- Add explicit policy that firmware pin constants and published wiring docs must stay aligned (docs-first or same PR).
- Add a quick start guide for first-time users and link it from `README.md`.
- Keep firmware behavior unchanged for OTA/web UI/streaming.

## Mapping to ACCEPTANCE.md
- `ACCEPTANCE.md` #7-10: preserve current streaming behavior (Range support and headers); no streaming code changes in this slice.
- `ACCEPTANCE.md` #13: tighten PR policy metadata checks in CI (`RISK`, `BREAKING`, `NEEDS_HIL`, version fields) while keeping CI green.

Issue-specific acceptance beyond `ACCEPTANCE.md`:
- Versioning discipline is explicitly defined through PR fields + changelog header checks.
- Pin/wiring alignment policy is documented with firmware-to-doc pin mapping and docs-first change rule.
- A quick start doc is added and linked from `README.md` with AP, UI, upload, browse, stream, and OTA basics.

## Mapping to docs/SMOKE_TEST_SPEC.md
- Preserve boot markers and startup flow.
- Preserve manual streaming smoke expectation (`Range: bytes=0-1023` -> `206` + `Content-Range`).
- Preserve OTA manual path (`/ota`) and expected success/failure behavior.

## Verification Plan
- Run `./ci.sh` to validate policy files and compile `esp32-s3-devkitc-1-n32r16v`.
- Confirm quick start instructions match current firmware behavior:
  - AP SSID/password and `http://192.168.4.1/` paths.
  - `/files` upload + browsing + stream links.
  - `/ota` upload/update flow.
- Confirm versioning and pin policy docs are linked and internally consistent.
