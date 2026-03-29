# FrameKit Architecture

## Layers
1. Contracts layer (`include/framekit/*`)
2. Runtime layer (`runtime/multiprocess_runtime`)
3. Integration layer (examples + future adapters)

## Ownership
- FrameKit defines app and multiprocess contracts.
- NetKit provides optional transport implementations.
- MediaKit remains optional and domain-specific.

## Boundary Contracts
- [FrameKit v2 Charter](charter-v2.md)
- [External Integration Boundary Contract](integration-boundary.md)
- [FrameKit v2 Stable Contract Inventory](v2-stable-contracts.md)
- [Lifecycle and Shutdown Semantics](lifecycle-semantics.md)
- [Loop Stage Graph and Profile/Policy Specification](loop-stage-graph-profiles.md)
