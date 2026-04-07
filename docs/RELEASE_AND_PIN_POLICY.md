# Release and Pin Alignment Policy

## Version format
- Canonical version format is `vX.Y.Z` (lowercase `v`) in:
  - `CHANGELOG.md` release headers
  - Git tags
  - GitHub Releases

## Qualifying PR version discipline
- Every PR must declare:
  - `VERSION_BUMP: none|patch|minor|major`
  - `RELEASE_VERSION: vX.Y.Z|n/a`
- If `RELEASE_VERSION` is not `n/a`, `CHANGELOG.md` must contain a matching header:
  - `## [vX.Y.Z] - YYYY-MM-DD`
- PR authors and reviewers must verify these fields and the changelog header before merge.

## Post-merge release discipline
1. Merge PR with matching changelog release header.
2. Create tag exactly matching changelog version (for example `v0.1.2`).
3. Publish GitHub Release from that tag.
4. Do not mix `Vx.y.z` and `vx.y.z` casing; always use lowercase `v`.

## Firmware/docs pin alignment (docs-first)
- Wiring docs and firmware constants must never drift.
- Source of truth in firmware:
  - `src/main.cpp` SD pin profiles:
    - `default-io13-12-11-10`: `CS=13`, `SCK=12`, `MISO=11`, `MOSI=10`
    - `alt-io4-5-6-7`: `CS=4`, `SCK=5`, `MISO=6`, `MOSI=7`
  - `src/main.cpp` AP password: `kDefaultApPassword="12345678"`
- Published docs that must match:
  - `README.md` wiring section
  - `docs/QUICK_START.md` AP access details

Pin change rule:
1. Update docs first or in the same PR as firmware pin constant changes.
2. If docs and firmware cannot both be updated in one PR, do not merge.
