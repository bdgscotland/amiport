# Port: expand

## Overview

| Field | Value |
|-------|-------|
| Program | expand |
| Version | 1.15 |
| Source | OpenBSD src/usr.bin/expand/expand.c (rev 1.15) |
| Category | 1 -- CLI tool |
| License | BSD 3-Clause |
| Original Author | University of California, Berkeley |
| Port Date | 2026-03-25 |

## Description

expand converts tabs to spaces in text files. The default tab stop is every 8 columns. Custom tab stops can be set with -t. Reads from files or stdin.

## Prior Art on Aminet

Exists in coreutils-m68k-bin (2010, ixemul-dependent, BETA). Our -noixemul standalone binary is lighter and dependency-free.

## Portability Analysis

Verdict: **EASY** -- single file, 170 lines, pure C89, minimal POSIX surface.

| Issue | Tier | Resolution |
|-------|------|------------|
| `err()` / `errx()` | 1 | `<amiport/err.h>` shim |
| `pledge()` | 1 | `#define pledge(p, e) (0)` stub |
| `exit(1)` codes | 1 | Change to `exit(10)` (RETURN_ERROR) |
| `__progname` | pitfall | Define directly, don't rely on weak symbol |

## Transformations Applied

<!-- Populated after Stage 3 -->

## Shim Functions Exercised

- `err()` / `errx()` from `<amiport/err.h>`
- `amiport_exit()` via `<amiport/stdlib.h>` macro

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68000+ |
| CFLAGS | `-O2 -noixemul -m68000 -Wall` |
| Libraries | `-lamiport` (posix-shim) |
| Binary size | |

## Test Results

<!-- Populated after Stage 5 -->

## Platform Compatibility Notes

No platform compatibility issues -- no custom allocators, no struct-by-value returns, no 68k alignment concerns.

## Known Limitations

- No multibyte/locale support (tab expansion is byte-based, correct for ISO 8859-1)

## Review

Reviewed with `/review-amiga`. Score: **READY**.

Performance optimized:
- `getchar()` loop replaced with `fgets()` + pointer scan (~40x fewer JSR calls per line)
- `putchar(' ')` loops replaced with `fwrite(spaces, 1, n, stdout)` (~7x fewer calls per tab)
- Modulo in single-stop case replaced with precomputed target (1 DIVU per tab vs 7)

Memory audit: **CLEAN** -- single allocation (amiport_expand_argv), atexit cleanup covers all exit paths.

## Knowledge Capture

No novel findings. All transformations are standard patterns.
