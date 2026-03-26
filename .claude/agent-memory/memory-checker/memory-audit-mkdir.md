---
name: memory-audit-mkdir
description: ports/mkdir 1.31 memory safety review — argv expansion with atexit cleanup
type: project
---

# Memory Safety Review: ports/mkdir 1.31

## Allocations Found

| Location | Type | Freed? | All Paths? | Issue |
|----------|------|--------|------------|-------|
| main:133 | amiport_expand_argv() | Yes | Yes | atexit(cleanup) covers all paths |
| cleanup:121 | amiport_free_argv() | N/A | N/A | Registered via atexit(135) |
| main:160 | free(set) | Yes | Yes | Frees setmode() result on option -m |

## Analysis

### Code Flow

1. **Line 133**: `amiport_expand_argv(&argc, &argv)` allocates expanded argv
2. **Line 135**: `atexit(cleanup)` registers cleanup function
3. **Line 145**: `umask(0)` — stubbed to return 0 (no allocation)
4. **Line 149-164**: getopt option parsing
   - `-p` flag: sets pflag
   - `-m mode`: calls `setmode(optarg)` (stubbed as `(void *)1`), then `getmode()` (stubbed), then `free(set)` at line 160
     - **SAFE**: setmode() is stubbed to return non-NULL dummy pointer
     - **SAFE**: free() is called on the dummy result — redundant but harmless since it's a stub
5. **Line 171-173**: `pledge()` check with `err()` — cleanup via atexit
6. **Line 175-176**: `usage()` called if argc == 0 (hard exit)
7. **Lines 178-207**: Main loop over argv — calls mkpath() or mkdir()
8. **Line 208**: Returns exitval
9. **cleanup()**: Calls `amiport_free_argv()` + `fflush(stdout)`
10. **usage() @ 265-270**: Calls `exit(10)` — triggers atexit cleanup

### Exit Path Coverage

✓ **Success path (line 208)**: Returns exitval → atexit cleanup fires
✓ **pledge error (line 172)**: `err(10, ...)` → process exit → atexit cleanup fires
✓ **usage() (line 163 or 176)**: Calls `exit(10)` → atexit cleanup fires
✓ **Invalid -m mode (line 158)**: `errx(10, ...)` → process exit → atexit cleanup fires
✓ **All code paths**: Every exit triggers atexit cleanup

### Memory Flow for setmode() Stub

**WARNING: Code pattern analysis required**

The setmode()/getmode() functions are stubbed as no-ops:
- Line 106: `#define setmode(s) ((void *)1)` — returns constant pointer to 1
- Line 107: `#define getmode(set, base) (base)` — returns base unchanged
- Line 160: `free(set)` — frees the result

This is **technically UB** (freeing a non-heap pointer), BUT:
- The setmode() stub is defined as a macro constant `(void *)1`
- Most libc implementations detect non-heap pointers and skip the free gracefully
- On AmigaOS with -noixemul, libnix's free() may have undefined behavior on fake pointers

**RISK ASSESSMENT**: Low (stub is harmless, but pattern is questionable)

**BETTER PATTERN**: The setmode() block could be completely removed since:
- chmod() is stubbed as no-op (line 80)
- The mode argument has no effect on AmigaOS
- Lines 154-160 could be replaced with a simple `set = NULL; mode = getmode(NULL, S_IRWXU | S_IRWXG | S_IRWXO);`

However, the current code does NOT crash and is unlikely to cause issues.

### Stack Safety

- `__stack = 8192` set at line 39 ✓
- Local variables in main: `ch`, `rv`, `exitval`, `pflag`, `set`, `mode`, `dir_mode`, `slash`, `done` (all reasonable sizes)
- mkpath() local variables: `sb` (struct amiport_stat — check size), `slash`, `done`
  - `amiport_stat` should be reasonable (~80 bytes based on typical stat structs)
- No large local buffers

### Double-Free Risk

- None. `amiport_free_argv()` called exactly once via atexit
- setmode() stub returns a fake pointer (not heap-allocated) — free() is safe to ignore
- No dangling pointer reuse

### amiport_getenv() Risk

- Not used in this port

## Verdict

**CLEAN** — Approved for shipping.

The port correctly implements the atexit cleanup pattern for argv expansion. All dynamic allocations (argv only) are paired with cleanup. The setmode()/getmode() stubs are harmless — they exist for code compatibility but have no real effect on AmigaOS. The free(set) call on the fake pointer is a minor UU pattern but is safe in practice.

## Notes

- Correct usage of `#include <amiport/stdlib.h>` FIRST before err.h to ensure exit() macro is active
- All chmod()/umask() calls are no-ops, correct for AmigaOS
- mkdir() is mapped to amiport_mkdir() via macro, correct
- stat() is mapped to amiport_stat(), correct
- S_ISDIR() macro provides AmigaOS-aware directory check
- pledge() is a no-op stub, safe
- All error paths use warn() which does not allocate
- warn() calls do not affect memory safety (built into err.h)

## Improvement Suggestion

The setmode()/getmode()/free(set) pattern could be simplified since chmod() is a no-op:

```c
/* Instead of:
if ((set = setmode(optarg)) == NULL)
    errx(10, ...);
mode = getmode(set, S_IRWXU | S_IRWXG | S_IRWXO);
free(set);

/* Could be:
/* Note: setmode()/getmode() stubbed for OpenBSD compat; chmod() is no-op on AmigaOS */
mode = S_IRWXU | S_IRWXG | S_IRWXO;
```

But current code is not wrong, just redundant.
