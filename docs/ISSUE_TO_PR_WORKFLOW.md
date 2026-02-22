# Issue → PR automation (Codex)

Goal: your only input is creating a GitHub Issue. A workflow then asks Codex to draft a PR.

## Requirements
- A repository secret named `OPENAI_API_KEY` must be set.
- Branch protection should require CI (`policy-gates`) and block direct pushes to `main`.
- The Codex workflow should only have the permissions it needs.

## How it works
- Trigger: `issues: opened` (new Issue created)
- Codex reads the issue body and produces:
  - a slice plan (acceptance criteria + test plan),
  - a branch with minimal changes,
  - a PR that references the issue and follows the PR template fields.

## Safety gates
- PRs must pass `.github/workflows/ci.yml` policy gates.
- PR template must include:
  - `RISK: ...`
  - `BREAKING: ...`
  - `NEEDS_HIL: no` (until HIL runner exists)
- Auto-merge should only be enabled when you explicitly apply the `automerge` label.

## Set the secret (Termux)
From repo root:
```bash
gh secret set OPENAI_API_KEY
# paste key, then Ctrl-D
```

## Notes
- If you later prefer OAuth/Codex subscription instead of an API key, we can adjust the workflow.

## Notification
On success, the workflow comments on the originating issue with `PR created: <link>`.
