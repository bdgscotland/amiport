# Port: column

## Overview

| Field | Value |
|-------|-------|
| Program | column |
| Version | 1.27 |
| Source | OpenBSD usr.bin/column (v1.27, Dec 2022) |
| Category | 1 -- CLI tool |
| License | BSD 3-Clause |
| Original Author | The Regents of the University of California |
| Port Date | 2026-03-26 |

## Description

column formats lists of items into multiple columns for display, similar to how `ls` formats directory listings. It supports three output modes: row-first fill (`-x`), column-first fill (default), and table alignment (`-t`) with custom field separators (`-s`). This is the OpenBSD implementation, a clean single-file BSD-licensed utility.

## Prior Art on Aminet

Researched via aminet-researcher agent. No modern standalone noixemul column port found on Aminet. This port provides a fresh, dependency-free binary.

## Portability Analysis

Verdict: **MODERATE** -- Single file, but heavy use of wchar/locale functions (mbtowc, wcschr, wcwidth, iswspace) that do not exist on AmigaOS 3.x.

| Issue | Tier | Resolution |
|-------|------|------------|
| `<locale.h>` / `setlocale()` | Tier 3 | Removed -- no locale support on AmigaOS |
| `<wchar.h>` / `mbtowc()` / `wcschr()` / `wcwidth()` | Tier 3 | Replaced with single-byte ASCII loops using `strchr()` / `isspace()` |
| `<wctype.h>` / `iswspace()` | Tier 3 | Replaced with `isspace()` on unsigned char cast |
| `<sys/ioctl.h>` / `TIOCGWINSZ` | Tier 1 | `amiport_ioctl()` from `<amiport/unistd.h>` (CSI query) |
| `getopt()` | Tier 1 | `<amiport/getopt.h>` |
| `getline()` | Tier 1 | `<amiport/stdio_ext.h>` |
| `reallocarray()` | Tier 1 | `<amiport/string.h>` macro to `amiport_reallocarray()` |
| `err()` / `errx()` / `warn()` | Tier 1 | `<amiport/err.h>` |
| `strtonum()` | Tier 1 | `<amiport/err.h>` |
| `getenv("COLUMNS")` | Tier 1 | `amiport_getenv()` (returns malloc'd string -- freed after use) |
| `pledge()` / `unveil()` | Stub | `#define pledge(p, e) (0)` |
| `__dead` annotation | Stub | Removed (OpenBSD-specific) |
| Exit codes | Tier 1 | `exit(1)` -> `exit(10)` (RETURN_ERROR) |
| `wchar_t *separator` | Tier 3 | Replaced with `char *separator` |

## Transformations Applied

| File | Line(s) | Original | Transformed | Comment |
|------|----------|----------|-------------|---------|
| column.c | 34 | (none) | `$VER: column 1.27` | Amiga version string |
| column.c | 37 | (none) | `long __stack = 16384` | Stack cookie |
| column.c | 39-43 | `<sys/types.h>`, `<sys/ioctl.h>`, `<locale.h>`, `<wchar.h>`, `<wctype.h>` | removed | Not available on AmigaOS |
| column.c | 49 | `<stdlib.h>` | `<amiport/stdlib.h>` | Activates exit() macro |
| column.c | 51 | `<unistd.h>` | `<amiport/unistd.h>` | Provides amiport_ioctl + TIOCGWINSZ |
| column.c | 53-57 | (none) | amiport headers | err.h, getopt.h, stdio_ext.h, string.h, glob.h |
| column.c | 60-61 | (none) | `#define pledge/unveil` | Stubbed as no-ops |
| column.c | 83 | `wchar_t *separator` | `char *separator` | No wchar on AmigaOS |
| column.c | 85-98 | (none) | `cleanup()` + `input_buf`/`sep_alloc` tracking | atexit cleanup for getline buffer and strdup'd separator |
| column.c | 112-115 | (none) | `amiport_expand_argv()` + `atexit(cleanup)` | Wildcard expansion and cleanup registration |
| column.c | 121-125 | `getenv("COLUMNS")` | `amiport_getenv()` + `free()` | Malloc'd return value freed after use |
| column.c | 127-129 | `ioctl(STDOUT_FILENO, TIOCGWINSZ, &win)` | `amiport_ioctl()` | CSI-based console size query |
| column.c | 146-150 | `mbstowcs()`/`wchar_t` separator conversion | `strdup(optarg)` | Plain char* -- no wchar |
| column.c | 294-384 | `mbtowc()`/`wcschr()`/`wcwidth()` loops | `strchr()`/`isspace()` byte loops | Single-byte processing |
| column.c | all | `exit(1)` / `err(1,...)` | `exit(10)` / `err(10,...)` | RETURN_ERROR |

## Shim Functions Exercised

- `amiport_err()`, `amiport_errx()`, `amiport_warn()`
- `amiport_strtonum()`
- `amiport_reallocarray()`
- `amiport_getenv()` (malloc'd return -- freed)
- `amiport_ioctl()` with `TIOCGWINSZ`
- `amiport_getline()`
- `amiport_expand_argv()`, `amiport_free_argv()`

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68000+ |
| CFLAGS | `-O2 -noixemul -m68000 -Wall` |
| Stack | 16384 bytes |
| Libraries | `-lamiport` (Tier 1 posix-shim) |
| Binary size | ~44KB |

## Test Results

**25/25 passed** (100%) on FS-UAE (A1200, Kickstart 3.1).

| Category | Count | Description |
|----------|-------|-------------|
| Functional (default/r_columnate) | 2 | Default column-first fill |
| Functional (-x/c_columnate) | 2 | Row-first fill mode |
| Functional (-c width) | 3 | Custom terminal width |
| Functional (-t table) | 3 | Table alignment mode |
| Functional (-s separator) | 1 | Custom field separator |
| Amiga-specific | 1 | Multiple WORK: file arguments |
| Edge cases | 3 | Empty file, whitespace-only, long items |
| Error paths | 3 | Invalid flag, missing file, -c 0 |
| Real-world | 3 | Narrow terminal, CSV table, -x fill |
| Stress | 4 | 500-item file through all three modes |

## Memory Safety

- `input_buf` (getline buffer) tracked at file scope and freed via `atexit(cleanup)`
- `sep_alloc` tracks strdup'd separator for cleanup
- `amiport_getenv("COLUMNS")` result freed immediately after use
- `amiport_free_argv()` called in cleanup for all exit paths including `err()`/`errx()`

## Performance Notes

No critical performance findings. The wchar-to-byte simplification is itself a performance win -- `strchr()` and `isspace()` are significantly faster than `mbtowc()`/`wcschr()` on 68000.

## Known Limitations

1. **No multibyte/Unicode support** -- All wchar_t processing replaced with single-byte ASCII/Latin-1. Characters outside the 8-bit range are not handled. This is inherent to AmigaOS 3.x.
2. **No locale-aware collation** -- `setlocale()` removed; sorting and comparison use C locale (byte-value order).
3. **Terminal width detection** -- Uses CSI window size query via `amiport_ioctl()`. Falls back to 80 columns if the query fails (e.g., when output is redirected).

## Review

Reviewed with `/review-amiga`. Score: **READY**.

| Dimension | Score |
|-----------|-------|
| Stack safety | OK (16KB cookie, no large local buffers) |
| Memory handling | OK (atexit cleanup for getline buf, separator strdup, argv) |
| Path handling | OK (WORK: volume paths, multiple file arguments) |
| Library cleanup | OK (no direct AmigaOS lib opens) |
| Conventions | OK (version string, exit codes, stack cookie) |
