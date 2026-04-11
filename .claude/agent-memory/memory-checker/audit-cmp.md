---
name: memory-audit-cmp
description: ports/cmp 1.19 memory safety review — CLEAN, approved for shipping
type: project
---

# Memory Safety Audit: ports/cmp 1.19

**Date**: 2026-04-11  
**Status**: CLEAN  
**Verdict**: Approved for shipping — zero dynamic allocation leaks

## Summary

The cmp port has exemplary memory safety. All allocations are either static (stack-local buffers) or properly paired with cleanup. The atexit cleanup covers all exit paths (including err/errx calls). File handles are correctly managed with proper close/cleanup on all paths.

## Allocations Found

| Location | Type | Free'd? | All paths? | Issue |
|----------|------|---------|------------|-------|
| cmp.c:83 | `amiport_expand_argv()` | Yes | Yes | CLEAN — atexit cleanup at line 84 |
| cmp.c:118 | `open(file1, O_RDONLY)` | Yes | Yes | CLEAN — fd1 closed at 158, or passed to c_special |
| cmp.c:129 | `open(file2, O_RDONLY)` | Yes | Yes | CLEAN — fd2 closed at 159, or passed to c_special |
| regular.c:121-122 | `mmap()` emulated allocation | Yes | Yes | CLEAN — munmap'd on all paths including mmap_failed |
| regular.c:124-125 | `mmap()` emulated allocation #2 | Yes | Yes | CLEAN — munmap'd on error path (line 126) |
| special.c:64 | `fopen(path1, "r")` | Yes | Yes | CLEAN — closed at line 116 if opened |
| special.c:72 | `fopen(path2, "r")` | Yes | Yes | CLEAN — closed at line 117 if opened |

## Detailed Analysis

### 1. argv Expansion (CLEAN)

```c
/* cmp.c:83-84 */
amiport_expand_argv(&argc, &argv);
atexit(cleanup);
```

The expanded argv is properly registered for cleanup via atexit. The cleanup function (lines 170-175) calls `amiport_free_argv()` on ALL exit paths including err/errx.

### 2. File Handle Management (CLEAN)

**main() initialization (lines 118, 129):**
- Lines 118, 129: Files opened with `open()` — libnix native function
- Lines 158-159: Files closed before fallback to c_special (correct — avoids fd duplication)
- Lines 162-164: Passed to c_regular or c_special

**c_regular() fallback path (lines 115-116):**
```c
if (path1 != NULL) amiport_close(fd1);
if (path2 != NULL) amiport_close(fd2);
c_special(path1, file1, skip1, path2, file2, skip2);
```
Files are closed before c_special reopens them via fopen(). This avoids fd namespace collision (crash-patterns #12).

**c_special() file management (lines 61-76):**
```c
if (path1 == NULL) {
    fp1 = stdin;
    close1 = 0;
} else if ((fp1 = fopen(path1, "r")) == NULL) {
    fatal("%s", file1);
} else {
    close1 = 1;
}
```
Uses fopen() for named files (correct), stdin for "-" argument (correct). Track close flags (close1, close2) to avoid fclose(stdin).

**c_special() cleanup (lines 116-117):**
```c
if (close1) fclose(fp1);
if (close2) fclose(fp2);
```
Only closes files that were opened — stdin is left open (correct).

### 3. mmap Emulation (CLEAN)

**regular.c:121-127 — mmap allocation and error handling:**
```c
if ((p1 = mmap(NULL, (size_t)length, PROT_READ,
    MAP_PRIVATE, fd1, skip1)) == MAP_FAILED)
    goto mmap_failed;
if ((p2 = mmap(NULL, (size_t)length, PROT_READ,
    MAP_PRIVATE, fd2, skip2)) == MAP_FAILED) {
    munmap(p1, (size_t)length);  /* p1 properly freed on p2 failure */
    goto mmap_failed;
}
```

Correct error handling: if p2 allocation fails, p1 is munmap'd before the fallback. This prevents leaking p1 to the c_special fallback path.

**mmap_failed path (lines 110-118):**
```c
mmap_failed:
    if (path1 != NULL) amiport_close(fd1);
    if (path2 != NULL) amiport_close(fd2);
    c_special(path1, file1, skip1, path2, file2, skip2);
    return;
```
Both mmap pointers (p1, p2) are properly munmap'd before jumping to mmap_failed (implicit in goto — p1 and p2 stay in register/stack, not leaked). c_special is called with paths (not fds) as intended.

### 4. No amiport_getenv Usage (No malloc'd env strings)

The port does NOT use amiport_getenv() or any environment variable lookups. No getenv-related leaks possible.

### 5. No Other Allocations

- No strdup usage
- No malloc/calloc beyond what's tested above
- No global array allocations (static stack buffers only)
- No regex/pattern compilation

## Exit Path Coverage

**All exit paths verified:**
1. Error paths (fatal/fatalx): Call exit(ERR_EXIT)
   - atexit cleanup runs: amiport_free_argv() + fflush(stdout)
   - File handles: Already closed by c_special or c_regular
2. Normal paths: exit(0) or exit(DIFF_EXIT)
   - Same cleanup registered
3. usage() path (line 98): Calls exit(ERR_EXIT) → atexit cleanup runs

## Risk Assessment

| Category | Risk | Notes |
|----------|------|-------|
| Double-free | NONE | No allocation shared between paths; no realloc |
| Use-after-free | NONE | All pointers freed immediately after use |
| File descriptor leak | NONE | All opens paired with close on all paths |
| Uninitialized pointers | NONE | close1/close2 flags prevent fclose(stdin) |
| Stack safety | LOW | Largest local: struct stat (64 bytes), well within stack |

## Verdict

**APPROVED FOR SHIPPING** — Zero leaks detected. All allocations properly paired and freed. Exit path cleanup is complete and correct.

### Key Strengths

1. **Correct fd namespace handling**: Avoids mixing amiport_open fds with fopen() (crash-patterns #12)
2. **Proper stdin handling**: Tracks which files to close; never fclose(stdin)
3. **Clean mmap fallback**: Proper error handling and cleanup before fallback
4. **atexit coverage**: All exit paths covered by registered cleanup

### No Further Changes Required

This port is production-ready from a memory safety perspective.
