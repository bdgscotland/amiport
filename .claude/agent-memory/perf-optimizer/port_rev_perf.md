---
name: port_rev_perf
description: Performance findings for ports/rev 1.16 — putchar-per-byte hotloop, multibyte dead branch, fwrite batch fix applied 2026-03-25
type: project
---

# Performance Findings: ports/rev 1.16

**Reviewed:** 2026-03-25

## Hot Paths

1. **rev_file() inner loop** — the entire program's work. For each line: reverse N bytes character by character, writing each with `putchar()`. For an 80-char line that was 81 `putchar()` calls (80 chars + newline). This is the only hot path; everything else (getline, fopen, fclose) is I/O-bound and not the bottleneck.

## Key Findings — Applied

### CRITICAL: putchar() per byte in hot loop (rev.c:160-167)

The original code called `putchar(*u)` for every byte of every reversed line, plus one more for the trailing newline. Each `putchar()` is a libnix JSR with stack frame setup and an EOF test (compare + conditional branch). On 7MHz 68000 that is ~50 cycles of overhead per call, before the actual I/O.

For an 80-char line: 81 `putchar()` calls = ~4050 cycles of overhead alone, per line.

**Fix applied:** Reverse the line in-place with a two-pointer swap (lo/hi walking toward the middle), then emit the whole reversed line with one `fwrite(p, 1, len, stdout)` + one `putchar('\n')`. This replaces N JSR-per-byte with a single `fwrite()` call that hands a contiguous buffer directly to the stdio layer.

Est. speedup on 80-char lines: ~40x fewer function calls for the output path. On real throughput terms, the bottleneck shifts to `amiport_getline()` (reading) and the swap loop — both are simple and fast.

### HIGH: multibyte global checked inside inner loop with dead branch on AmigaOS (rev.c:157)

`multibyte` is a global `int`. The original structure checked it inside the per-character iteration:

```c
for (t = p + len - 1; t >= p; --t) {
    te = t;
    if (multibyte)          /* global read + branch every char */
        ...
    for (u = t; u <= te; ++u)
        putchar(*u);        /* inner loop for multibyte grouping */
}
```

On AmigaOS `multibyte = 0` always (guarded by `#ifndef __AMIGA__`), but the compiler cannot elide a global read. The outer loop had two nested loops plus dead-branch overhead per character.

**Fix applied:** Moved the entire multibyte code path inside `#ifndef __AMIGA__`. On AmigaOS the preprocessor removes it entirely -- the compiler sees only the in-place swap + fwrite path, no branches, no globals in the hot loop.

### MEDIUM: Excess locals hurt register allocation (rev.c:128)

Original: `char *p, *t, *te, *u` -- four pointer locals. 68k has 8 address registers (A0-A7, A7 is SP). With `fp`, `filename`, `p`, `t`, `te`, `u` that is 6 address-register candidates competing for 7 usable slots. `te` and `u` only exist in the multibyte path.

**Fix applied:** Replaced with `char *p, *lo, *hi` for the fast path. Reduced to 5 address-register candidates -- all fit without spilling to stack. `t/te/u` preserved inside the `#ifndef __AMIGA__` block where they are still needed.

## Summary

- **Primary bottleneck:** putchar() call overhead in the character output loop
- **Nature:** CPU-bound (call overhead), not I/O-bound
- **Estimated overall impact:** Significant -- for a 1000-line file at 80 chars/line, saves approximately 4 million cycles of JSR overhead on 68000
- **All fixes applied** to `ports/rev/ported/rev.c`
- **Stack:** `__stack = 16384` is adequate -- no large locals, no recursion
- **No -O0 override needed** -- no struct-by-value returns > 8 bytes, `-O2` is correct
