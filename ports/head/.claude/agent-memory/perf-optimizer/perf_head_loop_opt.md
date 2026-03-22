---
name: head_loop_optimization
description: getc/putchar per-character loop replaced with fgets/fputs line loop in head v1.24
type: project
---

Replaced the hot `while (getc/putchar)` loop in `head_file()` with a `fgets/fputs` line-oriented loop.

**Why:** The original loop paid three branch instructions per byte: getc EOF check, putchar EOF check, newline+count test. On 68000 @ 7 MHz with chip RAM that is approximately 30 cycles per byte. For a typical 80-char line that is 2400 cycles in loop overhead per line, before any I/O. The putchar EOF check was the main waste — it fires on every character for an error that almost never happens.

**How to apply:** For any port that does per-character I/O with putchar/getc, prefer fgets+fputs if the algorithm is line-oriented. The `fgets` buffer must be stack-safe (check `__stack` cookie). Always use `strlen(buf)` to find the newline rather than tracking `ch` — fgets strips the loop entirely into libnix.

**Correctness detail:** fgets stops at `\n` OR at `bufsize-1` bytes. Only decrement the line counter when `buf[strlen(buf)-1] == '\n'`, so lines longer than the buffer do not incorrectly advance the counter. Buffer size chosen as 4096 (aligns with 8x AmigaDOS 512-byte sectors, well within 16KB stack budget).

**Estimated gain:** ~40x fewer branch instructions for 80-char lines. Noticeable on 68000; measurable on all targets when processing large files.
