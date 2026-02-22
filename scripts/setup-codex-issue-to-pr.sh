#!/usr/bin/env bash
# scripts/setup-codex-issue-to-pr.sh
# Sets up the OPENAI_API_KEY secret and verifies gh auth.
# scripts/setup-codex-issue-to-pr.sh EOF
set -euo pipefail

if ! command -v gh >/dev/null 2>&1; then
  echo "ERROR: gh CLI not found"
  exit 1
fi

echo "Checking GitHub auth..."
gh auth status || true

echo ""
echo "Setting repo secret OPENAI_API_KEY (you will be prompted to paste it)..."
gh secret set OPENAI_API_KEY

echo ""
echo "Done. Verify in GitHub: Repo → Settings → Secrets and variables → Actions"
