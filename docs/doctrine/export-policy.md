# Doctrine Export Policy

Status: Stable  
Last Reviewed: 2026-06-04

## Purpose

Defines which doctrine files are safe to copy into public repositories, templates, and generated project snapshots.

## Default Export Rule

Default doctrine exports must be contributor-neutral and public-safe.

Exported files must not contain:

- maintainer legal identity or private account ownership details
- private contact, recovery, credential, infrastructure, or endpoint details
- personal notes that are not required for outside contributors
- project-specific decisions from an unrelated repository

## Public Export Allowlist

The default bootstrap tools may export:

- `templates/AGENTS.md` as `AGENTS.md`
- `AI_CONTEXT.md`
- `coding.md`
- `doctrine-governance.md`
- `export-policy.md`
- `naming.md`
- `project-standards.md`
- `repo-management.md`
- `templates/repo-visibility-note-template.md`
- `templates/doctrine-change-record-template.md`

## Non-Exported Identity Overlay

Identity-specific files, account naming ladders, and maintainer-local preferences are source doctrine or private overlay material.

They must not be copied into public templates or generated repositories by default. If a downstream repository needs such material, document that decision locally and keep it out of reusable public template baselines unless it is contributor-neutral.

## Contamination Guard

Bootstrap and CI checks should fail when a public export contains maintainer-private identity markers or other explicitly non-exportable content.

When a guard fails, remove the non-exportable content from the exported file rather than weakening the guard.
