# Naming Conventions

Status: Draft  
Last Reviewed: 2026-06-04

## Scope

Applies to repositories, code, modules, and project naming.

## General Principles

- Names must be intentional.
- Avoid randomness.
- Avoid numbers unless technically required.
- Prefer clarity over cleverness.
- Avoid mixing unrelated brand, product, account, and environment naming scopes.

---

## Repository Naming

- Use lowercase.
- Use kebab-case.
- Keep names short and meaningful.
- Avoid vague names.

Examples:
- example-cli
- render-core
- workflow-service

---

## Class Naming (C++)

- Use PascalCase.
- Clear semantic meaning.
- Avoid unnecessary abbreviations.

Examples:
SessionManager
NodeGraph
SharedMemoryPort

---

## Variables

- Use camelCase.
- Descriptive but concise.
- Avoid single-letter variables except loops.

---

## Product / Space Naming

- Use environment or workspace names only for environment/workspace repositories.
- Do not combine product, organization, maintainer, and workspace names in the same naming scope unless the repository explicitly documents that convention.
