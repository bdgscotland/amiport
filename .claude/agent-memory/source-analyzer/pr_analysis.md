---
name: pr_analysis
description: OpenBSD pr v1.46 portability analysis: EASY verdict, Category 1 CLI, sigprocmask shim, vasprintf shim, strtonum shim, exit codes, pledge stub, _exit in signal handler, egetopt bundled
type: project
---

# OpenBSD pr v1.46 — Portability Analysis Memory

**Verdict: EASY**
**Category: 1 (CLI tool)**
**Source:** pr.c (1957 lines) + egetopt.c (206 lines) + pr.h + extern.h

## Source Files
- pr.c (1957 lines) — main logic, all 4 output modes (onecol/vertcol/horzcol/mulfile)
- egetopt.c (206 lines) — BUNDLED extended getopt (handles +page and -column non-standard options)
- pr.h — defines LBUF=8192, HDBUF=512, TIMEFMT, HDFMT
- extern.h — function prototypes

## Key Findings

### Tier 1 Shim Issues
1. `sys/stat.h`, `fstat()` — amiport_fstat() via <amiport/stat.h> — needs-shim
2. `sigprocmask/sigemptyset/sigaddset` — all covered by amiport/signal.h no-op shim
3. `vasprintf()` — covered by amiport/stdio_ext.h
4. `strtonum()` — covered by amiport/err.h (long long params, needs -std=gnu99 to compile)
5. `write(STDERR_FILENO, ...)` — covered by amiport/unistd.h write() macro + STDERR_FILENO define
6. `fileno(inf)` — libnix provides fileno() natively
7. `pledge("stdio rpath", NULL)` — stub as macro: #define pledge(p,e) (0)
8. `signal(SIGINT, ...)` — libnix signal() works for SIGINT; amiport shim for extended usage
9. `localtime()`, `strftime()` — libnix provides both
10. `time()` — amiport_time() via shim or libnix native
11. `fstat(fileno(inf), &statbuf)` — mixed namespace concern: fileno() returns libnix fd, fstat() needs libnix or amiport fstat on that fd. Use libnix fstat() directly or amiport_fstat() with fileno().

### Exit Codes (all need fix)
- exit(1) at lines 144, 166, 874 → exit(10)
- _exit(1) at line 1693 (in SIGINT handler terminate()) → _exit(10) or exit(10)

### egetopt — Bundled Getopt
- egetopt.c is a CUSTOM extended getopt that handles +N (page number) and -N (column count) syntax not supported by standard getopt
- DO NOT replace with amiport_getopt or libnix getopt — use as-is
- eoptind / eoptarg globals (extern in extern.h)

### GNU Extension
- Line 851: __attribute__((format(printf, 1, 2))) on ferrout() — harmless, bebbo-gcc supports this

### _exit() in Signal Handler
- Line 1693: terminate() calls _exit(1)
- In signal handlers, _exit() is correct (async-signal-safe). Do not change to exit().
- Change the value: _exit(10)

### SIGINT Deferred Error Pattern
- Uses volatile sig_atomic_t ferr to defer error output during SIGINT delivery
- sigprocmask used to protect the ferrlist linked list insertion (blocking SIGINT during malloc)
- On AmigaOS, sigprocmask is a no-op (amiport shim), but this is safe: AmigaOS signals are synchronous/cooperative, not preemptive. The ferr flag still works correctly.

### stat.st_mtime Usage
- Line 1486: timeptr = localtime(&(statbuf.st_mtime)) — uses file modification time for header
- amiport_stat/fstat populate st_mtime from AmigaDOS DateStamp. This works correctly (DateStamp to Unix epoch mapping is implemented in the shim).

### Memory Allocation
- LBUF (8192) and HDBUF (512) buffers are ALL heap-allocated via malloc/calloc — no large stack arrays
- Safe from stack overflow concern

### strtonum long long
- amiport_strtonum() uses long long parameters — requires -std=gnu99 flag
- Used for argument parsing only; values bounded by INT_MAX so no 64-bit arithmetic issues at runtime

### fclose(stdin) Guard
- All fclose(inf) calls already guarded with "if (inf != stdin)" — no fix needed

## Transformation Summary
1. Add -std=gnu99 to CFLAGS
2. Replace #include <sys/stat.h> with #include <amiport/stat.h>
3. Replace #include <signal.h> with #include <amiport/signal.h>
4. Replace #include <unistd.h> with #include <amiport/unistd.h>
5. Add #include <amiport/stdio_ext.h> for vasprintf
6. Add #include <amiport/err.h> for strtonum
7. Add #include <amiport/stdlib.h> for exit macro
8. Add pledge stub: #define pledge(p,e) (0)
9. Change exit(1) → exit(10) at 3 sites
10. Change _exit(1) → _exit(10) in terminate()
11. Add __stack cookie and $VER string
12. egetopt.c: no changes needed (pure C, no POSIX deps)

**Why: EASY**
- No fork/exec, no pthreads, no mmap, no sockets
- Pure stdio + file formatting
- All POSIX deps have shim coverage
- Bundled getopt avoids broken libnix getopt_long
