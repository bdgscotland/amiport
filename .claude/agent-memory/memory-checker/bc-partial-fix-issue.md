---
name: bc-partial-fix-issue
description: GNU bc 1.07.1 partial memory fixes — 4 applied, 4 critical leaks remain unfixed
type: project
---

# GNU bc 1.07.1 Memory Fixes — Partial Completion (2026-03-23)

## Status

**Verdict:** PARTIAL FIXES APPLIED — Cannot ship yet

Someone applied 4 out of ~8 critical memory leak fixes to ports/bc/ported/, but 4 critical leaks remain:

1. Math library strdups (6×) — lines 343-348 in main.c, NOT freed
2. Name array individual entries (f_names[1..n], v_names[*], a_names[*]) — NOT freed
3. Function body buffers (functions[].f_body) — NOT freed
4. Global constants + free list cache (_zero_, _one_, _two_, _bc_Free_list) — NOT freed

## What Was Fixed

✓ amiport_getenv("BC_ENV_ARGS") tracking (env_allocs[]) and cleanup
✓ strdup("BC_ENV_ARGS") freed after parse_args()
✓ POSIXLY_CORRECT getenv freed immediately
✓ BC_LINE_LENGTH getenv freed after atoi()

## What Wasn't

✗ Math lib strdups in open_new_file()
✗ Individual name string cleanup (only f_names[0] freed, rest leak)
✗ Function body buffer cleanup
✗ Global constants freed via bc_cleanup()
✗ _bc_Free_list cache cleanup

## Why This Matters

- Each run leaks 150+ bytes (minimal) to 1+ KB (complex programs)
- On AmigaOS with -noixemul, no automatic cleanup on exit
- 100 runs = 40 KB lost, 1000 runs = 400 KB lost
- Exceeds typical Amiga RAM (512KB-2MB) quickly
- **Cannot ship without completing these fixes**

## Required Fixes (Medium Complexity)

All fixes are straightforward for loops in bc_cleanup() — 2-3 hours total.

See `/Users/duncan/Developer/amiport/.claude/agent-memory/memory-checker/memory-audit-bc-recheck.md` for detailed code fixes.

## Next Steps

1. Apply remaining 5 fixes (detailed in re-audit)
2. Re-run memory-checker agent
3. Verify with test suite
4. Then proceed to perf-optimizer and publishing

