---
name: port_dirname_perf
description: Performance findings for ports/dirname 1.17 -- single-shot Cat 1 utility, no meaningful optimization opportunities reviewed 2026-03-25
type: project
---

# dirname 1.17 Performance Review

**Date:** 2026-03-25
**Verdict:** No findings. Clean.

## Summary

dirname is a single-invocation CLI: one pathname argument, one puts() call, exits.
The hot path (amiport_dirname) does three pointer-scan while loops over a static
1024-byte buffer. At -O2 on 68000 the worst-case is ~6144 cycles for a maximal
1023-byte path -- under 1ms at 7.14 MHz.

## No Action Required

- Static buffer (BSS, not stack) -- no stack risk
- Pointer-based scanning -- no array indexing overhead
- Single strlen + single memcpy -- called once
- No output loops, no file I/O
- Startup cost (libnix + amiport_expand_argv) dominates wall time but is outside ported source

## Pattern Note

Single-shot Cat 1 utilities (basename, echo, true, false, dirname) are inherently
throughput-irrelevant. Future reviews can fast-path to "no findings" after confirming
there are no hot loops, no per-character I/O, and no repeated allocations.
