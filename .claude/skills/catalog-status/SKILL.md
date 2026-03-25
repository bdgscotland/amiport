---
name: catalog-status
description: Print the porting tech tree catalog dashboard — readiness tier distribution, top shim unlock opportunities, and next batch recommendation.
user_invocable: true
---

# /catalog-status — Catalog Dashboard

Print the current state of the porting catalog.

## Usage

```
/catalog-status                           # Full dashboard
/catalog-status cli                       # Filter by category
/catalog-status cli a1200_accel           # Filter by category + hardware profile
```

## Implementation

Run the scoring script:

```bash
python3 scripts/catalog-score.py --status [--category <arg1>] [--profile <arg2>]
```

If the user provides arguments:
- First argument → `--category`
- Second argument → `--profile`

## Output

The script prints:
- Candidate count and ported count
- Readiness tier distribution (ready/feasible/blocked/infeasible)
- Top 5 shim unlock opportunities (which shims to implement next)
- Shim coverage percentage
- Next batch recommendation (top 5 ready candidates)

## No Catalog?

If `data/catalog.json` doesn't exist, tell the user:
"Catalog not found. Dispatch the `catalog-engineer` agent to enumerate candidates and run dry-run analysis."
