# Loop Stage Graph and Profile/Policy Specification

Status: Draft
Last Reviewed: 2026-03-29

## Purpose

Define the baseline FrameKit v2 loop stage graph, required ordering constraints,
loop profiles, and execution policy contract dimensions.

## Baseline Stage Graph

Default stage order for the baseline host loop:

1. BeginFrame
2. ProcessPlatformEvents
3. NormalizeInput
4. DispatchImmediateEvents
5. PreUpdate
6. Update
7. PostUpdate
8. DispatchDeferredEvents
9. PreRender
10. Render
11. PostRender
12. EndFrame

### Ordering Constraints

- Stage order is strict unless a profile explicitly marks a stage optional.
- `ProcessPlatformEvents` must complete before `NormalizeInput`.
- `NormalizeInput` must complete before `Update`.
- `DispatchImmediateEvents` occurs before `Update`; deferred events are drained
  in `DispatchDeferredEvents`.
- `PreUpdate` and `PostUpdate` are guaranteed wrappers around `Update`.
- `PreRender` and `PostRender` are guaranteed wrappers around `Render` when
  rendering is enabled.
- `EndFrame` is the only legal point for frame-finalization observers.

## Profile Model

Profiles constrain stage usage and default policy behavior.

Canonical profile names are: GUI, Headless, Hybrid, and Deterministic Host.

### GUI (Interactive)

- Target: desktop UI and graphics-forward hosts.
- Required stages: all baseline stages.
- Defaults: variable timing, `slip` overrun mode, single-threaded or split
  update/render.

### Headless

- Target: service/automation hosts without window rendering.
- Required stages: BeginFrame, ProcessPlatformEvents, NormalizeInput,
  DispatchImmediateEvents, PreUpdate, Update, PostUpdate,
  DispatchDeferredEvents, EndFrame.
- Optional stages: PreRender, Render, PostRender.
- Defaults: externally-clocked or fixed timing, bounded catch-up overrun mode,
  single-threaded.

### Hybrid

- Target: tooling applications mixing interactive views and background work.
- Required stages: all baseline stages.
- Defaults: variable or fixed timing, bounded catch-up overrun mode,
  split update/render permitted.

### Deterministic Host

- Target: replay/test/simulation-like hosts requiring repeatability.
- Required stages: baseline order; rendering may be disabled by policy.
- Defaults: fixed timing, fail-fast or bounded catch-up overrun mode,
  single-threaded preferred.
- Rule: execution order and timing inputs must be deterministic for identical
  inputs.

## Policy Contract

Loop policy is expressed using three orthogonal dimensions.

### Timing Mode

- `variable_delta`: frame delta follows wall-clock pacing.
- `fixed_delta`: frame delta is constant, with accumulator-driven stepping.
- `externally_clocked`: host application supplies frame tick cadence.

### Overrun Mode

- `slip`: process one step and allow frame drift.
- `catch_up_bounded`: run bounded extra update steps to reduce drift.
- `drop_noncritical`: skip only stages that are explicitly marked optional or
  degradable by the active profile and policy.
- `fail_fast`: treat sustained overrun as a contract failure.

### Threading Mode

- `single_threaded`: all loop stages execute on one thread.
- `split_update_render`: update and render execute on coordinated threads.
- `host_managed`: host application owns threading and synchronization strategy.

## Policy Compatibility Rules

- `fixed_delta` + `catch_up_bounded` requires an explicit max catch-up step
  budget.
- `fail_fast` cannot silently degrade to `slip`.
- `drop_noncritical` must preserve non-optional ordering guarantees, including
  `PreUpdate`/`Update`/`PostUpdate` and, when rendering is enabled,
  `PreRender`/`Render`/`PostRender` wrapper integrity.
- `host_managed` must preserve baseline stage ordering guarantees.
- Any profile-specific optional stage disablement must be declared in policy.

## Acceptance Mapping

- Baseline stage graph documented: see Baseline Stage Graph.
- Profiles documented: GUI, Headless, Hybrid,
  Deterministic Host.
- Policy contract documented: Timing Mode, Overrun Mode, Threading Mode.
