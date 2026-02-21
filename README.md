# ESP32-S3 Media Player (automation-first)

Goal: an ESP32-S3 device that serves files from microSD over a local Wi‑Fi AP and enables playback/viewing on a phone
without accumulating storage on the phone (prefer streaming via HTTP Range requests).

This repo is set up for **Pipeline A**:
spec → issues → PRs → CI gates → (optional) auto-merge for low-risk changes.

## Quick start (repo bootstrap)
1. Unzip the bootstrap bundle into the repo root.
2. Commit and push.
3. Create labels (script provided).

See:
- `ACCEPTANCE.md` for v0.1 binary checks
- `.github/` for PR template, issue template, and CI gate
- `docs/` for the automation policy

## Streaming approach
Preferred: HTTP streaming with `Accept-Ranges: bytes` and correct `206 Partial Content` responses to Range requests.
Most mobile players will only buffer small portions when Range is supported.

## License
MIT (see `LICENSE`).

## Issue → PR automation
See `docs/ISSUE_TO_PR_WORKFLOW.md` and `.github/workflows/codex-issue-to-pr.yml`.
