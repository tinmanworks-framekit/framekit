# FrameKit Overview

FrameKit is a modular framework for building C/C++ applications that may run as single-process tools or coordinated multiprocess systems.

Current baseline includes:
- process role and topology contracts
- transport-selection contracts
- multiprocess runtime skeleton
- lifecycle state machine and kernel runtime start/stop orchestration
- loop profile and execution-policy validation contracts
- loop stage graph runner with deterministic baseline-stage execution order
- service context freeze/lookup semantics and teardown ordering
- module dependency DAG validation with deterministic startup/shutdown ordering
- event bus immediate/deferred dispatch baseline with consume and priority semantics
- selective-linking guidance for optional module usage

For v2 scope and boundaries, see the [FrameKit v2 Charter](charter-v2.md).
