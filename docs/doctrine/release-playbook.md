# Release Workflow Playbook

Status: Stable  
Last Reviewed: 2026-03-29

## Purpose

Operational runbook for PR-only branch governance under Doctrine v0.2.0.

This playbook is repository-specific for FrameKit and governs release
promotion behavior for this repository.

## Alignment Note

- This repository enforces `release/* -> master` promotion.
- The `develop -> master` model documented in
   [Repository Management](repo-management.md) applies to the Doctrine
   repository workflow section only and is not the release path for this
   repository.

## Branch Strategy

- `feature/*` and `fix/*` branches merge into `develop` via PR.
- `release/*` branches are used for release stabilization and promotion.
- Promote to stable with PR `release/* -> master`.
- Back-propagate with PR `release/* -> develop` after `master` merge when release-only commits are not already present in `develop`.
- No direct pushes to protected branches.

## Required Checks

Current required status checks are:

- `develop`: `smoke`, `pr-title`, `issue-link`
- `master`: `smoke`, `pr-title`, `issue-link`, `master-promotion`

Workflow names backing these checks:

- `CI (C++ / CMake)` -> `smoke`
- `CI (PR Title)` -> `pr-title`
- `CI (Issue Link)` -> `issue-link`
- `CI (Master Promotion)` -> `master-promotion`

The `master-promotion` status context is emitted by job `master-promotion` in
workflow file `.github/workflows/ci-master-promotion.yml`.

## Pre-Flight Checklist

Before creating a release branch:

- `develop` is green and up to date with intended integration changes.
- Release scope and deferred items are documented.
- Signed commits are enabled locally and required remotely.
- Branch protections confirm PR-only merges on `master` and `develop`.

## Release Promotion Checklist

1. Create release branch from `develop`.
2. Apply stabilization updates only (docs, release notes, version and packaging touches, final fixes).
3. Open PR `release/* -> master`.
4. Ensure all required checks pass:
   - `smoke`
   - `pr-title`
   - `issue-link`
   - `master-promotion`
5. Merge PR after checks are green.
6. Create signed tag on `master` and push tag.

## Post-Merge Checklist

1. If needed, open PR `release/* -> develop` to back-propagate release-only commits.
2. Verify `develop` and `master` both contain release stabilization commits.
3. Delete merged `release/*` branch.
4. Record release evidence links in the release issue/checklist.

## Failure and Recovery

### `master-promotion` check failed

Cause: PR into `master` does not come from `release/*`.

Recovery:

1. Close the invalid PR.
2. Create a `release/*` branch from `develop`.
3. Re-open promotion PR from `release/*` to `master`.

### Required check missing or pending

Cause: Workflow not active, check name drift, or queued jobs.

Recovery:

1. Confirm workflow files exist in `.github/workflows/` and are active in GitHub Actions.
2. Confirm `.github/workflows/ci-master-promotion.yml` contains job name `master-promotion`.
3. Confirm branch protection required check names exactly match:
   - `smoke`
   - `pr-title`
   - `issue-link`
   - `master-promotion` (for `master`)
4. Re-run workflows or update branch protection check list to match canonical names.

## Branch Protection Consistency Validation

Validated policy for this repository:

- `develop` required checks: `smoke`, `pr-title`, `issue-link`
- `master` required checks: `smoke`, `pr-title`, `issue-link`,
  `master-promotion`

Validation should be re-run whenever branch protection or workflow names
change.

Last validation run: 2026-03-29 via GitHub API branch protection queries.

## Emergency Hotfix

- Use `release/hotfix-*` branch naming.
- Merge `release/hotfix-* -> master` first.
- Merge same branch into `develop` immediately after.
- Preserve signed-commit and signed-tag requirements.

## Quick Verification Commands

```bash
# Required checks on master

gh api repos/<org>/<repo>/branches/master/protection --jq '.required_status_checks.contexts'

# Required checks on develop

gh api repos/<org>/<repo>/branches/develop/protection --jq '.required_status_checks.contexts'

# Admin bypass disabled

gh api repos/<org>/<repo>/branches/master/protection --jq '.enforce_admins.enabled'
gh api repos/<org>/<repo>/branches/develop/protection --jq '.enforce_admins.enabled'
```
