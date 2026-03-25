# Port: rev

## Overview

| Field | Value |
|-------|-------|
| Program | rev |
| Version | 1.16 |
| Source | OpenBSD src/usr.bin/rev/rev.c (rev 1.16) |
| Category | 1 -- CLI tool |
| License | BSD 3-Clause |
| Original Author | University of California, Berkeley |
| Port Date | 2026-03-25 |

## Description

rev reads files (or stdin) and reverses each line character by character. Useful for shell scripting and text manipulation.

## Prior Art on Aminet

No existing rev port found on Aminet. Not part of GNU coreutils (it's a BSD/util-linux utility). Geek Gadgets did not include it. This is a genuinely new addition to the AmigaOS ecosystem.

## Portability Analysis

Verdict: **EASY** -- single file, 130 lines, pure C89, minimal POSIX surface.

| Issue | Tier | Resolution |
|-------|------|------------|
| `err()` / `warn()` | 1 | `<amiport/err.h>` shim |
| `pledge()` | 1 | `#define pledge(p, e) (0)` stub |
| `getline()` | 1 | `<amiport/stdio_ext.h>` shim |
| `exit(1)` codes | 1 | Change to `exit(10)` (RETURN_ERROR) |
| `MB_CUR_MAX` | pitfall | Guard with `#ifndef __AMIGA__` (crash-patterns #11) |
| `fclose(stdin)` | pitfall | Guard with `if (fp != stdin)` |
| `__progname` | pitfall | Define directly, don't rely on weak symbol |

## Transformations Applied

| Line(s) | Original | Transformed | Comment |
|----------|----------|-------------|---------|
| | `<err.h>` | `<amiport/err.h>` | Shim for err/errx/warn |
| | `<stdlib.h>` | `<amiport/stdlib.h>` | Activates exit macro |
| | -- | `<amiport/stdio_ext.h>` | Provides getline shim |
| | `pledge()` | `#define pledge(p, e) (0)` | OpenBSD sandbox stub |
| | `err(1, ...)` | `err(10, ...)` | RETURN_ERROR for AmigaDOS |
| | `exit(1)` | `exit(10)` | RETURN_ERROR for AmigaDOS |
| | `MB_CUR_MAX > 1` | `#ifndef __AMIGA__` guard | Crash-patterns #11 |
| | `fclose(fp)` | `if (fp != stdin) fclose(fp)` | Never fclose(stdin) |
| | `extern __progname` | `char *__progname = "rev"` | Weak symbol fix |
| | -- | `__stack = 16384` | Stack cookie |
| | -- | `$VER` string | AmigaOS version identification |

## Shim Functions Exercised

- `err()` / `warn()` from `<amiport/err.h>`
- `amiport_getline()` from `<amiport/stdio_ext.h>`
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

- MB_CUR_MAX returns >1 on libnix even without locale support -- multibyte path disabled via compile-time guard
- No struct-by-value returns, no custom allocators

## Known Limitations

- Multibyte/UTF-8 support disabled (AmigaOS has no wchar support)
- Operates on raw bytes, not characters -- correct for ISO 8859-1 (Amiga standard)

## Review

Reviewed with `/review-amiga`. Score: **READY** after fixes applied:
- Fixed double-free bug: `getline_buf = NULL` after `free(p)` (AN_MemCorrupt Guru on multi-file usage)
- Return codes 1 changed to 5 (RETURN_WARN) for partial failures
- Added `atexit(cleanup)` for getline buffer + fflush(stdout)

Performance optimized:
- Replaced per-character `putchar()` loop with in-place reversal + single `fwrite()` (~40x fewer I/O calls per line)
- Removed multibyte code path entirely on AmigaOS via `#ifndef __AMIGA__` (eliminates global read + dead branch per character)
- Reduced pointer locals from 4 to 3 for better 68k register allocation

Memory audit: **CLEAN** after double-free fix -- getline buffer tracked via static pointer, freed in atexit on error paths, cleared after normal free.

## Knowledge Capture

Novel finding: **atexit cleanup of getline buffer requires NULL-clearing after free()** to prevent double-free on multi-file invocations. The static tracking pointer persists across rev_file() calls. This is a general pattern for any program using getline() with atexit cleanup -- documented as a learning in PORT.md. No update to known-pitfalls needed as this is standard C memory management, not an AmigaOS-specific issue.
