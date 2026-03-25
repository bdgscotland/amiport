---
name: port_expand_perf
description: Performance findings for ports/expand 1.15 — getchar/putchar hotloop, line-buffered fgets rewrite, tab-expansion I/O batching reviewed 2026-03-25
type: project
---

# Performance Findings: ports/expand 1.15

**Reviewed:** 2026-03-25

## Hot Paths

1. **main() inner while loop (expand.c:113-161)** — `while ((c = getchar()) != EOF)`. Every byte of every input file passes through this loop. For a 4KB file that is ~4000 `getchar()` calls plus up to ~4000 `putchar()` calls for non-tab characters. This is the only hot path; argument parsing and `getstops()` run once at startup.

## Key Findings

### CRITICAL: getchar() per byte — entire program throughput bottleneck (expand.c:113)

Each `getchar()` call is a libnix JSR + stack frame + buffer-check + macro expansion overhead. On 7MHz 68000 that is approximately 40-60 cycles of overhead per character *before* any work is done. On a 40KB source file with ~41000 characters, that is 1.6-2.5 million cycles of call overhead just for input.

`putchar()` in the non-tab path (expand.c:152-154) has the same overhead: every non-tab byte emits one JSR call.

**Fix:** Replace the `getchar()`/`putchar()` character-at-a-time loop with a `fgets()`-per-line approach. Read a full line into a stack buffer, scan it with a pointer, and batch the output with `fwrite()` for non-tab runs and `putchar_span()` calls for spaces expanded from tabs.

Estimated impact on a typical 80-char-line source file: ~40x fewer function calls for the I/O path.

### HIGH: putchar() loop for tab-space expansion emits one call per space (expand.c:117-121, 124-128, 139-142)

Three separate `do { putchar(' '); column++; } while (...)` loops. For an 8-space tab stop, each tab expansion costs 8 `putchar()` JSRs. For a file with many leading tabs (e.g., deeply indented C source), a line with 4 tabs of 8 spaces = 32 individual `putchar()` calls just for the spaces.

**Fix:** Calculate how many spaces to emit, then call `fwrite(spaces_buf, 1, count, stdout)` with a pre-filled static spaces buffer. A `static const char spaces[256]` of all-spaces covers any realistic tab expansion count in a single call.

### MEDIUM: `column % tabstops[0]` modulo in single-stop tab loop (expand.c:127-128)

```c
while (((column - 1) % tabstops[0]) != (tabstops[0] - 1))
```

68000 DIVU instruction: 76-140 cycles. This executes for every space emitted when expanding a tab with a single custom stop. For a tab-to-8 conversion with column=0, that is 7 DIVU operations per tab, ~700 cycles each = ~4900 cycles per tab just for the modulo.

**Fix:** If `tabstops[0]` is a power of 2, replace with bitwise AND: `((column - 1) & (tabstops[0] - 1)) != (tabstops[0] - 1)`. Check at startup: `if ((tabstops[0] & (tabstops[0]-1)) == 0) use_shift = 1;`. Alternatively, pre-compute the next tab stop position before the loop and use `while (column < nextstop)`.

Better: replace the whole while loop with a count computation + fwrite, eliminating the per-iteration test entirely.

### LOW: `column & 07` bitmask is already optimal (expand.c:120)

The default 8-space tab stop expansion uses `column & 07` (bitwise AND with 7). This is correct and optimal — no division needed. No change required.

### LOW: `nstops == 0` / `nstops == 1` branch checks inside tight loop (expand.c:116, 123)

These are evaluated on every `\t` character, but `\t` is relatively rare in most text. The branch predictor on 68020+ handles these well. Not worth restructuring.

## Summary

- **Primary bottleneck:** `getchar()`/`putchar()` call overhead — every byte costs a JSR. This is CPU-bound on 68000, I/O-bound only on 68030+ with cache.
- **Estimated overall impact for CRITICAL+HIGH fixes:** Significant — for a typical 80-char source file, eliminates ~35-40 function calls per line, reducing cycle count by 60-70% on the output path.
- **Stack:** `__stack = 32768` is adequate. Adding a `static char linebuf[4096]` (static, not local) for the fgets approach uses no stack at all.
- **No -O0 override needed** — no struct-by-value returns > 8 bytes, `-O2` is correct.
- **All findings documented** — none applied yet (pending user review per feedback_apply_perf_findings.md).
