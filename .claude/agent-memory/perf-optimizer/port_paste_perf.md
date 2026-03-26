---
name: port_paste_perf
description: Performance findings for ports/paste 1.27 — modulo in parallel inner loop, putchar per delimiter, overall I/O-bound reviewed 2026-03-26
type: project
---

# Performance Findings: ports/paste 1.27

**Reviewed:** 2026-03-26

## Hot Paths

1. **parallel() inner loop (paste.c:182-223)** — Outer loop runs until all files exhausted; inner SIMPLEQ_FOREACH iterates all open files per output row. Each iteration may call putchar() for a delimiter and fputs() for the line.
2. **sequential() while loop (paste.c:247-263)** — getline() per line, putchar() per delimiter, fputs() for line output.

## Key Findings

### MEDIUM: `% delimcnt` modulo inside inner parallel loop (paste.c:187, 202, 217)

```c
if ((ch = delim[(lp->cnt - 1) % delimcnt]))
    putchar(ch);
```

Three separate modulo operations per loop iteration in the parallel inner loop. On 68000, DIVU is 76-140 cycles. In the common single-delimiter case (`delimcnt == 1`), the modulo is always 0 -- a predictable no-op. For multi-delimiter inputs the cost is real but inputs with many files are rare in practice.

**Fix:** Hoist the delimiter index as a counter variable that wraps, or special-case `delimcnt == 1`:
```c
/* if delimcnt == 1, skip the modulo -- delim[0] is always it */
int di = (lp->cnt - 1) % delimcnt;
```
Or use a modular counter that increments and wraps, avoiding the division entirely.
Estimated: LOW-MEDIUM impact on typical 1-delimiter invocations.

### LOW: putchar() per delimiter (paste.c:188, 203, 218, 251)

One `putchar()` call per column delimiter. For files with N columns there are N-1 putchar calls per output row. For wide inputs (many files) this adds up, but the getline() cost dominates.

**Fix:** Collect delimiters into a small buffer and emit with fwrite. Not worth it unless column count is very large (>20).

### NO ISSUES: getline() + fputs() I/O pattern

paste uses `getline()` for input and `fputs(line, stdout)` for output. Both are correctly line-buffered. No fgetc/getchar per-byte loops found.

### NO ISSUES: Stack size

`__stack = 8192` is adequate. No large local buffers found. getline() allocates dynamically on heap.

## Summary

- **Primary bottleneck:** I/O-bound (disk reads for multiple input files, stdout writes).
- **Estimated overall impact:** Marginal — the modulo fix is only meaningful for multi-delimiter invocations with many columns.
- **No critical performance issues found.**
