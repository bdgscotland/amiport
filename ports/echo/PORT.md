# Port: echo

## Overview

| Field | Value |
|-------|-------|
| Program | echo |
| Version | 1.12 (port revision: 1) |
| Source | OpenBSD (GitHub mirror) |
| Category | 1 -- CLI |
| License | BSD-3-Clause |
| Original Author | University of California, Berkeley |
| Port Date | 2026-03-26 |

## Description

echo writes its arguments to standard output, separated by spaces, followed by a newline. It supports `-n` to suppress the trailing newline, `-e` to enable interpretation of backslash escape sequences (including octal `\0nnn`, hex `\xhh`, `\n`, `\t`, `\c` to stop output, and others), and `-E` to disable escape processing. Unlike most utilities, echo does not use getopt -- flags are parsed manually, and unrecognized flags are printed as regular arguments per POSIX.

## Prior Art on Aminet

No existing standalone 68k port of OpenBSD echo found on Aminet. AmigaDOS has a built-in `Echo` command, but it lacks `-e` escape sequence processing and `-n` newline suppression. This port provides the full BSD echo feature set.

## Portability Analysis

Verdict: **EASY** -- Single file (194 lines), no Tier 2/3 dependencies. Pure argument processing with no file I/O or system calls beyond stdio.

| Issue | Tier | Resolution |
|-------|------|------------|
| `<unistd.h>` | Tier 1 | Replaced with `<amiport/unistd.h>` |
| `<err.h>` | Tier 1 | Replaced with `<amiport/err.h>` |
| `<stdlib.h>` | Tier 1 | Replaced with `<amiport/stdlib.h>` (exit macro) |
| `pledge()` | Stub | `#define pledge(p, e) (0)` |
| Exit codes | Tier 1 | `err(1,...)` changed to `err(10,...)` (RETURN_ERROR) |
| Wildcard globbing | Tier 1 | `amiport_expand_argv()` added |

## Transformations Applied

| File | Line(s) | Original | Transformed | Comment |
|------|----------|----------|-------------|---------|
| echo.c | 36 | `#include <unistd.h>` | `#include <amiport/unistd.h>` | amiport shim |
| echo.c | 37 | `#include <err.h>` | `#include <amiport/err.h>` | amiport shim |
| echo.c | 38 | (none) | `#include <amiport/stdlib.h>` | exit() macro |
| echo.c | 39 | (none) | `#include <amiport/glob.h>` | argv expansion |
| echo.c | 42-43 | `pledge()` calls | `#define pledge(p, e) (0)` | OpenBSD sandbox stub |
| echo.c | 46 | (none) | `$VER: echo 1.12` | Amiga version string |
| echo.c | 49 | (none) | `long __stack = 8192` | Stack cookie |
| echo.c | 53-59 | (none) | `cleanup()` function | atexit handler for argv/stdout |
| echo.c | 68 | (none) | `amiport_expand_argv()` | AmigaOS wildcard expansion |
| echo.c | 70 | (none) | `atexit(cleanup)` | Register cleanup for all exit paths |
| echo.c | 73 | `err(1, "pledge")` | `err(10, "pledge")` | Amiga RETURN_ERROR |

## Shim Functions Exercised

- `amiport_expand_argv()` -- wildcard expansion (Amiga shell does not glob)
- `amiport_free_argv()` -- free expanded argv in atexit cleanup
- `amiport_exit()` -- via `<amiport/stdlib.h>` exit macro
- `amiport_err()` -- error reporting with exit code

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68000+ |
| CFLAGS | `-O2 -noixemul -m68000 -Wall` |
| Libraries | `-lamiport` (posix-shim) |
| Stack size | 8192 bytes |
| Binary size | 23 KB |

## Test Results

Tested via FS-UAE (A1200, Kickstart 3.1). ARexx harness, TAP output.

| Metric | Value |
|--------|-------|
| Total tests | 37 |
| Passed | 37 |
| Failed | 0 |
| Pass rate | 100% |

Categories tested: basic output, `-n` flag, `-e` escape sequences (all types: `\n`, `\t`, `\\`, `\c`, `\a`, `\b`, `\e`, `\f`, `\r`, `\v`, octal `\0nnn`, hex `\xhh`), `-E` flag, combined flags (`-ne`, `-en`, `-nE`), edge cases (unknown flags, trailing backslash, empty string), Amiga WORK: paths, real-world scenarios, stress tests.

## Memory Safety

**Verdict: CLEAN** -- Approved for shipping (memory-checker agent, 2026-03-26).

All dynamic allocations (argv expansion) paired with atexit cleanup. No manual free() calls, no double-free risk. No amiport_getenv() usage. Stack usage minimal -- no large local buffers.

## Performance Notes

No perf-optimizer report generated. The program is trivially I/O-bound with no hot loops -- it prints arguments and exits. No optimization opportunities.

## Known Limitations

None identified. All 37 FS-UAE tests pass. The program is functionally complete.
