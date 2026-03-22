---
name: Stack depth difference vamos vs FS-UAE
description: Real AmigaOS call chains consume 2-4KB more stack than vamos. buf[4096] stack vars in functions that call fgets/fputs can push over the limit and cause ACPU_LineF crashes.
type: feedback
---

Real AmigaOS adds 2-4KB of hidden stack depth vs vamos for the same call chain.

**Why:** vamos intercepts system calls at the API level with minimal stack. Real AmigaOS goes through full library dispatch vectors, filesystem handler packet passing, console.device buffers. Each adds overhead vamos skips.

**How to apply:** Any function with a large local buffer (≥2KB) that calls I/O functions (fgets, fputs, read, write) is at risk on real AmigaOS even if vamos passes. Fix: change `char buf[N]` to `static char buf[N]` when the function is not recursive. Also increase `__stack` to 65536 as insurance.

**Alert signature:** `ACPU_LineF (0x8000000B)` on FS-UAE, clean run on vamos. Binary has no F-line instructions (verify with `m68k-amigaos-objdump -d`). Crash = corrupt return address hit data with `0xFx` byte.

**Example:** head port, `head_file()` had `char buf[4096]`. After fix: `static char buf[4096]` + `__stack=65536`. Crash resolved.
