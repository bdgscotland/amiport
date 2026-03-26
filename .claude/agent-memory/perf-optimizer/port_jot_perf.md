---
name: port_jot_perf
description: Performance findings for ports/jot 1.56 — printf per output value in hot loop, rand() modulo bias, reviewed 2026-03-26
type: project
---

# jot 1.56 Performance Notes

**Why:** jot's primary use is generating long sequences (jot 1000000). The inner loop calls putdata() -> printf() once per value.

## Hot Path
- `jot.c:291-293` — main generation loop: `for (i=1, x=begin; i<=reps || inf_loop; i++, x+=step)` calling putdata() every iteration.
- `jot.c:352-378` — putdata(): one printf() call per value.

## Findings
- **MEDIUM** [jot.c:352-378] — putdata() calls printf(format, ...) once per output value. For large `reps` (e.g., `jot 1000000`), this is 1M printf() calls each with format-string parsing. Since `format` is fixed for the entire run, the format-string scan overhead is paid every iteration. This cannot be easily eliminated without restructuring (pre-parsing the format), but caching the output type (int/long/float/char) to skip the branch chain in putdata() would help.
- **MEDIUM** [jot.c:375-376] — `fputs(sepstring, stdout)` called every iteration when `!last`. For the default `sepstring="\n"`, this is a function call per line. A `putchar('\n')` fast path when sepstring is a single character would save the JSR overhead.
- **LOW** [jot.c:333] — `rand() % (int)uintx` — standard modulo bias, but since arc4random_uniform() was already replaced with rand(), this is unavoidable without implementing rejection sampling. Not worth fixing.
- **LOW** [jot.c:43] — `long __stack = 32768` — 32 KB. Adequate for this program's stack usage (no large local arrays).

## Verdict
I/O-bound for large sequences. The printf-per-value pattern is the expected bottleneck. No CRITICAL issues.

**How to apply:** The fputs single-char fast path is a worthwhile LOW fix. The printf chain is inherent to the program's design.
