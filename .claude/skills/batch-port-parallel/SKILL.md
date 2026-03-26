---
name: batch-port-parallel
description: Dispatch multiple ports from the catalog in parallel using isolated worktrees. Each port gets its own worker agent. FS-UAE testing and reviews run serially after all workers complete.
argument-hint: [count] [category] [profile]
user_invocable: true
allowed-tools: Read, Write, Edit, Bash, Grep, Glob, Agent
---

# /batch-port-parallel — Parallel Batch Porting

Dispatch N ports simultaneously, each in its own isolated worktree.

## Usage

```
/batch-port-parallel              # 5 Cat 1 ports on a1200_accel (default)
/batch-port-parallel 3            # 3 ports
/batch-port-parallel 5 cli        # 5 Cat 1 CLI ports
/batch-port-parallel 3 scripting stock_a1200
```

## Arguments

Parse `$ARGUMENTS` as: `[count] [category] [profile]`
- **count**: Number of ports to dispatch (default: 5, max: 8)
- **category**: `cli` (default), `scripting`, `console`, `network`
- **profile**: `a1200_accel` (default), `stock_a1200`

## Pre-flight

```bash
docker info --format '{{.ServerVersion}}' 2>/dev/null || echo 'DOCKER_NOT_RUNNING'
vamos --version 2>&1 | head -1 || echo 'VAMOS_NOT_INSTALLED'
python3 scripts/catalog-score.py --validate 2>&1 | tail -1
```

If Docker is not running, STOP — builds will fail. If vamos is missing, workers can still build but not smoke-test.

## Algorithm

### Phase 1: Select Candidates

```bash
python3 scripts/catalog-score.py --score --next <count> --category <category> --profile <profile>
```

Parse the output to get candidate names, source URLs, and readiness scores. If fewer than `count` candidates are ready, dispatch what's available and report the shortfall.

Filter out candidates with `status: "in_progress"` or `status: "failed"` unless they are stale retries (>24h old).

### Phase 2: Mark In-Progress

For each candidate, set `status: "in_progress"` in `data/catalog.json`. Write all updates in a single atomic edit to avoid partial state.

### Phase 3: Dispatch Workers (PARALLEL)

Dispatch all workers simultaneously using the Agent tool. **All dispatches MUST be in a single message** to maximize parallelism.

For each candidate:

```
Agent(
    subagent_type="port-worker",
    description="Port <name> to AmigaOS",
    isolation="worktree",
    run_in_background=true,
    mode="bypassPermissions",
    model="sonnet",
    prompt="Port the C program '<name>' to AmigaOS 3.x.

Source: <source_url_or_description>
Category: <category>
Hardware profile: <profile>

This is an automated batch port. Follow your pipeline (stages 0-4b + test suite generation) completely. Report your structured summary when done."
)
```

Key dispatch parameters:
- `isolation: "worktree"` — each worker gets its own repo copy
- `run_in_background: true` — all workers run concurrently
- `mode: "bypassPermissions"` — workers need to run make, bash, etc. without prompts
- `model: "sonnet"` — cost-effective for routine Cat 1 ports

After dispatching all workers, tell the user:

> Dispatched N workers in parallel worktrees. You'll be notified as each completes.
> Estimated time: 5-10 minutes for Cat 1 CLI ports.

### Phase 4: Collect Results

As each worker completes, parse its structured summary. Track:
- Which ports succeeded (STATUS: SUCCESS)
- Which failed (STATUS: FAILED) — note the reason
- Which were skipped (STATUS: SKIP or INFEASIBLE) — note why
- Which worktree branches have changes

For failed ports, update catalog status to `"failed"` with the error reason.
For skipped ports, update catalog status to `"skipped"`.

### Phase 5: Merge Worktrees

For each successful port, the worktree created a branch with the port's files. Present the user with the list of completed ports and ask to merge:

> N ports completed successfully:
> - foo (15 tests, 12KB binary)
> - bar (18 tests, 8KB binary)
> - baz (20 tests, 24KB binary)
>
> Merge all to main?

On approval, merge each worktree branch to the current branch. If there are merge conflicts (unlikely since each port is in its own directory), resolve them.

### Phase 6: FS-UAE Testing and Bug Fix Loop (SERIAL — CRITICAL)

**Only ONE FS-UAE instance can run at a time.** `build/system.hdf` and `build/amiga/` are shared singletons. This constraint applies to both the initial test pass AND any fix-rebuild-retest cycles.

Process each successful, merged port **one at a time through the full loop**:

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

**Do NOT dispatch FS-UAE tests in the background.** Each `make test-fsemu` must complete (including any fix loops) before starting the next port. The fix loop needs FS-UAE to verify the fix worked — if two ports are both in fix loops, they'll corrupt each other's test state.

Report results as each port completes so the user sees progress.

### Phase 7: Reviews (PARALLEL)

For all ports that passed FS-UAE testing, dispatch review agents in parallel:

```
Agent(subagent_type="memory-checker", run_in_background=true, prompt="Check ports/<name>/ ...")
Agent(subagent_type="perf-optimizer", run_in_background=true, prompt="Optimize ports/<name>/ ...")
```

These can run in parallel because they are read-only analysis of different port directories.

If reviews find CRITICAL issues, fix them, rebuild, and **re-run FS-UAE serially** (same constraint as Phase 6 — one at a time through the fix loop).

### Phase 8: Catalog Update

For each completed port:
1. Move candidate from `candidates[]` to `ported[]` in `data/catalog.json`
2. Set `measured_binary_kb`, `test_count`, `test_pass_rate`
3. Add row to `PORTS.md`
4. Run `python3 scripts/catalog-score.py --score` to update the dashboard

### Phase 9: Summary

Print a final batch report:

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

## Error Handling

- **Worker timeout**: If a worker hasn't returned after 20 minutes, it's likely stuck. Note it as failed.
- **Docker down mid-batch**: Workers will fail at build stage. Their STATUS: FAILED reports will say why.
- **Shim gaps**: If multiple workers report the same missing shim function, note it as a batch-level blocker. Suggest `/extend-shim <function>` before retrying.
- **Catalog conflicts**: If catalog.json was modified externally during the batch, re-read before Phase 8 updates.

## Limits

- **Max 8 concurrent workers.** Beyond this, context saturation from result summaries degrades the main session. The docs recommend 3-5; 8 is the practical ceiling.
- **Cat 1 only for unattended batch.** Cat 2+ ports may need interactive decisions (Tier 2/3 tradeoffs). Use `/batch-port` (sequential) for Cat 2+.
- **FS-UAE stays serial.** Parallel FS-UAE requires Level B infrastructure changes (per-slot staging dirs). See `docs/porting-guide.md` "Parallel Porting" section.
