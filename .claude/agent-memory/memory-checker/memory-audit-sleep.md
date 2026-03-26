---
name: memory-audit-sleep
description: ports/sleep 1.29 memory safety review — argv expansion with atexit cleanup
type: project
---

# Memory Safety Review: ports/sleep 1.29

## Allocations Found

| Location | Type | Freed? | All Paths? | Issue |
|----------|------|--------|------------|-------|
| main:89 | amiport_expand_argv() | Yes | Yes | atexit(cleanup) covers all paths |
| cleanup:71 | amiport_free_argv() | N/A | N/A | Registered via atexit(91) |

## Analysis

### Code Flow

1. **Line 89**: `amiport_expand_argv(&argc, &argv)` allocates expanded argv
2. **Line 91**: `atexit(cleanup)` registers cleanup function
3. **Lines 93-94**: `err()` on pledge failure — cleanup via atexit
4. **Lines 98-103**: getopt option parsing (no options)
5. **Lines 107-108**: `usage()` called if argc != 1 (hard exit)
6. **Lines 111-137**: Parsing seconds/fraction — validates input, no allocations
7. **Lines 146-156**: Delay() call (AmigaOS sleep)
8. **Line 158**: Returns 0
9. **cleanup()**: Calls `amiport_free_argv()` + `fflush(stdout)`
10. **usage() @ 162-166**: Calls `exit(10)` — triggers atexit cleanup

### Exit Path Coverage

✓ **Success path (line 158)**: Returns 0 → atexit cleanup fires
✓ **pledge error (line 94)**: `err(10, ...)` → process exit → atexit cleanup fires
✓ **usage() (line 101 or 108)**: Calls `exit(10)` → atexit cleanup fires
✓ **errx() on invalid seconds (line 117)**: Process exit → atexit cleanup fires
✓ **errx() on seconds overflow (line 120)**: Process exit → atexit cleanup fires
✓ **errx() on invalid fraction (line 133)**: Process exit → atexit cleanup fires
✓ **All code paths**: Every exit triggers atexit cleanup

### Stack Safety

- `__stack = 8192` set at line 37 ✓
- Local variables: `tv_sec`, `tv_nsec`, `t`, `cp`, `ch`, `i`, `ticks` (all on stack, reasonable sizes)
- No large local buffers (char arrays, structs with large fields)
- No recursive functions
- Parsing logic is iterative with bounded loops (string length limited by argv)

### Double-Free Risk

- None. `amiport_free_argv()` called exactly once via atexit
- No manual free() calls anywhere
- No pointer reuse or dangling references

### amiport_getenv() Risk

- Not used in this port

## Verdict

**CLEAN** — Approved for shipping.

The port correctly follows the atexit cleanup pattern for argv expansion. All dynamic allocations are paired with cleanup. All exit paths (normal return, usage(), or error handling via errx()) trigger the cleanup function via atexit. Stack usage is conservative with no large local buffers.

## Notes

- Excellent error handling with errx() on validation failures
- Stack overflow risk is minimal — parsing loop iterates over input string, no buffer overflows
- Delay() is a single AmigaOS call, no complex timing logic
- getprogname() macro (from amiport/utsname.h) uses __progname correctly
- pledge() is a no-op stub, safe
