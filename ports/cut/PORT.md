# Port: cut

## Overview

| Field | Value |
|-------|-------|
| Program | cut |
| Version | 1.28 |
| Source | OpenBSD usr.bin/cut (cut.c v1.28, 2023-03-08) |
| Category | 1 — CLI tool |
| License | BSD 3-Clause |
| Original Author | University of California, Berkeley |
| Port Date | 2026-03-20 |

## Description

POSIX `cut` utility — extract selected fields, bytes, or characters from each line of input. Supports `-b` (bytes), `-c` (characters), `-f` (fields) with custom delimiter (`-d`), line suppression (`-s`), and multibyte-aware byte selection (`-n`).

## Portability Analysis

**Verdict: PORTABLE** — Single-file, pure stdin/stdout text processing. No Tier 2 or Tier 3 issues.

| Issue | Tier | Resolution |
|-------|------|------------|
| `<err.h>` (err, errx, warn, strtonum) | Tier 1 | `<amiport/err.h>` shim |
| `<unistd.h>` (getopt) | Tier 1 | `<amiport/unistd.h>` + `<amiport/getopt.h>` |
| `pledge()` | Stub | Macro `#define pledge(p, e) (0)` |
| `strsep()` | Missing | Inline BSD implementation |
| `getline()` | Missing | Inline implementation (fgetc loop) |
| `_POSIX2_LINE_MAX` | Missing | Defined as 2048 |
| `mblen()` / `setlocale()` | Incompatible | Replaced with strlen/hardcoded len=1 (C locale only) |
| `ssize_t` | Missing | Replaced with `long` |
| Exit codes (1) | Convention | All changed to 10 (RETURN_ERROR) |

## Transformations Applied

| Line(s) | Original | Transformed | Comment |
|----------|----------|-------------|---------|
| 37 | — | `$VER: cut 1.0 (20.03.2026)` | Amiga version string |
| 40 | — | `long __stack = 32768` | Stack cookie |
| 44 | `<err.h>` | `<amiport/err.h>` | BSD err/warn/strtonum shim |
| 51-52 | `<unistd.h>` | `<amiport/unistd.h>` + `<amiport/getopt.h>` | getopt shim |
| 55-56 | `pledge()` calls | `#define pledge(p, e) (0)` | OpenBSD stub |
| 59-61 | `_POSIX2_LINE_MAX` | `#define _POSIX2_LINE_MAX 2048` | POSIX constant |
| 64-89 | `strsep()` | Inline `amiport_strsep()` | BSD function not in libnix |
| 92-140 | `getline()` | Inline `amiport_getline()` | GNU function not in libnix |
| 166 | `setlocale(LC_CTYPE, "")` | Commented out | Not reliable on AmigaOS |
| 186-188 | `mblen(optarg, MB_CUR_MAX)` | `strlen(optarg)` | mblen unreliable in libnix |
| 372 | `mblen(cp, MB_CUR_MAX)` | `len = 1` | Byte-only mode (C locale) |
| Various | `exit(1)`, `err(1,...)`, `errx(1,...)` | `exit(10)`, `err(10,...)`, `errx(10,...)` | Amiga RETURN_ERROR |
| Various | `ssize_t` | `long` | Type not available |

## Shim Functions Exercised

- `amiport_err()` — print error + errno + exit
- `amiport_errx()` — print error + exit
- `amiport_warn()` — print warning + errno
- `amiport_strtonum()` — safe string-to-number
- `amiport_getopt()` — POSIX option parsing

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68000+ |
| CFLAGS | `-O2 -noixemul -m68000 -Wall` |
| Libraries | `-lamiport` (posix-shim) |
| Binary size | ~30 KB |

## Test Results

Tested via vamos. All tests pass.

| Test | Command | Input | Expected | Result |
|------|---------|-------|----------|--------|
| Field extract | `cut -d: -f2` | `one:two:three` | `two` | PASS |
| Byte extract | `cut -b1-3,5` | `abcdefgh` | `abce` | PASS |
| Suppress no-delim | `cut -d: -f1 -s` | `no-delim\nhas:delim` | `has` | PASS |

## Known Limitations

- **No multibyte character support**: The `-c` (character) and `-n` (no-split multibyte) options operate in byte mode only, since AmigaOS libnix doesn't provide reliable `mblen()`. This matches the behavior of most Amiga text tools which assume single-byte encodings (ISO 8859-1).
- **Line length**: Maximum line position is 2048 (`_POSIX2_LINE_MAX`), matching POSIX minimum. Lines longer than this in position specification will error.

## Amiga Review

Reviewed with `/review-amiga`. Score: **READY**. No critical issues. Stack, memory, paths, library cleanup, and conventions all pass.

## Memory Safety (Stage 6b)

Audited by `memory-checker` agent (2026-03-21). Verdict: **CLEAN**.

All 2 dynamic allocations (realloc calls) properly freed. File handles balanced. Realloc uses intermediate pointers (safe pattern). Global `cut_line` buffer explicitly freed before exit on all paths. No leaks, no double-frees, no use-after-free.
