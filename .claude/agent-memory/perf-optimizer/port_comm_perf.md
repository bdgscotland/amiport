---
name: port_comm_perf
description: Performance findings for ports/comm 1.11 (OpenBSD comm) — printf hotpath, strcoll vs strcasecmp, amiport_check_break overhead, fputs replacement reviewed 2026-03-24
type: project
---

# Performance Findings: ports/comm 1.11

## Context
OpenBSD comm v1.11 — Category 1 CLI. Compares two sorted files line by line.
Source: single file, ports/comm/ported/comm.c (~215 lines)

## Hot Paths
1. Main for loop (lines 125-172) — one iteration per line pair. Per iteration:
   - amiport_check_break() call (always)
   - up to 2x fgets()
   - 1x function pointer call (strcoll or strcasecmp)
   - 0-1x printf("%s%s", ...) for output
2. show() loop (lines 186-194) — drains remainder of one file after other is exhausted.
   Same structure: amiport_check_break + printf + fgets per iteration.

## Findings

### HIGH: printf("%s%s") for two-segment output — use fputs + fputs
printf("%s%s", col, line) on each matching line calls the full format engine.
For the common case (no column = "" prefix, col3 = "\t\t"), col is a literal from tabs[].
fputs(col, stdout) + fputs(line, stdout) avoids format string parsing entirely.
On 7MHz 68000, printf format dispatch is ~50-100 cycles overhead vs direct write.
Impact: significant for large identical files piped to col3 (the common "both columns" case).

### HIGH: strcoll() is far slower than strcmp() — default compare function
The default compare is strcoll (set at line 88). On AmigaOS, setlocale(LC_ALL, "") with
libnix's locale stubs means LC_COLLATE is always "C", so strcoll() is functionally
identical to strcmp() but with locale dispatch overhead on every call.
Since we override localeconv() elsewhere (lua port pattern), and libnix has no real
locale support, replacing strcoll with strcmp as the default saves the dispatch overhead
per line pair. When -f is given, strcasecmp is already optimal.

### MEDIUM: amiport_check_break() called every iteration of the main loop
For files with thousands of lines, amiport_check_break() fires on every loop iteration.
The function likely reads a signal/event flag. If it has any OS call overhead (e.g.,
CheckSignal via exec.library), this costs real cycles on each iteration.
Batching: call every N iterations (e.g., N=64) using a counter. Ctrl-C latency increases
by at most 64 line-comparison cycles -- imperceptible to users.

### MEDIUM: amiport_check_break() called every iteration of show() loop
Same as above for the show() drain loop.

### LOW: Function pointer for compare -- one indirection per line pair
compare is a function pointer (line 79). On 68000, indirect JSR costs 4 extra cycles
over a direct JSR. For most workloads this is negligible.
Not worth hoisting via branch at call site given the code clarity cost.

### NO ISSUE: fgets vs fgetc
fgets() is already used correctly. No fgetc hotpath issue.

### NO ISSUE: Static line buffers
line1/line2 are already `static char[]` (correctly noted in code comment at line 78).
Stack safety already handled.

### NO ISSUE: Stack depth
No recursion. show() is a simple iterative drain. Stack is fine at 32768.

## Recommended Changes (priority order)
1. Replace printf("%s%s", col, line) with fputs(col, stdout); fputs(line, stdout) — 3 sites
2. Replace default compare = strcoll with compare = strcmp (locale is always C on AmigaOS)
3. Add iteration counter for amiport_check_break() batching (every 64 iterations)

## Overall Assessment
- Primary bottleneck: I/O bound (fgets + printf per line). printf replacement is the key win.
- Estimated overall impact: noticeable for large files (10K+ lines)
- No structural issues. Very clean loop already using fgets and static buffers.
