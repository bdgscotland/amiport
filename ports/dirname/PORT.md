# Port: dirname

## Overview

| Field | Value |
|-------|-------|
| Program | dirname |
| Version | 1.17 (port revision: 1) |
| Source | OpenBSD dirname.c v1.17 (2019-01-25) |
| Category | 1 -- CLI tool |
| License | ISC |
| Original Author | Todd C. Miller |
| Port Date | 2026-03-25 |

## Description

dirname strips the last component from a filename, returning the directory portion. Given `/usr/bin/grep`, it returns `/usr/bin`. Essential for shell scripting.

## Prior Art on Aminet

No 68k AmigaOS port found on Aminet. GNU coreutils packages exist but are PPC-only (AmigaOS 4.x). The jffabre Unix commands collection included a dirname with `--amiga` switch for colon-based path support, but the source is lost (site offline).

## Portability Analysis

Verdict: **EASY** -- 57 lines, pure C89, no Tier 2/3 issues.

| Issue | Tier | Resolution |
|-------|------|------------|
| `err.h` | 1 | `<amiport/err.h>` |
| `pledge()` | 1 | Stubbed as macro `#define pledge(p, e) (0)` |
| `__dead` attribute | 1 | Removed (OpenBSD-specific) |
| `__progname` extern | 1 | Defined directly to prevent weak symbol stripping |
| `exit(1)` / `err(1,...)` | 1 | Changed to `exit(10)` / `err(10,...)` (RETURN_ERROR) |
| `dirname()` input corruption | 1 | Safe -- result consumed immediately, argv[0] not reused |

## Transformations Applied

| Line(s) | Original | Transformed | Comment |
|----------|----------|-------------|---------|
| 20 | `<err.h>` | `<amiport/err.h>` | AmigaOS err shim |
| 24 | `<stdlib.h>` | `<amiport/stdlib.h>` | Activates exit macro |
| 26 | `<unistd.h>` | `<amiport/unistd.h>` | AmigaOS unistd |
| 28-30 | (added) | `<amiport/glob.h>`, `<amiport/getopt.h>` | Argv expansion, getopt |
| 33-36 | (added) | `$VER` string, `__stack` | AmigaOS boilerplate |
| 40 | `extern __progname` | `char *__progname = "dirname"` | Weak symbol fix |
| 43 | (added) | `#define pledge/unveil` | Stubs |
| 47 | `static void __dead usage` | `static void usage` | Removed __dead |
| 50-54 | (added) | `cleanup()` + `atexit()` | Exit path cleanup |
| 64 | (added) | `amiport_expand_argv()` | Wildcard expansion |
| 70,91,101 | `err(1,...)` / `exit(1)` | `err(10,...)` / `exit(10)` | AmigaOS exit codes |

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
| Binary size | 33,936 bytes |

## Test Results

Tested via vamos and FS-UAE. See test-fsemu-cases.txt.

## Platform Compatibility Notes

No platform compatibility issues. Pure scalar C with no alignment concerns, no struct returns, no 64-bit types.

dirname() from libnix modifies its input in-place (crash-patterns #18). Safe in this program because the result is consumed by puts(dir) before argv[0] is accessed again.

## Known Limitations

- Only handles `/` as path separator. AmigaOS paths with `:` (e.g., `Work:data/file.txt`) return `.` instead of `Work:data`. This matches POSIX behavior but differs from what an Amiga-native implementation might do.

## Review

TBD
