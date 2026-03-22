---
name: FileInfoBlock alignment crash
description: Stack-allocated FileInfoBlock causes DOS Recoverable Alert on 68000 — no Enforcer hit, fix via AllocDosObject(DOS_FIB)
type: project
---

When `amiport_stat()` or `amiport_fstat()` crashes on real AmigaOS hardware as a Recoverable Alert (`Error: 0100 000F`) with zero Enforcer hits, the root cause is a stack-allocated `struct FileInfoBlock` that is only 2-byte aligned. AmigaDOS `Examine()`/`ExamineFH()` require longword (4-byte) alignment. The 68000 traps with an Address Error inside the ROM before Enforcer can see it.

**Why:** `-m68000` gcc only guarantees 2-byte stack alignment for structs. The FIB is 260 bytes and triggers the alignment check in dos.library on the 68000 (68020+ does not trap on misaligned access in most modes).

**Fix applied (2026-03-21):** All stack-allocated FIBs in `lib/posix-shim/src/stat.c` and `lib/posix-shim/src/file_io.c` replaced with `AllocDosObject(DOS_FIB, NULL)` / `FreeDosObject(DOS_FIB, fib)`. `dir_ops.c` was already safe (FIB embedded in a struct allocated via `AllocVec` which guarantees longword alignment).

**Detection:** `grep 'struct FileInfoBlock [a-z]'` in shim sources — any stack-allocated FIB is a latent crash on 68000 hardware.

**How to apply:** Any future shim function that calls Examine/ExamineFH/ExNext must use `AllocDosObject(DOS_FIB, NULL)` rather than a stack or static local FIB.
