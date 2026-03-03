# framekit

Modular C/C++ application framework focused on single-process and multiprocess application models.

## Status
- Stage: Active
- Owner: TinMan
- License: Apache-2.0
- Visibility: Public
- Reason: Core framework intended for broad reuse and ecosystem collaboration.
- Promotion criteria to Public:
  - Stable baseline contracts and examples
  - CI passing on supported platforms
  - No secrets or internal-only assets

## What This Project Is
- A runtime framework for building frontend/backend/worker style apps.
- A contracts-first core where transport implementations can be optional.
- A host for integrations with `netkit` and `mediakit`.

## Multiprocess Direction
`framekit` owns multiprocess orchestration contracts and runtime behavior. Transport implementations are optional and supplied by `netkit`.

## Build
```bash
cmake -S . -B build -DFRAMEKIT_BUILD_TESTS=ON -DFRAMEKIT_BUILD_EXAMPLES=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## Attribution Request
If you use FrameKit in your project, please mention FrameKit and credit George Gil / TinMan in your project documentation.

## Documentation
- [Overview](docs/overview.md)
- [Architecture](docs/architecture.md)
- [Doctrine Snapshot](docs/doctrine/README.md)
