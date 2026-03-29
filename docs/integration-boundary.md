# External Integration Boundary Contract

Status: Draft
Last Reviewed: 2026-03-29

## Purpose

Define strict integration boundaries so FrameKit remains a generic application
host framework and does not absorb domain-runtime semantics.

## Core Boundary Rules

1. FrameKit core must not depend on any domain runtime implementation.
2. Domain runtimes must not depend on FrameKit internals.
3. Integration occurs only in composition applications through explicit bridge
   modules and services.

## Dependency Separation Requirements

- FrameKit contracts and runtime targets remain domain-agnostic.
- Domain runtime repositories must not import FrameKit-only internal symbols.
- Shared behavior is expressed through generic host contracts, not domain terms.

## Composition-Only Integration Model

Integration pattern:

1. Application links FrameKit and any external runtime dependencies.
2. Application defines bridge module(s) that map host lifecycle hooks to
   runtime lifecycle hooks.
3. Application registers runtime-facing services in the scoped service context.
4. Runtime-specific behavior remains outside FrameKit core and contracts.

## Allowed Integration Hooks

- Lifecycle hooks
- Update and render stage hooks
- Typed event publication/subscription
- Scoped service registration and lookup

## Prohibited Patterns

- Embedding domain-specific execution semantics in FrameKit contracts.
- Domain-specific plugin/add-on systems implemented inside FrameKit core.
- Cross-repo circular dependencies between host and domain runtime repositories.

## Verification Checklist

- [ ] FrameKit has no compile-time dependency on domain runtime repos.
- [ ] Domain runtime repos have no compile-time dependency on FrameKit internals.
- [ ] Integration behavior is implemented in composition applications only.
- [ ] Architecture docs link this boundary contract.

## NodeX Context (Informative)

NodeX is an example domain runtime system used to validate this boundary.
The same separation rules apply to any external runtime.
