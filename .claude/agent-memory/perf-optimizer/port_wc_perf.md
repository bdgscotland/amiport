---
name: port_wc_perf
description: Performance findings for ports/wc — hotpath summary and recommendations reviewed 2026-03-22
type: project
---

Key hot paths in ports/wc (OpenBSD wc 1.32):

1. doword loop (cnt(), line 234-245) — main byte-scanning loop. Calls isspace() per byte inside
   inner for loop. isspace() is a library call on libnix; on 68000 that's ~20 cycles of call
   overhead per byte on top of the test itself. HIGH impact: replace with a lookup table or
   hand-rolled whitespace test (== ' ', == '\t', == '\n', == '\r', == '\f', == '\v').

2. int64_t counters (linect, wordct, charct) — 64-bit arithmetic emulated on 32-bit 68k.
   Each increment (++linect, ++wordct, ++charct) compiles to addq.l + addx.l (2 instructions).
   For linect and wordct this is acceptable (rare increments). charct += len every _MAXBSIZE
   block is fine. Not worth changing unless profiling shows it matters.

3. gotsp declared as short (line 200) — 16-bit boolean. On 68000 this is loaded as a 16-bit
   value. Declare as int for natural register width. Minor but free win.

4. bufsz never updated after realloc (line 222-224) — bufsz stays 0 after first realloc(),
   so every call to cnt() triggers a redundant realloc(_MAXBSIZE) even though the buffer is
   already that size. Add: bufsz = _MAXBSIZE; after the realloc. Wastes a library call per file.

5. _MAXBSIZE = 64KB read buffer — correct and good. I/O is the dominant cost for large files;
   larger reads amortize the AmigaDOS Read() call overhead. This is correct as-is.

6. doline-only path (line 257-268) — tight, no isspace(), just `*C == '\n'`. This is the
   fastest path and is already well-structured. No change needed.

7. dochar-only path (line 277-295) — uses fstat() fast path for regular files (no byte scan
   at all). Optimal.

**Primary bottleneck:** I/O bound for large files (disk DMA speed). For small files or stdin,
the doword path's isspace() call cost dominates.

**Why:** isspace() as a library call in a per-byte loop is the classic 68k performance trap.
On a 7MHz 68000 reading a 1MB file, that's ~1M isspace() calls. Replacing with an inline
whitespace test or a 256-byte lookup table eliminates the call overhead entirely.

**How to apply:** When advising on wc performance, lead with the isspace() replacement.
The gotsp/short and bufsz fixes are secondary but cheap to apply.
