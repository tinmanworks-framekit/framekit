# ADR-0002: Module Loading Model (Static Baseline)

Status: Accepted
Date: 2026-03-29
Last Reviewed: 2026-03-29

## Context

Phase 0 requires deterministic module lifecycle behavior and dependency-safe
startup/shutdown ordering.

## Decision

Use a static module registration baseline for v2 contracts:

- Modules are declared before startup.
- Dependencies form a validated DAG.
- Startup uses deterministic topological order.
- Shutdown uses reverse topological order.

Dynamic loading remains optional future work and is out of baseline scope.

## Consequences

- Predictable startup/shutdown behavior.
- Stronger validation before entering Running state.
- Runtime extensibility is intentionally constrained in baseline v2.

## Related Docs

- [Module Contract and Dependency DAG Semantics](../module-contract-dag-semantics.md)
