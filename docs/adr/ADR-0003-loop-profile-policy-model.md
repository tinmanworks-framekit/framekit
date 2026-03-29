# ADR-0003: Loop Profiles and Policy Model

Status: Accepted
Date: 2026-03-29
Last Reviewed: 2026-03-29

## Context

FrameKit must support multiple host styles (interactive, headless, hybrid,
deterministic) without fragmenting core loop contracts.

## Decision

Define a common baseline stage graph and a profile/policy model:

- Baseline stages provide stable ordering guarantees.
- Profiles specify required/optional stage usage.
- Policy dimensions are orthogonal: timing, overrun, threading.

## Consequences

- Shared core behavior across host styles.
- Controlled flexibility via explicit policy contracts.
- Deterministic Host profile remains strict by design.

## Related Docs

- [Loop Stage Graph and Profile/Policy Specification](../loop-stage-graph-profiles.md)
