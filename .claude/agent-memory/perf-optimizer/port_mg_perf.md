---
name: port_mg_perf
description: Performance findings for ports/mg (Micro GNU Emacs) — display rendering, ttyio buffer, ldelete memmove, ntabstop division, ffgetline getc loop reviewed 2026-03-24
type: project
---

# Performance Findings: mg (Micro GNU Emacs)

**Reviewed:** 2026-03-24
**Binary:** 197KB, -O2, 68000, __stack=65536

## Hot Paths Identified
1. `update()` + `uline()` in display.c — called on every keystroke
2. `ttputc()` → `ttflush()` in ttyio.c — per-character output buffering
3. `vtputc()` / `vtpute()` in display.c — per-character virtual screen writes
4. `forwsrch()` / `backsrch()` in search.c — forward/backward plain search
5. `re_backsrch()` in re_search.c — O(n²) character-by-character backward regex search
6. `ldelete()` in line.c — character-by-character memmove scrunch loop
7. `ffgetline()` in fileio.c — getc() per character during file load
8. `ntabstop()` in util.c — division in hot display inner loop
9. `getcolpos()` in util.c — called from modeline on EVERY keystroke
10. `modeline()` in display.c — snprintf + getcolpos per update

## Key Findings

### CRITICAL
- `ffgetline()` uses `getc()` per character — replace with `fgets()` for file load
- `re_backsrch()` is O(n²): calls `regexec()` once per character position per line for backward search

### HIGH
- `ldelete()` line 411-413: character-by-character scrunch loop — should be `memmove()`
- `ntabstop()`: uses division `(col + tabw) / tabw * tabw` called per-character in vtputc/getcolpos hot paths — replace with subtraction trick
- `NOBUF = 512` in ttyio.c — too small for an 80-column screen repaint; increase to 4096

### MEDIUM
- `getcolpos()` called from `modeline()` on every keystroke even when cursor hasn't moved
- `snprintf(bf, 5, "\\%o", c)` in vtputc/vtpute/update() — called for non-printable chars; rare but allocates stack frame each time
- `modeline()` calls `getcolpos()` which iterates from line start to dot position — O(line length) per redraw

### LOW
- `tgoto()` uses `sprintf()` internally — called per cursor move; ANSI escape format is simple enough to build directly
- `lrealloc()` allocates exact size per insert — no growth factor, causes O(n²) reallocs on long typing runs
- `bcopy()` used in several places where `memcpy()` is available and cleaner

## Stack Safety
__stack = 65536 is correct and adequate. No large local arrays found in hot paths (NLINE=256 in file.c is heap-allocated).
The `adjustname()` function has a static `char fnb[PATH_MAX]` (1024 bytes) — safe because it uses a static buffer.
