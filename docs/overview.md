# FrameKit Overview

FrameKit is a modular framework for building C/C++ applications that may run as single-process tools or coordinated multiprocess systems.

Current baseline includes:
- process role and topology contracts
- transport-selection contracts
- multiprocess runtime skeleton
- lifecycle state machine and kernel runtime start/stop orchestration
- loop profile and execution-policy validation contracts
- profile-aware fault-policy runtime with bounded degrade budgets and fail-fast escalation integrated into platform stage-failure handling
- loop stage graph runner with deterministic baseline-stage execution order
- platform/window host baseline runtime integrated with stage-driven event pump and render hooks
- input normalization and window-aware immediate routing runtime integrated with loop stages
- service context freeze/lookup semantics and teardown ordering
- module dependency DAG validation with deterministic startup/shutdown ordering
- event bus immediate/deferred dispatch baseline with consume and priority semantics
- selective-linking guidance for optional module usage

Public include layout is organized by concern under `include/framekit/`:
- `spec/`, `lifecycle/`, `loop/`, `platform/`, `input/`, `event/`, `module/`, `service/`, `fault/`, `kernel/`, `multiprocess/`, and `ipc/`

For v2 scope and boundaries, see the [FrameKit v2 Charter](charter-v2.md).
