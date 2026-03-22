# ADR-017: PreToolUse Hooks for Structural Enforcement

## Status

Accepted

## Date

2026-03-21

## Context

The porting pipeline relies on agents following rules: don't edit upstream source in `original/`, don't create files in the project root, don't call the compiler directly, don't manually edit ported source (use the code-transformer agent). These rules were initially documented only in CLAUDE.md and agent prompts.

In practice, agents violate prose rules. A build-manager agent that hits a compilation error may "helpfully" edit the ported source directly instead of delegating to the code-transformer. An agent debugging a test may create temporary files in the project root. These violations are silent — they succeed, the agent doesn't know it broke a rule, and the mess accumulates.

Two enforcement approaches were considered:

1. **Behavioral rules in prose** — document rules in CLAUDE.md and agent definitions, trust agents to follow them. Zero infrastructure cost but unreliable.
2. **Structural enforcement via hooks** — use Claude Code's PreToolUse hook mechanism to intercept tool calls before execution and block violations. The hook script runs on every Edit/Write/Bash call, checks the target path or command against rules, and returns a deny response with an explanation. The agent cannot bypass this — the tool call never executes.

## Decision

Use PreToolUse hooks in `.claude/settings.json` to enforce structural rules at the tool invocation level. Four hooks enforce the core invariants:

| Hook | What it blocks | Why |
|------|---------------|-----|
| `block-original-edits.sh` | Edit/Write to paths containing `/original/` | Upstream source is read-only reference |
| `block-root-files.sh` | Edit/Write of non-config files in project root | Prevents agent debris accumulation |
| `block-direct-gcc.sh` | Direct `m68k-amigaos-gcc`/`ld`/`as` in Bash | Forces use of `make` or toolchain wrappers |
| `enforce-agents.sh` | Edit/Write to `ported/*.c` files | Forces code-transformer or debug-agent dispatch |

Hook scripts live in `scripts/hooks/` and are configured in `.claude/settings.json` under `hooks.preToolUse`. Each script receives the tool name and parameters as JSON on stdin, inspects the relevant field (file path or command), and exits 0 (allow) or 2 (deny with message).

Additionally, a `hookify` plugin provides configurable rules for lighter-weight enforcement (e.g., blocking test file creation in the project root).

## Consequences

### Positive

- Rule violations are impossible, not just discouraged — the tool call is rejected before execution
- Agents get an immediate error message explaining what to do instead, enabling self-correction
- Rules are centralized in `.claude/settings.json` rather than scattered across agent prompts
- Works for any agent regardless of model, prompt, or persona — enforcement is at the infrastructure layer

### Negative

- Hook scripts add latency to every tool call (typically <50ms, but cumulative across a port)
- Agents that hit a hook must reason about the alternative path, which sometimes requires multiple attempts
- Hook logic must be maintained alongside the rules — a new directory convention requires a new or updated hook

### Neutral

- Complements (does not replace) prose documentation — CLAUDE.md still explains "why," hooks enforce "what"
- The pattern is extensible to future rules (e.g., blocking writes to `lib/` without `/extend-shim`)
