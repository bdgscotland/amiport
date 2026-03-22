# ADR-019: Agent Persona and Tool Matrix

## Status

Accepted

## Date

2026-03-21

## Context

The porting pipeline has distinct stages with different risk profiles. Analysis is read-only — it should never modify files. Source transformation requires careful, rule-based edits to ported source. Building needs iterative compile-fix loops. Testing needs shell access but should not edit source. Memory checking must be read-only (it reports, never fixes). Publishing to Aminet is irreversible and must never happen without human approval.

A single "porting agent" with full permissions would be simpler but dangerous: a memory-checker that can edit source might "fix" issues it finds, bypassing the code-transformer's transformation rules. A test-runner with write access might create test artifacts outside the port directory. An aminet-publisher with autonomous execution might upload an untested port.

## Decision

Define 12 specialized agents in `.claude/agents/`, each with a specific persona, model selection, and constrained tool access:

| Agent | Model | Key Tools | What's Restricted |
|-------|-------|-----------|-------------------|
| `aminet-researcher` | sonnet | WebSearch, WebFetch, Read | No Edit, no Write, no Bash |
| `source-analyzer` | sonnet | Read, Grep, Glob | No Edit, no Write — read-only analysis |
| `code-transformer` | sonnet | Read, Edit, Write, Grep | Full edit access to `ported/` only (hook-enforced) |
| `build-manager` | sonnet | Bash, Read, Edit, Write | Iterative compile-fix; edits `ported/` and Makefile |
| `test-runner` | sonnet | Bash, Read | No Edit — runs tests, reports results |
| `port-coordinator` | sonnet | All tools + Agent dispatch | Orchestrates other agents; uses worktree isolation |
| `dependency-auditor` | sonnet | Read, Grep, WebSearch | No Edit — research and reporting only |
| `memory-checker` | sonnet | Read, Grep, Glob | No Edit, no Write — **mandatory** stage, report only |
| `perf-optimizer` | sonnet | Read, Grep, Glob, Bash | Read + analysis; suggests changes, doesn't apply them |
| `hardware-expert` | sonnet | Read, Grep, WebSearch, Edit | Dual-role: consult (read-only) + audit (edit reference docs) |
| `debug-agent` | sonnet | All tools | Full access — autonomous crash fix loop with git stash rollback |
| `aminet-publisher` | sonnet | Bash, Read, Write | Curated — **never publishes without explicit human approval** |

### Design principles

1. **Least privilege** — each agent gets only the tools it needs. Read-only agents cannot write. Analysis agents cannot execute.
2. **Model selection by task complexity** — sonnet for all agents (fast, capable). The port-coordinator may escalate to opus for complex judgment calls.
3. **Mandatory vs optional** — memory-checker is mandatory for every port (AmigaOS has no memory protection). perf-optimizer is optional (performance tuning is nice-to-have).
4. **Human gates** — aminet-publisher requires explicit approval before any upload. debug-agent caps fix attempts at 5 before escalating.
5. **Hook enforcement** — tool restrictions in agent definitions are reinforced by PreToolUse hooks (ADR-017). Even if an agent's prompt is modified, the hooks block violations at the infrastructure level.

## Consequences

### Positive

- Risk-appropriate permissions — a reporting agent cannot accidentally modify source code
- Clear responsibility boundaries — when a build fails, you know the build-manager owns it, not a test-runner that got creative
- Agents can be improved independently — updating the memory-checker's analysis rules doesn't risk breaking the build pipeline
- Mandatory gates (memory-checker) ensure safety-critical steps cannot be skipped

### Negative

- 12 agent definitions to maintain — changes to the pipeline may require updating multiple agent files
- Agent dispatch overhead — each agent launch has startup cost; a monolithic agent would be faster for simple ports
- Agents cannot help each other directly — if the build-manager discovers a transformation error, it must fail and let the coordinator re-dispatch the code-transformer rather than fixing it inline

### Neutral

- The agent definitions in `.claude/agents/` serve as both configuration and documentation of each agent's role
- New agents can be added for new pipeline stages without modifying existing agents
- The port-coordinator agent orchestrates the full sequence, so users typically interact with `/port-project` rather than individual agents
