#!/usr/bin/env bash
# ci.sh
# Single CI entrypoint: policy gates + (future) firmware build/tests + artifact export.
# ci.sh EOF
set -euo pipefail

echo "== ci.sh: policy gates =="

# PR template / policy checks are enforced in GitHub Actions via .github/workflows/ci.yml.
# Locally, we can at least sanity-check required files exist.
required=(
  "ACCEPTANCE.md"
  "docs/AUTOMATION_POLICY.md"
  "docs/SMOKE_TEST_SPEC.md"
  ".github/pull_request_template.md"
)

for f in "${required[@]}"; do
  if [[ ! -f "$f" ]]; then
    echo "MISSING: $f"
    exit 1
  fi
done

echo "OK: required policy files present."

if [[ -f "platformio.ini" ]]; then
  echo "Detected PlatformIO project; running build for esp32-s3-devkitc-1-n32r16v."
  if ! command -v pio >/dev/null 2>&1; then
    echo "PlatformIO not found; installing with pip user base..."
    python3 -m pip install --user platformio
    export PATH="$HOME/.local/bin:$PATH"
  fi
  pio run -e esp32-s3-devkitc-1-n32r16v
elif [[ -f "CMakeLists.txt" ]] && [[ -d "main" || -d "components" ]]; then
  echo "Detected ESP-IDF-style layout; run idf.py build here (future)."
else
  echo "No firmware build configured yet (expected at bootstrap stage)."
fi

echo "== ci.sh: done =="
