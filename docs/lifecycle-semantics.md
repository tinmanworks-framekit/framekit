# Lifecycle and Shutdown Semantics

Status: Draft
Last Reviewed: 2026-03-29

## Purpose

Define the legal lifecycle states, transition rules, and shutdown guarantees for
FrameKit v2 host runtime behavior.

## Lifecycle States

1. Bootstrapping
2. Initializing
3. Running
4. Stopping
5. Stopped
6. Faulted

## Legal Transition Table

| Current State | Allowed Next State(s) | Notes |
|---|---|---|
| Bootstrapping | Initializing, Faulted | Bootstrap failure transitions directly to Faulted |
| Initializing | Running, Stopping, Faulted | Stop request during init transitions to Stopping |
| Running | Stopping, Faulted | Normal operation state |
| Stopping | Stopped, Faulted | Fault during stop transitions to Faulted |
| Stopped | Bootstrapping | Restart requires full bootstrap |
| Faulted | Stopping, Stopped, Bootstrapping | Policy-dependent recovery path |

Transitions not listed above are invalid and must be rejected.

## Startup Semantics

1. Kernel enters Bootstrapping exactly once per start request.
2. Kernel enters Initializing after bootstrap prerequisites complete.
3. Kernel enters Running only after required services/modules are ready.
4. Any required contract failure before Running transitions to Faulted.

## Shutdown Semantics

### Ordered Shutdown Sequence

1. Enter Stopping and stop accepting new registrations.
2. Stop modules in reverse dependency order.
3. Drain deferred events according to shutdown policy.
4. Teardown platform/window host resources.
5. Teardown services in reverse registration order.
6. Flush diagnostics and transition to Stopped.

### Shutdown Guarantees

- Shutdown is deterministic and ordered.
- Reverse-order teardown is enforced for dependency safety.
- Completion is observable via Stopped state transition.
- Failure in shutdown path transitions to Faulted and reports diagnostics.

## Faulted-State Semantics

Faulted indicates a contract-level failure that prevents normal operation.

Faulted requirements:

1. Must capture fault diagnostics context.
2. Must prohibit transition to Running without new bootstrap.
3. May allow controlled transition to Stopping for cleanup.
4. Must expose explicit recovery path policy (restart or hard stop).

## Policy Notes

- Fail-fast is required for invalid transitions and required contract failures.
- Degrade behavior is profile-policy controlled and documented separately in the
  error policy matrix.
