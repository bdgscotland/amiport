# Use Pipeline Agents — Not Manual Work

**Always use the project's custom agents and skills for porting work. Never do their jobs manually or with generic agents.**

## Agent Dispatch Table

| Task | Agent / Skill | Never Do Instead |
|------|--------------|------------------|
| Check if port exists | `aminet-researcher` agent | Don't use general-purpose agent for Aminet searches |
| Analyze source | `source-analyzer` agent or `/analyze-source` | Don't manually grep for POSIX calls |
| Transform source | `code-transformer` agent or `/transform-source` | Don't manually rewrite POSIX calls |
| Build for Amiga | `build-manager` agent or `/build-amiga` | Don't run compiler commands directly |
| Test via vamos | `test-runner` agent or `/test-amiga` | Don't run vamos manually |
| Full pipeline | `/port-project` | Don't run stages by hand |
| Audit dependencies | `dependency-auditor` agent | Don't manually check library deps |
| Amiga code review | `/review-amiga` | Don't skip the review step |
| Performance tuning | `perf-optimizer` agent | Don't hand-optimize 68k code |
| Publish to Aminet | `aminet-publisher` agent | Don't publish manually |
| Design test suites | `test-designer` agent | Don't manually write test-fsemu-cases.txt |
| Add missing shim fn | `/extend-shim` | Don't manually add shim functions |

## How to Dispatch

Use the `Agent` tool with `subagent_type` set to the agent name from `.claude/agents/`:

```
subagent_type: "aminet-researcher"
subagent_type: "source-analyzer"
subagent_type: "build-manager"
subagent_type: "test-runner"
subagent_type: "port-coordinator"
```

## Entry Point

For any new port, always start with `/port-project` — it orchestrates the full pipeline (Stage 0 through Stage 6) and dispatches the right agents at each step.
