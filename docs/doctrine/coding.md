# Coding Principles

Status: Stable  
Last Reviewed: 2026-02-28

## Scope

Defines engineering and coding preferences.

Repository structure and documentation rules are defined in `project-standards.md`.

---

## Architecture Preference

Prefer:

- Modular architecture
- Clear separation of concerns
- Layered systems
- Minimal coupling
- Explicit interfaces

Avoid:

- Monolithic logic
- Hidden dependencies
- Overengineering abstractions

---

## Language Usage

### C/C++

Use for:

- Systems programming
- Performance-critical code
- Engines
- Embedded work

Prefer:

- RAII
- Modern C++ features where beneficial
- Clear ownership semantics

---

### Python

Use for:

- Tooling
- Backend services
- Data processing
- Experimentation

Prefer:

- Clear structure
- Minimal global state
- Explicit dependencies

---

## Build Systems

Prefer:

- CMake for C/C++
- Native ecosystem tools otherwise

Builds must be reproducible.

---

## General Principles

- Code clarity over cleverness
- Explicit over implicit
- Stability over novelty
- Maintainability over shortcuts

## Developer Experience

- Minimize setup burden for contributors.
- Prefer automation for repetitive environment and bootstrap steps.
- Keep dependency checks explicit and actionable.
- Provide graceful fallback paths when optional tools are missing.

---

## Comments

Explain:

- Why something exists
- Non-obvious decisions

Avoid obvious comments.
