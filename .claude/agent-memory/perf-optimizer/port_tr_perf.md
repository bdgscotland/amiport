---
name: port_tr_perf
description: Performance findings for ports/tr 1.22 — getchar/putchar per-byte hotloops, translate[] table lookup good, int table wastes memory reviewed 2026-03-26
type: project
---

## tr 1.22 — Performance Analysis

**Primary bottleneck:** I/O-bound (getchar/putchar per-byte)

### Hot Paths
- All four main loops in main(): getchar/putchar per-byte — highest impact
- delete[], squeeze[], translate[] are int arrays (NCHARS=256 ints = 1024 bytes each) — wasteful but in BSS

### HIGH Findings
1. getchar/putchar per-byte in all 4 hot loops — classic fgetc/putchar per-byte antipattern.
   All four loops (delete+squeeze, delete-only, squeeze-only, translate) call getchar() once
   per byte and putchar() once per emitted byte. Replace with fread block + loop over buffer +
   fwrite. Est: 3-5x for translate path on large files.

### MEDIUM Findings
2. translate[], delete[], squeeze[] declared as int[NCHARS=256] — costs 3072 bytes BSS.
   Translate could be unsigned char[256] (256 bytes). Delete/squeeze could be unsigned char[256].
   Saves 2816 bytes BSS. On 68000 with Chip RAM, MOVE.L to 256 ints is slower than MOVE.B to
   256 chars for cache-miss initialization.
3. setup() uses bzero(table, NCHARS * sizeof(int)) = 1024 bytes — with char arrays this would
   be 256 bytes, 4x fewer writes.

### LOW
4. str.c next() function called in setup() loops — function call overhead for each char in
   string spec. Acceptable since setup is one-time not per-byte.

### Stack / Safety
- __stack = 16384 is adequate. No large locals. tr is non-recursive.
