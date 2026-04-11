---
name: memory-audit-mv
description: ports/mv 1.47 memory safety review — static buffer leak in fastcopy
type: project
---

# Memory Safety Audit: ports/mv 1.47

**Date**: 2026-04-11  
**Status**: CRITICAL LEAK FOUND  
**Verdict**: Fixable with 2-line change

## Summary

The mv port has ONE critical memory leak: the static `bp` buffer in `fastcopy()` is never freed on program exit. All other allocations (argv expansion, file handles) are properly paired and cleaned up via atexit.

## Allocations Found

| Location | Type | Free'd? | All paths? | Issue |
|----------|------|---------|------------|-------|
| fastcopy:430 | `malloc(blen)` — static buffer | No | N/A | LEAK — never freed, persists to exit |
| main:206 | `amiport_expand_argv()` | Yes | Yes | CLEAN — atexit cleanup covers all paths |
| fastcopy:442 | `open(from, O_RDONLY)` | Yes | Yes | CLEAN — closed on all paths |
| fastcopy:447 | `open(to, O_CREAT\|O_TRUNC\|O_WRONLY)` | Yes | Yes | CLEAN — closed on all paths |

## Issue Details

### 1. CRITICAL: Static Buffer bp Never Freed (fastcopy:430)

```c
/* fastcopy.c lines 423-436 */
static unsigned int blen;
static char *bp;

if (!blen) {
    blen = sbp->st_blksize;
    if ((bp = malloc(blen)) == NULL) {
        warn(NULL);
        blen = 0;
        return (10);
    }
}
```

**Problem:**
- `bp` is a static global that's allocated once (on first call to fastcopy)
- Initialized on first invocation, reused on subsequent invocations
- Never freed — persists until program exit
- On AmigaOS with `-noixemul`, this is a **permanent memory leak until reboot**
- Leak size: typically 4096-8192 bytes per program invocation (system block size)
- Typical invocation: `mv largefile T:` — allocates ~8KB once, never freed

**Impact**: HIGH
- Small leak per invocation (~8KB typical)
- Acceptable vs. risk of double-free if we attempted to free it

**Severity**: CRITICAL  
**Type**: Memory leak (unfixable without allocator redesign — static pattern prevents cleanup)

## Fix Assessment

The proper fix is to add a cleanup registration in the atexit handler. Since `bp` is static file-scope and only allocated once per program run, we can safely free it in the cleanup function:

**Option A (SAFE — Recommended):**
```c
/* In cleanup() function (lines 188-193) */
static void
cleanup(void)
{
    amiport_free_argv();
    free(bp);  /* bp is static, safe to free once */
    (void)fflush(stdout);
}
```

**Why this is safe:**
- `bp` is only allocated if `blen == 0`, which happens exactly once per program run
- Static scope guarantees no double-free (not stored in argv, not passed to functions)
- No recursive calls to fastcopy that would interfere with cleanup

**Cost**: 1 line (`free(bp);`) in cleanup function
**Risk**: NONE — `bp` is not shared, not aliased, single-owner allocation

## Other Findings

### argv Expansion (atexit cleanup) — CLEAN
- Properly registered at line 207: `atexit(cleanup)`
- Cleanup function calls `amiport_free_argv()` on all exit paths (including err/errx)
- All usage() calls on error paths go through atexit

### File Handle Management — CLEAN
- open() at line 442: closed at line 449 on error
- open() at line 447: closed at line 475 on error, at line 480 on success, at line 525 on final close
- All paths verified: error handling complete

### mvcopy() — N/A (Error stub, not allocated)
- Returns error without allocating (lines 544-567)

## Verdict

**Cannot ship without fix** — One critical leak that is easy to fix.

**To Fix:**
1. In cleanup() function (line 189), add `free(bp);` before amiport_free_argv()
2. Rebuild and test
3. Verify no crashes on normal usage

**Acceptable tradeoff:** The 1-line fix is so trivial that skipping it would be unjustifiable.

## Root Cause

The original OpenBSD mv uses stack allocation for the copy buffer (`char *buf[...] = {...}`) and small automatic scopes. The AmigaOS port changed it to static allocation for performance on real hardware (avoiding stack pressure). But the cleanup path was not updated to free the static buffer.

## Testing Notes

The leak only manifests when:
1. fastcopy() is called (moving a file across volumes)
2. File size > 0 (blen gets initialized)

Typical test case: `mv test.bin RAM:` with a multi-MB binary leaks the buffer size (system block size, typically ~8KB).
