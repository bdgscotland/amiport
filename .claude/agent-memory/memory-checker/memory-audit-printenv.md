---
name: memory-audit-printenv
description: ports/printenv 1.8 memory safety review — argv expansion with direct atexit callback
type: project
---

# Memory Safety Review: ports/printenv 1.8

## Allocations Found

| Location | Type | Freed? | All Paths? | Issue |
|----------|------|--------|------------|-------|
| main:173 | amiport_expand_argv() | Yes | Yes | atexit(amiport_free_argv) covers all paths |
| print_all_env:105 | AllocDosObject(DOS_EXALLCONTROL) | Yes | Yes | FreeDosObject at line 153 on all paths |
| print_all_env:98 | Lock("ENV:", SHARED_LOCK) | Yes | Yes | UnLock at line 154 on all paths |

## Analysis

### Code Flow - main()

1. **Line 173**: `amiport_expand_argv(&argc, &argv)` allocates expanded argv
2. **Line 174**: `atexit(amiport_free_argv)` registers direct cleanup callback (not a wrapper function)
3. **Line 177-178**: `pledge()` check with `err()` — cleanup via atexit
4. **Line 180-183**: No args case — calls `print_all_env()` then `exit(0)` — atexit cleanup fires
5. **Line 191-196**: Specific variable lookup — calls `GetVar()`, then `exit(0)` or `exit(10)` — atexit cleanup fires
6. **Line 200**: Returns with `exit(10)` if variable not found — atexit cleanup fires

**EXIT PATH COVERAGE**: ✓ All paths call either `exit(0)`, `exit(10)`, or return from main, triggering atexit

### Code Flow - print_all_env()

1. **Line 98**: `Lock("ENV:", SHARED_LOCK)` — allocates lock
2. **Line 99-102**: Error handling — `return` on failure (lock NULL)
   - **SAFE**: UnLock(lock) is NOT called (lock is NULL), correct behavior
3. **Line 105**: `AllocDosObject(DOS_EXALLCONTROL)` — allocates control struct
4. **Line 106-109**: Error handling — `UnLock(lock)` and `return` on failure
   - **SAFE**: Lock freed before return, correct
5. **Lines 110-112**: Initialize control struct fields
6. **Lines 114-151**: ExAll() loop
   - Inner loop processes entries
   - `break` on error — exits loop
   - `continue` on no entries — skips to next iteration
   - All paths remain inside the function
7. **Line 153**: `FreeDosObject(DOS_EXALLCONTROL, eac)` — frees control struct on all paths
   - **ANALYSIS**: Control struct freed AFTER the do-while loop completes, BEFORE UnLock
   - **SAFE**: Whether loop completes normally or breaks, line 153 is reached
8. **Line 154**: `UnLock(lock)` — frees lock on all paths
   - **SAFE**: Called after all processing is complete

**EXIT PATH COVERAGE (print_all_env)**:
✓ Early return on Lock failure (line 101): No unlock needed, lock is NULL
✓ Early return on AllocDosObject failure (line 108): UnLock called at line 107 first
✓ Break on ExAll error (line 118): Falls through to FreeDosObject + UnLock
✓ Normal loop completion: Falls through to FreeDosObject + UnLock
✓ All paths: Lock and control struct freed

### Memory Allocations Summary

| Function | Allocation | Freed | Path Coverage |
|----------|-----------|-------|-----------------|
| main | argv expansion (amiport_expand_argv) | amiport_free_argv via atexit | ALL |
| main | None other | N/A | N/A |
| print_all_env | Lock | UnLock (line 154) | ALL except early NULL return |
| print_all_env | AllocDosObject | FreeDosObject (line 153) | ALL paths after allocation succeeds |

### Double-Free Risk

- **amiport_free_argv**: Called once via atexit, safe
- **UnLock(lock)**: Called once at line 154, after lock allocation succeeds or not allocated (NULL check at 99)
- **FreeDosObject**: Called once at line 153, after allocation succeeds
- **No dangling pointer reuse**: All pointers are either NULL-checked or properly freed

### Stack Safety

- `__stack = 8192` set at line 61 ✓
- Local variables in main: `val[ENV_VAL_BUF]` (256 bytes stack), `vlen` (LONG), `verstag` suppress (unused)
  - **RISK**: 256-byte stack local — total stack depth with function calls should be safe with 8KB stack
  - **ANALYSIS**: GetVar() is a POSIX-like call — estimated hidden stack depth ~500-1000 bytes on AmigaOS
  - **VERDICT**: 256 + 1000 + margin = ~1500 bytes reasonable within 8KB stack
- Local variables in print_all_env: `lock`, `eac`, `ead`, `exall_buf[1024]` (static), `more`, `val[256]`, `vlen`
  - **LARGE BUFFER**: exall_buf is `static` (not stack), safe
  - **STACK**: val[256] + other locals = ~300 bytes stack
  - **RISK**: ExAll() + GetVar() + printf() chain could add 1-2KB hidden stack depth
  - **VERDICT**: Reasonable within 8KB with margins

### amiport_getenv() Risk

- Not used. Uses AmigaOS GetVar() directly instead.
- **CORRECT APPROACH**: Avoids amiport_getenv() which would leak malloc'd strings if not freed

## Verdict

**CLEAN** — Approved for shipping.

The port correctly implements argv cleanup via atexit and proper resource cleanup in print_all_env(). All allocations (lock, control struct, argv) are freed on all code paths. No dangling pointers, double-frees, or use-after-free issues detected. Stack usage is conservative with large buffers made static.

## Notes

- **Excellent pattern**: Using AmigaOS ExAll() directly instead of relying on non-existent environ[]
- **Correct atexit usage**: `atexit(amiport_free_argv)` passes the function pointer directly (no wrapper needed here)
- **Safe early returns**: Lock check at line 99 correctly avoids UnLock on NULL
- **Safe error path**: AllocDosObject error at 106 correctly UnLocks before returning
- **Correct API usage**: ExAll()/AllocDosObject/GetVar/UnLock follow ADCD patterns correctly
- **Static buffer pattern**: exall_buf is static (1024 bytes), not stack-allocated — good memory discipline
- **No environment pollution**: Iterates ENV: instead of extern environ[] which doesn't exist on -noixemul

## Potential Improvements

None required. The code is safe and exemplary.

The only minor observation is that pledge() is a no-op stub, but the pattern is correct for cross-platform code.
