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
| Debug crashes | `debug-agent` agent or `/debug-amiga` | Don't manually debug Enforcer hits |
| Check memory safety | `memory-checker` agent | Don't skip memory audit — AmigaOS has no memory protection |
| Hardware validation | `hardware-expert` agent | Don't guess at hardware architecture facts |
| Add missing shim fn | `/extend-shim` | Don't manually add shim functions |
| Website operations | `site-manager` agent | Don't manually rsync or debug PHP |
| Publish to amiport site | `amiport-publisher` agent | Don't manually set package status or skip test gates |

## How to Dispatch

Use the `Agent` tool with `subagent_type` set to the agent name from `.claude/agents/`:

```
subagent_type: "aminet-researcher"      # Stage 0: research
subagent_type: "dependency-auditor"     # Stage 0b: dep audit (conditional)
subagent_type: "source-analyzer"        # Stage 1: analyze
subagent_type: "code-transformer"       # Stage 3: transform
subagent_type: "hardware-expert"        # Stage 3b: hardware review (Cat 3+)
subagent_type: "build-manager"          # Stage 4: build
subagent_type: "test-designer"          # Stage 5b: test suite generation
subagent_type: "test-runner"            # Stage 5: vamos testing
subagent_type: "memory-checker"         # Stage 6b: memory safety (mandatory)
subagent_type: "perf-optimizer"         # Stage 6c: performance (mandatory)
subagent_type: "profiler"               # Stage 6d: empirical runtime measurement (optional)
subagent_type: "debug-agent"            # On crash: autonomous fix loop
# port-coordinator: DEPRECATED — cannot dispatch subagents from within an agent.
# For complex multi-file ports, orchestrate from main session dispatching each
# specialized agent directly (source-analyzer, code-transformer, build-manager, etc.)
subagent_type: "aminet-publisher"       # Publishing (never automatic)
subagent_type: "site-manager"           # Website deployment and testing
subagent_type: "amiport-publisher"      # Test-gated publishing to amiport site
```

## Entry Point

For any new port, always start with `/port-project` — it orchestrates the full pipeline (Stage 0 through Stage 7) and dispatches the right agents at each step.
