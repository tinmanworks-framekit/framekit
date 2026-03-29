# FrameKit v2 Post-Release Roadmap

Status: Stable
Last Reviewed: 2026-03-29

## Purpose

Record the post-release follow-up map for the FrameKit v2 line after the
published `v2.0.0` release. This roadmap closes planning issues #113, #114,
#115, and #116 by defining the concrete work sequence, dependencies, and
validation expectations for each track without starting implementation.

## Track Summary

| Issue | Track | Outcome |
| --- | --- | --- |
| #113 | Optional dynamic module loading | Define the opt-in loader contract, runtime hooks, and safety rules before any implementation work starts. |
| #114 | Scheduler and execution services | Additive execution-service and scheduling work scoped to flexible v2 areas only. |
| #115 | Platform expansion and Cocoa validation | Fold existing deferred Cocoa work into one sequenced backend and validation plan. |
| #116 | Ecosystem integration and composition | Expand composition examples and guidance without coupling FrameKit to external runtimes. |

## Recommended Execution Order

1. Platform expansion and Cocoa validation (`#115`)
2. Ecosystem integration and composition (`#116`)
3. Scheduler and execution services (`#114`)
4. Optional dynamic module loading (`#113`)

Rationale:

- Platform and integration work provide real validation pressure on the v2
  contracts before more invasive extensibility work begins.
- Scheduler work should be informed by actual integration and runtime needs.
- Dynamic loading carries the highest compatibility and lifecycle risk and
  should remain last until the static baseline has more downstream proof.

## Track #113: Optional Dynamic Module Loading

### Constraints

- Follow the deferred future-work path in [ADR-0002](../adr/ADR-0002-module-loading-model.md).
- Remain opt-in and additive within the flexible-area rules in
  [FrameKit v2 Stable Contract Inventory](../v2-stable-contracts.md).
- Preserve deterministic startup and shutdown guarantees from the static
  baseline.

### Follow-Up Sequence

1. Define the loader-facing contract surface:
   module identity, manifest shape, discovery model, and explicit opt-in
   registration hooks.
2. Specify runtime integration points:
   load timing, service registration window, module graph validation, and
   failure/rollback behavior.
3. Define lifecycle safety rules:
   legal load states, unload restrictions, dependency refusal semantics, and
   fault escalation on invalid operations.
4. Add contract tests and a minimal example host that exercises load, refusal,
   and unload edge cases.
5. Update ADRs and user-facing documentation only after the contract shape is
   proven by tests.

### Validation Expectations

- Deterministic startup/shutdown behavior remains unchanged when dynamic
  loading is disabled.
- Runtime load failures do not leave the module graph or service context in a
  partially committed state.
- Unload rules are explicit and test-covered for dependency and lifecycle
  violations.

## Track #114: Scheduler and Execution Services

### Constraints

- Keep the work additive and backward-compatible within the flexible-area
  rules in [FrameKit v2 Stable Contract Inventory](../v2-stable-contracts.md).
- Do not alter frozen loop-stage ordering or baseline lifecycle semantics.

### Follow-Up Sequence

1. Define the execution-service abstraction exposed to applications and modules,
   including registration, ownership, and shutdown guarantees.
2. Specify task submission semantics:
   queue ownership, cancellation boundaries, result delivery, and fault
   propagation.
3. Introduce scheduler policy variants as optional adapters:
   single-threaded baseline, fixed pool, and stage-affine execution.
4. Add observability hooks for queue depth, latency, starvation, and budget
   tracking.
5. Add validation coverage:
   unit tests for semantics, integration tests for shutdown/fault cases, and
   benchmark baselines for representative workloads.

### Validation Expectations

- Applications that do not opt into execution services preserve current
  deterministic behavior.
- Scheduler-enabled modes have explicit fault and shutdown guarantees.
- Benchmarks and tests demonstrate no regression in baseline single-threaded
  operation.

## Track #115: Platform Expansion and Cocoa Validation

### Constraints

- Fold existing deferred Cocoa issues #33 and #34 into one sequenced platform
  track instead of creating duplicate planning work.
- Keep backend implementation details behind the existing minimal platform
  contract.

### Follow-Up Sequence

1. Audit issue #33 and issue #34 against the current v2 platform/window
   contract and confirm which gaps remain relevant after `v2.0.0`.
2. Define the backend delivery slices:
   adapter implementation, event pump integration, and render hook behavior.
3. Define CI/runtime validation requirements for macOS:
   compile coverage, sample launch coverage, and behavior checks that prove the
   backend matches current host semantics.
4. Update the support matrix and platform docs once the backend and validation
   path are real.

### Validation Expectations

- macOS backend behavior is validated against the same lifecycle, input, and
  event-routing guarantees as other supported hosts.
- CI coverage proves the backend still builds and examples still launch on
  macOS after future changes.

## Track #116: Ecosystem Integration and Composition

### Constraints

- Preserve the separation rules in [External Integration Boundary Contract](../integration-boundary.md).
- Keep FrameKit domain-agnostic and avoid compile-time dependency on external
  runtimes such as NodeX or MediaKit.

### Follow-Up Sequence

1. Define the composition-example matrix:
   single-process host, multiprocess host, and optional transport-backed
   integration example expectations.
2. Document bridge-module patterns for lifecycle, services, events, and
   transport handoff without embedding domain semantics into FrameKit.
3. Expand example and guide coverage for selective linking, NetKit-enabled
   paths, and service-boundary composition.
4. Add validation expectations for examples:
   build coverage, launch coverage, and documentation consistency checks.

### Validation Expectations

- Example applications remain composition-only and do not introduce circular
  dependencies with external repos.
- Integration guidance stays aligned with the current boundary contract and
  selective-linking rules.

## Completion Criteria

This roadmap is complete when:

- Each track has a sequenced implementation map.
- Dependencies and validation expectations are explicit.
- No post-release track is started until it is intentionally moved out of
  backlog and into active issue execution.
