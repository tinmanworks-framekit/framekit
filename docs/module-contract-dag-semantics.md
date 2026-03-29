# Module Contract and Dependency DAG Semantics

Status: Draft
Last Reviewed: 2026-03-29

## Purpose

Define FrameKit v2 module lifecycle semantics, dependency declaration rules,
startup ordering, DAG validation behavior, and reverse-order shutdown
requirements.

## Module Contract

Each module exposes the following contract metadata:

- Stable module identifier.
- Declared dependency list (required and optional).
- Lifecycle entry points for initialize, start, stop, and teardown.
- Capability metadata for diagnostics and startup reporting.

Dynamic module loading/discovery is out of scope for this contract.

## Module Lifecycle

Module lifecycle states:

1. Declared
2. Resolved
3. Initializing
4. Active
5. Stopping
6. Stopped
7. Faulted

### Lifecycle Rules

- Declared modules transition to Resolved only after dependency resolution.
- Resolved modules transition to Initializing in deterministic startup order.
- Initializing transitions to Active only after successful startup callback.
- Startup failure transitions module to Faulted and aborts further startup.
- Active modules transition to Stopping when host shutdown begins.
- Stopping modules transition to Stopped after teardown callback completion.
- Faulted modules must not transition to Active without full host restart.

## Dependency DAG Semantics

Dependencies define a directed acyclic graph (DAG):

- Edge direction: consumer module -> dependency module.
- Required dependency edges must resolve at bootstrap time.
- Optional dependency edges may remain unresolved without invalidating startup.

### DAG Validation Rules

Validation is fail-fast and deterministic.

1. Module identifiers must be unique.
2. Every required dependency must resolve to a declared module.
3. Dependency cycles are invalid and block startup.
4. Self-dependency is invalid.
5. Duplicate dependency entries are invalid.
6. Validation result must include actionable diagnostics for invalid graphs.

### Deterministic Topological Order

- Startup order is derived from topological sort of the validated DAG.
- If multiple valid orders exist, tie-break by stable module identifier.
- Order must be reproducible for identical module sets and declarations.

## Startup Ordering Guarantees

- A module starts only after all required dependencies are Active.
- Optional dependency availability is observable but not required to start.
- Startup proceeds in deterministic topological order.
- On startup fault, host transitions to fault handling semantics.

## Shutdown Semantics

### Reverse-Order Shutdown Rules

- Shutdown order is reverse topological order of started modules.
- Dependents are stopped before their dependencies.
- Each module receives stop/teardown callbacks at most once.
- Shutdown continues best-effort after non-fatal module stop failures, while
  recording diagnostics.
- Fatal shutdown contract violations transition host to Faulted.

### Shutdown Guarantees

- Dependency safety is preserved through reverse-order teardown.
- Shutdown sequence is deterministic for identical startup state.
- Module state transitions are observable for diagnostics and testing.

## Acceptance Mapping

- Module lifecycle documented: Module Lifecycle section.
- DAG validation rules documented: Dependency DAG Semantics section.
- Reverse-order shutdown rules documented: Shutdown Semantics section.
