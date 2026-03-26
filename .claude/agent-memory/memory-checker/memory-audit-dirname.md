---
name: memory-audit-dirname
description: ports/dirname 1.17 memory safety review (2026-03-25)
type: project
---

# Memory Audit Report: dirname 1.17

## Status: CLEAN

All dynamic allocations properly freed. Zero memory leaks. No double-free risks. Approved for shipping.

## Audit Summary

| Metric | Result |
|--------|--------|
| Total allocations | 1 (argv expansion) |
| Properly freed | 1 |
| Leaks found | 0 |
| Unsafe realloc patterns | 0 |
| Double-free risks | 0 |
| Critical issues | 0 |
| Verdict | **APPROVED** |

## Key Findings

### 1. argv Expansion (Lines 130-132)
- **Allocation:** `amiport_expand_argv(&argc, &argv)` from `<amiport/glob.h>`
- **Freed:** YES — via `atexit(cleanup)` at line 132
- **Cleanup function:** Lines 116-121, calls `amiport_free_argv()` then `fflush(stdout)`
- **Ownership:** EXCLUSIVE — no shared pointers
- **All paths covered:** YES — atexit catches all exits including `err()`, `errx()`, `usage()`, and normal return

### 2. Static Buffer (Line 58)
- **Buffer:** `static char dirname_buf[1024]` — custom POSIX-correct dirname() implementation
- **Type:** Static allocation in DATA segment — no free() needed
- **Lifetime:** Program lifetime — cleaned up automatically on process exit
- **Usage:** SAFE — result consumed immediately by `puts(dir)` before argv[0] reused

### 3. Exit Path Coverage
- Line 136: `err(10, "pledge")` — err() calls exit() → cleanup
- Line 155: `err(10, "%s", argv[0])` — err() calls exit() → cleanup
- Line 157: `return 0` (main) — cleanup runs before process exit
- Line 165: `exit(10)` (usage) — cleanup runs before process exit

ALL paths covered by atexit cleanup. No leaks possible.

### 4. Memory Safety Patterns (All Correct)

✓ No `realloc()` without intermediate pointer
✓ No `malloc()` without matching `free()`
✓ No double-free risks from array growth
✓ No uninitialized array entries in cleanup
✓ No `amiport_getenv()` results left unfreed
✓ No file handle leaks
✓ No amiport_open() + fdopen() namespace mismatch
✓ __progname weak symbol correctly handled (defined externally, not redefined locally)

### 5. No Unsafe Patterns from Known Pitfalls

✓ No directory path corruption from dirname() — result consumed before argv[0] reused
✓ No stack overflow risk — stack cookie 16KB, minimal usage
✓ No alignment issues — no custom allocators
✓ No struct-by-value > 8 bytes
✓ No vsnprintf(NULL, 0) pattern
✓ No amiport_open() + fdopen() mismatch
✓ No fclose(stdin)
✓ No obsolete() string leaks (program has no obsolete() function)

## Verdict: **SHIPPING APPROVED**

Memory safety is verified. Zero dynamic allocation leaks. All cleanup paths correct.

**Port:** dirname 1.17 (OpenBSD)
**Date:** 2026-03-25
**Reviewer:** memory-checker agent
