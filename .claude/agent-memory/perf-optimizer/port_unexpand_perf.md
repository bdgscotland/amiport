---
name: port_unexpand_perf
description: Performance findings for ports/unexpand 1.13 — dead linebuf copy loop, printf("%s") on already-buffered output, global buffers, reviewed 2026-03-26
type: project
---

# unexpand 1.13 Performance Notes

**Why:** unexpand is a stream filter; every line of every input file goes through fgets + tabify + printf.

## Hot Path
- `unexpand.c:102-109` — outer loop: fgets() per line, then a `for` loop over linebuf, then tabify(), then printf().
- `unexpand.c:103-106` — the `for (cp = linebuf; *cp; cp++) continue;` loop after fgets — this walks to end of linebuf to find the null terminator, then sets `cp[-1] = 0`. But fgets writes into genbuf, NOT linebuf — linebuf is populated by tabify(). This for-loop is scanning uninitialized/stale data from a previous iteration. It appears to be a latent bug in the original source (dead store on linebuf before tabify fills it). The loop does nothing useful except potentially clearing the last character of whatever was left in linebuf from the previous call.
- `unexpand.c:108` — `printf("%s", linebuf)` — correct but uses printf format overhead. `fputs(linebuf, stdout)` is faster and semantically identical.

## Findings
- **HIGH** [unexpand.c:103-106] — The loop `for (cp = linebuf; *cp; cp++) continue; if (cp > linebuf) cp[-1] = 0;` runs over linebuf BEFORE tabify() has populated it. This is scanning whatever was left from the previous call (or uninitialized memory on the first call). This is logically dead code that wastes cycles per line scanning stale buffer data. It should be removed — tabify() handles all the linebuf population.
- **MEDIUM** [unexpand.c:108] — `printf("%s", linebuf)` should be `fputs(linebuf, stdout)`. Eliminates format-string parsing overhead. One call per input line.
- **LOW** [unexpand.c:55-56] — `genbuf[BUFSIZ]` and `linebuf[BUFSIZ]` are global arrays. This is already correct — global (not stack) allocation avoids stack pressure. BUFSIZ is typically 1024.
- **LOW** [unexpand.c:102] — `fgets(genbuf, BUFSIZ, stdin)` — BUFSIZ-sized reads. Fine. No issue here.

## Verdict
The dead linebuf scan loop is the most interesting finding — it wastes cycles and is logically wrong (but harmless because tabify() overwrites linebuf anyway). The printf->fputs change is easy and worthwhile.

**How to apply:** Remove lines 103-106 (the dead linebuf scan). Change line 108 from printf("%s", linebuf) to fputs(linebuf, stdout).
