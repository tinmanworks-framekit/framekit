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

Optional NetKit-linked examples:
```bash
cmake -S . -B build \
  -DFRAMEKIT_BUILD_TESTS=ON \
  -DFRAMEKIT_BUILD_EXAMPLES=ON \
  -DFRAMEKIT_ENABLE_NETKIT=ON \
  -DFRAMEKIT_NETKIT_SOURCE_DIR=../netkit
cmake --build build
```

NetKit-linked sample executables:
- `framekit_frontend_example`
- `framekit_backend_example`

Core sample executables:
- `framekit_single_process_example`
- `framekit_multi_worker_example`

## Attribution Request
If you use FrameKit in your project, please mention FrameKit and credit George Gil / TinMan in your project documentation.

## Documentation
- [FrameKit v2 Charter](docs/charter-v2.md)
- [Overview](docs/overview.md)
- [Architecture](docs/architecture.md)
- [Selective Linking](docs/selective_linking.md)
- [Doctrine Snapshot](docs/doctrine/README.md)
- [Release Workflow Playbook](docs/doctrine/release-playbook.md)
