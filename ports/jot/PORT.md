# Port: jot

## Overview

| Field | Value |
|-------|-------|
| Program | jot |
| Version | 1.56 (port revision: 1) |
| Source | OpenBSD usr.bin/jot (v1.56, Aug 2021) |
| Category | 1 -- CLI tool |
| License | BSD-3-Clause |
| Original Author | John Kunze, The Regents of the University of California |
| Port Date | 2026-03-26 |

## Description

jot prints sequential or random data -- it generates sequences of numbers, characters, or formatted strings to stdout. It supports ascending and descending sequences, configurable step sizes, floating-point precision control, custom printf-style format strings, random output with range constraints, and custom separators. This is the OpenBSD implementation, commonly used in shell scripts for loop generation, test data creation, and numeric formatting.

## Prior Art on Aminet

No existing jot port found on Aminet for AmigaOS 3.x.

## Portability Analysis

Verdict: **PORTABLE** -- Pure Tier 1 POSIX dependencies. Single-file program with no complex system call requirements. The main porting effort was C99-to-C89 conversion and OpenBSD-specific removals.

| Issue | Tier | Resolution |
|-------|------|------------|
| `<stdbool.h>` | C99 | Removed; `bool`/`true`/`false` defined as macros (int/1/0) |
| `<stdint.h>` / `uint32_t` | C99 | Removed; `uint32_t` replaced with `unsigned long` |
| `<stdlib.h>` | Tier 1 | Replaced with `<amiport/stdlib.h>` for exit() macro |
| `<unistd.h>` | Tier 1 | Replaced with `<amiport/unistd.h>` |
| `<err.h>` (err/errx/warn/strtonum) | Tier 1 | Replaced with `<amiport/err.h>` shim |
| `<getopt.h>` (getopt) | Tier 1 | Replaced with `<amiport/getopt.h>` -- libnix getopt_long is broken |
| `asprintf()` | Tier 1 | Provided via `<amiport/stdio_ext.h>` macro |
| `arc4random_uniform()` / `arc4random()` | Tier 1 | Replaced with `rand()` -- sufficient for non-cryptographic use |
| `pledge()` | Stub | Macro stub: `#define pledge(p, e) (0)` |
| `__dead` attribute | OpenBSD | Removed -- not available in C89/bebbo-gcc |
| `infinity` variable name | N/A | Renamed to `inf_loop` -- conflicts with math.h `infinity()` declaration |
| Exit codes | Tier 1 | All `exit(1)` / `errx(1, ...)` changed to `exit(10)` / `errx(10, ...)` for Amiga RETURN_ERROR |
| Wildcard expansion | Tier 1 | Added `amiport_expand_argv()` -- Amiga shell does not glob |

## Transformations Applied

| File | Line(s) | Original | Transformed | Comment |
|------|----------|----------|-------------|---------|
| jot.c | 39-40 | (none) | `$VER: jot 1.56 (26.03.2026)` | Amiga version string |
| jot.c | 43 | (none) | `long __stack = 32768` | Stack cookie -- Amiga default 4KB too small |
| jot.c | 46 | (none) | `#define pledge(p, e) (0)` | Stub OpenBSD pledge() |
| jot.c | 49 | `<stdlib.h>` | `<amiport/stdlib.h>` | Activates exit() macro |
| jot.c | 51 | `<unistd.h>` | `<amiport/unistd.h>` | Amiga unistd shim |
| jot.c | 53 | `<err.h>` | `<amiport/err.h>` | Provides err/errx/strtonum |
| jot.c | 55 | (none) | `<amiport/stdio_ext.h>` | Provides asprintf() |
| jot.c | 57 | (none) | `<amiport/glob.h>` | Provides amiport_expand_argv() |
| jot.c | 59 | `<getopt.h>` | `<amiport/getopt.h>` | Broken libnix getopt workaround |
| jot.c | 64 | `#include <stdbool.h>` | removed | C99 header not available in C89 |
| jot.c | 65 | `#include <stdint.h>` | removed | C99 header; uint32_t replaced |
| jot.c | 69-74 | (none) | bool/true/false macros | C89 bool emulation |
| jot.c | 95 | `bool infinity` | `bool inf_loop` | Name conflict with math.h |
| jot.c | 106 | `__dead` | removed | OpenBSD-specific noreturn attribute |
| jot.c | 109-114 | (none) | `cleanup()` function | atexit cleanup for argv expansion + stdout flush |
| jot.c | 128 | (none) | `amiport_expand_argv()` | Amiga shell wildcard expansion |
| jot.c | 130 | (none) | `atexit(cleanup)` | Register cleanup for all exit paths |
| jot.c | 134 | `err(1, "pledge")` | `err(10, "pledge")` | Amiga RETURN_ERROR |
| jot.c | 295-298 | `bool use_unif; uint32_t pow10, uintx` | `int use_unif; unsigned long pow10, uintx` | C89 types |
| jot.c | 331-333 | `arc4random_uniform(uintx)` | `rand() % (int)uintx` | No arc4random on AmigaOS |
| jot.c | 338 | `arc4random()` | `rand() / ((double)RAND_MAX + 1.0)` | Scale rand() to [0,1) |
| jot.c | 388 | `exit(1)` | `exit(10)` | Amiga RETURN_ERROR in usage() |
| jot.c | all | `errx(1, ...)` | `errx(10, ...)` | Amiga RETURN_ERROR throughout |

## Shim Functions Exercised

- `amiport_err()`, `amiport_errx()`, `amiport_warnx()`
- `amiport_strtonum()`
- `amiport_expand_argv()`, `amiport_free_argv()`
- `amiport_asprintf()`
- `amiport_getopt()`

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68000+ |
| CFLAGS | `-O2 -noixemul -m68000 -Wall` |
| LDFLAGS | `-lamiport -lm` |
| Stack cookie | 32768 (32 KB) |
| VAMOS_STACK | default |

## Test Results

37 tests in `test-fsemu-cases.txt`. Categories tested:

| Category | Count | Description |
|----------|-------|-------------|
| Basic sequences | 3 | Sequential output, explicit begin/end, custom start |
| Descending | 1 | Reverse sequence 10 to 1 |
| Floating point | 3 | Float ranges, -p precision, integer rounding |
| Step size | 1 | Explicit step=2 |
| Flag -b | 1 | Boring/word repeat |
| Flag -w | 4 | Format strings (%d, %03d, %x, %.1f) |
| Flag -s | 2 | Comma and space separators |
| Flag -c | 2 | ASCII character output |
| Flag -r | 3 | Random output (line count and RC only) |
| Flag -n | 1 | Suppress final newline |
| Combined flags | 1 | -w + -s together |
| Edge cases | 3 | Single rep, begin==end, negative range |
| Error paths | 7 | Bad args, too many args, impossible step, bad format, precision limits |
| Real-world | 3 | Array index generation, octal output, countdown |
| Stress/precision | 5 | 100-line, 1000-line, high-precision float, descending float |

Notable edge cases tested:
- `jot 5 -3 3` spans negative-to-positive range through zero
- `jot -p 3 5 0.000 1.000` verifies IEEE 754 exact step representation on 68k FPU
- `jot 1000` stress tests loop stability and output buffering
- Random tests verify RC=0 without checking non-deterministic output values

## Memory Safety

**Verdict: CLEAN -- Approved for shipping.**

Memory-checker audit (2026-03-26) found one minor leak: the `asprintf(&format)` calls in `getformat()` overwrite the global `format` pointer, orphaning previous allocations. Only the final allocation persists at program end (~8-16 bytes) and is never freed. This is unavoidable without major refactoring and acceptable for a single-invocation CLI tool.

All exit paths (err, errx, usage, normal return) are covered by `atexit(cleanup)` which calls `amiport_free_argv()` and `fflush(stdout)`.

## Performance Notes

Perf-optimizer review (2026-03-26) -- I/O-bound for large sequences. No CRITICAL issues.

- **MEDIUM** [putdata()] -- `printf(format, ...)` called once per output value. For `jot 1000000`, this is 1M printf calls with format-string parsing. Inherent to the program's design.
- **MEDIUM** [fputs separator] -- `fputs(sepstring, stdout)` called every iteration. A `putchar('\n')` fast path for the default single-newline separator would save JSR overhead per line.
- **LOW** [rand() modulo bias] -- `rand() % uintx` has standard modulo bias, but unavoidable without implementing rejection sampling. Acceptable for non-cryptographic use.

## Known Limitations

1. **Random number quality** -- `rand()` replaces OpenBSD's `arc4random_uniform()`. Output is not cryptographically random and has modulo bias for non-power-of-2 ranges. Sufficient for sequence generation and test data.
2. **Infinite sequences require Ctrl-C** -- `jot -b x 0` enters an infinite loop (reps==0 means infinity in OpenBSD). Programs must call `amiport_check_break()` for clean interruption; jot does not currently do this.
3. **Floating-point precision** -- 68k software floating point matches IEEE 754 but accumulated step errors may differ from native hardware FPU results for very long float sequences.

## Review

Reviewed by memory-checker and perf-optimizer agents. Score: **READY**.
