---
name: batch-port-parallel
description: Dispatch multiple ports from the catalog in parallel using specialized agents. Runs the same pipeline stages as /port-project but overlaps them across ports for ~3x throughput.
argument-hint: [count] [category] [profile]
user_invocable: true
allowed-tools: Read, Write, Edit, Bash, Grep, Glob, Agent
---

# /batch-port-parallel — Parallel Batch Porting

Port N programs simultaneously by running specialized pipeline agents in parallel across ports. Uses the SAME agents as `/port-project` (source-analyzer, code-transformer, build-manager, test-designer, test-runner) — not a generalist substitute.

## Usage

```
/batch-port-parallel              # 3 Cat 1 ports on a1200_accel (default)
/batch-port-parallel 5            # 5 ports
/batch-port-parallel 3 cli        # 3 Cat 1 CLI ports
/batch-port-parallel 3 scripting stock_a1200
```

## Arguments

Parse `$ARGUMENTS` as: `[count] [category] [profile]`
- **count**: Number of ports to dispatch (default: 3, max: 10)
- **category**: `cli` (default), `scripting`, `console`, `network`
- **profile**: `a1200_accel` (default), `stock_a1200`

## Pre-flight

```bash
docker info --format '{{.ServerVersion}}' 2>/dev/null || echo 'DOCKER_NOT_RUNNING'
vamos --version 2>&1 | head -1 || echo 'VAMOS_NOT_INSTALLED'
python3 scripts/catalog-score.py --validate 2>&1 | tail -1
make build-shim 2>&1 | tail -3
```

If Docker is not running, STOP. Build the shim library FIRST — all ports share it.

## Architecture

```
PARALLEL ACROSS PORTS, SPECIALIZED AGENTS PER STAGE:

Stage 0:  [aminet-researcher A] [aminet-researcher B] [aminet-researcher C]  ← all background
          ↓                     ↓                     ↓
Stage 1:  [source-analyzer A]   [source-analyzer B]   [source-analyzer C]    ← as each returns
          ↓                     ↓                     ↓
Stage 2:  [setup dirs A]        [setup dirs B]        [setup dirs C]         ← inline (fast)
          ↓                     ↓                     ↓
Stage 3:  [code-transformer A]  [code-transformer B]  [code-transformer C]   ← background
          ↓                     ↓                     ↓
Stage 4:  [build-manager A]     [build-manager B]     [build-manager C]      ← background
          ↓                     ↓                     ↓
Stage 5b: [test-designer A]     [test-designer B]     [test-designer C]      ← background
          ↓                     ↓                     ↓
Stage 5:  [vamos test A]        [vamos test B]        [vamos test C]         ← background
          ↓                     ↓                     ↓
Stage 5c: FS-UAE A → FS-UAE B → FS-UAE C                                     ← SERIAL (one at a time)
          ↓                     ↓                     ↓
Stage 6:  [memory-checker A]    [memory-checker B]    [memory-checker C]     ← background
          [perf-optimizer A]    [perf-optimizer B]    [perf-optimizer C]     ← background
```

Each column is one port. Each row dispatches ALL ports at that stage in parallel.
As each agent returns, its port advances to the next stage independently.

## Algorithm

### Phase 1: Select Candidates

```bash
python3 scripts/catalog-score.py --score --next <count> --category <category> --profile <profile>
```

Parse output. Filter out `status: "in_progress"` or `status: "failed"` (unless stale >24h). If fewer candidates than requested, dispatch what's available.

### Phase 2: Mark In-Progress

Set `status: "in_progress"` for all candidates in `data/catalog.json`.

### Phase 3: Stage 0 — Research (PARALLEL)

Dispatch `aminet-researcher` for ALL ports simultaneously in a single message:

```
Agent(subagent_type="aminet-researcher", run_in_background=true,
      prompt="Check if <name> already exists for AmigaOS 3.x on Aminet...")
Agent(subagent_type="aminet-researcher", run_in_background=true,
      prompt="Check if <name> already exists for AmigaOS 3.x on Aminet...")
...
```

As each returns: if SKIP (existing port found), remove from the active batch. If PROCEED, advance to Stage 1.

### Phase 4: Stage 1 — Analyze (PARALLEL)

For all ports that passed Stage 0, dispatch `source-analyzer` in parallel:

```
Agent(subagent_type="source-analyzer", run_in_background=true,
      prompt="Analyze <source-path> for Amiga portability. Report tier classification, POSIX deps, and verdict.")
```

As each returns: if INFEASIBLE, mark failed. If feasible, advance to Stage 2.

### Phase 5: Stage 2 — Setup Dirs (INLINE)

For each port that passed analysis, set up the port directory structure inline (fast, no agent needed):

1. Create `ports/<name>/original/` and `ports/<name>/ported/`
2. Copy/download source files to `original/`, then to `ported/`
3. Create Makefile from template
4. Create PORT.md from template

This runs in the main session sequentially — it's fast (<10s per port).

### Phase 6: Stage 3 — Transform (PARALLEL)

Dispatch `code-transformer` for all ready ports simultaneously:

```
Agent(subagent_type="code-transformer", run_in_background=true,
      prompt="Transform ports/<name>/ported/ source files for AmigaOS. Apply all transformation rules. Check shim availability in lib/posix-shim/include/amiport/.")
```

As each returns: verify transformed files exist in `ported/`. If transform failed, mark as failed.

### Phase 7: Stage 4 — Build (PARALLEL)

Dispatch `build-manager` for all transformed ports simultaneously:

```
Agent(subagent_type="build-manager", run_in_background=true,
      prompt="Build ports/<name> for AmigaOS. Iterate up to 5 times on compile errors.")
```

As each returns: check for binary at `ports/<name>/<name>`. If build failed, mark as failed.

### Phase 8: Stage 5b — Test Design (PARALLEL)

Dispatch `test-designer` for all built ports simultaneously:

```
Agent(subagent_type="test-designer", run_in_background=true,
      prompt="Design comprehensive FS-UAE test suite for ports/<name>. Generate test-fsemu-cases.txt per docs/test-coverage-standard.md.")
```

### Phase 9: Stage 5 — vamos Test (PARALLEL)

Dispatch `test-runner` for all ports with test suites:

```
Agent(subagent_type="test-runner", run_in_background=true,
      prompt="Test ports/<name> via vamos ONLY. Run: make test TARGET=ports/<name>. Do NOT run make test-fsemu or launch FS-UAE — vamos only. Report TAP pass/fail counts.")
```

vamos tests are independent per port — safe to overlap. **CRITICAL: The prompt MUST say "vamos ONLY, do NOT run FS-UAE"** — test-runner agents will otherwise run FS-UAE, causing parallel collisions with shared system.hdf.

### Phase 10: Stage 5c — FS-UAE Testing (SERIAL — CRITICAL)

**Only ONE FS-UAE instance can run at a time.** `build/system.hdf` and `build/amiga/` are shared singletons.

Process each port **one at a time through the full fix loop**:

```
for each port:
  1. make test-fsemu TARGET=ports/<name>
  2. if tests pass → move to next port
  3. if tests FAIL:
     a. Analyze failure (wrong output? crash? timeout?)
     b. If crash → dispatch debug-agent, apply fix, rebuild
     c. If wrong output → fix transform, rebuild
     d. Re-run make test-fsemu TARGET=ports/<name>
     e. Repeat until pass or 3 fix attempts exhausted
  4. if 3 attempts exhausted → mark as FAILED, move to next port
```

**Do NOT dispatch FS-UAE tests in the background.** The fix loop needs FS-UAE to verify — two ports in fix loops corrupt shared state.

### Phase 11: Stage 6 — Reviews (PARALLEL)

For all ports that passed FS-UAE, dispatch review agents in parallel:

```
Agent(subagent_type="memory-checker", run_in_background=true, prompt="Check ports/<name>/ for memory safety...")
Agent(subagent_type="perf-optimizer", run_in_background=true, prompt="Optimize ports/<name>/ for 68k performance...")
```

Dispatch ALL memory-checkers and perf-optimizers for all passing ports in a single message.

If reviews find CRITICAL issues: fix, rebuild, re-run FS-UAE **serially** (same constraint as Phase 10).

### Phase 11b: PORT.md Enrichment (PARALLEL — MANDATORY)

**PORT.md quality must match single-port `/port-project` quality.** Batch ports must NOT have thin boilerplate PORT.md files. For all passing ports, dispatch agents to enrich PORT.md with:

1. **Description** — 2-3 sentences, not one line
2. **Portability Analysis** — Table of every POSIX dep, tier, resolution (read `/* amiport: */` comments)
3. **Transformations Applied** — Table with File, Line, Original, Transformed, Comment
4. **Build Configuration** — Stack size, CFLAGS, VAMOS_STACK, special flags
5. **Test Results** — Count, pass rate, categories tested, notable edge cases
6. **Memory Safety** — Verdict from memory-checker, allocation patterns
7. **Performance Notes** — Key perf-optimizer findings
8. **Known Limitations** — Anything discovered during testing

Dispatch in parallel (5 ports per agent max):
```
Agent(subagent_type="general-purpose", run_in_background=true,
      prompt="Enrich PORT.md for ports/<names>. Read ported source, agent-memory, test results. Match quality of ports/grep/PORT.md.")
```

**Do NOT skip this step.** Thin PORT.md files are a documentation debt that makes the port invisible to future maintainers.

### Phase 12: Catalog Update

For each completed port:
1. Move candidate from `candidates[]` to `ported[]` in `data/catalog.json`
2. Set `measured_binary_kb`, `test_count`, `test_pass_rate`
3. **Sync site catalog:** `cp data/catalog.json site/data/catalog.json` (MANDATORY — see `.claude/rules/catalog-sync.md`)
4. Add row to `PORTS.md`
5. Run `python3 scripts/catalog-score.py --score`

### Phase 12b: Publish to amiport + Commit

For each completed port, dispatch `amiport-publisher` to package and deploy. Then commit all changes and push:

```bash
git add -A ports/<name>/ site/ data/ .claude/ PORTS.md scripts/
git commit -m "feat: add <names> ports"
git push
```

**Do NOT wait for the user to ask.** Publishing and committing are part of the standard pipeline.

### Phase 13: Summary

```
BATCH COMPLETE: N/M ports succeeded

  Ported:
    foo 1.0 — 15/15 tests pass, 12KB binary
    bar 2.1 — 18/18 tests pass, 8KB binary

  Failed:
    baz — build failed: missing shim function amiport_mmap()

  Skipped:
    qux — existing Aminet port found

  Next steps:
    - Run /batch-port-parallel again for the next batch
    - Run make test-fsemu TARGET=ports/<name> VISUAL=1 for Cat 3+ visual tests
    - Run /publish-package for Aminet/amiport publishing
```

## Pipelining — Don't Wait for All Ports at Each Stage

**CRITICAL:** Do NOT wait for all ports to finish a stage before starting the next. As EACH port's agent returns, immediately dispatch that port's next stage. The pipeline should look like:

```
Port A: [research]→[analyze]→[transform]→[build]→[test-design]→[vamos]→[FS-UAE]
Port B:    [research]→[analyze]→[transform]→[build]→[test-design]→[vamos]─[FS-UAE]
Port C:      [research]→[analyze]→[transform]→[build]→[test-design]→[vamos]──[FS-UAE]
```

NOT:

```
All: [research A,B,C] → wait → [analyze A,B,C] → wait → [transform A,B,C] → wait
```

The whole point is pipelining. A fast port shouldn't wait for a slow one.

## Error Handling

- **Agent timeout**: If an agent hasn't returned after 15 minutes, it's likely stuck. Note it and move the port to FAILED.
- **Docker down mid-batch**: Build agents will fail. Mark those ports as FAILED.
- **Shim gaps**: If multiple ports report the same missing shim function, pause the batch and suggest `/extend-shim <function>` before continuing.
- **Catalog conflicts**: Re-read catalog.json before Phase 12 updates.

## Limits

- **Max 10 concurrent ports.** Each port may have 1-2 agents running at different stages. With 1M context window, 10 ports is feasible. Pipeline aggressively — don't wait for all ports at each stage.
- **Cat 1 only for unattended batch.** Cat 2+ may need Tier 2/3 tradeoff decisions. Use `/port-project` for Cat 2+.
- **FS-UAE stays serial.** See `docs/porting-guide.md` "Parallel Porting" section.
- **Build shim first.** If two ports both need `/extend-shim`, they'll conflict. Build the shim before starting the batch.
