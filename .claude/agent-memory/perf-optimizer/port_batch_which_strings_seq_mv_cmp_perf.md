---
name: port_batch_which_strings_seq_mv_cmp_perf
description: Performance findings for ports/which 1.27, strings 1.0, seq 1.8, mv 1.47, cmp 1.19 — batch review 2026-04-11
type: project
---

## which 1.27
- CLEAN. PATH search is I/O bound (stat() per directory entry). No hot CPU paths.
- __stack = 16384: correct, no large locals.

## strings 1.0
- HIGH: fgetc() inner loop at strings.c:59. Every byte = JSR overhead. Switch to fread() with static 8KB read buffer.
- HIGH: amiport_check_break() called every byte. Batch check to every 1024 bytes — CheckSignal() is a kernel call (~50 cycles).
- putchar() calls at lines 90/94/100 are acceptable — only fired on string boundaries, not every byte.
- __stack = 8192: safe, static buf[8192] is file-scoped, no large locals.

## seq 1.8
- MEDIUM only: printf(fmt, cur) per value in main loop. For integer ranges this is expensive on 68000. fputs(sep,...) per iteration adds another call. Not HIGH because seq output is typically small.
- No stack issues. generate_format() buf[256] is static.
- CLEAN of HIGH issues otherwise — single-shot tool, computation is trivial vs. output I/O.

## mv 1.47
- CLEAN. fastcopy() uses malloc'd block (sbp->st_blksize), read/write loop — correct approach.
- No hot CPU paths; every mv is 1-2 renames or a copy loop bounded by file size.
- __stack = 16384: correct.

## cmp 1.19
- HIGH (special.c): c_special() uses getc(fp1)/getc(fp2) per byte in inner loop at special.c:87-88. fread() into dual buffers would be 3-5x faster.
- regular.c: mmap path correct — pointer comparison loop at regular.c:135 is fast. No issue.
- amiport_check_break() absent from c_special inner loop — cmp on large files is uninterruptible (MEDIUM concern, not HIGH for perf).
- __stack = 16384: safe, no large locals in any of the 4 files.
