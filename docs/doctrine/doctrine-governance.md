# Doctrine Governance

Status: Draft  
Last Reviewed: 2026-02-28

## Purpose

Defines how doctrine changes are proposed, reviewed, promoted, and tracked.
This keeps doctrine enforceable and avoids policy drift.

## Scope

Applies to all markdown standards in this repository.

## Rule Precedence

When two doctrine rules conflict, resolve in this order:

1. `repo-management.md`
2. `project-standards.md`
3. `coding.md`
4. `naming.md`
5. `identity.md`
6. `usernames.md`
7. `AI_CONTEXT.md`

Conflict handling:

- The stricter rule wins.
- If strictness is equal, the file with the newer `Last Reviewed` date wins.
- If still ambiguous, create a Doctrine Change Record before applying either rule.

## Change Control Workflow

1. Create a branch from `develop`.
2. Edit doctrine files and update `Last Reviewed`.
3. Add a Doctrine Change Record from template:
   - `templates/doctrine-change-record-template.md`
4. If the change affects existing repositories, include migration steps and deadline.
5. Update `RELEASE_NOTES.md` for the release that introduces the change.
6. Merge into `develop`, then release using the repo workflow in `repo-management.md`.

## Status Promotion Rules

Promote `Draft -> Stable` only when all are true:

- Rule language is specific and testable.
- At least one concrete example is included where needed.
- No unresolved TODO markers remain.
- Rule was validated in at least one real repository.
- No conflicts with higher-precedence doctrine files.

## Release Gates

Before publishing a doctrine release tag:

- Tag must be signed (`git tag -s`).
- Tag must point to the intended release commit on `master`.
- GitHub must show the tag as verified (requires SSH key registered as a Signing Key).
- If an unsigned tag was pushed, replace it with a signed tag on the same commit and force-push the tag.

## Review Cadence

- Full doctrine review every 90 days.
- Targeted review after incidents (security, naming collisions, publish mistakes, broken standards).
- Any file older than 180 days without review should be treated as potentially stale.

## Deviation Policy

Deviations are allowed only when documented.

Required deviation fields:

- Reason
- Scope
- Owner
- Expiration date
- Rollback or convergence plan

Store deviations in the corresponding Doctrine Change Record.

## Metadata Requirement

Every doctrine markdown file must include:

- `Status: Draft | Stable | Deprecated`
- `Last Reviewed: YYYY-MM-DD`
