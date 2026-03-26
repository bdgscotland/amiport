---
name: port_join_perf
description: Performance findings for ports/join 1.34 — mbssep ndelim recount in inner loop (HIGH), outfield ferror per field (MEDIUM), needsep branch per field (LOW) reviewed 2026-03-26
type: project
---

## join 1.34 — Performance Analysis

**Primary bottleneck:** CPU-bound (field splitting + output per field)

### Hot Paths
- slurp(): calls mbssep() in a while loop for every field of every line
- outtwoline()/outoneline(): calls outfield() for every output field
- outfield(): calls putchar (tabchar), fputs (field), ferror (stdout) per field

### HIGH Findings
1. mbssep() recomputes ndelim (strlen equivalent) on EVERY CALL (join.c line ~419-422).
   ```c
   ndelim = 0;
   while (delim[ndelim] != '\0')
       ndelim++;
   ```
   tabchar is at most 2 bytes. This loop runs for every field of every line. Fix: cache ndelim
   as a file-scope static, set once when tabchar changes (only in option parsing). Since tabchar
   never changes during processing, `static int ndelim = 2;` initialized at startup eliminates
   this loop entirely. Est: minor but measurable on files with many fields per line.

2. mbssep() inner loop (join.c line ~423-431): for each byte of the field string, iterates
   over all ndelim delimiters. With ndelim=2 (spans mode, space+tab) this is 2 compares per
   byte. With ndelim=1 (-t mode) it's 1. Since AmigaOS is ASCII-only and tabchar is always
   1-2 chars, replace mbssep() with a direct strsep() equivalent using strchr() for the
   common -t mode (ndelim=1) or two explicit compares for spans mode. The function call
   overhead of mbssep() is the real cost here for multi-field lines.

### MEDIUM Findings
3. outfield() calls ferror(stdout) TWICE per field: once at line ~547 and once at line ~557.
   ferror() is not free on AmigaOS — it checks the FILE* error flag through the stdio struct.
   Two ferror() calls per field on a file with 5 fields and 1000 lines = 10,000 ferror calls.
   Fix: check ferror once at the end of outoneline()/outtwoline() rather than inside outfield.
   The error state can't change mid-line on AmigaOS (no signals, no concurrent writers).

4. outoneline()/outtwoline() call putchar('\n') then immediately check ferror(stdout).
   Same issue as #3. A single ferror check at the end of the join loop is sufficient.

### LOW
5. needsep global variable read/written in outfield() — fine, just a flag.
6. slurp() grows fields array in +50 increments with reallocarray — for typical data fine.
   For files with hundreds of fields, consider larger initial alloc (50 is a bit small for
   header-heavy CSVs). Minor.

### Stack / Safety
- __stack = 16384. No large local arrays. join uses heap for all buffers. Clean.
- MB_CUR_MAX: the -t option check was replaced with strlen(optarg) != 1 — correct and clean.
