---
name: audit_jq_port
description: Hardware audit of ports/jq/ported/ — alignment, dtoa byte order, stack depth, output buffer flush safety
type: project
---

Audit date: 2026-03-23. Port: jq 1.7.1, compiled -O0 -m68000 -noixemul.

Summary: 3 issues found (0 errors, 2 warnings, 1 note). 1 fix applied (note in jv.c comment).

## CLEAN items
- exec_stack.h ALIGNMENT=4 clamp: correct. 68000 requires 4-byte longword alignment; the clamp prevents misaligned stack_ptr (int) reads that would cause address error trap.
- MurmurHash3 memcpy fix: correct. Original uint32_t* cast was a bus-error risk on 68000. memcpy safe on all variants.
- IEEE_MC68k flag: correct. 68k is big-endian; most-significant byte at lowest address. word0=L[0] is correct.
- DMA/Chip RAM: no concern. jq is pure CPU/heap, no custom chip access.
- jv struct double alignment: safe. double at offset 8 in jv struct — 4-byte aligned, correct for m68k-amigaos ABI.
- util.c buf[4096]: heap-allocated struct member, not a stack local — crash-patterns #10 does not apply.

## Issues requiring action

### Warning: __stack vs VAMOS_STACK mismatch (main.c:23, Makefile:24)
- __stack = 65536 (64KB) but VAMOS_STACK = 262144 (256KB)
- 4:1 ratio means stack overflows on real AmigaOS with deep JSON won't appear in vamos tests
- -O0 frames are larger than -O2, amplifying the gap
- Hidden dos.library stack cost (2-4KB) further reduces usable depth
- FIX: increase __stack to 131072 (128KB) minimum

### Warning: amiport_print_buf not flushed on all exit paths (jv_print.c:76-85)
- amiport_print_buf is an app-level 4KB buffer that bypasses libnix's FILE flush chain
- fclose(stdout) in main.c:817 does NOT flush it
- Current design relies on jv_dumpf() being called before exit — holds for normal operation
- On error paths with no output, buffer is empty so no data is lost in practice
- FIX: add atexit(amiport_flush_print_buf) in main() — follow known-pitfalls atexit pattern

### Note: misleading comment about memcpy codegen (jv.c:1186) — FIXED
- Comment claimed "generates MOVE.L when aligned" — only true at -O2+, not -O0
- Fixed in place to accurately describe -O0 behavior

**Why:** These are real correctness risks on 68000 real hardware that vamos does not expose.
**How to apply:** When revisiting the jq port (stack increase, atexit flush), these are the open items.
