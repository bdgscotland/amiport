# Port: paste

## Overview

| Field | Value |
|-------|-------|
| Program | paste |
| Version | 1.27 (port revision: 1) |
| Source | OpenBSD (GitHub mirror) |
| Category | 1 -- CLI |
| License | BSD-3-Clause |
| Original Author | Adam S. Moskowitz, UC Berkeley |
| Port Date | 2026-03-26 |

## Description

paste merges corresponding lines from multiple files side by side, separated by tab characters (or a custom delimiter specified with `-d`). In parallel mode (default), it reads one line from each file and joins them. In sequential mode (`-s`), it concatenates all lines from each file onto a single output line. Delimiters cycle when multiple characters are specified. This is useful for building columnar reports, CSV files, and joining data from multiple sources.

## Prior Art on Aminet

No existing standalone 68k port of OpenBSD paste found on Aminet. No equivalent columnar file-merging utility identified for AmigaOS. This port provides the full POSIX paste interface.

## Portability Analysis

Verdict: **EASY** -- Single file (305 lines), no Tier 2/3 blockers. Main portability issues are `<sys/queue.h>` SIMPLEQ macros (inlined) and `getline()` (shimmed).

| Issue | Tier | Resolution |
|-------|------|------------|
| `<sys/queue.h>` SIMPLEQ macros | Tier 1 | Inlined SIMPLEQ_HEAD, SIMPLEQ_ENTRY, SIMPLEQ_HEAD_INITIALIZER, SIMPLEQ_INSERT_TAIL, SIMPLEQ_FOREACH |
| `getline()` | Tier 1 | `<amiport/stdio_ext.h>` provides `amiport_getline()` |
| `ssize_t` | Tier 1 | Via `<amiport/unistd.h>` (defined as `long`) |
| `__dead` macro | BSD-ism | `#define __dead __attribute__((__noreturn__))` |
| `<err.h>` | Tier 1 | Replaced with `<amiport/err.h>` |
| `<stdlib.h>` | Tier 1 | Replaced with `<amiport/stdlib.h>` (exit macro) |
| `<unistd.h>` | Tier 1 | Replaced with `<amiport/unistd.h>` |
| `getopt()` | Tier 1 | `<amiport/getopt.h>` (libnix getopt_long is broken) |
| `pledge()` | Stub | `#define pledge(p, e) (0)` |
| `__progname` | Tier 1 | Provided by `amiport_expand_argv()` -- extern declaration only |
| `fclose(stdin)` risk | Safety | Guarded with `if (fp != stdin) fclose(fp)` in both parallel() and sequential() |
| Exit codes | Tier 1 | `err(1,...)` / `errx(1,...)` / `exit(1)` changed to 10 (RETURN_ERROR) |
| Wildcard globbing | Tier 1 | `amiport_expand_argv()` added |

## Transformations Applied

| File | Line(s) | Original | Transformed | Comment |
|------|----------|----------|-------------|---------|
| paste.c | 36-50 | `#include <sys/queue.h>` | Inline SIMPLEQ macros | 5 macros inlined at top of file |
| paste.c | 53 | (none) | `#define __dead __attribute__((__noreturn__))` | BSD macro stub |
| paste.c | 56 | (none) | `#define pledge(p, e) (0)` | OpenBSD sandbox stub |
| paste.c | 61 | `#include <err.h>` | `#include <amiport/err.h>` | amiport shim |
| paste.c | 66 | `#include <stdlib.h>` | `#include <amiport/stdlib.h>` | exit() macro |
| paste.c | 69 | `#include <unistd.h>` | `#include <amiport/unistd.h>` | amiport shim |
| paste.c | 71 | `#include <getopt.h>` | `#include <amiport/getopt.h>` | Working getopt |
| paste.c | 73 | (none) | `#include <amiport/stdio_ext.h>` | getline() shim |
| paste.c | 75 | (none) | `#include <amiport/glob.h>` | argv expansion + __progname |
| paste.c | 78 | (none) | `$VER: paste 1.27` with `__attribute__((used))` | Amiga version string |
| paste.c | 81 | (none) | `long __stack = 8192` | Stack cookie |
| paste.c | 92-97 | (none) | `cleanup()` function | atexit handler |
| paste.c | 107-109 | (none) | `amiport_expand_argv()` + `atexit(cleanup)` | Standard boilerplate |
| paste.c | 113 | `err(1, "pledge")` | `err(10, "pledge")` | Amiga RETURN_ERROR |
| paste.c | 161-162 | `ssize_t len` | `long len` | amiport_getline returns long |
| paste.c | 168 | `err(1, NULL)` | `err(10, NULL)` | Amiga RETURN_ERROR |
| paste.c | 173 | `err(1, "%s", p)` | `err(10, "%s", p)` | Amiga RETURN_ERROR |
| paste.c | 193,257 | `err(1, ...)` | `err(10, ...)` | Amiga RETURN_ERROR |
| paste.c | 198-199 | `fclose(lp->fp)` | `if (lp->fp != stdin) fclose(lp->fp)` | Guard against fclose(stdin) |
| paste.c | 260-262 | `fclose(fp)` | `if (fp != stdin) fclose(fp)` | Guard against fclose(stdin) |
| paste.c | 233-234 | `ssize_t len` | `long len` | amiport_getline returns long |
| paste.c | 294 | `errx(1, ...)` | `errx(10, ...)` | Amiga RETURN_ERROR |
| paste.c | 303 | `exit(1)` | `exit(10)` | Amiga RETURN_ERROR in usage() |

## Shim Functions Exercised

- `amiport_expand_argv()` -- wildcard expansion
- `amiport_free_argv()` -- free expanded argv in atexit cleanup
- `amiport_exit()` -- via exit macro
- `amiport_err()`, `amiport_errx()`, `amiport_warn()` -- error reporting
- `amiport_getopt()` -- option parsing
- `amiport_getline()` -- line reading (via getline macro)

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68000+ |
| CFLAGS | `-O2 -noixemul -m68000 -Wall` |
| Libraries | `-lamiport` (posix-shim) |
| Stack size | 8192 bytes |
| Binary size | 37 KB |

## Test Results

Tested via FS-UAE (A1200, Kickstart 3.1). ARexx harness, TAP output.

| Metric | Value |
|--------|-------|
| Total tests | 24 |
| Passed | 24 |
| Failed | 0 |
| Pass rate | 100% |

Categories tested: default parallel mode (2 files, 3 files, short file padding), `-d` flag (comma, colon, cyclic two-char, explicit tab `\t`, null byte `\0`), `-s` sequential mode (single file, multiple files, with custom delimiter, cyclic delimiters), error paths (invalid flag RC=10, nonexistent file RC=10), edge cases (empty file, single file), Amiga WORK: paths, real-world scenarios (CSV construction, cyclic delimiters with padding), stress tests (20-line parallel, 20-line sequential, 3-file parallel), precision (`-d\n` newline delimiter in sequential mode).

## Memory Safety

No dedicated memory-checker report generated. Source analysis indicates:
- argv expansion freed via `atexit(cleanup)` on all exit paths
- `getline()` buffer allocated once per function, freed with `free(line)` at function end
- On `err()` exit paths, the getline buffer and `lp` structs leak (minor, single-invocation program)
- `lp` structs from `parallel()` malloc are never freed (program exits after function returns)
- `fclose(stdin)` correctly guarded in both parallel() and sequential()

**Verdict: ACCEPTABLE** -- Minor leaks on error paths are inherent to the upstream design. Not a concern for a single-invocation CLI tool on AmigaOS.

## Performance Notes

Reviewed by perf-optimizer agent (2026-03-26). Key findings:

- **MEDIUM: modulo in parallel inner loop** -- Three `% delimcnt` operations per iteration. On 68000, DIVU is 76-140 cycles. In the common single-delimiter case, the modulo is always 0. Multi-delimiter inputs with many files are rare in practice. Estimated LOW-MEDIUM impact.
- **LOW: putchar() per delimiter** -- One putchar call per column delimiter. Dominated by getline() I/O cost.
- **NO ISSUES: getline() + fputs() I/O pattern** -- Correctly line-buffered, no fgetc per-byte loops.
- **Primary bottleneck: I/O-bound** (disk reads + stdout writes). No critical performance issues found.

## Known Limitations

1. **`-` for stdin** -- Using `-` as a filename to read from stdin works but stdin can only be used once in the file list. Multiple `-` arguments will read from the same stream, which may produce unexpected interleaving.
2. **Minor memory leaks on error paths** -- getline buffer and SIMPLEQ list nodes are not freed when err() terminates the program. Acceptable for a single-invocation CLI tool.
