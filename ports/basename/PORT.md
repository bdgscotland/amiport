# Port: basename

## Overview

| Field | Value |
|-------|-------|
| Program | basename |
| Version | 1.14 (port revision: 1) |
| Source | OpenBSD basename.c v1.14 (2016-10-28) |
| Category | 1 -- CLI tool |
| License | BSD-3-Clause |
| Original Author | The Regents of the University of California |
| Port Date | 2026-03-25 |

## Description

basename strips directory and suffix components from filenames. Given a pathname, it removes any prefix ending in `/` and optionally removes a trailing suffix. Essential for shell scripting.

## Prior Art on Aminet

No 68k AmigaOS port found on Aminet. GNU coreutils packages exist but are PPC-only (AmigaOS 4.x). The jffabre Unix commands collection may have included basename but the source is lost (site offline).

## Portability Analysis

Verdict: **EASY** -- 76 lines, pure C89, no Tier 2/3 issues.

| Issue | Tier | Resolution |
|-------|------|------------|
| `err.h` | 1 | `<amiport/err.h>` |
| `pledge()` | 1 | Stubbed as macro `#define pledge(p, e) (0)` |
| `__dead` attribute | 1 | Removed (OpenBSD-specific) |
| `__progname` extern | 1 | Defined directly to prevent weak symbol stripping |
| `exit(1)` / `err(1,...)` | 1 | Changed to `exit(10)` / `err(10,...)` (RETURN_ERROR) |

## Transformations Applied

| Line(s) | Original | Transformed | Comment |
|----------|----------|-------------|---------|
| 34 | `<err.h>` | `<amiport/err.h>` | AmigaOS err shim |
| 38 | `<stdlib.h>` | `<amiport/stdlib.h>` | Activates exit macro |
| 41 | `<unistd.h>` | `<amiport/unistd.h>` | AmigaOS unistd |
| 43 | (added) | `<amiport/glob.h>` | Argv expansion |
| 46-49 | (added) | `$VER` string, `__stack` | AmigaOS boilerplate |
| 53 | `extern __progname` | `char *__progname = "basename"` | Weak symbol fix |
| 56 | (added) | `#define pledge(p, e) (0)` | Stub |
| 58 | `static void __dead usage` | `static void usage` | Removed __dead |
| 60-65 | (added) | `cleanup()` + `atexit()` | Exit path cleanup |
| 74 | (added) | `amiport_expand_argv()` | Wildcard expansion |
| 79,100 | `err(1,...)` / `exit(1)` | `err(10,...)` / `exit(10)` | AmigaOS exit codes |

## Shim Functions Exercised

- `amiport_err()` / `amiport_errx()` -- error reporting with AmigaOS exit codes
- `amiport_exit()` -- exit with cleanup
- `amiport_expand_argv()` / `amiport_free_argv()` -- wildcard expansion
- `amiport_getopt()` -- option parsing

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68000+ |
| CFLAGS | `-O2 -noixemul -m68000 -Wall` |
| Libraries | `-lamiport` (posix-shim) |
| Binary size | 34,028 bytes |

## Test Results

Tested via vamos and FS-UAE. See test-fsemu-cases.txt.

## Platform Compatibility Notes

No platform compatibility issues. Pure scalar C with no alignment concerns, no struct returns, no 64-bit types.

## Known Limitations

- Only handles `/` as path separator. AmigaOS paths with `:` (e.g., `Work:file.txt`) are treated as a single filename component. This matches POSIX behavior but differs from what an Amiga-native implementation might do.

## Review

TBD
