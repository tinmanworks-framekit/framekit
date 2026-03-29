# Release Notes

## FrameKit v2.0.0

Status: Draft
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
- Release promotion PR: pending
- Signed tag: pending
