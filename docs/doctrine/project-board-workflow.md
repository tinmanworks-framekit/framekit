# Project Board Workflow

Status: Draft
Last Reviewed: 2026-03-29

## Purpose

Define how work moves through the FrameKit v2 project board and how issue tracking aligns with doctrine requirements.

## Board Scope

- Project: `FrameKit v2 Development`
- Owner: `tinmanworks-framekit`
- Work model: issue-driven only for implementation and architecture changes

## Columns and State Mapping

Board status values:

- `Backlog`
- `Ready` (used as Todo equivalent)
- `In progress`
- `In review`
- `Done`

Policy mapping:

- `Backlog` -> ideas and unsized items
- `Ready` -> issue is scoped and implementable
- `In progress` -> active implementation or documentation work
- `In review` -> PR or review evidence exists
- `Done` -> merged and validated, or closed with accepted rationale

## Required Project Fields

Standard fields already present:

- `Status`
- `Priority`
- `Repository`
- `Iteration`

Custom fields added for v2 planning:

- `Depends On` (TEXT)
- `Risk` (SINGLE_SELECT: Low, Medium, High, Critical)
- `Architecture Area` (SINGLE_SELECT)

## Automation Rules

Automation must remain predictable and minimal.

Recommended behavior:

1. New scoped issues start in `Ready`.
2. Start of implementation moves item to `In progress`.
3. Opening a PR moves item to `In review`.
4. Merge or accepted closure moves item to `Done`.

When built-in automation is unavailable or insufficient, status updates are performed manually and must be reflected in issue comments.

## Issue Discipline

- No implementation without an issue.
- Every issue includes scope, non-scope, and acceptance criteria.
- Keep changes issue-scoped with small commits.
- Link evidence (tests, docs, commit references, PR) before moving to `Done`.
