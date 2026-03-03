# AGENTS.md

This repository follows Tinman Doctrine.

## Agent Operating Rules

1. Read repository guidance first:
   - `AI_CONTEXT.md`
   - `REPO_POLICY.md`
   - `docs/overview.md`
   - `docs/architecture.md`
   - `docs/doctrine/*.md` (if present)
2. Prefer doctrine defaults:
   - Clarity over cleverness
   - Explicit interfaces and low coupling
   - Stable and maintainable structure
3. Keep governance and docs in sync:
   - Update docs when behavior or architecture changes
   - Preserve repository visibility rules
   - Avoid secrets in commits and docs
4. Delivery quality bar:
   - Keep changes small and focused
   - Ensure lint/tests/build pass for touched areas
5. If local repo policy conflicts with doctrine snapshot, follow local repo files and call out the conflict explicitly.
