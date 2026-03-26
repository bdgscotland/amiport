# Port: col

## Overview

| Field | Value |
|-------|-------|
| Program | col |
| Version | 1.20 (port revision: 1) |
| Source | OpenBSD usr.bin/col (v1.20, Dec 2022) |
| Category | 1 -- CLI tool |
| License | BSD-3-Clause |
| Original Author | Michael Rendell (Memorial University of Newfoundland), The Regents of the University of California |
| Port Date | 2026-03-26 |

## Description

col filters reverse line feeds and other special control sequences from input. It is most commonly used to post-process nroff/troff output, stripping bold overprinting (char-BS-char), underline overprinting (underscore-BS-char), and reverse line feeds (ESC-7, ESC-8, ESC-9, VT). With `-b`, backspace overprinting is resolved to the last character at each position. With `-x`, space-to-tab compression is suppressed. The program buffers lines internally and uses a free-list memory pool to manage LINE structures efficiently.

## Prior Art on Aminet

No existing col port found on Aminet for AmigaOS 3.x.

## Portability Analysis

Verdict: **PORTABLE** -- Single-file program with standard POSIX dependencies. The main challenges were OpenBSD-specific functions (strtonum, reallocarray, err/errx) and the line buffer memory management requiring atexit cleanup for AmigaOS.

| Issue | Tier | Resolution |
|-------|------|------------|
| `<stdlib.h>` | Tier 1 | Replaced with `<amiport/stdlib.h>` for exit() macro |
| `<unistd.h>` | Tier 1 | Replaced with `<amiport/unistd.h>` |
| `<err.h>` (err/errx/warn/warnx) | Tier 1 | Replaced with `<amiport/err.h>` shim |
| `<getopt.h>` (getopt) | Tier 1 | Replaced with `<amiport/getopt.h>` -- libnix getopt_long is broken |
| `strtonum()` | Tier 1 | Provided via `<amiport/err.h>` shim |
| `reallocarray()` | Tier 1 | Provided via `<amiport/string.h>` macro to `amiport_reallocarray()` |
| `pledge()` | Stub | Macro stub: `#define pledge(p, e) (0)` |
| Exit codes | Tier 1 | All `exit(1)` / `err(1, ...)` / `errx(1, ...)` changed to `exit(10)` / `err(10, ...)` / `errx(10, ...)` |
| Wildcard expansion | Tier 1 | Added `amiport_expand_argv()` -- Amiga shell does not glob |
| Line buffer cleanup | N/A | Added `cleanup()` function with `cleanup_lines()` to free line buffers on error exit paths |

## Transformations Applied

| File | Line(s) | Original | Transformed | Comment |
|------|----------|----------|-------------|---------|
| col.c | 37 | `<err.h>` | `<amiport/err.h>` | libnix has no err/errx/warn/strtonum |
| col.c | 40 | `<stdlib.h>` | `<amiport/stdlib.h>` | Activates exit() macro |
| col.c | 41 | `<unistd.h>` | `<amiport/unistd.h>` | Amiga unistd shim |
| col.c | 42 | (none) | `<amiport/getopt.h>` | Broken libnix getopt workaround |
| col.c | 43 | (none) | `<amiport/string.h>` | Provides reallocarray() macro |
| col.c | 44 | (none) | `<amiport/glob.h>` | Provides amiport_expand_argv() and __progname |
| col.c | 48 | (none) | `$VER: col 1.20 (26.03.2026)` | Amiga version string |
| col.c | 51 | (none) | `LONG __stack = 32768` | Stack cookie -- 32 KB for line buffer malloc/realloc |
| col.c | 54 | (none) | `#define pledge(p, e) (0)` | Stub OpenBSD pledge() |
| col.c | 110 | `err(1, "stdout")` | `err(10, "stdout")` | Amiga RETURN_ERROR in PUTC macro |
| col.c | 112-128 | (none) | `cleanup()` function | atexit: frees all line buffers, argv, flushes stdout |
| col.c | 147-149 | (none) | `amiport_expand_argv()` + `atexit(cleanup)` | Wildcard expansion + cleanup |
| col.c | 152 | `err(1, "pledge")` | `err(10, "pledge")` | Amiga RETURN_ERROR |
| col.c | 171 | `errx(1, "bad -l argument")` | `errx(10, "bad -l argument")` | Amiga RETURN_ERROR |
| col.c | 519 | `errx(1, "too many lines")` | `errx(10, "too many lines")` | Amiga RETURN_ERROR |
| col.c | 522 | `errx(1, "too many reverse line feeds")` | `errx(10, "too many reverse line feeds")` | Amiga RETURN_ERROR |
| col.c | 563 | `reallocarray(p, n, size)` | same (shimmed via macro) | reallocarray -> amiport_reallocarray |
| col.c | 564 | `err(1, NULL)` | `err(10, "realloc failed")` | Amiga RETURN_ERROR with explicit message |
| col.c | 572 | `exit(1)` | `exit(10)` | Amiga RETURN_ERROR in usage() |

## Shim Functions Exercised

- `amiport_err()`, `amiport_errx()`, `amiport_warnx()`
- `amiport_strtonum()`
- `amiport_reallocarray()`
- `amiport_expand_argv()`, `amiport_free_argv()`
- `amiport_getopt()`

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68000+ |
| CFLAGS | `-O2 -noixemul -m68000 -Wall` |
| LDFLAGS | `-lamiport` |
| Stack cookie | 32768 (32 KB) |
| VAMOS_STACK | default |

## Test Results

28 tests in `test-fsemu-cases.txt`. Categories tested:

| Category | Count | Description |
|----------|-------|-------------|
| Flag -b | 3 | Bold overprint, underline overprint, CR overwrite |
| Flag -f | 1 | Half-line-feed preservation in output |
| Flag -h | 1 | Explicit tab compression (same as default) |
| Flag -l | 2 | Custom buffer size (-l 10, -l 1) |
| Flag -x | 1 | Space-to-tab compression disabled |
| Plain passthrough | 3 | Text with no reverse LF, multi-line, default tab compression |
| Error paths | 4 | Unknown flag, -l 0, -l abc, extra file argument |
| Edge cases | 4 | Empty input, ESC-7 reverse LF, ESC-9 half LF, SO/SI character sets |
| Real-world nroff | 4 | Simulated man page with bold headings, description, underlined words |
| Stress | 4 | 500-line file (first/last line), -l 500 large buffer (first/last line) |
| Precision | 1 | Exact 9-space gap preservation with -x |
| Amiga-specific | 1 | WORK: volume path resolution |

Notable edge cases tested:
- Bold overprint (char-BS-char) correctly resolved with -b
- ESC-7 reverse line feed backs up one full line, overprinting correctly
- ESC-9 half line feed rounds up without -f, preserved with -f
- SO/SI character set shifts pass through in output
- 500-line stress test exercises free-list pool allocation (64-line chunks) and buffer flush logic (max_bufd_lines=256)

Note: col reads from stdin only. All tests use `test-col-run.rexx` (ARexx wrapper) to redirect input files via Execute script, since the test harness cannot use `<` redirection.

## Memory Safety

**Verdict: CRITICAL LEAKS FOUND -- Fixed with cleanup_lines().**

Memory-checker audit (2026-03-26) found four distinct allocation patterns:

| Leak | Location | Size | Fixable? | Status |
|------|----------|------|----------|--------|
| Line freelist | `alloc_line()` allocates 64 LINE structs at once; original pointer never freed | ~2560 bytes | Difficult | Accepted -- pool recycling pattern |
| sorted[] buffer | Static pointer in `flush_line()`, never freed | ~4 KB | Difficult | Accepted -- function-scoped static |
| count[] buffer | Static pointer in `flush_line()`, never freed | ~512 bytes | Difficult | Accepted -- function-scoped static |
| l->l_line buffers on error paths | LINE character buffers leaked when err/errx exits before flush_lines() | ~10-50 KB | **Yes** | **Fixed** |

**Fix applied:** The `cleanup()` function includes `cleanup_lines()` which walks the `lines` linked list and frees all `l->l_line` character buffers on error exit paths (errx from addto_lineno overflow, bad -l argument, etc.). This addresses the largest leak (~10-50 KB depending on input size).

The remaining leaks (freelist pool ~2560 bytes, sorted/count ~4.5 KB) total ~7 KB and are inherent to the program's architecture -- the freelist uses a slab-allocation pattern where the original block pointer is lost after recycling, and sorted/count are function-scoped statics inaccessible from the cleanup function. These are acceptable for a single-invocation CLI tool.

## Performance Notes

Perf-optimizer review (2026-03-26) -- character-at-a-time I/O is the primary bottleneck.

- **HIGH** [getchar() per-byte input] -- Main loop calls `getchar()` once per input byte. On 68000, each call is ~10-15 cycles of libnix function overhead. For a 100 KB document, that is 100K+ function dispatches. A `fread()`-into-buffer refactor would give ~2-3x throughput improvement but is architecturally invasive -- the escape sequence handling (ESC + follow byte via nested `getchar()`) would need restructuring.
- **HIGH** [putchar() per-byte output] -- The `PUTC` macro calls `putchar()` per byte throughout `flush_line()` and `flush_blanks()`. Space padding, tab emission, and character output each generate individual `putchar()` calls. Collecting output into a buffer and using a single `fwrite()` per `flush_line()` call would eliminate hundreds of function calls per line. Estimate: 2-4x speedup in output-heavy scenarios.
- **MEDIUM** [modulo in tab compression] -- `last_col % 8` in tab emission uses DIVU (44 cycles on 68000). Could use `last_col & 7` (1-8 cycles). Minor per-tab improvement.

## Known Limitations

1. **Stdin only** -- col reads exclusively from standard input. It does not accept filename arguments (extra arguments cause usage error). All test input must be redirected via the ARexx wrapper script.
2. **Memory pool not freed** -- The LINE freelist pool (~2560 bytes) and sort buffers (~4.5 KB) are never freed. This is inherent to the slab-allocation design and acceptable for single invocations.
3. **Character-at-a-time I/O** -- The per-byte getchar/putchar pattern is the fundamental performance bottleneck. This is architectural and would require significant restructuring to improve.
4. **Half-line feed output** -- When `-f` is not used, half-line feeds (ESC-9) are rounded up to full lines. This matches the OpenBSD behavior but may produce slightly different spacing than the original nroff intent.

## Review

Reviewed by memory-checker and perf-optimizer agents. Score: **READY** (after cleanup_lines fix applied).
