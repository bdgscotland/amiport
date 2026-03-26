# Port: sleep

## Overview

| Field | Value |
|-------|-------|
| Program | sleep |
| Version | 1.29 (port revision: 1) |
| Source | OpenBSD (GitHub mirror) |
| Category | 1 -- CLI |
| License | BSD-3-Clause |
| Original Author | University of California, Berkeley |
| Port Date | 2026-03-26 |

## Description

sleep suspends execution for a specified number of seconds, supporting fractional values (e.g., `sleep 0.5` for half a second). The OpenBSD implementation includes input validation, 32-bit overflow detection, and nanosecond-precision parsing. On AmigaOS, the POSIX `nanosleep()` call is replaced with AmigaDOS `Delay()`, which operates in 1/50-second ticks with round-up to ensure minimum wait times.

## Prior Art on Aminet

No existing standalone 68k port of OpenBSD sleep found on Aminet. AmigaDOS has a built-in `Wait` command but it only accepts whole-second granularity. This port provides fractional-second precision (1/50s resolution via Delay ticks) and stricter input validation with proper error codes.

## Portability Analysis

Verdict: **EASY** -- Single file (171 lines), but required removal of the SIGALRM signal handler (dead code on AmigaOS) and replacement of `nanosleep()`/`struct timespec` with AmigaDOS `Delay()` and plain `long` variables.

| Issue | Tier | Resolution |
|-------|------|------------|
| `nanosleep()` / `struct timespec` | Tier 1 | Replaced with AmigaDOS `Delay()` and plain `long` variables |
| `signal(SIGALRM, alarmh)` | Tier 3 | Removed entirely -- SIGALRM does not exist on AmigaOS; handler was dead code |
| `alarmh()` signal handler | Tier 3 | Removed -- `_exit(0)` in signal handler is unreachable |
| `<sys/time.h>` | Tier 1 | Replaced with `<proto/dos.h>` for Delay() |
| `<signal.h>` | Tier 1 | Removed -- no signal support needed |
| `timespecclear()` / `timespecisset()` | BSD macro | Replaced with explicit `tv_sec = 0; tv_nsec = 0` and `if (tv_sec > 0 \|\| tv_nsec > 0)` |
| `<err.h>` | Tier 1 | Replaced with `<amiport/err.h>` |
| `<stdlib.h>` | Tier 1 | Replaced with `<amiport/stdlib.h>` (exit macro) |
| `<unistd.h>` | Tier 1 | Replaced with `<amiport/unistd.h>` |
| `getprogname()` | Tier 1 | Via `<amiport/utsname.h>` macro (uses __progname) |
| `pledge()` | Stub | `#define pledge(p, e) (0)` |
| Exit codes | Tier 1 | `errx(1,...)` changed to `errx(10,...)`, `exit(1)` to `exit(10)` |
| Wildcard globbing | Tier 1 | `amiport_expand_argv()` added |

## Transformations Applied

| File | Line(s) | Original | Transformed | Comment |
|------|----------|----------|-------------|---------|
| sleep.c | 34 | (none) | `$VER: sleep 1.29` | Amiga version string |
| sleep.c | 37 | (none) | `long __stack = 8192` | Stack cookie |
| sleep.c | 42 | `#include <sys/time.h>` | `#include <proto/dos.h>` | Delay() instead of nanosleep() |
| sleep.c | 46 | `#include <err.h>` | `#include <amiport/err.h>` | amiport shim |
| sleep.c | 48 | `#include <signal.h>` | (removed) | SIGALRM not available on AmigaOS |
| sleep.c | 51 | `#include <stdlib.h>` | `#include <amiport/stdlib.h>` | exit() macro |
| sleep.c | 54 | `#include <unistd.h>` | `#include <amiport/unistd.h>` | amiport shim |
| sleep.c | 56-60 | (none) | `<amiport/getopt.h>`, `<amiport/utsname.h>`, `<amiport/glob.h>` | getopt, getprogname, argv expansion |
| sleep.c | 63 | (none) | `#define pledge(p, e) (0)` | OpenBSD sandbox stub |
| sleep.c | 68-73 | (none) | `cleanup()` function | atexit handler |
| sleep.c | 80-81 | `struct timespec rqtp` | `long tv_sec; long tv_nsec;` | No struct timespec on AmigaOS |
| sleep.c | 89-91 | (none) | `amiport_expand_argv()` + `atexit(cleanup)` | Standard boilerplate |
| sleep.c | 96 | `signal(SIGALRM, alarmh)` | (removed) | SIGALRM does not exist |
| sleep.c | 110-112 | `timespecclear(&rqtp)` | `tv_sec = 0; tv_nsec = 0;` | Explicit zero init |
| sleep.c | 117-120 | `errx(1,...)` | `errx(10,...)` | Amiga RETURN_ERROR |
| sleep.c | 139-156 | `nanosleep(&rqtp, NULL)` | `Delay(ticks)` with tick calculation | ns-to-ticks conversion with round-up |
| sleep.c | 165 | `exit(1)` | `exit(10)` | Amiga RETURN_ERROR in usage() |
| sleep.c | 168-170 | `alarmh()` signal handler | (removed with comment) | Dead code on AmigaOS |

## Shim Functions Exercised

- `amiport_expand_argv()` -- wildcard expansion
- `amiport_free_argv()` -- free expanded argv in atexit cleanup
- `amiport_exit()` -- via exit macro
- `amiport_errx()` -- error reporting with exit code
- `amiport_getopt()` -- option parsing
- `getprogname()` -- program name for usage() via __progname

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68000+ |
| CFLAGS | `-O2 -noixemul -m68000 -Wall` |
| Libraries | `-lamiport` (posix-shim) |
| Stack size | 8192 bytes |
| Binary size | 29 KB |

## Test Results

Tested via FS-UAE (A1200, Kickstart 3.1). ARexx harness, TAP output.

| Metric | Value |
|--------|-------|
| Total tests | 25 |
| Passed | 25 |
| Failed | 0 |
| Pass rate | 100% |

Categories tested: zero-second sleep, whole seconds, fractional seconds (0.1, 0.02, 0.001, 0.5, 0.0), leading-dot fractional (.5), error paths (unknown option, two arguments, non-numeric, negative number, invalid fraction, invalid integer suffix), edge cases (32-bit overflow, huge number, empty string, dot-only, 10-digit fractional), Amiga-specific (argv expansion integrity, Delay tick mapping), real-world scenarios (inter-command delay, install-script wait), precision tests (9-digit fractional, sub-tick round-up).

## Memory Safety

**Verdict: CLEAN** -- Approved for shipping (memory-checker agent, 2026-03-26).

All dynamic allocations (argv expansion) paired with atexit cleanup. No manual free() calls, no double-free risk. Stack usage minimal -- no large local buffers. All error paths (errx, usage) trigger atexit cleanup. Delay() is a single AmigaOS call with no memory implications.

## Performance Notes

No perf-optimizer report generated. The program parses one argument, calls Delay() once, and exits. No optimization opportunities exist.

## Known Limitations

1. **Resolution is 1/50 second (20ms)** -- AmigaDOS Delay() operates in ticks (1 tick = 20ms). Sub-tick values are rounded up to 1 tick. Nanosecond precision from the input is parsed but not achievable.
2. **Delay() is not interruptible** -- Unlike POSIX nanosleep() which can be interrupted by signals, AmigaDOS Delay() blocks until complete. Ctrl-C will not interrupt a long sleep.
3. **32-bit overflow on large values** -- The program correctly detects and rejects values >= 2^31 seconds (~68 years). This is a consequence of using 32-bit `long` for seconds on 68k.
