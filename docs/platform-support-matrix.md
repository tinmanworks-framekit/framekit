# Platform Support Matrix

This matrix documents the currently supported host targets and the validation paths that gate changes.

## Runtime/Backend Matrix

| Host platform | Platform backend path | Status | Primary validation path |
| --- | --- | --- | --- |
| Linux | Stub/manual host injection (`platform_backend_factory_stub.cpp`) | Supported | `CI (C++ / CMake)` Linux jobs: configure, build, and contract tests |
| macOS | Cocoa backend (`cocoa_platform_backend.mm`) with `FRAMEKIT_ENABLE_COCOA=ON` | Supported | `CI (C++ / CMake)` macOS jobs: configure/build, contract tests, and `example-launch` CTest checks |

## Validation Notes

- Cocoa backend support is opt-in via `FRAMEKIT_ENABLE_COCOA=ON`.
- Example runtime launch checks are codified as CTest cases:
  - `framekit_example_launch_single_process`
  - `framekit_example_launch_multi_worker`
- macOS CI executes launch validation through CTest labels:
  - contract/runtime suite: `ctest --test-dir build --output-on-failure -LE example-launch`
  - launch checks: `ctest --test-dir build --output-on-failure -L example-launch`

## Scope Boundaries

- This matrix describes FrameKit host/runtime validation only.
- Optional NetKit/MediaKit integrations remain composition-level and are tracked separately from host backend support.
