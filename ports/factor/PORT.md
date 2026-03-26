# Port: factor

## Overview

| Field | Value |
|-------|-------|
| Program | factor |
| Version | 1.30 |
| Source | OpenBSD games/factor (v1.30, September 2016) |
| Category | 1 -- CLI tool |
| License | BSD-3-Clause |
| Original Author | Landon Curt Noll, The Regents of the University of California |
| Port Date | 2026-03-26 |

## Description

factor decomposes a number into its prime factors. Given one or more numbers as arguments, it prints each number followed by its prime factorization in ascending order. When run with no arguments, it reads numbers from standard input. The implementation uses a precomputed prime table (all primes up to 65537) for trial division of 32-bit values, and an Eratosthenes sieve extension (`pr_bigfact`) for factoring 64-bit values whose factors exceed the table.

## Prior Art on Aminet

No existing factor port found on Aminet for AmigaOS 3.x.

## Portability Analysis

Verdict: **MODERATE** -- Multi-file port (3 source files + 1 header). The core algorithm is portable C, but the 64-bit integer support (`u_int64_t`, `strtoull`, `%llu` printf format) requires careful handling on 68k where 64-bit arithmetic is software-emulated. The 256 KB sieve table must be static to avoid stack overflow.

| Issue | Tier | Resolution |
|-------|------|------------|
| `<stdlib.h>` | Tier 1 | Replaced with `<amiport/stdlib.h>` (exit macro) |
| `<unistd.h>` | Tier 1 | Replaced with `<amiport/unistd.h>` |
| `<err.h>` (err/errx) | Tier 1 | Replaced with `<amiport/err.h>` (shimmed) |
| `u_int64_t` / `unsigned long long` | Tier 1 | Typedef guard added; bebbo-gcc supports `long long` via software emulation |
| `strtoull()` | Tier 1 | Available in libnix |
| `%llu` printf format | Tier 1 | Works in libnix (software 64-bit) |
| `getprogname()` | Tier 1 | Provided via `<amiport/utsname.h>` |
| `pledge()` / `unveil()` | Stub | Macro stubs |
| `__dead` attribute | Tier 1 | Defined as `__attribute__((__noreturn__))` |
| `isblank()` | Tier 1 | Available in libnix via `<ctype.h>` |
| Exit codes | Tier 1 | `err(1,...)`/`errx(1,...)`/`exit(1)` changed to 10 (RETURN_ERROR) |
| `char table[TABSIZE]` (256 KB) | Amiga | Made `static` to avoid stack allocation (Guru on AmigaOS) |

## Transformations Applied

| File | Line(s) | Original | Transformed | Comment |
|------|----------|----------|-------------|---------|
| factor.c | 57 | `#include <err.h>` | `#include <amiport/err.h>` | Shimmed err/errx |
| factor.c | 60 | `#include <stdlib.h>` | `#include <amiport/stdlib.h>` | Exit macro |
| factor.c | 61 | (none) | `#include <sys/types.h>` | Needed for u_int32_t |
| factor.c | 63 | `#include <unistd.h>` | `#include <amiport/unistd.h>` | POSIX shim |
| factor.c | 64-66 | (none) | `#include <amiport/getopt.h>`, `<amiport/utsname.h>`, `<amiport/glob.h>` | getopt, getprogname, argv expansion |
| factor.c | 68-76 | (none) | `u_int64_t` typedef guard | Ensure 64-bit type exists |
| factor.c | 79-80 | (none) | `#define pledge(p, e) (0)` / `#define unveil(p, f) (0)` | Stubs |
| factor.c | 82-85 | (none) | `#ifndef __dead` / `#define __dead __attribute__((__noreturn__))` | OpenBSD compat |
| factor.c | 88 | (none) | `static const char *verstag = "$VER: factor 1.30 (26.03.2026)";` | Version string |
| factor.c | 91 | (none) | `LONG __stack = 16384;` | Stack cookie (16 KB) |
| factor.c | 94 | (none) | `int __nowild = 1;` | Suppress wildcard expansion (numeric args) |
| factor.c | 115-119 | (none) | `cleanup()` with `fflush(stdout)` | atexit cleanup |
| factor.c | 133 | `err(1, "pledge")` | `err(10, "pledge")` | RETURN_ERROR |
| factor.c | 150,159,163,167 | `err(1,...)`/`errx(1,...)` | `err(10,...)`/`errx(10,...)` | RETURN_ERROR |
| factor.c | 174,178,180 | `err(1,...)`/`errx(1,...)` | `err(10,...)`/`errx(10,...)` | RETURN_ERROR |
| factor.c | 258 | `char table[TABSIZE];` | `static char table[TABSIZE];` | 256 KB local array would blow stack; made static |
| factor.c | 350 | `__dead static void usage()` | `static void usage()` | Removed `__dead` -- amiport_exit() not declared noreturn |
| factor.c | 355 | `exit(1)` | `exit(10)` | RETURN_ERROR |

## Shim Functions Exercised

- `amiport_err()`, `amiport_errx()`
- `amiport_exit()` (via `<amiport/stdlib.h>` macro)
- `getprogname()` (via `<amiport/utsname.h>`)
- `getopt()`, `optind` (via `<amiport/getopt.h>`)

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68000+ |
| CFLAGS | `-O2 -noixemul -m68000 -Wall -Iported` |
| Libraries | `-lamiport` (Tier 1 posix-shim) |
| Stack size | 16384 bytes (16 KB) |
| Binary size | ~82 KB (includes precomputed prime table and sieve pattern) |

## Test Results

39 tests, all passing. Tested via FS-UAE (Category 1 CLI tool).

| Category | Count | Description |
|----------|-------|-------------|
| Small primes | 6 | Factor 2, 3, 5, 7, 11, 13 |
| Composites | 5 | Factor 12, 49, 100, 360, 1024 |
| Large 32-bit primes | 4 | 997, 7919, 999999937, INT32_MAX (Mersenne M31) |
| Edge cases | 6 | Factor 0 (exit immediately), 1 (special case), 65537 (Fermat F4), UINT32_MAX, 4294967291, Fermat F5 |
| Multiple arguments | 2 | Two args, five prime args |
| Stdin mode | 3 | Two numbers, five numbers, blank lines skipped |
| Error paths | 6 | Negative arg, non-numeric, -h flag, negative stdin, non-numeric stdin, alphabetic prefix |
| Amiga-specific | 2 | WORK: volume, __nowild verification |
| Real-world | 3 | Primorial(19), seconds in a day, LCM(1..13) |
| Stress / precision | 2 | Large prime full sieve traversal, Mersenne M31 |

## Memory Safety

Verdict: **CLEAN** -- No dynamic heap allocation. The `buf[100]` for stdin input is stack-allocated. The `table[TABSIZE]` sieve (256 KB) is static. The prime table and pattern data are const arrays in `pr_tbl.c` and `pattern.c`. The only cleanup needed is `fflush(stdout)` via atexit, which ensures buffered factor output is flushed on err/errx exit paths.

## Performance Notes

64-bit arithmetic on 68000 is software-emulated via libgcc, making each `val % (long)*fact` and `val /= (long)*fact` operation significantly more expensive than on native 32-bit. For values within 32-bit range, the trial division loop is tight and uses the precomputed prime table directly. The `pr_bigfact` sieve extension is the expensive path -- it computes Eratosthenes sieves in 256 KB windows. The `fflush(stdout)` calls after each factor group provide user feedback during long factorizations.

## Known Limitations

1. **Values > 2^31 cause DivByZero Guru Meditation** -- The 64-bit division emulation in libgcc/libnix triggers a hardware DIVU trap for large dividends on 68k. Factoring numbers that require `pr_bigfact()` (values with factors > 65537^2 = ~4.3 billion) will crash with Guru #80000005 (DivByZero). This limits practical use to values whose largest prime factor fits in the precomputed table, or to values below approximately 2^31. Values like `4294967295` (UINT32_MAX) and `4294967297` (Fermat F5) work because their factors are all small enough to be found by the 32-bit sieve without entering `pr_bigfact`.
2. **256 KB static sieve table** -- The `table[TABSIZE]` array in `pr_bigfact` is 256 KB. This is allocated in BSS (static), not on the stack, but it does increase the binary's memory footprint. On systems with limited RAM, this could be significant.
3. **No `-h` help flag** -- The `-h` option triggers `usage()` and exits with RC=10, consistent with OpenBSD behavior where `-h` is not a recognized flag.
