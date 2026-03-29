# FrameKit Overview

FrameKit is a modular framework for building C/C++ applications that may run as single-process tools or coordinated multiprocess systems.

Current baseline includes:
- process role and topology contracts
- transport-selection contracts with multiprocess runtime transport-factory integration baseline
- multiprocess runtime baseline with deterministic default liveness/supervisor behavior, ProcessSpec restart-budget enforcement, partial-launch rollback guarantees, and rollback-failure cleanup state retention
- lifecycle state machine and kernel runtime start/stop orchestration
- loop profile and execution-policy validation contracts
- profile-aware fault-policy runtime with bounded degrade budgets and fail-fast escalation integrated into platform stage-failure handling
- loop stage graph runner with deterministic baseline-stage execution order and fixed-delta overrun semantics (slip, bounded catch-up, drop-noncritical, fail-fast)
- platform/window host baseline runtime integrated with stage-driven event pump and render hooks
- input normalization and window-aware immediate routing runtime integrated with loop stages
- service context freeze/lookup semantics and teardown ordering
- module dependency DAG validation with deterministic initialize/start orchestration and reverse-order stop/teardown
- event bus immediate/deferred dispatch baseline with consume and priority semantics
- selective-linking guidance for optional module usage

Public include layout is organized by concern under `include/framekit/`:
- `spec/`, `lifecycle/`, `loop/`, `platform/`, `input/`, `event/`, `module/`, `service/`, `fault/`, `kernel/`, `multiprocess/`, and `ipc/`

For v2 scope and boundaries, see the [FrameKit v2 Charter](charter-v2.md).
For latest contract-drift evidence and remediation traceability, see the [API/Docs Contract Alignment Audit (2026-03-29)](doctrine/api-doc-contract-alignment-2026-03-29.md).
