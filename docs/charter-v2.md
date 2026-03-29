# FrameKit v2 Charter

Status: Draft
Last Reviewed: 2026-03-29

## Purpose

FrameKit v2 provides a minimal, reusable C++ application host framework.
It standardizes application bootstrap, lifecycle, loop orchestration, windows,
input/events, module composition, service access, diagnostics, configuration,
and timing utilities.

## Target Use Cases

- Native desktop or tooling applications that need clean lifecycle structure.
- Single-process or coordinated multiprocess applications.
- Host applications that compose external systems without inheriting domain
  semantics into FrameKit core.

## Non-goals

FrameKit v2 is not:

- A node/graph system.
- An execution engine or scheduler platform.
- A simulation or dataflow framework.
- A domain plugin ecosystem.
- A rendering engine ownership layer.

## Design Philosophy

- Explicit over implicit.
- Minimal default behavior, composable extensions.
- Stable contracts and low coupling.
- Deterministic lifecycle and shutdown behavior.
- Cross-platform readiness without backend lock-in.

## Scope Boundaries

In scope:

- Application kernel and lifecycle phases.
- Run loop orchestration and profile/policy model.
- Platform and window host abstraction.
- Input normalization and typed event dispatch.
- Module lifecycle and dependency ordering.
- Scoped service registry and access.
- Logging/diagnostics, config, and timing primitives.

Out of scope:

- Node graph logic and execution semantics.
- Domain runtime logic (including NodeX internals).
- Domain-specific simulation or workflow behavior.

## Integration Boundary

FrameKit remains domain-agnostic.

- FrameKit must not depend on NodeX.
- NodeX must not depend on FrameKit.
- Composition applications may integrate both through bridge modules/services.

## Governance Notes

- Implementation changes must be issue-driven and board-tracked.
- Changes should be committed in small issue-scoped steps.
- Contract and behavior changes require documentation updates in the same flow.
