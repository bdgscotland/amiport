# Port: tr

## Overview

| Field | Value |
|-------|-------|
| Program | tr |
| Version | 1.22 |
| Source | OpenBSD usr.bin/tr (v1.22, Dec 2022) |
| Category | 1 -- CLI Filter |
| License | BSD 3-Clause |
| Original Author | The Regents of the University of California |
| Port Date | 2026-03-26 |

## Description

tr translates, squeezes, or deletes characters from standard input, writing to standard output. It supports character ranges (a-z), POSIX character classes ([:upper:], [:lower:]), complement (-c/-C), delete (-d), squeeze (-s), and combined modes (-ds). This is the OpenBSD implementation, a multi-file port (tr.c, str.c, extern.h) with a sophisticated string-parsing engine for character set specifications.

## Prior Art on Aminet

Researched via aminet-researcher agent. No modern standalone noixemul port found. GNU textutils packages on Aminet from the late 1990s include tr but require ixemul.library. This port provides a clean, standalone, noixemul binary.

## Portability Analysis

Verdict: **STRAIGHTFORWARD** -- Multi-file but no complex POSIX dependencies. The main challenges are header replacements and exit code mapping.

| Issue | Tier | Resolution |
|-------|------|------------|
| `<stdlib.h>` | Tier 1 | Replaced with `<amiport/stdlib.h>` for exit() macro |
| `<unistd.h>` | Tier 1 | Replaced with `<amiport/unistd.h>` |
| `<err.h>` (err/errx) | Tier 1 | Replaced with `<amiport/err.h>` |
| `<getopt.h>` (getopt) | Tier 1 | Replaced with `<amiport/getopt.h>` -- libnix getopt broken |
| `reallocarray()` | Tier 1 | Via `<amiport/string.h>` macro |
| `pledge()`/`unveil()` | Stub | Macro stub: `#define pledge(p, e) (0)` |
| Exit codes | Tier 1 | `exit(1)` -> `exit(10)` (RETURN_ERROR) throughout |
| Wildcard expansion | Tier 1 | `amiport_expand_argv()` with `__nowild = 1` (string specs, not filenames) |

## Transformations Applied

| File | Line(s) | Original | Transformed | Comment |
|------|----------|----------|-------------|---------|
| tr.c | 36 | `<stdlib.h>` | `<amiport/stdlib.h>` | Activates amiport_exit() macro |
| tr.c | 38 | `<unistd.h>` | `<amiport/unistd.h>` | POSIX shim |
| tr.c | 39-41 | (none) | `<amiport/glob.h>`, `<amiport/getopt.h>`, `<amiport/err.h>` | Shim headers |
| tr.c | 44-45 | `pledge()`/`unveil()` | `#define pledge(p, e) (0)` | OpenBSD-specific stubs |
| tr.c | 48 | (none) | `int __nowild = 1;` | Suppress wildcard expansion for char-set args |
| tr.c | 51 | (none) | `$VER` string | Amiga version string |
| tr.c | 54 | (none) | `long __stack = 16384;` | Stack cookie |
| tr.c | 116-118 | (none) | `amiport_expand_argv()` + `atexit(cleanup)` | Argv expansion + cleanup on all exit paths |
| tr.c | 213 | `errx(1, ...)` | `errx(10, ...)` | RETURN_ERROR |
| tr.c | 269 | `exit(1)` | `exit(10)` | RETURN_ERROR |
| str.c | 39-43 | `<stdlib.h>`, `<err.h>` | `<amiport/stdlib.h>`, `<amiport/string.h>`, `<amiport/err.h>` | Shim headers |
| str.c | 175+ | `errx(1, ...)` (7 sites) | `errx(10, ...)` | RETURN_ERROR throughout |
| str.c | 182,194 | `reallocarray(...)` | via `<amiport/string.h>` macro | Overflow-safe allocation |

## Shim Functions Exercised

- `amiport_err()`, `amiport_errx()`, `amiport_warnx()`
- `amiport_reallocarray()`
- `amiport_expand_argv()`, `amiport_free_argv()`
- `amiport_exit()` (via macro)

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68000+ |
| CFLAGS | `-O2 -noixemul -m68000 -Wall` |
| Libraries | `-lamiport` (posix-shim) |
| Stack size | `__stack = 16384` (16 KB) |
| Source files | tr.c, str.c (2 files + extern.h) |

## Test Results

27 tests via FS-UAE. Pass rate: 27/27 (100%).

| Category | Count | Description |
|----------|-------|-------------|
| Functional | 10 | Basic translation, ranges, ROT13, digit-to-letter |
| Delete (-d) | 2 | Vowel deletion, range deletion |
| Squeeze (-s) | 2 | Space squeeze, letter squeeze |
| Combined flags | 3 | -ds, -cd, -dc modes |
| Escape sequences | 1 | Octal escape \011 (tab) |
| Error paths | 4 | Invalid flag, wrong argc for -d/-ds/translate |
| Exit codes | 2 | RC=0 success, RC=10 error |
| Edge cases | 3 | Empty file, multiline translate, multiline delete |
| Real-world | 3 | Phone digit extraction, whitespace normalization, CRLF strip |
| Stress | 2 | 200-line translate, 200-line -ds |

## Memory Safety

- `atexit(cleanup)` registered immediately after `amiport_expand_argv()` to catch err()/errx() exit paths
- `cleanup()` calls `amiport_free_argv()` and `fflush(stdout)`
- Character class sets allocated via `reallocarray()` in str.c are static and persist for program lifetime (acceptable -- single invocation)
- Reviewed by memory-checker agent

## Performance Notes

- tr processes one character at a time via `getchar()`/`putchar()` -- could benefit from `fread()`/`fwrite()` block I/O for large inputs (3-5x speedup on 68000)
- The 256-entry translate[] and delete[] tables are pre-computed, so per-character overhead is minimal (single array lookup)

## Known Limitations

1. **POSIX character classes untestable on AmigaDOS** -- `[:upper:]`, `[:lower:]`, `[:digit:]` etc. work correctly in the binary, but cannot be tested via the FS-UAE harness because AmigaDOS interprets `[` as a volume name prompt. Character class tests are verified via vamos only. Range equivalents (a-z, A-Z) are tested on FS-UAE.
2. **No multibyte/locale support** -- AmigaOS 3.x is ASCII-only. The `[=equiv=]` equivalence class feature works for ASCII but has no locale-specific behavior.
3. **Bracket syntax `[x*n]`** -- Sequence repeat specifications like `[a*3]` in string2 cannot be tested on FS-UAE because `[` triggers AmigaDOS volume requests. Verified via vamos.

## Review

Reviewed with `/review-amiga` and `memory-checker` agent. Score: **READY**.
