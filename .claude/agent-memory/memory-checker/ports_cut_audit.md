---
name: ports/cut Memory Safety Audit
description: Complete memory safety review of ports/cut — allocations, frees, file handles, exit paths
type: project
---

# Memory Safety Audit: ports/cut

**Date:** 2026-03-21
**Port:** cut (OpenBSD 1.28)
**File:** `/Users/duncan/Developer/amiport/ports/cut/ported/cut.c`
**Status:** PASS — No critical issues found

## Executive Summary

The ports/cut implementation is **memory-safe and well-documented**. All allocations are properly freed on all exit paths, realloc calls use safe intermediate pointers, and file handles are correctly balanced. The developer made explicit design decisions to account for AmigaOS's lack of automatic process memory cleanup, lifting the `cut_line` buffer from a static local to a global so it can be freed in `main()` before exit.

**Verdict:** CLEAN — Ready for production

---

## Allocations Found

### 1. Global Buffer: `cut_line` (lines 152-153)

| Location | Type | Freed? | All paths? | Issue |
|----------|------|--------|------------|-------|
| lines 152-153 | Global `char *cut_line` + `size_t cut_linesz` | Yes | Yes | **SAFE** — Intentionally global for cleanup |

**Details:**
- Allocated dynamically in `amiport_getline()` at lines 108, 126
- Explicitly freed in `main()` at line 260 before every exit path
- Design rationale (documented at lines 149-151, 255-258): AmigaOS has no automatic memory reclaim, so the buffer is lifted to global scope so `main()` can clean it up before exit
- Check: Line 259 verifies `cut_line != NULL` before freeing
- Exit path coverage: Line 263 `exit(rval)` occurs AFTER the free (line 260)

**No leak risk.**

### 2. realloc in amiport_getline() — Line 108

| Location | Type | Freed? | All paths? | Issue |
|----------|------|--------|------------|-------|
| line 108 | `realloc(buf, alloc)` | N/A | N/A | **SAFE** — Intermediate pointer used at line 126 |

**Details:**
```c
// Line 104-112: Initial allocation path
buf = *lineptr;
alloc = *n;
if (buf == NULL || alloc == 0) {
    alloc = 128;
    buf = (char *)realloc(buf, alloc);  // Line 108
    if (buf == NULL)
        return -1;  // Caller retains responsibility
    *lineptr = buf;
    *n = alloc;
}
```

**Analysis:**
- Line 108 is the initial allocation. If `buf` is `NULL`, the realloc is safe (equivalent to malloc).
- If realloc fails, the function returns `-1` without updating `*lineptr` or `*n` — the caller retains the original pointer they passed in (or NULL).
- The caller (`getline()` is a standard library function) is responsible for cleanup on error.
- **However, this is the internal implementation of `getline()` — the real issue is the CALLER's responsibility.**

**No unsafe pattern here.**

### 3. realloc in amiport_getline() — Line 126

| Location | Type | Freed? | All paths? | Issue |
|----------|------|--------|------------|-------|
| line 126 | `realloc(buf, alloc)` | N/A | Yes | **SAFE** — Intermediate pointer `newbuf` used |

**Details:**
```c
// Lines 123-135: Growth path
if ((size_t)(len + 1) >= alloc) {
    char *newbuf;
    alloc = alloc * 2;
    newbuf = (char *)realloc(buf, alloc);  // Line 126
    if (newbuf == NULL) {
        /* amiport: preserve old pointer so caller can free on failure */
        return -1;  // buf still valid, caller's responsibility
    }
    buf = newbuf;  // Line 131: only update buf if realloc succeeded
    *lineptr = buf;
    *n = alloc;
    p = buf + len;
}
```

**Analysis:**
- Line 126: `realloc()` result goes into intermediate `newbuf`, not directly into `buf`.
- Line 127: If realloc fails, returns `-1` **WITHOUT updating `buf`** — the old pointer is preserved for the caller.
- Line 131: Only updates `buf` if realloc succeeded.
- Comment at line 128-129: Correctly documents the pattern.

**This is the CORRECT realloc pattern. No leak risk.**

---

## File Handles

### fopen/fclose Pairs (lines 240-242)

| Location | Type | Closed? | All paths? | Issue |
|----------|------|---------|------------|-------|
| line 240 | `fopen(*argv, "r")` | Yes (line 242) | Yes | **SAFE** — Paired fclose |

**Details:**
```c
// Lines 235-247
if (*argv)
    for (; *argv; ++argv) {
        if (strcmp(*argv, "-") == 0)
            fcn(stdin, "stdin");
        else {
            if ((fp = fopen(*argv, "r"))) {  // Line 240
                fcn(fp, *argv);
                (void)fclose(fp);             // Line 242
            } else {
                rval = 10;
                warn("%s", *argv);
            }
        }
    }
else {
    // ... stdin path (line 253) — no open
}
```

**Analysis:**
- Line 240: `fopen()` only enters the block if it succeeds (conditional open).
- Line 242: `fclose()` always executes immediately after `fcn(fp)` — guaranteed closure on all paths through that block.
- Line 237-238: stdin path uses `stdin` directly, no fopen.
- Line 249-253: stdin path (else clause) uses `stdin` directly, no fopen.
- **No file handle leak. All opened files are closed.**

---

## Exit Paths

### main() exit handlers (lines 235-263)

| Path | Allocation State | Cleanup? | Exit | Issue |
|------|------------------|----------|------|-------|
| Normal (via stdin at line 253) | `cut_line` allocated in getline | **Yes** — freed at line 260 | exit(0) at line 263 | SAFE |
| Error opening file (line 244) | `cut_line` allocated in getline | **Yes** — freed at line 260 | exit(10) at line 263 | SAFE |
| Early exit via usage() (line 214, 222) | `cut_line` NOT allocated yet | N/A — usage() calls exit(10) immediately | exit(10) in usage() | SAFE — no allocations yet |
| Early exit via err() (line 179, 251) | `cut_line` NOT allocated yet | N/A — err() calls exit(10) immediately | exit(10) in err() | SAFE — no allocations yet |
| Early exit via errx() (line 282, 328) | `cut_line` NOT allocated yet | N/A — errx() calls exit(10) immediately | exit(10) in errx() | SAFE — no allocations yet |

**Analysis of main() cleanup (lines 255-263):**
```c
if (cut_line != NULL) {
    free(cut_line);
    cut_line = NULL;
}
exit(rval);
```

- Line 259: Null-check before free (safe even if never allocated).
- Line 260: Free call.
- Line 261: Nullify pointer (good practice, prevents double-free on imaginary exit).
- Line 263: `exit(rval)` is the ONLY exit from main() after the free — guaranteed execution.

**All exit paths after allocation reach this cleanup code. No leak risk.**

---

## Use-After-Free Analysis

### cut_line buffer usage

**Allocation points:**
- Lines 108, 126: `amiport_getline()` allocates/reallocates
- Lines 111-112, 132-134: Updates `*lineptr` and `*n` via pointers

**Usage points:**
- Line 383: `getline(&cut_line, &cut_linesz, fp)` — reads into buffer
- Line 384-385: `cut_line[linelen - 1]` — reads character within valid bounds
- Line 387-398: Pointer arithmetic and character reads within valid bounds

**Free point:**
- Line 260: `free(cut_line)` in main()

**Risk check:**
- ✓ No access to `cut_line` after line 260 free
- ✓ No recursive calls to getline() after free
- ✓ All function calls (fcn, fputs, puts, putchar) complete BEFORE free
- ✓ No global pointers aliasing `cut_line` after free

**No use-after-free risk.**

---

## Double-Free Risk Analysis

### Allocation responsibility

- `cut_line` is a **global**, allocated by `amiport_getline()` (internal function)
- No circular allocations
- No multiple cleanup paths for the same `cut_line`
- Free happens exactly once at line 260

**Risk check:**
- ✓ Only one `free()` call on `cut_line` in the entire program
- ✓ No stack-allocated memory being freed
- ✓ No static memory being freed
- ✓ No double-free via cleanup handlers or signal handlers

**No double-free risk.**

---

## Buffer Overflow Analysis

### Static buffers

1. **dchar[5]** (line 146):
   ```c
   char dchar[5];
   ```
   - Filled at lines 199-200 via memcpy
   - Line 198: Length check: `dlen >= (int)sizeof(dchar)` → usage()
   - Safe: length checked before copy

2. **positions[_POSIX2_LINE_MAX + 1]** (line 268):
   ```c
   char positions[_POSIX2_LINE_MAX + 1];
   ```
   - Filled at lines 332, 344 via memset
   - Line 329-330: Bounds check on `maxval`
   - Line 336-337: Auto-stop logic clamps bounds
   - Safe: bounds-checked

3. **cut_line (dynamic)**:
   - Allocated and grown by `amiport_getline()`
   - Line 123: Doubles allocation size when full: `alloc = alloc * 2`
   - Line 141: Null-terminated: `*p = '\0'`
   - Safe: dynamically sized and terminated

**No buffer overflow risk.**

---

## Summary Table

| Category | Count | Status | Risk |
|----------|-------|--------|------|
| Total allocations | 2 (realloc at 108, 126) | Proper | None |
| File handles opened | 1 (fopen at 240) | Proper | None |
| File handles closed | 1 (fclose at 242) | Balanced | None |
| Exit paths | 5 (lines 179, 214, 222, 251, 263) | Cleanup on all | None |
| Realloc calls | 2 | Safe pattern | None |
| Double-free risks | 0 | N/A | None |
| Use-after-free risks | 0 | N/A | None |
| Buffer overflows | 0 | N/A | None |
| Global buffers freed before exit | 1 (cut_line) | Yes | None |

---

## Verdict

### Status: ✅ CLEAN

**No critical issues, no warnings, no deferred maintenance.**

### Strengths

1. **Correct realloc pattern** — Intermediate pointer `newbuf` at line 126 prevents pointer loss on failure.
2. **Global buffer design** — Intentional decision to lift `cut_line` from static local to global so main() can free it. Documented with clear comments.
3. **Null-check before free** — Line 259 guards against double-free (though not possible here, it's defensive).
4. **Exit path coverage** — All paths through main() that allocate also reach the cleanup code before exit.
5. **File handle hygiene** — fopen/fclose properly balanced in a conditional block.
6. **Bounds checking** — All static buffers (dchar, positions) are bounds-checked before writes.

### No Issues Found

- ✓ No memory leaks
- ✓ No double-frees
- ✓ No use-after-free
- ✓ No unsafe realloc patterns
- ✓ No file handle leaks
- ✓ No buffer overflows
- ✓ No uninitialized variables

---

## Notes for Future Ports

This port demonstrates best practices for AmigaOS memory safety:

1. **Lift allocations to main scope** when they need cleanup — globals are not evil if there's a documented reason.
2. **Always use intermediate pointers for realloc** — never lose the original pointer on failure.
3. **Explicit null-check before free** — good defensive pattern.
4. **Document non-obvious memory decisions** — lines 149-151 and 255-258 explain WHY the buffer is global.
5. **Exit code compliance** — all exit(1) replaced with exit(10) (RETURN_ERROR).

This port is production-ready from a memory safety perspective.
