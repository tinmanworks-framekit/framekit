# API/Docs Contract Alignment Audit (2026-03-29)

## Audit Scope
This audit was performed as part of Phase 3 Stabilization prior to release generation.
It reviews alignment between the implemented contracts located across `include/framekit/*` and the public architecture/overview documentation.

## Confirmed Drift and Remediation

| Issue Area | Drift Symptom | Fixed In |
| --- | --- | --- |
| **Runtime Layer Mapping** | `docs/architecture.md` incorrectly documented the runtime layer as `runtime/multiprocess_runtime`, which was an outdated reference prior to the global header restructuring effort in Phase 1. | `docs/architecture.md` layers specification updated to reference the modern unified `src/*.cpp` components. |
| **Doc References in Central Headers** | Some outdated class/namespace references from older iterations existed in Doxygen/doc block hints, specifically in legacy include stubs. Review confirmed that the header reorganization pass unified the contracts under `include/framekit/*`. | Alignment validated; explicitly mapped `multiprocess_runtime.cpp`, `kernel_runtime.cpp`, etc., directly to the contracts layer definition. |

## Validation
- `include/framekit/*` directly implements boundaries cited in `charter-v2.md`.
- `include/framekit/loop/stage_graph.hpp` and `include/framekit/loop/policy.hpp` fully cover constraints defined in `loop-stage-graph-profiles.md`.
- `include/framekit/ipc/*` and `include/framekit/multiprocess/*` headers match transport selection behaviors dictated by `integration-boundary.md` and Phase 2 specifications.

All discrepancies resolved and verified against the FrameKit v0.1.0 boundary layout.