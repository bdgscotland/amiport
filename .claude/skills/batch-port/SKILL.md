---
name: batch-port
description: Dispatch the next ready porting candidate from the catalog. Session-resilient — dispatches ONE port per invocation. Re-run to continue the batch.
user_invocable: true
---

# /batch-port — Autonomous Batch Porting

Dispatch the next best porting candidate from the catalog.

## Usage

```
/batch-port                                # Next best Cat 1 candidate for a1200_accel
/batch-port cli a1200_accel                # Explicit category + profile
/batch-port cli stock_a1200                # Different hardware profile
/batch-port scripting a1200_accel          # Category 2 candidates
```

## Session-Resilient Design

This skill dispatches **ONE port per invocation.** After the port completes (or the session ends), re-run `/batch-port` to dispatch the next candidate. The catalog.json status field IS the queue — it persists across sessions.

## Algorithm

1. Run `python3 scripts/catalog-score.py --score` to re-score all candidates (accounts for any shim changes since last run)
2. Run `python3 scripts/catalog-score.py --next 1 --category <cat> --profile <prof>` to get the best candidate
3. If no ready candidates: print recommendations (top shim unlocks) and stop
4. Check `publishable` field — skip if false
5. Set candidate `status: "in_progress"` in catalog.json (atomic write)
6. Dispatch the `catalog-engineer` agent with the candidate details:
   - Agent dispatches `/port-project` for the candidate
   - For Cat 1: use `--unattended` mode (auto-answer decisions)
   - For Cat 2+: standard interactive mode
7. On success:
   - Move candidate data to `ported[]` in catalog.json
   - Collect measured binary size + stack from compiled port
   - Update PORTS.md with new row
   - Re-run scoring to update the catalog
8. On failure:
   - Set candidate `status: "failed"` with error reason
   - Continue (user re-runs for next candidate)

## Default Arguments

- Category: `cli` (Cat 1 — the only fully autonomous category)
- Profile: `a1200_accel` (the standard FS-UAE test config)

## Stale In-Progress

If a candidate has `status: "in_progress"` (stale from a crashed session), treat it as a retry candidate. Re-dispatch it before selecting a new one.

## After Batch Completion

Run `python3 scripts/catalog-score.py --status` to see the updated dashboard.
Run `python3 scripts/catalog-score.py --diff data/catalog-before.json` to see what changed (if you saved a snapshot before the batch).
