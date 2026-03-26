---
name: port_nl_perf
description: Performance findings for ports/nl 1.8 — printf per line in hotpath (HIGH), printf format string is runtime variable reviewed 2026-03-26
type: project
---

## nl 1.8 — Performance Analysis

**Primary bottleneck:** CPU-bound (printf per line with format string computed at runtime)

### Hot Paths
- filter(): called for every line — uses getline() then printf(format, width, line) + fputs(sep)
  + fwrite(buffer, linelen). The getline() + fwrite() path is good. The printf() is the issue.

### HIGH Findings
1. printf(format, width, line) in filter() inner loop (nl.c line ~358) — `format` is a runtime
   string pointer (FORMAT_LN/FORMAT_RN/FORMAT_RZ selected by -n flag). On every numbered line
   this calls printf() twice: once for the line number (printf(format, width, line)), once
   implicitly via fputs(sep). printf() with a runtime format string cannot be optimized by the
   compiler and costs ~100+ cycles per call on 68000. For a 1000-line file = 2000+ printf calls.
   Fix: select a function pointer or use a switch at the top of filter() to pick a specialized
   output function per format type, replacing printf(format,...) with snprintf into a small
   static buffer then fputs. Or: precompute the format string as a literal and use fprintf
   with a fixed format. Est: 40-60% reduction in per-line overhead on the number path.

2. printf("%*s", width, "") for unnumbered lines — this calls printf for every blank/header/
   footer line just to print spaces. Replace with a static pad string pre-filled to `width`
   spaces at startup, then fputs(padstr, stdout). Est: 50% reduction on unnumbered-line path.

### MEDIUM Findings
3. fputs(sep, stdout) after every numbered line — sep defaults to "\t" (one character). When
   sep is a single char (the common case), replace with putchar(*sep). Saves the strlen+loop
   overhead inside fputs for the 99% case. Check: `if (sep[1] == '\0') putchar(sep[0]); else fputs(sep, stdout);`

### LOW
4. getline() buffer reallocation: buffersize starts at 0 meaning first getline always allocates.
   Pre-setting `buffersize = 512; buffer = malloc(512);` before the loop avoids the first realloc.
   Minor but avoids one malloc+free cycle per file.
5. regexec() call for number_regex section type — Tier 2 posix-emu, unavoidable overhead.
   Only triggers when -b p<regex> is active, so low impact for normal use.

### Stack / Safety
- __stack = 16384. filter() uses heap-allocated buffer (getline). No stack array concerns.
- NL_TEXTMAX=256 errorbuf in parse_numbering() is fine (256 bytes, called once).
- MB_CUR_MAX not present in this file. Clean.
