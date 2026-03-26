---
name: port_cksum_perf
description: Performance findings for ports/cksum 1.0 — buffer size mismatch in crc32.c, large stack-allocated buffers, print call overhead reviewed 2026-03-26
type: project
---

# cksum 1.0 — Performance Findings

Reviewed 2026-03-26. Source: ports/cksum/ported/ (4 files).

## Hotpath

The inner loops in crc(), csum1(), csum2(), crc32() — all process every byte of every input file.
These are the sole performance-critical paths.

## Issues

### HIGH — crc32.c:104 — BUFSIZ (1024) vs 16384 buffer mismatch
crc.c and sum.c use `u_char buf[16 * 1024]` (optimal for raw read()).
crc32.c uses `char buf[BUFSIZ]` = 1024 bytes — 16x smaller per read call.
On 7MHz 68000 each amiport_read() call costs ~30-50 cycles of function overhead.
For a 100KB file: 100 calls with BUFSIZ vs 7 calls with 16KB. Fix: change to `u_char buf[16 * 1024]`.

### MEDIUM — crc32.c:104 — 16KB local buffer on stack
After fixing BUFSIZ->16384, the buffer is 16KB on stack. With __stack=65536 in cksum.c
and 8KB AmigaOS library overhead margin, 16KB is within bounds. But crc.c and sum.c
already do this successfully. Note: make the fixed buffer `static` is NOT safe because
crc/csum1/csum2/crc32 can each be called in sequence for multiple files, but they are
never called concurrently or recursively on AmigaOS. Could use static to save stack if
needed, but current 65536-byte stack cookie provides adequate room.

### LOW — sum.c:53-56, 65-69, 78-82 — Multiple printf calls per output line
pcrc/psum1/psum2 each call printf 2-3 times. Small impact (output path, not inner loop).
Collapsing to a single printf with conditional format would eliminate 1-2 JSR overheads.
Not the hot path so LOW priority.

## Summary
- Primary bottleneck: I/O throughput (read() syscall overhead and chip RAM bandwidth)
- The 16KB buffer in crc/sum is already optimal
- crc32.c is the outlier — its 1024-byte buffer causes 16x more read() calls
- Overall: clean port, one clear HIGH fix needed
