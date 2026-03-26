---
name: memory-audit-rmdir
description: ports/rmdir 1.15 memory safety review — argv expansion with atexit cleanup
type: project
---

# Memory Safety Review: ports/rmdir 1.15

## Allocations Found

| Location | Type | Freed? | All Paths? | Issue |
|----------|------|--------|------------|-------|
| main:83 | amiport_expand_argv() | Yes | Yes | atexit(cleanup) covers all paths |
| cleanup:72 | amiport_free_argv() | N/A | N/A | Registered via atexit(85) |

## Analysis

### Code Flow

1. **Line 83**: `amiport_expand_argv(&argc, &argv)` allocates expanded argv
2. **Line 85**: `atexit(cleanup)` registers cleanup function
3. **Lines 87-88**: `err()` on pledge failure — cleanup via atexit
4. **Lines 91-103**: getopt option parsing
5. **Lines 102-103**: `usage()` called if argc == 0 (hard exit)
6. **Lines 105-111**: Main loop over argv — no allocations, only system calls
7. **Line 112**: Returns errors value
8. **cleanup()**: Calls `amiport_free_argv()` + `fflush(stdout)`
9. **usage() @ 142-146**: Calls `exit(10)` — triggers atexit cleanup

### Exit Path Coverage

✓ **Success path (line 112)**: Returns errors → atexit cleanup fires
✓ **pledge error (line 88)**: `err(10, ...)` → process exit → atexit cleanup fires
✓ **usage() (line 97 or 103)**: Calls `exit(10)` → atexit cleanup fires
✓ **All code paths**: Every exit triggers atexit cleanup

### Stack Safety

- `__stack = 8192` set at line 60 ✓
- Local variables in main: `ch`, `errors`, `pflag` (all on stack)
- rm_path() local variables: `path` (parameter), `p` (pointer into path)
- No dynamic allocations in rm_path() except inherited argv

### Double-Free Risk

- None. `amiport_free_argv()` called exactly once via atexit
- No manual free() calls anywhere in code
- No dangling pointer usage

### amiport_getenv() Risk

- Not used in this port

## Verdict

**CLEAN** — Approved for shipping.

The port correctly implements the atexit cleanup pattern. All dynamic allocations (argv expansion) are paired with a cleanup function registered at the start of main. All code paths that exit (via return, usage() exit(), or err() termination) trigger the cleanup via atexit.

## Notes

- Correct usage of __progname (extern declaration, not re-defined) ✓
- Simple recursive path-stripping logic in rm_path() — no allocations
- warn() calls on error do not affect memory safety (no allocations)
- stderr flushing prevents buffered output loss on exit
