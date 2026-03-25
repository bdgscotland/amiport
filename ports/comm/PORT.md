# Port: comm

## Overview

| Field | Value |
|-------|-------|
| Program | comm |
| Version | 1.11 |
| Source | OpenBSD src/usr.bin/comm/comm.c (rev 1.11) |
| Category | 1 -- CLI tool |
| License | BSD 3-Clause |
| Original Author | University of California, Berkeley (Case Larsen) |
| Port Date | 2026-03-24 |

## Description

comm reads two sorted files and produces three text columns as output: lines only in file1, lines only in file2, and lines common to both files. Flags -1, -2, -3 suppress the respective columns. The -f flag enables case-insensitive comparison.

## Prior Art on Aminet

No standalone comm binary found on Aminet. The adtools/coreutils GitHub repo (2017, source-only, no releases) includes comm but requires the full adtools environment. Geek Gadgets textutils (1990s) would have included comm but is 25+ years old with no current maintenance. A fresh standalone port is warranted.

## Portability Analysis

Verdict: **EASY** -- single file, 179 lines, pure C89, minimal POSIX surface.

| Issue | Tier | Resolution |
|-------|------|------------|
| `err()` / `errx()` | 1 | `<amiport/err.h>` shim |
| `pledge()` | 1 | `#define pledge(p, e) (0)` stub |
| `exit(1)` codes | 1 | Change to `exit(10)` (RETURN_ERROR) |
| `LINE_MAX` | 1 | Guard with `#ifndef LINE_MAX` fallback |
| `setlocale()` / `strcoll()` | 0 | Available in libnix, works for ASCII |
| `getopt()` | 0 | Short options only, libnix works |

## Transformations Applied

| Line(s) | Original | Transformed | Comment |
|----------|----------|-------------|---------|
| | `<err.h>` | `<amiport/err.h>` | Shim for err/errx/warn |
| | `<stdlib.h>` | `<amiport/stdlib.h>` | Activates exit macro |
| | `pledge()` | `#define pledge(p, e) (0)` | OpenBSD sandbox stub |
| | `err(1, ...)` | `err(10, ...)` | RETURN_ERROR for AmigaDOS |
| | `exit(1)` | `exit(10)` | RETURN_ERROR for AmigaDOS |
| | -- | `LINE_MAX` guard | Fallback if not in limits.h |
| | -- | `__stack = 32768` | Stack cookie |
| | -- | `$VER` string | AmigaOS version identification |

## Shim Functions Exercised

- `err()` from `<amiport/err.h>`
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

No platform compatibility issues expected -- no custom allocators, no struct-by-value returns, no 68k alignment concerns.

## Known Limitations

- `strcoll()` collation depends on AmigaOS locale, may differ from POSIX for non-ASCII
- `fclose(stdout)` at exit closes the shell output handle -- safe at program exit but noted

## Review

Reviewed with `/review-amiga`. Score: **READY** after fixes applied:
- Added `amiport_check_break()` to both loops (Ctrl-C handling)
- Replaced `fclose(stdout)` with `fflush(stdout)` (avoids closing shell handle)
- Made line buffers `static` (reduces stack pressure)

Performance optimized:
- `printf` replaced with `fputs` (avoids format dispatch overhead)
- `strcoll` replaced with `strcmp` (libnix has no real locale support)
- Ctrl-C check batched every 64 iterations (reduces OS call overhead)

Memory audit: **CLEAN** -- no dynamic allocation, no leaks, all exit paths safe.

## Knowledge Capture

No novel findings. All transformations are standard patterns already documented in transformation-rules.md and known-pitfalls.md.
