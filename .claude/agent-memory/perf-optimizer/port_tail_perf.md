---
name: port_tail_perf
description: Performance findings for ports/tail — hotpath summary and key recommendations reviewed 2026-03-21
type: project
---

Key hot paths in ports/tail (OpenBSD tail 1.24):

1. tfprint() in forward.c — putchar() loop for output. WR() (write syscall) used in reverse/read
   paths but tfprint() still uses putchar(). HIGH impact fix: replace with fread/write block copy.

2. rlines() in forward.c — fseeko+getc per character for regular file RLINES mode (default `tail -n`).
   This is a one-seek-per-char backward scan. The upstream comment says "not a problem with smart stdio"
   but AmigaOS stdio is not smart — no read-ahead when seeking backward. Fixing requires buffered
   backward scan, but this is the hot case for the most common usage.

3. r_buf() 128KB nodes in reverse.c — BSZ is 128*1024. On a 2MB Amiga this is a concern.
   Reducing to 8192 or 4096 would be safer and not hurt throughput.

4. lines() in read.c — realloc every 1024 chars for the accumulation buffer is OK.
   recallocarray every 1024 lines is also OK (not per-char). Not a primary concern.

5. -f polling at Delay(50) = 1 second is correct and appropriate.

**Why:** rlines() is the critical case — every `tail file` with no flags hits RLINES on a regular file,
which calls rlines(). On a slow FFS filesystem, the one-seek-per-char pattern is the dominant cost.

**How to apply:** When advising on tail performance, lead with tfprint() replacement and
rlines() buffered-seek fix. The 128KB allocation is a secondary flag, not a crasher.
