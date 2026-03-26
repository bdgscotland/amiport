---
name: memory-audit-echo
description: ports/echo 1.12 memory safety review — argv expansion with atexit cleanup
type: project
---

# Memory Safety Review: ports/echo 1.12

## Allocations Found

| Location | Type | Freed? | All Paths? | Issue |
|----------|------|--------|------------|-------|
| main:68 | amiport_expand_argv() | Yes | Yes | atexit(cleanup) covers all paths |
| cleanup:57 | amiport_free_argv() | N/A | N/A | Registered via atexit(70) |

## Analysis

### Code Flow

1. **Line 68**: `amiport_expand_argv(&argc, &argv)` allocates expanded argv
2. **Line 70**: `atexit(cleanup)` registers cleanup function
3. **Lines 72-73**: `err()` on pledge failure — cleanup via atexit
4. **Lines 76-110**: Main option/argument processing — all exit via `return` at end of main
5. **cleanup()**: Calls `amiport_free_argv()` + `fflush(stdout)`

### Exit Path Coverage

✓ **Success path (line 109)**: Returns 0 → atexit cleanup fires
✓ **pledge error (line 73)**: `err(10, ...)` → process exit → atexit cleanup fires
✓ **All code paths**: No early returns with untracked allocations

### Stack Safety

- `__stack = 8192` set at line 49 ✓
- Local variables: `nflag`, `eflag`, `p` (all on stack, small)
- `escape()` function: reads string, no allocations
- No large local buffers

### Double-Free Risk

- None. `amiport_free_argv()` called exactly once via atexit
- No manual free() calls anywhere in code

### amiport_getenv() Risk

- Not used in this port

## Verdict

**CLEAN** — Approved for shipping.

The port correctly follows the atexit cleanup pattern. Every dynamic allocation (argv expansion) is paired with a cleanup function registered before any code that could exit prematurely. The pledge() error path correctly uses err() which terminates, triggering cleanup via atexit.

## Notes

- Perfect example of atexit cleanup done correctly (see known-pitfalls.md "atexit() for argv Expansion Cleanup")
- stderr/stdout flushing in cleanup prevents buffered output loss on exit
- No unusual memory patterns or risks detected
