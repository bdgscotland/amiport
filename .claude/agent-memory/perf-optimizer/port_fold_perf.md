---
name: port_fold_perf
description: Performance findings for ports/fold 1.18 — getchar per-byte inner loop, fwrite for output blocks, MB_CUR_MAX correctly guarded reviewed 2026-03-26
type: project
---

## fold 1.18 — Performance Analysis

**Primary bottleneck:** I/O-bound (getchar per-byte inner loop)

### Hot Paths
- fold(): inner `while ((ch = getchar()) != EOF)` — reads one byte at a time into heap buffer

### HIGH Findings
1. getchar() per-byte read loop (fold.c line ~200) — each call is a JSR + stack frame + buffer
   check through libnix. For a 10KB file this is ~10,240 getchar calls. On 7MHz 68000 each
   getchar is ~30-40 cycles overhead beyond the actual I/O. Replace with fread(buf, 1, bufsz, stdin)
   inside the outer while loop, then process the read block byte-by-byte in memory. The output
   path (fwrite + putchar('\n') + memmove) is already block-oriented — only input needs fixing.
   Est: 2-3x speedup on large files.

### MEDIUM Findings
2. memmove(buf, cp, np-cp) on every line break / newline — slides remaining buffer contents
   to the front. On 68000 without cache, memmove is cheap if the remaining fragment is small
   (typical for 80-char lines). No change needed unless profiling shows otherwise.

### LOW
3. strlen(line_buf) called after fgets in each line iteration — but fold.c uses getchar() not
   fgets, so this is N/A. Fold's fwrite/memmove output approach is already efficient.
4. isu8cont() is `return 0` on AmigaOS (correctly guarded with #ifndef __AMIGA__) — no overhead.
5. MB_CUR_MAX is properly guarded with #ifndef __AMIGA__ — no runtime call on Amiga. Clean.

### Stack / Safety
- __stack = 16384. fold() uses static char *buf (heap-allocated). No stack array concerns.
- buf starts at 2048 bytes and doubles as needed — reasonable for Amiga memory.
