# Changelog

## Unreleased

## v2.1.0 - 2026-06-05
- Added opt-in dynamic module loader contract surfaces, load/unload decision helpers, and rollback planning helpers.
- Added execution service registry contracts, task submission semantics, scheduler policy metrics, and a non-gating execution benchmark harness.
- Added composition guidance and NetKit-backed example validation hooks.
- Added CI-validated macOS Cocoa launch checks and promoted Cocoa backend support documentation to supported opt-in status.
- Hardened service teardown, external-owned service registration behavior, and platform startup rollback cleanup.
- Aligned release evidence, Cocoa support wording, and core v2 contract documentation status with the validated release baseline.

## v2.0.0
- First public release of the rewritten, incompatible FrameKit v2 line.
- Added a contracts-first public API under `include/framekit/*`.
- Added runtime implementations for lifecycle, loop, input, platform, fault policy, kernel, and multiprocess orchestration.
- Added optional NetKit-linked examples and selective-linking guidance.
- Added governance, ADR, and release workflow documentation for the v2 line.
