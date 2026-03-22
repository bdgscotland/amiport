# ADR-020: Git Hooks for Documentation and Hygiene Validation

## Status

Accepted

## Date

2026-03-21

## Context

The project has extensive documentation that must stay synchronized: CLAUDE.md, README.md, docs/architecture.md, docs/porting-guide.md, and the port-project skill all reference agents, skills, and pipeline stages. When a new agent is added or a skill is renamed, every reference must be updated. With multiple agents creating ports concurrently, stray files also accumulate — test artifacts in the project root, binaries in the wrong directory, versioned readme files instead of the canonical `<name>.readme`.

Two enforcement points exist:

1. **Runtime hooks** (ADR-017) — block violations during a Claude session via PreToolUse hooks. These prevent agents from creating bad state but don't catch human commits or drift that accumulates across sessions.
2. **Commit-time hooks** — run validation before `git commit` succeeds. These catch everything, regardless of who or what made the change.

## Decision

Use `.githooks/pre-commit` (configured via `git config core.hooksPath .githooks`, run by `make setup`) to validate documentation consistency and port hygiene at commit time.

The pre-commit hook runs three checks:

1. **`make check-docs`** — Validates that all agent names referenced in CLAUDE.md, README.md, architecture.md, and the port-project skill match actual agent definitions in `.claude/agents/`. Catches renamed or deleted agents that leave dangling references.

2. **`make check-agents`** — Validates agent and skill frontmatter fields (required fields present, model names valid, no orphaned references).

3. **Root file check** — Scans for files in the project root that don't belong (test binaries, `.o` files, stray source files). Whitelists known root files (Makefile, README.md, CLAUDE.md, etc.).

4. **Port hygiene check** — For any modified port directory, verifies the binary exists at `ports/<name>/<name>`, the readme exists at `ports/<name>/<name>.readme`, and no artifacts are in `ported/` besides `.c` and `.h` files.

`make setup` is mandatory after cloning — without it, `core.hooksPath` isn't set and pre-commit validation is skipped. This is documented prominently in CLAUDE.md and checked by the SessionStart hook (`check-toolchain.sh`).

## Consequences

### Positive

- Documentation drift is caught before it enters the commit history, not discovered later
- Stray files are blocked at commit time — the project root stays clean
- Port directory hygiene is validated automatically, not reliant on agent discipline
- Works for both human commits and agent-initiated commits

### Negative

- `make setup` is a manual step that can be forgotten — new contributors may not run it
- Pre-commit hook adds ~2 seconds to every commit (running check-docs and check-agents)
- False positives require updating the whitelist (e.g., when a new root-level config file is added)

### Neutral

- Complements runtime hooks (ADR-017) — runtime hooks prevent bad state during sessions, commit hooks catch everything at commit time
- The same validation targets (`check-docs`, `check-agents`) run in CI (ADR-015), so commit-hook failures are also caught by the CI pipeline as a safety net
- GitHub Actions CI serves as the backstop for contributors who bypass local hooks
