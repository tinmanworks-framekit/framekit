# Service Context Contract and Freeze Policy

Status: Draft
Last Reviewed: 2026-03-29

## Purpose

Define FrameKit v2 service context semantics for registration windows, freeze
point behavior, typed lookup, ownership/lifetime model, and thread-safety
expectations.

## Service Context Model

The service context is a scoped registry used by the host and modules for
contract-based service discovery.

Service context phases:

1. Open (registration allowed)
2. Frozen (registration prohibited)
3. Teardown (lookup and usage wind-down)

## Registration Window

Registration window rules:

- Service registration is allowed only while the context is Open.
- Registrations occur during bootstrap/initialization boundaries.
- Duplicate registration for the same key/type is invalid unless explicitly
  declared as replaceable by policy.
- Late registration attempts after freeze are contract violations.

## Freeze Point Semantics

Freeze point rules:

- Context transitions from Open to Frozen exactly once per host start cycle.
- Freeze occurs before entering Running state.
- After freeze, registry membership is immutable.
- Freeze completion is observable for diagnostics/testing.

## Typed Lookup Semantics

Typed lookup requirements:

- Services are resolved by explicit contract type/key.
- Successful lookup returns the registered contract-compatible instance.
- Missing required service lookups must fail-fast with diagnostic context.
- Optional lookup path must return explicit absence without side effects.
- Lookup behavior is deterministic for identical registry content.

## Ownership and Lifetime Model

Ownership model rules:

- Context owns service lifetime unless service is explicitly external-owned.
- Ownership mode must be declared at registration time.
- Context-owned services are torn down during shutdown in reverse registration
  order.
- External-owned services must not be destroyed by the context.

## Threading Assumptions and Safety

Thread-safety expectations:

- Registration and freeze transitions are single-writer operations.
- Read-only lookup after freeze is safe for concurrent access when host policy
  enables multithreaded execution.
- Any mutable service state remains the responsibility of service
  implementation; registry guarantees do not imply object-level thread safety.
- Host-managed threading mode must preserve freeze-before-concurrent-lookup
  ordering.

## Failure Semantics

- Registration after freeze is a contract error.
- Required service missing at startup transitions host to fault handling.
- Ownership/lifetime violations are diagnostics-level errors and may escalate
  to fail-fast by policy.

## Acceptance Mapping

- Registration/freeze semantics documented: Registration Window and Freeze Point
  Semantics sections.
- Typed lookup semantics documented: Typed Lookup Semantics section.
- Thread-safety expectations documented: Threading Assumptions and Safety
  section.
