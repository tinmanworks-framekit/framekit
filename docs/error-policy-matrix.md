# Error Policy Matrix by Loop Profile

Status: Draft
Last Reviewed: 2026-03-29

## Purpose

Define fail-fast vs degrade behavior for FrameKit v2 fault classes across loop
profiles, including mandatory fail-fast invariants and bounded degrade limits.

## Profiles

- GUI Interactive
- Headless Service
- Hybrid Tool
- Deterministic Host

## Fault Classes

- Bootstrap
- Module
- Event
- Render
- Timing
- Shutdown

## Policy Outcome Legend

- Fail-fast: transition to Faulted and begin controlled shutdown.
- Degrade (bounded): continue with constrained behavior under explicit limits.
- Degrade unavailable: degrade path is disallowed for that profile/fault class.

## Policy Matrix

| Fault Class | GUI Interactive | Headless Service | Hybrid Tool | Deterministic Host |
|---|---|---|---|---|
| Bootstrap | Fail-fast | Fail-fast | Fail-fast | Fail-fast |
| Module | Fail-fast for required module; Degrade (bounded) for optional module | Fail-fast for required module; Degrade (bounded) for optional module | Fail-fast for required module; Degrade (bounded) for optional module | Fail-fast |
| Event | Degrade (bounded) for noncritical handler failures; Fail-fast for contract violations | Degrade (bounded) for noncritical handler failures; Fail-fast for contract violations | Degrade (bounded) for noncritical handler failures; Fail-fast for contract violations | Fail-fast |
| Render | Degrade (bounded) via surface/feature disable; Fail-fast for core host contract violations | Degrade unavailable when render disabled; otherwise same as GUI | Degrade (bounded) via view/surface isolation | Fail-fast |
| Timing | Degrade (bounded) via catch-up/slip policy | Degrade (bounded) via externally-clocked backpressure/catch-up | Degrade (bounded) via profile policy | Fail-fast |
| Shutdown | Degrade (bounded) for non-fatal teardown failures with diagnostics | Degrade (bounded) for non-fatal teardown failures with diagnostics | Degrade (bounded) for non-fatal teardown failures with diagnostics | Fail-fast |

## Always Fail-Fast Invariants

The following always require fail-fast, regardless of profile:

1. Invalid lifecycle transition.
2. Dependency cycle or unresolved required module dependency.
3. Registration after service-context freeze.
4. Corrupted event queue invariants (ordering/ownership corruption).
5. Determinism contract breach in Deterministic Host profile.
6. Any fault that compromises safe shutdown sequencing.

## Degrade Limits

Degrade paths are bounded and must escalate to fail-fast when limits are
exceeded.

- Module degrade: optional module disablement only; required module faults are
  never degradable.
- Event degrade: isolate failing handler instance up to configured threshold;
  repeated threshold breach escalates to fail-fast.
- Render degrade: disable affected surface/view path only; complete render-host
  loss escalates to fail-fast.
- Timing degrade: bounded catch-up step budget required; sustained overrun past
  budget escalates to fail-fast.
- Shutdown degrade: continue teardown best-effort only for non-fatal failures;
  fatal ordering violations escalate to fail-fast.

## Observability Requirements

- Every degrade decision emits diagnostics with fault class, profile, and limit
  counter context.
- Escalation from degrade to fail-fast emits explicit transition reason.
- Policy decision points are testable via deterministic hooks/log events.

## Acceptance Mapping

- Matrix covers bootstrap/module/event/render/timing/shutdown: Policy Matrix.
- Always-fail-fast invariants documented: Always Fail-Fast Invariants section.
- Degrade limits documented: Degrade Limits section.
