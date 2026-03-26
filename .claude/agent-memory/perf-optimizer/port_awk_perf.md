---
name: port_awk_perf
description: Performance findings for ports/awk (BWK One True Awk 2024.12.25) — hash function multiply, gototab bsearch per character, getc per character, SUBSEP strlen per iteration, is_number in fldbld per field, DEBUG branch overhead
type: project
---

# AWK Performance Review (2026-03-25)

## Hot Paths Identified
1. readrec (lib.c:261) — getc() per character, inner loop of record reading
2. fldbld (lib.c:504-511) — is_number() call per field, every record processed
3. hash (tran.c:254-261) — `31 * hashval` multiply per character, called on every symbol lookup
4. match/pmatch/nematch (b.c:710, 740, 796) — get_gototab bsearch per character during NFA execution
5. makearraystring (run.c:514) — getsval(subseploc) + strlen per subscript in inner loop
6. fnematch (b.c:861) — getc per character for RS regex matching

## Findings

### CRITICAL
- hash(): `31 * hashval` is a 32-bit MULU on 68000 (38-70 cycles per char). Replace with shift+add: `(hashval << 5) - hashval`. Eliminates MULU instruction entirely.

### HIGH
- readrec inner loop (lib.c:262): getc() per character. This is the innermost I/O loop for every record. Standard approach: use fread/fgets into a local buffer. However: readrec uses ungetc() and has complex RS logic, so getc is entrenched in the design. Document as known bottleneck.
- get_gototab (b.c:627): bsearch() call on every character during NFA execution. bsearch has JSR overhead + function pointer comparison per iteration. For typical ASCII programs, gototab entries are small (< 20). A manual linear scan would be faster for small N and eliminate indirect function call overhead.
- fldbld is_number (lib.c:504-511): is_number() called on every field of every record — even for known-non-numeric fields. Expensive: calls strtod inside is_valid_number. Upstream design, difficult to change without semantic risk.
- makearraystring (run.c:514): `strlen(getsval(subseploc))` called on every subscript separator evaluation in a loop. Cache before the loop.

### MEDIUM
- format() malloc (run.c:1124): `malloc(fmtsz)` where fmtsz=recsize=8192 on EVERY printf/sprintf call. Use a static buffer with growth.
- u8_rune/u8_nextlen (run.c): Two-deep function call chain called per character during NFA execution (match, pmatch, nematch, fnematch). For pure ASCII input (awk_mb_cur_max==1), u8_rune is just `*rune = c; return 1` — this could be inlined with a fast path.
- catstr (tran.c:556-573): Two malloc+snprintf+free calls to concatenate two strings, plus setsymtab. The intermediate `p` malloc is unnecessary — compute total length once and build directly into newbuf.

### LOW
- adjbuf (run.c:140): `minlen % quantum` is a 32-bit DIVU (120-158 cycles on 68000) called on every buffer growth. Already guarded by `if (minlen > *psiz)` so not in the common path. Low priority.
- DPRINTF macros: `#define DEBUG` is active, meaning every DPRINTF() evaluates `if (dbg)` at runtime. All hot paths (execute, fldbld, readrec) have DPRINTF calls. Remove `#define DEBUG` in Makefile build or convert to compile-time disable.

## Applied Fixes
- hash() shift+add: APPLIED 2026-03-25
- makearraystring subsep strlen: APPLIED 2026-03-25
- get_gototab linear scan for small N: RECOMMENDED, not yet applied
- format() static fmt buffer: RECOMMENDED, not yet applied
