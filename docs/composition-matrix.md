# Composition Matrix

Status: Stable
Last Reviewed: 2026-03-30

## Purpose

Define the supported composition modes for integrating FrameKit with optional
runtime dependencies while preserving the external integration boundary.

## Composition Modes

| Mode | Build options | Example binaries | Primary use |
| --- | --- | --- | --- |
| Single-process host | `FRAMEKIT_ENABLE_NETKIT=OFF` | `framekit_single_process_example` | Local deterministic host lifecycle with no external transport dependency |
| Multiprocess host (local launcher) | `FRAMEKIT_ENABLE_NETKIT=OFF` | `framekit_multi_worker_example` | Parent + worker topology and liveness semantics without external IPC runtime |
| Multiprocess host + transport-backed integration | `FRAMEKIT_ENABLE_NETKIT=ON` and `FRAMEKIT_NETKIT_SOURCE_DIR=<path>` | `framekit_frontend_example`, `framekit_backend_example` | Composition boundary where FrameKit host contracts coordinate with NetKit transport adapters |

## Bridge Module Patterns

Bridge modules live in composition applications, not FrameKit core and not
external runtime repositories.

### 1) Lifecycle Bridge

- Convert app startup intent into `ApplicationSpec` and runtime configuration.
- Own startup/shutdown ordering for FrameKit host and external runtime startup.
- Keep fallback and retry behavior in composition code rather than FrameKit.

### 2) Service Bridge

- Register integration-facing services in scoped service context.
- Expose minimal host-facing interfaces rather than runtime concrete types.
- Teardown external resources through composition-owned shutdown hooks.

### 3) Event Bridge

- Translate runtime-native events into FrameKit event bus payloads.
- Keep domain payload schemas and event typing outside FrameKit contracts.
- Map subscription boundaries in composition modules only.

### 4) Transport Bridge

- Select transport kind through application configuration.
- Bind runtime transport factories and endpoint naming in composition wiring.
- Keep transport-specific diagnostics and adaptation logic outside FrameKit core.

## Validation Expectations

- Core composition paths validate with FrameKit CI build/test coverage.
- Cocoa-enabled macOS paths validate launch checks through CTest `example-launch`
  labels.
- NetKit-backed examples validate through CTest `integration-example` labels in
  NetKit-enabled CI jobs.

## Boundary Alignment

This matrix is constrained by the
[External Integration Boundary Contract](integration-boundary.md):

- FrameKit remains domain-agnostic.
- External runtime repositories remain FrameKit-independent.
- Composition behavior is implemented only in integration applications.