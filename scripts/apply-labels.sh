#!/usr/bin/env bash
# scripts/apply-labels.sh
# Applies standard labels used by the automation policy.
# scripts/apply-labels.sh EOF
set -euo pipefail

if ! command -v gh >/dev/null 2>&1; then
  echo "ERROR: gh CLI not found"
  exit 1
fi

labels=(
  "automerge|Auto-merge eligible|0E8A16"
  "needs-human|Requires human decision|D93F0B"
  "feature|New feature|1D76DB"
  "bug|Bug fix|D73A4A"
  "docs|Docs only|0075CA"
  "ci|CI/automation|5319E7"
)

for item in "${labels[@]}"; do
  IFS='|' read -r name desc color <<<"$item"
  gh label create "$name" --description "$desc" --color "$color" --force >/dev/null
  echo "label: $name"
done
