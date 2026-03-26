# Port: printf

## Overview

| Field | Value |
|-------|-------|
| Program | printf |
| Version | 1.28 (port revision: 1) |
| Source | OpenBSD (GitHub mirror) |
| Category | 1 -- CLI |
| License | BSD-3-Clause |
| Original Author | University of California, Berkeley |
| Port Date | 2026-03-26 |

## Description

printf formats and prints data according to a format string, supporting all standard conversion specifiers (%d, %s, %f, %x, etc.), field widths, precision, flag characters, and escape sequences. It reuses the format string to consume excess arguments, making it useful for generating structured output in shell scripts. This is the OpenBSD implementation, which includes the %b (echo-style escape) extension.

## Prior Art on Aminet

No existing standalone 68k port of printf found on Aminet. The AmigaDOS shell has no built-in printf equivalent -- only Echo with limited formatting.

## Portability Analysis

Verdict: **SIMPLE** -- Single-file, no external dependencies beyond libc. Only POSIX removals (pledge, unistd.h) and exit code adjustments needed.

| Issue | Tier | Resolution |
|-------|------|------------|
| `<unistd.h>` | Tier 1 | Removed -- only used for pledge() |
| `pledge()` | Stub | Macro stub: `#define pledge(p, e) (0)` |
| `<err.h>` (err/errx/warn/warnx/warnc) | Tier 1 | Replaced with `<amiport/err.h>` |
| `<stdlib.h>` | Tier 1 | Replaced with `<amiport/stdlib.h>` (exit macro) |
| `__dead` attribute | Tier 1 | Defined as `__attribute__((__noreturn__))` |
| Exit codes | Tier 1 | `exit(1)` -> `exit(10)` (RETURN_ERROR) |
| Wildcard expansion | Tier 1 | `amiport_expand_argv()` via `<amiport/glob.h>` |
| `strtod()` / floating-point printf | Tier 1 | Available in libnix; requires `-lm` |

## Transformations Applied

| File | Line(s) | Original | Transformed | Comment |
|------|----------|----------|-------------|---------|
| printf.c | 33 | `#include <err.h>` | `#include <amiport/err.h>` | amiport shim |
| printf.c | 37 | `#include <stdlib.h>` | `#include <amiport/stdlib.h>` | exit() macro |
| printf.c | 39 | `#include <unistd.h>` | removed | Only used for pledge() |
| printf.c | 40 | (none) | `#include <amiport/glob.h>` | Wildcard expansion |
| printf.c | 43-44 | `pledge()` call | `#define pledge(p, e) (0)` | Stub |
| printf.c | 46 | `__dead` | `#define __dead __attribute__((__noreturn__))` | GCC attribute |
| printf.c | 49 | (none) | `$VER: printf 1.28 (26.03.2026)` | Amiga version string |
| printf.c | 52 | (none) | `long __stack = 8192` | Stack cookie |
| printf.c | 71-76 | (none) | `cleanup()` function | atexit handler for argv/fflush |
| printf.c | 104 | (none) | `amiport_expand_argv()` | Amiga shell glob |
| printf.c | 106 | (none) | `atexit(cleanup)` | All exit paths cleanup |
| printf.c | 109 | `err(1, "pledge")` | `err(10, "pledge")` | RETURN_ERROR |
| printf.c | 179 | `return(1)` | `return(10)` | RETURN_ERROR |
| printf.c | 236 | `return(1)` | `return(10)` | RETURN_ERROR |
| printf.c | 380 | `rval = 1` | `rval = 10` | RETURN_ERROR |
| printf.c | 386 | `rval = 1` | `rval = 10` | RETURN_ERROR |
| printf.c | 511 | `rval = 1` | `rval = 10` | RETURN_ERROR |
| printf.c | 522 | `exit(1)` | `exit(10)` | RETURN_ERROR |

## Shim Functions Exercised

- `amiport_err()`, `amiport_errx()`, `amiport_warn()`, `amiport_warnx()`, `amiport_warnc()`
- `amiport_expand_argv()`, `amiport_free_argv()`
- `amiport_exit()` (via exit() macro)

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68000+ |
| CFLAGS | `-O2 -noixemul -m68000 -Wall` |
| LDFLAGS | `-lamiport -lm` |
| Libraries | libamiport.a (posix-shim), libm.a (math) |
| Stack size | 8192 bytes |
| Binary size | ~51 KB |

## Test Results

Tested via FS-UAE (Category 1 CLI tool). 51 tests, 100% pass rate.

| Category | Count | Status |
|----------|-------|--------|
| Basic string/escape output | 2 | PASS |
| Integer format specifiers (%d, %i, %o, %u, %x, %X) | 6 | PASS |
| Float format specifiers (%f, %e, %g, %G) | 5 | PASS |
| String/char format specifiers (%s, %c) | 2 | PASS |
| Width and precision | 7 | PASS |
| Flag characters (#, +) | 3 | PASS |
| Multiple arguments and format reuse | 4 | PASS |
| %b echo-style escape processing | 4 | PASS |
| Numeric argument conversions (hex, octal, quoted char) | 3 | PASS |
| Escape sequences (%%) | 1 | PASS |
| -- separator | 1 | PASS |
| Error paths (no args, invalid directive) | 2 | PASS |
| Edge cases (empty string, zero, default args) | 3 | PASS |
| Amiga-specific (WORK: paths) | 1 | PASS |
| Real-world usage | 2 | PASS |
| Stress tests | 5 | PASS |

## Memory Safety

No agent-memory from memory-checker found. Manual analysis: single-file program with no dynamic allocation beyond `realloc()` for mklong() static buffer (never freed -- acceptable, reused across calls). `amiport_expand_argv()` cleaned up via atexit handler. No memory leaks on normal or error exit paths.

## Performance Notes

Simple format-processing loop with no hot paths. The mklong() function uses a static buffer with geometric growth -- avoids repeated malloc. No performance concerns on 68k.

## Known Limitations

1. **Dynamic width/precision untestable on AmigaDOS** -- The `%*d` and `%.*f` format specifiers work in the code, but cannot be tested via the FS-UAE harness because AmigaDOS expands `*` as a wildcard before printf receives the argument.
2. **`%a`/`%A` hex float format** -- Depends on libnix printf support for hex float notation. Untested but present in the code path.
