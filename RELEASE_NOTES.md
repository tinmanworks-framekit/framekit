# Release Notes

## FrameKit v2.1.0

Status: Published
Last Updated: 2026-06-05

### Summary

FrameKit v2.1.0 promotes the post-v2.0.0 stabilization and additive contract
work now validated on `develop`.

This release keeps the v2 contracts source-compatible while adding optional
execution-service, scheduler-metrics, dynamic-module decision, integration, and
platform-validation surfaces.

### Highlights

- Added dynamic module loader contract surfaces for manifests, load/unload
  decision evaluation, and rollback planning without enabling default runtime
  dynamic loading.
- Added execution service registry contracts, task submission semantics,
  scheduler policy metrics, and a non-gating benchmark harness.
- Added composition guidance and NetKit-backed example validation hooks.
- Added CI-validated macOS Cocoa launch checks and documented Cocoa as supported
  opt-in backend support.
- Hardened service teardown, external-owned service registration behavior, and
  platform startup rollback cleanup.

### Compatibility

- FrameKit v2.1.0 is source-compatible with the v2.0.0 public baseline.
- New dynamic-loading and execution-service surfaces are additive and opt-in.
- Runtime dynamic module loading remains intentionally inactive unless future
  integration work wires it into a host.

### Release Evidence

- Release tracking issue: #153
- Promotion PR: #154
- Signed tag: v2.1.0

## FrameKit v2.0.0

Status: Published
Last Updated: 2026-03-29

### Summary

FrameKit v2.0.0 is the first public release of the rewritten FrameKit v2 line.
It replaces the earlier baseline with a contracts-first, incompatible release.

### Highlights

- Stable contract headers organized under `include/framekit/*`.
- Runtime implementations for lifecycle, loop, input, platform, fault policy, kernel, and multiprocess orchestration.
- Optional NetKit-linked frontend/backend examples and selective-linking guidance.
- Cross-platform CI coverage for core, NetKit-enabled, and sanitizer validation paths.
- Governance, ADR, and release-playbook documentation aligned to the v2 release flow.

### Compatibility

- This release is intentionally not backward compatible with the earlier FrameKit line.
- Historical `v0.1.0` artifacts remain archived and are not the basis for this release.

### Release Evidence

- CI expansion issue: #107
- Release cutover issue: #108
- Release promotion PR: #111
- Signed tag: v2.0.0
