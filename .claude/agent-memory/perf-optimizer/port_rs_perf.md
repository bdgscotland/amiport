---
name: port_rs_perf
description: Performance findings for ports/rs 1.30 — putchar per padding space in prints(), strlen in mbsavis per token, modulo in prepfile reviewed 2026-03-26
type: project
---

# Performance Findings: ports/rs 1.30

**Reviewed:** 2026-03-26

## Hot Paths

1. **putfile() -> prints() (rs.c:240-276)** — Outputs every cell of the reshaped array. For each cell: padding spaces emitted via putchar() loop, then fputs() for the cell string.
2. **getfile() loop (rs.c:171-237)** — Reads and tokenises all input. mbsavis() called once per token to get display width.
3. **prepfile() (rs.c:289-359)** — One-time setup: computes column widths, does modulo arithmetic.

## Key Findings

### HIGH: putchar(osep) per padding space in prints() (rs.c:270-275)

```c
n = (flags & ONEOSEPONLY ? 1 : colwidths[col] - ep->w);
if (flags & RIGHTADJUST)
    while (n-- > 0)
        putchar(osep);
fputs(ep->s, stdout);
while (n-- > 0)
    putchar(osep);
```

For a default 80-column output with 2-char gutter and 6-char content, each cell emits ~2 padding spaces via putchar(). For a 100-element array reshaped to 10 columns x 10 rows = 100 calls to prints(), each emitting 2 putchar() calls = 200 putchar() overhead calls just for padding. On wider gutters (gutter=4, maxwidth=4) the waste is higher: 4 putchar() calls per cell.

**Fix:** Use fwrite() with a pre-filled static spaces/separator buffer:
```c
static const char osep_buf[256]; /* filled with osep at startup */
/* ... */
if (n > 0)
    fwrite(osep_buf, 1, (size_t)n, stdout);
```
Fill osep_buf at startup when osep is known. For osep=' ' (default) a static all-spaces array suffices.

Est: 3-5x fewer function calls for the padding path on typical output. For large arrays with wide gutters, eliminates hundreds of putchar() JSRs.

### MEDIUM: strlen() called per token in mbsavis() stub (rs.c:83)

```c
static int
mbsavis(char **dstpp, const char *src)
{
    *dstpp = (char *)src;
    return (int)strlen(src);
}
```

`mbsavis()` is called once per element during `getfile()`. Since it's a stub that calls `strlen()`, every token parsed from input costs one strlen() traversal. For N tokens this is O(N * avg_token_len) byte touches. This is MEDIUM rather than HIGH because `strsep()` already walked the string to find the separator -- the length is implicitly available as `(p_after - p_before)` from the parsing loop.

**Fix:** Since `strsep()` advances `p`, the token length is known: `ep->w = p - token_start - 1` (where token_start was saved before strsep). This eliminates the strlen entirely for the common tokenization path. The ONEPERLINE path still needs strlen(curline) but that's one call per line, not per token.

**Constraint:** The fix requires refactoring getfile() to save the pre-strsep pointer, which adds local variable complexity. The saving is real but moderate.

### MEDIUM: `nelem % ocols` and `nelem % orows` modulo in prepfile() (rs.c:317, 320, 322)

```c
orows = nelem / ocols + (nelem % ocols ? 1 : 0);
ocols = nelem / orows + (nelem % orows ? 1 : 0);
```

Two division + two modulo operations in prepfile(). On 68000, DIVS is 158 cycles worst case. But prepfile() runs once, so this is a one-time cost. Not worth changing.

**Verdict:** LOW -- one-time startup cost, not a loop.

### NO ISSUES: get_line() uses getline() (rs.c:372)

Input reading uses `amiport_getline()` via the macro -- reads whole lines, not character-at-a-time. Correct approach.

### NO ISSUES: Stack size

`__stack = 16384` (16KB) is adequate. No large local arrays. `colwidths` is heap-allocated in prepfile(). curline is heap-allocated by getline(). The `struct entry` array is heap-allocated via getptrs().

## Summary

- **Primary bottleneck:** Output path -- putchar() per padding character in prints(). For large arrays with gutter space, this dominates.
- **Estimated overall impact:** Noticeable for the prints() fwrite fix on large reshaped arrays. The mbsavis strlen fix is meaningful for large inputs with many tokens.
- **No stack safety issues found.**
