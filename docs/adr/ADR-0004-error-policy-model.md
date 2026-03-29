# ADR-0004: Error Policy Matrix and Escalation Rules

Status: Accepted
Date: 2026-03-29

## Context

Different loop profiles require different tolerance for operational faults while
still enforcing critical safety and contract invariants.

## Decision

Adopt a fault-class-by-profile policy matrix with:

- explicit fail-fast conditions,
- bounded degrade paths,
- mandatory escalation from degrade to fail-fast when limits are exceeded.

Certain invariants are always fail-fast regardless of profile.

## Consequences

- Error handling is explicit and testable.
- Profile behavior remains predictable under fault.
- Deterministic profile enforces strict escalation rules.

## Related Docs

- [Error Policy Matrix by Loop Profile](../error-policy-matrix.md)
