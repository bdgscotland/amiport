# Port: uniq

## Overview

| Field | Value |
|-------|-------|
| Program | uniq |
| Version | 1.33 |
| Source | OpenBSD CVS (rev 1.33, 2022-01-01) |
| Category | 1 — CLI |
| License | BSD-3-Clause |
| Original Author | OpenBSD Project |
| Port Date | 2026-03-22 |

## Description

Standard POSIX uniq utility — filters adjacent repeated lines from input, with options for counting occurrences (-c), showing only duplicates (-d), showing only unique lines (-u), skipping fields (-f) or characters (-s), and case-insensitive comparison (-i).

## Prior Art on Aminet

An ancient (~1990s) uniq exists in `util/cli/UnixCmd.lha` (jffabre) — a Minix-based port compiled with SAS/C. It lacks modern flags and has no maintenance. Our OpenBSD 1.33 port provides a complete, well-tested implementation.

## Portability Analysis

Verdict: **EASY** — 258 lines, single file, all Tier 1 issues.

| Issue | Tier | Resolution |
|-------|------|------------|
| `<err.h>` — err/errx/strtonum | 1 | `<amiport/err.h>` |
| `<unistd.h>` — getopt, ssize_t | 1 | `<amiport/unistd.h>` + `<amiport/getopt.h>` |
| `pledge()` calls | 1 | Stub macro `#define pledge(p,e) (0)` |
| `getline()` | 1 | `<amiport/stdio_ext.h>` |
| `getprogname()` / `__progname` | 1 | Local definition — weak symbol from libamiport.a doesn't survive linking |
| `__dead` attribute | 1 | `#define __dead __attribute__((__noreturn__))` |
| `setlocale()` | 1 | Stub in `<amiport/unistd.h>` |
| MB_CUR_MAX in `skip()` | arch | Replaced entire multibyte path with single-byte logic |
| `long long` globals | perf | Downgraded to `long` — eliminates 64-bit emulation overhead |
| Exit code `1` → `10` | 1 | Amiga RETURN_ERROR convention |

## Transformations Applied

| Line(s) | Original | Transformed | Comment |
|----------|----------|-------------|---------|
| headers | `<err.h>` | `<amiport/err.h>` | err/errx/strtonum shim |
| headers | `<stdlib.h>` | `<amiport/stdlib.h>` | exit() macro |
| headers | `<unistd.h>` | `<amiport/unistd.h>` | setlocale stub |
| headers | `<locale.h>` | removed | stub in amiport/unistd.h |
| headers | `<wchar.h>`, `<wctype.h>` | removed | multibyte path replaced |
| headers | — | added `<amiport/getopt.h>`, `<amiport/stdio_ext.h>`, `<amiport/string.h>`, `<amiport/glob.h>` | shim functions |
| 66-67 | `pledge()` | `#define pledge(p,e) (0)` | OpenBSD sandbox stub |
| 78-79 | `long long` globals | `long` / `unsigned long` | 64-bit emulation removal |
| 101-103 | — | `amiport_expand_argv()` + `atexit(cleanup)` | wildcard expansion + cleanup |
| 190 | `(iflag ? strcasecmp : strcmp)(p, t)` | `cmpfn(p, t)` | hoisted function pointer |
| 226 | `%4llu` | `%4lu` | matches unsigned long |
| 253-271 | mbtowc/iswblank/mblen | single-byte isblank inline + str++ | crash-patterns #11 prevention |
| 296-316 | obsolete() malloc | + tracking in obsolete_allocs[] | memory leak fix |
| 309 | `getprogname()` | `__progname` | weak symbol workaround |
| all | `err(1,...)` / `errx(1,...)` | `err(10,...)` / `errx(10,...)` | RETURN_ERROR |
| 313 | `exit(1)` | `exit(10)` | RETURN_ERROR |

## Shim Functions Exercised

- `amiport_expand_argv()` / `amiport_free_argv()` — wildcard expansion + __progname init
- `amiport_err()` / `amiport_errx()` — BSD error reporting
- `amiport_strtonum()` — safe string-to-number conversion
- `amiport_getline()` — POSIX getline() implementation
- `amiport_getopt()` — option parsing
- `amiport_strlcpy()` — safe string copy

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68000+ |
| CFLAGS | `-O2 -noixemul -m68000 -Wall` |
| Libraries | `-lamiport` (posix-shim) |
| Binary size | 41,844 bytes |

## Test Results

**29/29 passed** on FS-UAE (A1200, Kickstart 3.1).

Covers all five required test categories:
- **Functional** (17): all flags (-c, -d, -u, -i, -f, -s), combinations, obsolete syntax, output file
- **Error path** (5): unknown flag, too many args, missing file, invalid -f/-s values
- **Exit code**: RC 0 on 24 tests, RC 10 on 5 tests
- **Edge case** (4): empty file, single line, no trailing newline, mixed input
- **Amiga-specific** (2): WORK: volume path, T: temp output

## Known Limitations

- No multibyte/locale support (AmigaOS is always C locale). The `-f` and `-s` flags skip bytes, not multibyte characters. This matches behavior on any POSIX system in the C locale.

## Review

Reviewed with `/review-amiga`. Score: **READY**.

- Stack safety: OK (`__stack = 32768`, no large local buffers)
- Memory handling: CLEAN after fixes (obsolete() tracked, prevline freed on error path, atexit runs)
- Performance: Optimized (64-bit removal, inlined isblank, hoisted cmpfn, simplified skip loop)
- Conventions: OK ($VER string, exit codes, pledge stubbed)

### Bug Found & Fixed During Port

**`__progname` crash**: The weak symbol `__progname` from `libamiport.a` was not surviving linking despite the containing object (`argv_expand.o`) being pulled in. This caused `usage()` to crash when dereferencing an invalid pointer (address `0xc80000`). On FS-UAE, this manifested as repeated 'H' characters flooding the console before a Guru Meditation. Fixed by defining `__progname` directly in the ported source.
