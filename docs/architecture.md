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
