---
name: ports/col memory safety audit
description: Memory safety review of col 1.20 (2026-03-26)
type: reference
---

# Memory Safety Audit: col 1.20

**Status: CRITICAL LEAKS FOUND**
**Verdict: Cannot ship — 3 unfixable memory leaks, 1 fixable leak**

## Allocations Found

| Location | Type | Free'd? | All paths? | Issue |
|----------|------|---------|------------|-------|
| col.c:528 | xreallocarray(NULL, NALLOC, sizeof(LINE)) | Partial | No | **LEAK** — line_freelist allocated once, never freed on exit |
| col.c:305-307 | xreallocarray(l->l_line, need, 2*sizeof(CHAR)) | Yes | Yes | Safe — freed at 359 in flush_lines() |
| col.c:424-426 | xreallocarray(sorted, ...) | Partial | No | **LEAK** — static sorted pointer, allocated on first call to flush_line(), never freed on exit |
| col.c:429-430 | xreallocarray(count, ...) | Partial | No | **LEAK** — static count pointer, allocated on first call to flush_line(), never freed on exit |

## Detailed Analysis

### 1. alloc_line() Line Freelist Leak (line 528)

**Code (lines 521-538):**
```c
LINE *alloc_line(void)
{
    LINE *l;
    int i;

    if (!line_freelist) {
        l = xreallocarray(NULL, NALLOC, sizeof(LINE));  /* LINE 528 */
        line_freelist = l;
        for (i = 1; i < NALLOC; i++, l++)
            l->l_next = l + 1;
        l->l_next = NULL;
    }
    l = line_freelist;
    line_freelist = l->l_next;

    memset(l, 0, sizeof(LINE));
    return (l);
}
```

**Issue:**
- First call allocates `NALLOC` (64) LINE structs via xreallocarray()
- Pointer stored in static `line_freelist`
- Freelist is recycled for subsequent alloc_line() calls
- **But: the original allocation pointer is never freed** — only individual LINE nodes are recycled
- On program exit, the entire 64-line block allocated at line 528 leaks permanently

**Size:** 64 * sizeof(LINE) = 64 * ~40 bytes = ~2560 bytes per invocation

**Severity:** CRITICAL — persistent memory loss

### 2. sorted[] Buffer Leak (lines 414-425)

**Code (lines 413-425):**
```c
if (l->l_needs_sort) {
    static CHAR *sorted;
    static size_t count_size, i, sorted_size;
    static int *count, save, tot;

    if (l->l_lsize > sorted_size) {
        sorted_size = l->l_lsize;
        sorted = xreallocarray(sorted,
            sorted_size, sizeof(CHAR));  /* LINE 424 */
    }
```

**Issue:**
- `sorted` is a **static** pointer declared inside flush_line()
- On first call with l->l_needs_sort=1, sorted is allocated
- On subsequent calls, sorted is reallocated with xreallocarray()
- **xreallocarray pattern is: `p = xreallocarray(p, ...)` which loses the old pointer if realloc fails**
  - However, this is the *safe* pattern (using the same variable)
  - The real issue: **sorted is never freed on exit**
- Static variables persist across function calls but are not freed at program end

**Size:** Variable, up to 4KB (typical line with 1000 chars * 4 bytes per CHAR struct)

**Severity:** CRITICAL — persistent allocation, though typically small

### 3. count[] Buffer Leak (lines 416, 427-430)

**Code (lines 427-430):**
```c
if (l->l_max_col >= count_size) {
    count_size = l->l_max_col + 1;
    count = xreallocarray(count,
        count_size, sizeof(int));  /* LINE 429 */
}
```

**Issue:**
- `count` is static, same scope as sorted
- Never freed on exit
- Size: (l->l_max_col + 1) * sizeof(int) = variable, typically 128-512 bytes

**Severity:** CRITICAL — persistent allocation

### 4. l->l_line Reallocation (lines 301-308)

**Code:**
```c
if (l->l_line_len + 1 >= l->l_lsize) {
    size_t need;

    need = l->l_lsize ? l->l_lsize : 45;
    l->l_line = xreallocarray(l->l_line,
        need, 2 * sizeof(CHAR));  /* LINE 305 */
    l->l_lsize = need * 2;
}
```

**Freed at:**
```c
flush_lines(int nflush)
{
    LINE *l;

    while (--nflush >= 0) {
        l = lines;
        lines = l->l_next;
        if (l->l_line) {
            flush_blanks();
            flush_line(l);
        }
        if (l->l_line || l->l_next)
            nblank_lines++;
        if (l->l_line)
            (void)free((void *)l->l_line);  /* LINE 359 */
        free_line(l);
    }
```

**Status:** SAFE — l->l_line is freed in flush_lines() when the LINE is destroyed. However, this assumes all allocated LINEs are processed before exit.

**Critical gap:** What if the program exits early (via err/errx/exit) before all LINEs are flushed?

**Scenario:** If an error occurs at line 509 (addto_lineno overflow error), the main loop exits via errx(), skipping lines 323-341 (final flush). Allocated l->l_line buffers in the `lines` linked list are never freed.

**This is a LEAK on error paths.**

## Error Path Analysis

**Exit paths in main() (lines 120-341):**

1. Line 142: `err(10, "pledge")` — err() calls atexit cleanup then exits
2. Line 161-162: `errx(10, "bad -l argument")` — skips flush_lines() cleanup ⚠️ **LEAK**
3. Line 168: `usage()` → `exit(10)` — skips cleanup ⚠️ **LEAK**
4. Line 509: `errx(10, "too many lines")` — skips cleanup ⚠️ **LEAK**
5. Line 512: `errx(10, "too many reverse line feeds")` — skips cleanup ⚠️ **LEAK**
6. Line 341: Normal exit(0) — cleanup via atexit ✓

**atexit cleanup (lines 113-118):**
```c
static void cleanup(void)
{
    amiport_free_argv();
    (void)fflush(stdout);
}
```

**Critical issue:** cleanup() does NOT free line buffers or static sorted/count pointers!

## Fixability Analysis

### Leak 1: line_freelist (UNFIXABLE without code change)
- Static allocation created once at first alloc_line() call
- Original pointer lost after recycling
- Fix: Track original pointer and free in atexit cleanup
- Workaround: Accept ~2560 byte leak

### Leak 2 & 3: sorted/count (FIXABLE)
- Static pointers in flush_line()
- Can be freed in atexit cleanup if we export them or use a different pattern
- However: These are truly static to flush_line() scope — not directly accessible from main
- Workaround: Accept ~4KB leak

### Leak 4: l->l_line on error paths (FIXABLE)
- Linked list of LINE structures, each with allocated l->l_line buffer
- Must free entire linked list on error
- Fix: Add a cleanup function that walks the `lines` linked list and frees all l->l_line buffers
- This is addressable without major refactoring

## Summary

| Leak | Location | Type | Size | Path | Fixable? | Verdict |
|------|----------|------|------|------|----------|---------|
| 1 | alloc_line():528 | Line freelist | ~2560 bytes | All | DIFFICULT | Unfixable without refactoring |
| 2 | flush_line():424 | sorted buffer | ~4KB | All | DIFFICULT | Requires scope change |
| 3 | flush_line():429 | count buffer | ~512 bytes | All | DIFFICULT | Requires scope change |
| 4 | main():l->l_line | Character buffer | ~10-50KB | Error paths | YES | Add cleanup_lines() to atexit |

**Total leak: ~16-60 KB per invocation on AmigaOS (depending on input size)**

## Verdict

**CRITICAL** — Cannot ship without fixes. Four distinct allocation patterns leak memory.

### Required Fixes

1. **Add a cleanup_lines() function:**
```c
static void cleanup_lines(void)
{
    LINE *l, *next;
    l = lines;
    while (l != NULL) {
        next = l->l_next;
        if (l->l_line)
            free(l->l_line);
        free_line(l);
        l = next;
    }
    lines = NULL;
}
```

2. **Call from atexit cleanup:**
```c
static void cleanup(void)
{
    cleanup_lines();
    amiport_free_argv();
    (void)fflush(stdout);
}
```

3. **Address line_freelist leak:**
   - Option A: Track original pointer and free in atexit
   - Option B: Accept the ~2560 byte leak (acceptable for a single invocation)

4. **Address sorted/count leaks:**
   - Option A: Move static pointers to file scope and export them for cleanup
   - Option B: Accept the ~4KB leak

### Minimal Fix (Recommended)

Implement cleanup_lines() and add it to atexit cleanup. This addresses leak 4 (~10-50KB) which is the largest. Leaks 1-3 total ~7KB and are acceptable for a single-invocation tool.

---

**NOTE:** This audit required pointer ownership analysis. The sorted/count allocations are truly hidden inside flush_line() and cannot be freed without refactoring the function scope or using global pointers. The line_freelist is similarly trapped by the recycling pattern.

