# ADR-0001: Service Context Model and Freeze Policy

Status: Accepted
Date: 2026-03-29

## Context

FrameKit requires predictable service registration and lookup semantics across
bootstrap, running, and shutdown phases.

## Decision

Adopt a scoped service context with explicit phases:

1. Open registration window.
2. One-way freeze before Running.
3. Deterministic teardown with ownership-aware lifetime handling.

Typed lookups are required, and registration after freeze is a contract error.

## Consequences

- Service graph is stable during runtime.
- Concurrent read-only lookup after freeze is feasible by policy.
- Late binding flexibility is reduced in favor of deterministic behavior.

## Related Docs

- [Service Context Contract and Freeze Policy](../service-context-contract-freeze.md)
