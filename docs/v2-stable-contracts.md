# FrameKit v2 Stable Contract Inventory

Status: Draft
Last Reviewed: 2026-03-29

## Purpose

Define which FrameKit v2 contracts are frozen at v2.x and which areas remain
flexible for iterative implementation and adapter evolution.

## Stability Policy

1. Frozen contracts are semantically stable for the v2 major line.
2. Flexible areas can evolve in v2 minor releases if backward compatibility is
   preserved.
3. Breaking changes to frozen contracts require a new major version.

## Frozen Contracts (v2.x)

The following contracts are frozen for v2:

1. Lifecycle phase model and legal transition semantics.
2. Kernel startup and shutdown guarantees.
3. Run loop stage model and required ordering constraints.
4. Baseline loop profile semantics (GUI, Headless, Hybrid, Deterministic Host).
5. Execution policy contract shape (timing mode, overrun mode, threading mode).
6. Module contract (identity, dependency declaration, lifecycle entry points).
7. Service context contract (registration window, freeze point, typed lookup).
8. Event bus semantics (sync dispatch, deferred queue, consume/propagation).
9. Input normalization and window-aware routing semantics.
10. Platform/window minimal abstraction contract.
11. Fault classification and deterministic shutdown guarantees.

## Flexible Areas (v2 minors)

The following areas are flexible with compatibility constraints:

1. Backend adapter implementations and internal platform details.
2. Diagnostics sink implementations and output formatting.
3. Task service internals and scheduling strategy details.
4. Additional loop profiles beyond the baseline set.
5. Optional dynamic module loading model (deferred, opt-in).
6. Configuration source/provider adapters and tooling integrations.

## Rationale

- Frozen contracts protect application structure and long-term interoperability.
- Flexible areas protect delivery velocity and platform evolution.
- This split avoids over-freezing implementation details while preserving core
  architectural guarantees.

## Versioning Implications

### v2 Patch

- Bug fixes and internal implementation corrections only.
- No semantic contract changes.

### v2 Minor

- Backward-compatible additive capability in flexible areas.
- New optional profiles/adapters permitted if they do not alter frozen
  semantics.

### v3 Major

- Required for breaking changes to any frozen contract.
- Must include migration notes for affected consumers.

## Change Control

- Any proposed change to frozen contracts requires an ADR and explicit major
  version planning.
- Flexible-area evolution still requires issue tracking, acceptance criteria,
  and update notes when user-visible behavior changes.
