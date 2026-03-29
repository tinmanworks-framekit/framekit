# Event and Input Semantics Contract

Status: Draft
Last Reviewed: 2026-03-29

## Purpose

Define FrameKit v2 event dispatch semantics, deferred event queue behavior,
propagation/consume rules, and normalized input routing across one or more
windows.

## Event Dispatch Model

FrameKit event dispatch has two paths:

1. Immediate synchronous dispatch.
2. Deferred queued dispatch.

### Immediate Dispatch

- Event is delivered in the caller's execution flow.
- Handlers execute in deterministic registration order.
- Immediate handler failures follow configured error policy.

### Deferred Dispatch

- Event is enqueued for later delivery.
- Queue drain occurs at the loop's `DispatchDeferredEvents` stage.
- Deferred events preserve FIFO ordering within the same priority class.
- Deferred handlers observe world state after update-stage completion for the
  current frame.

## Propagation and Consume Rules

Event propagation contract:

- Handlers are invoked in deterministic sequence.
- Handler can mark event as consumed.
- Consumed events stop propagation unless policy allows monitor-only handlers.
- Monitor handlers, when enabled, observe consumed events without mutating
  consume state.
- Re-entrant dispatch must preserve ordering guarantees and must not corrupt
  queue state.

## Deferred Queue Contract

Queue behavior requirements:

- Queue supports bounded capacity policy per host profile.
- Capacity overflow handling is policy-driven (drop_noncritical, fail_fast,
  or backpressure signaling).
- Queue drain is repeatable and observable for diagnostics.
- Shutdown drain policy is explicit (drain all, drain bounded, or discard
  with diagnostics).

## Input Normalization

Input normalization converts backend-specific events into canonical host input
records.

Normalization requirements:

- Device-specific payloads map to stable input event types.
- Pointer, keyboard, and text input share normalized timestamp/source metadata.
- Modifier state is normalized before dispatch.
- Backend-specific extension fields are optional and non-breaking.

## Multi-Window Input Routing

Routing model:

- Every input event carries a window target identifier.
- Focused-window routing is default for keyboard/text input.
- Pointer routing uses hit-test or capture semantics as policy defines.
- Window-targeted handlers run before global handlers.
- Missing/invalid target window follows policy (drop with diagnostics or route
  to fallback global handler).

## Threading and Ordering Assumptions

- Event queue mutation is synchronized according to threading mode.
- Immediate dispatch ordering is preserved within a thread context.
- Cross-thread publishers must hand off through synchronized queue boundaries.

## Acceptance Mapping

- Sync + deferred semantics documented: Event Dispatch Model and Deferred Queue
  Contract sections.
- Propagation/consume rules documented: Propagation and Consume Rules section.
- Multi-window input routing documented: Multi-Window Input Routing section.
