# Port: colrm

## Overview

| Field | Value |
|-------|-------|
| Program | colrm |
| Version | 1.14 |
| Source | OpenBSD usr.bin/colrm (v1.14, December 2022) |
| Category | 1 -- CLI tool |
| License | BSD-3-Clause |
| Original Author | The Regents of the University of California |
| Port Date | 2026-03-26 |

## Description

colrm removes selected columns from each line of standard input. Given a start column, it removes everything from that column to end of line. Given start and stop columns, it removes only that range and preserves the rest. Tab characters are handled correctly, expanding to their column positions rather than treating them as single characters. colrm is commonly used in shell pipelines to trim fixed-width output or strip fields from columnar data.

## Prior Art on Aminet

No existing colrm port found on Aminet for AmigaOS 3.x.

## Portability Analysis

Verdict: **MODERATE** -- Single-file port with locale/wchar dependencies that must be removed, plus standard POSIX shim requirements (getline, err, getopt).

| Issue | Tier | Resolution |
|-------|------|------------|
| `<stdlib.h>` | Tier 1 | Replaced with `<amiport/stdlib.h>` (exit macro) |
| `<unistd.h>` | Tier 1 | Replaced with `<amiport/unistd.h>` |
| `<err.h>` (err/errx) | Tier 1 | Replaced with `<amiport/err.h>` (shimmed) |
| `<locale.h>` / `setlocale()` | Tier 1 | Removed -- setlocale() is a no-op stub in amiport |
| `<wchar.h>` / `mbtowc()` / `wcwidth()` | Tier 3 | Compiled out with `#ifndef __AMIGA__` -- ASCII-only path |
| `getline()` | Tier 1 | Provided via `<amiport/stdio_ext.h>` macro |
| `getopt()` / `optind` | Tier 1 | Used `<amiport/getopt.h>` (libnix getopt_long is broken) |
| `pledge()` | Stub | Macro stub: `#define pledge(p, e) (0)` |
| `ssize_t` return for getline | Tier 1 | Changed to `size_t` to match amiport_getline() signature |
| Exit codes | Tier 1 | `exit(1)` / `err(1,...)` / `errx(1,...)` changed to exit(10) / err(10,...) / errx(10,...) |

## Transformations Applied

| File | Line(s) | Original | Transformed | Comment |
|------|----------|----------|-------------|---------|
| colrm.c | 34 | (none) | `static const char *verstag = "$VER: colrm 1.14 (26.03.2026)";` | Amiga version string |
| colrm.c | 37 | (none) | `long __stack = 16384;` | Stack cookie (16 KB) |
| colrm.c | 38 | `#include <locale.h>` | removed | setlocale() is a no-op on AmigaOS |
| colrm.c | 40 | `#include <stdlib.h>` | `#include <amiport/stdlib.h>` | Activates exit() macro |
| colrm.c | 42 | `#include <unistd.h>` | `#include <amiport/unistd.h>` | POSIX shim |
| colrm.c | 43 | (none) | `#include <amiport/getopt.h>` | libnix getopt_long broken |
| colrm.c | 44 | `#include <wchar.h>` | removed | No wchar on AmigaOS 3.x |
| colrm.c | 35 | `#include <err.h>` | `#include <amiport/err.h>` | Shimmed err/errx |
| colrm.c | 57-59 | (none) | `#include <amiport/stdio_ext.h>` / `#include <amiport/glob.h>` | getline() and argv expansion |
| colrm.c | 64 | (none) | `#define pledge(p, e) (0)` | Stub |
| colrm.c | 70-82 | (none) | `cleanup()` function with getline_buf tracking | atexit cleanup for err/errx exit paths |
| colrm.c | 88 | `ssize_t linesz` | `size_t linesz` | Matches amiport_getline() signature |
| colrm.c | 93-94 | `wchar_t wc;` | `#ifndef __AMIGA__` guarded | wchar not available |
| colrm.c | 97-99 | (none) | `amiport_expand_argv()` + `atexit(cleanup)` | Wildcard expansion and cleanup registration |
| colrm.c | 101 | `setlocale(LC_CTYPE, "");` | removed | No locale support |
| colrm.c | 106 | `err(1, "pledge")` | `err(10, "pledge")` | RETURN_ERROR |
| colrm.c | 121-126 | `errx(1, ...)` | `errx(10, ...)` | RETURN_ERROR |
| colrm.c | 135 | `err(1, ...)` | `err(10, ...)` | RETURN_ERROR |
| colrm.c | 200-211 | `mbtowc(&wc, p, ...)` / `wcwidth(wc)` | `#ifndef __AMIGA__` / `len = 1; width = 1;` | ASCII-only: every byte has width 1 |
| colrm.c | 237 | (none) | `getline_buf = NULL;` | Prevent double-free in atexit cleanup |
| colrm.c | 239-241 | `err(1, ...)` | `err(10, ...)` | RETURN_ERROR for ferror |
| colrm.c | 249 | `exit(1)` | `exit(10)` | RETURN_ERROR in usage() |

## Shim Functions Exercised

- `amiport_err()`, `amiport_errx()`
- `amiport_getline()` (via `<amiport/stdio_ext.h>` macro)
- `amiport_expand_argv()`, `amiport_free_argv()`
- `amiport_exit()` (via `<amiport/stdlib.h>` macro)
- `getopt()`, `optind` (via `<amiport/getopt.h>`)

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68000+ |
| CFLAGS | `-O2 -noixemul -m68000 -Wall` |
| Libraries | `-lamiport` (Tier 1 posix-shim) |
| Stack size | 16384 bytes (16 KB) |
| Binary size | ~37 KB |

## Test Results

28 tests, all passing. Tested via FS-UAE (Category 1 CLI tool).

| Category | Count | Description |
|----------|-------|-------------|
| Functional | 7 | No-args passthrough, single start, start+stop range, start=1, start=stop, beyond line length |
| Multi-line | 1 | Varying-length lines shorter than start column |
| Tab handling | 3 | Tab expansion across column ranges, tab preservation before/after range |
| Edge cases | 5 | Empty file, single char with start=1/2/1-1, no trailing newline |
| Error paths | 4 | Column 0 invalid, non-numeric arg, stop < start, too many args |
| Amiga-specific | 2 | WORK: volume stdin redirect, shared test-empty.txt |
| Real-world / stress | 6 | Colon-delimited field removal, fixed-width trim, 300-line stress, 320-char long line, precision passthrough |

Notable edge cases tested: tab expansion at column boundaries, lines shorter than start column, file with no trailing newline, and a 320-character line with 120-column middle removal.

## Memory Safety

Verdict: **CLEAN** -- Single heap allocation pattern: `getline()` manages one buffer that grows as needed. The `getline_buf` pointer is tracked for `atexit` cleanup, with NULL-after-free to prevent double-free (the known pitfall from the rev port). `amiport_expand_argv()` is cleaned up via `amiport_free_argv()` in the atexit handler. All exit paths (including `err()`/`errx()`) are covered by atexit registration.

## Performance Notes

The main loop processes one character at a time via pointer arithmetic. For ASCII-only mode (AmigaOS), the inner loop is tight: `len = 1; width = 1;` with no function call overhead from `mbtowc()`/`wcwidth()`. The `getline()` buffer is reused across lines (no per-line allocation). Tab expansion uses bitwise arithmetic `(column + TAB) & ~(TAB - 1)` which is efficient on 68000.

## Known Limitations

1. **No multibyte/wchar support** -- The `mbtowc()`/`wcwidth()` code path is compiled out on AmigaOS. All characters are treated as single-byte ASCII with display width 1. Double-width CJK characters or UTF-8 input will produce incorrect column alignment.
2. **No locale support** -- `setlocale()` call is removed. Column counting assumes ASCII encoding.
