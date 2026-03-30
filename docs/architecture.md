# FrameKit Architecture

## Layers
1. Contracts layer (`include/framekit/*`)
2. Runtime layer (`src/*.cpp` runtime components such as `multiprocess_runtime.cpp`, `loop_stage_graph.cpp`, and `kernel_runtime.cpp`)
3. Integration layer (examples + future adapters)

## Ownership
- FrameKit defines app and multiprocess contracts.
- NetKit provides optional transport implementations.
- MediaKit remains optional and domain-specific.

## Boundary Contracts
- [FrameKit v2 Charter](charter-v2.md)
- [External Integration Boundary Contract](integration-boundary.md)
- [FrameKit v2 Stable Contract Inventory](v2-stable-contracts.md)
- [Platform Support Matrix](platform-support-matrix.md)
- [Lifecycle and Shutdown Semantics](lifecycle-semantics.md)
- [Loop Stage Graph and Profile/Policy Specification](loop-stage-graph-profiles.md)
- [Module Contract and Dependency DAG Semantics](module-contract-dag-semantics.md)
- [Service Context Contract and Freeze Policy](service-context-contract-freeze.md)
- [Event and Input Semantics Contract](event-input-semantics.md)
- [Error Policy Matrix by Loop Profile](error-policy-matrix.md)
- [API/Docs Contract Alignment Audit (2026-03-29)](doctrine/api-doc-contract-alignment-2026-03-29.md)

## Architecture Decisions
- [ADR Index](adr/README.md)
- [ADR-0001: Service Context Model and Freeze Policy](adr/ADR-0001-service-context-model.md)
- [ADR-0002: Module Loading Model (Static Baseline)](adr/ADR-0002-module-loading-model.md)
- [ADR-0003: Loop Profiles and Policy Model](adr/ADR-0003-loop-profile-policy-model.md)
- [ADR-0004: Error Policy Matrix and Escalation Rules](adr/ADR-0004-error-policy-model.md)
