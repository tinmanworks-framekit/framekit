# Dynamic Module Loader Contract (2026-03-30)

Status: Draft
Last Reviewed: 2026-03-30

## Purpose

Define the first opt-in contract surface for post-release dynamic module
loading work in track #113 without enabling runtime dynamic loading by default.

## Contract Surface

Header:

- `include/framekit/module/dynamic_loader.hpp`

Primary contract types:

- `DynamicModuleManifest`
- `DynamicModuleDecision`
- `DynamicModuleRefusalReason`
- `DynamicModuleLoadState`
- `DynamicLoaderHostPhase`
- `IDynamicModuleLoader`
- `ValidateDynamicModuleManifest(...)`
- `EvaluateDynamicLoadRequest(...)`
- `EvaluateDynamicUnloadRequest(...)`
- `BuildDynamicRollbackPlan(...)`
- `AssessDynamicRollbackResult(...)`

## Baseline Rules

- Manifest `module_id` and `module_version` must be non-empty.
- Required and optional dependency entries must be non-empty.
- A module cannot depend on itself.
- Dependency entries must be unique within each list.
- A dependency cannot appear in both required and optional lists.

## Safety Position

- This slice introduces contract shapes and deterministic validation only.
- No runtime behavior change is introduced for static baseline startup/shutdown.
- Dynamic loading remains opt-in and inactive unless a future slice wires
  runtime integration hooks.
- Load/unload decision helpers provide deterministic refusal reasons without
  mutating baseline startup/shutdown behavior.

## Follow-up Slices

- Runtime load/unload state integration and refusal semantics.
- Rollback/fault handling for partial dynamic load failures.
- Explicit unload restrictions and dependency-safe refusal paths.

## Rollback Planning Notes

- `BuildDynamicRollbackPlan(...)` derives deterministic reverse-order unload
  steps for partially loaded dynamic modules.
- `AssessDynamicRollbackResult(...)` classifies rollback success/failure to
  support future fault escalation logic.
