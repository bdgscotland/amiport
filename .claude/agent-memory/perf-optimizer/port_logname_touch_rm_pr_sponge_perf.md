---
name: port_logname_touch_rm_pr_sponge_perf
description: Performance findings for ports/logname 1.10, touch 1.27, rm 1.45, pr 1.46, sponge 0.1 — batch review 2026-04-11
type: project
---

## logname 1.10
Clean bill of health. Trivial single-shot utility. No hot paths.

## touch 1.27
No meaningful optimization opportunities. Timestamp-setting utility with no I/O loops.

## rm 1.45
- check() getchar() drain loop is only used for interactive -i mode, not a hot path.
- rm_tree() FTS traversal is OS-bound. Nothing to optimize in C code.
- CLEAN for perf purposes.

## pr 1.46 — PRIMARY HOT PATHS
- inln(): getc() per byte in tight inner loop — HIGH. The no-expansion path (else branch,
  line 1226-1231) calls getc() once per character with no tab expansion. For lines up to
  LBUF (8192) chars this is thousands of getc calls per page. fgets + memchr(\n) would
  eliminate most of these. The truncate-drain loop (line 1262-1265) is a separate
  getc-per-byte loop for lines exceeding column width.
- otln() putchar() in tight loop — HIGH. Non-contracted output path (line 1421) uses
  fwrite() which is correct. But contracted output path (ogap > 0, default) has
  putchar(*buf++) per non-space character (line 1367) plus putchar(' ') per space.
- prtail() putchar('\n') loop — MEDIUM. Each newline padding call is a separate putchar.
  For a 66-line page with 5-line tail, that is 10 putchar calls for TAILLEN+padding.
  Could batch with fwrite("\n\n\n\n\n...", cnt, stdout) but impact is low since it is
  not in the inner line loop.
- addnum() line % 10 and line /= 10 — LOW. 32-bit division on 68000 is ~150 cycles each.
  Used once per output line for -n mode only. Not hot enough to matter.

## sponge 0.1
- concat.c sponge_concat(): already uses 4096-byte static buffer, read/write loop is
  optimal for a single-pass stdin drain. CLEAN.
- eprintf.c xvprintf(): strlen(fmt) called twice on error paths only. Not hot.
- CLEAN for perf purposes.
