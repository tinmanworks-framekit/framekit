# Repo Visibility Note Template

Status: Stable  
Last Reviewed: 2026-02-28

Copy into each repository `README.md` or `POLICY.md`.

```md
## Visibility

Visibility: Public | Private | Internal
Reason: <why this visibility is correct right now>
Promotion criteria to Public:
- <criterion 1>
- <criterion 2>
- <criterion 3>
```

## Public Promotion Gate

Confirm before flipping to public:

- [ ] No secrets (`.env`, keys, tokens, passwords, private endpoints)
- [ ] `.gitignore` is correct
- [ ] License is present
- [ ] README has purpose and build/run instructions
- [ ] Private assets/binaries removed
- [ ] Personal/internal notes moved out
