# Port: pr

## Overview

| Field | Value |
|-------|-------|
| Program | pr |
| Version | 1.46 |
| Source | OpenBSD usr.bin/pr (v1.46) |
| Category | 1 -- CLI |
| License | BSD-3-Clause |
| Original Author | The Regents of the University of California |
| Port Date | 2026-04-11 |
| Binary Size | 58 KB |
| Source Files | 2 (pr.c, egetopt.c) |

## Description

Paginate and format text files for printing or display. Supports multi-column
output (-N), custom headers (-h), page numbering, line numbering (-n), column
separators (-s), merge mode (-m) for parallel file display, form-feed handling,
and double spacing (-d). IEEE Std 1003.1-2008 (POSIX.1) compliant. This is the
largest and most complex utility in the 5-port batch.

## Prior Art on Aminet

No POSIX pr page formatter found on Aminet. An older "Pr" utility on Fish Disk
is a hardware print spooler, not a text formatter. No equivalent functionality
exists for AmigaOS shell scripting.

## Portability Analysis

Verdict: **MODERATE** -- Two source files (pr.c at ~1800 lines, egetopt.c
bundled). Uses signal handling, stat for file date headers, vasprintf for
deferred error output, and isatty for terminal detection. Requires -std=gnu99
for strtonum long long usage.

| Issue | Tier | Resolution |
|-------|------|------------|
| `stat()` / `struct stat` | Tier 1 | `<amiport/sys/stat.h>` with AMIPORT_NO_STAT_MACROS |
| `fstat()` / `fileno()` | N/A | Replaced with `amiport_stat(path)` |
| `<signal.h>` (sigset_t, sigprocmask) | Tier 1 | `<amiport/signal.h>` shim |
| `isatty(fileno(stdout))` | Tier 1 | Replaced with `IsInteractive(Output())` |
| `vasprintf()` | Tier 1 | `<amiport/stdio_ext.h>` |
| `strtonum()` | Tier 1 | `<amiport/err.h>` |
| `<err.h>` | Tier 1 | Replaced with `<amiport/err.h>` |
| `pledge()` | Stub | `#define pledge(p, e) (0)` |
| `getopt()` (egetopt) | N/A | Bundled egetopt.c (extended getopt for pr) |
| Exit codes | Tier 1 | `exit(1)` -> `exit(10)` (RETURN_ERROR) |
| `long long` in strtonum | Tier 1 | Requires -std=gnu99 |

## Transformations Applied

| File | Change | Comment |
|------|--------|---------|
| pr.c | `<sys/stat.h>` -> `<amiport/sys/stat.h>` | AMIPORT_NO_STAT_MACROS prevents macro collision |
| pr.c | `<signal.h>` -> `<amiport/signal.h>` | sigset_t, sigprocmask, sigemptyset, sigaddset |
| pr.c | `<stdlib.h>` -> `<amiport/stdlib.h>` | exit macro activation |
| pr.c | `<unistd.h>` -> `<amiport/unistd.h>` | STDERR_FILENO, write |
| pr.c | Added `<amiport/err.h>` | strtonum() |
| pr.c | Added `<amiport/stdio_ext.h>` | vasprintf() |
| pr.c | Added `<amiport/glob.h>` | Wildcard expansion |
| pr.c | Added `<proto/dos.h>` | IsInteractive()/Output() for isatty |
| pr.c | `isatty(fileno(stdout))` -> `IsInteractive(Output())` | Direct AmigaOS call |
| pr.c | `fstat(fileno(inf))` -> `amiport_stat(path)` | fstat/fileno cross-namespace issue |
| pr.c | `st_mtime` cast to `time_t` for `localtime()` | st_mtime is ULONG in shim |
| pr.c | struct ferrlist moved before cleanup() | Forward declaration for atexit |
| pr.c | cleanup() frees ferrout linked list | vasprintf'd strings never freed otherwise |
| pr.c | free(indy) added in cleanup path | calloc'd index array was leaking |
| pr.c | All `exit(1)` -> `exit(10)` | RETURN_ERROR for AmigaOS |
| pr.c | `_exit(1)` -> `_exit(10)` | Signal handler exit path |
| pr.c | `pledge()` stubbed | No-op macro |
| pr.c | Added `__stack = 32768` | Larger stack for multi-column formatting |
| pr.c | Added `$VER` string | AmigaOS version identification |
| pr.c | Added atexit cleanup | Free argv, ferrout list, flush stdout |

## Build Configuration

| Setting | Value |
|---------|-------|
| Stack size | 32768 bytes |
| CFLAGS | `-std=gnu99` (strtonum uses long long) |
| Link libraries | libamiport.a (posix-shim) |
| Objects | pr.o, egetopt.o |
| Special flags | AMIPORT_NO_STAT_MACROS in stat.h include |

## Test Results

- **Total tests:** 32
- **Passed:** Needs retest after memory/perf fixes
- **Pass rate:** Pending retest
- **Notable findings:** Multi-column output, merge mode, and header formatting
  all verified. The egetopt parser correctly handles pr's extended option
  syntax (e.g., -2 for two columns, +5 for start at page 5).

## Memory Safety

**Verdict: CLEAN (after fixes).** Two memory issues were identified and fixed:

1. **ferrout linked list leak (FIXED):** The `ferrout()` function builds a
   linked list of vasprintf'd error messages for deferred output. The list
   nodes and their `buf` strings were never freed. Fixed by adding cleanup
   code in the atexit handler that walks the list and frees each node.

2. **indy index array leak (FIXED):** The `indy` array allocated via
   `calloc()` in column formatting was not freed when the function returned.
   Fixed by adding `free(indy)` in the cleanup path.

## Performance Notes

**HIGH finding from perf-optimizer:** The pr utility processes files
character-at-a-time in several formatting paths. For large files with many
pages, the overhead of individual fgetc() calls through libnix is measurable
on 68000. However, the formatting logic is tightly coupled to per-character
state (column tracking, tab expansion, line counting) making bulk fgets()
replacement impractical without restructuring.

Stack usage is within bounds at 32768 bytes. The largest local buffers are
the page formatting arrays allocated via calloc (heap), not stack.

## Known Limitations

- **Terminal width detection:** pr uses a hardcoded default of 72 columns.
  The `-w` flag overrides this. No automatic terminal width detection.
- **Form feed handling:** AmigaOS console does not process form feed (0x0C)
  characters the same way as Unix terminals. Page breaks in output appear as
  blank lines rather than actual page ejects.
- **Signal handling approximate:** SIGINT is caught to flush output before
  exit, but AmigaOS signal delivery differs from POSIX. The signal handler
  uses `_exit(10)` for the safest exit path.
- **File date in headers:** Uses `amiport_stat()` to get modification time
  for the default header date. AmigaOS epoch (1978) is correctly offset.
