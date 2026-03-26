# Port: join

## Overview

| Field | Value |
|-------|-------|
| Program | join |
| Version | 1.34 |
| Source | OpenBSD usr.bin/join (v1.34, Dec 2022) |
| Category | 1 -- CLI Filter |
| License | BSD 3-Clause |
| Original Author | Steve Hayman, Michiro Hikida, David Goodenough, UC Berkeley |
| Port Date | 2026-03-26 |

## Description

join performs a relational join on two sorted input files, matching lines by a common join field (default: first field). It supports custom join fields (-1, -2, -j), output field selection (-o), custom field separators (-t), unpairable line output (-a, -v), and empty field replacement (-e). This is the OpenBSD implementation with wchar/multibyte delimiter support replaced by ASCII-only processing.

## Prior Art on Aminet

Researched via aminet-researcher agent. No modern standalone noixemul port found.

## Portability Analysis

Verdict: **MODERATE** -- Single file but has wchar/multibyte field separator code (mbssep, wchar_t tabchar) that must be rewritten for ASCII, plus the obsolete() argv rewrite pattern that leaks memory.

| Issue | Tier | Resolution |
|-------|------|------------|
| `<stdlib.h>` | Tier 1 | Replaced with `<amiport/stdlib.h>` |
| `<unistd.h>` | Tier 1 | Replaced with `<amiport/unistd.h>` |
| `<err.h>` (err/errx/warn/warnx) | Tier 1 | Replaced with `<amiport/err.h>` |
| `<getopt.h>` (getopt) | Tier 1 | Replaced with `<amiport/getopt.h>` |
| `<locale.h>` (setlocale) | Tier 1 | Removed; stubbed via `<amiport/unistd.h>` |
| `<wchar.h>` (wchar_t, putwchar, mbtowc, wcslen) | Tier 3 | Removed; tabchar/mbssep rewritten for ASCII |
| `getline()` | Tier 1 | Via `<amiport/stdio_ext.h>` macro |
| `reallocarray()` | Tier 1 | Via `<amiport/string.h>` macro |
| `ssize_t` | Tier 1 | Replaced with `long` |
| `pledge()`/`unveil()` | Stub | Macro stubs |
| `MB_CUR_MAX` | Pitfall | Guarded -- strlen() check instead of mbtowc (crash-patterns #11) |
| `obsolete()` argv rewrite | Memory | malloc'd strings tracked for atexit cleanup |
| Exit codes | Tier 1 | `exit(1)` -> `exit(10)` (RETURN_ERROR) |

## Transformations Applied

| File | Line(s) | Original | Transformed | Comment |
|------|----------|----------|-------------|---------|
| join.c | 44 | `<err.h>` | removed | err.h via amiport/err.h |
| join.c | 46 | `<locale.h>` | removed | setlocale stubbed |
| join.c | 48 | `<stdlib.h>` | `<amiport/stdlib.h>` | Exit macro |
| join.c | 50 | `<unistd.h>` | `<amiport/unistd.h>` | POSIX shim |
| join.c | 51 | `<wchar.h>` | removed | No wchar on AmigaOS |
| join.c | 53-57 | (none) | glob.h, string.h, err.h, getopt.h, stdio_ext.h | Shim headers |
| join.c | 60-61 | pledge/unveil | `#define pledge(p, e) (0)` | Stubs |
| join.c | 111 | `wchar_t tabchar[]` | `char tabchar[]` | ASCII-only delimiters |
| join.c | 120 | `char *mbssep(char **, const char *)` | Rewritten for ASCII | Was wchar_t* parameter |
| join.c | 129-131 | (none) | `obsolete_allocs[]` tracking | Memory leak prevention |
| join.c | 134-142 | (none) | `cleanup()` | Frees obsolete allocs + argv |
| join.c | 224 | `mbtowc(tabchar, optarg, MB_CUR_MAX)` | `strlen(optarg) != 1` | ASCII-only tab check |
| join.c | 326 | `ssize_t len` | `long len` | C89 type |
| join.c | 416-442 | `mbssep()` wchar version | Rewritten for ASCII | Single-byte delimiter matching |
| join.c | 554 | `putwchar(*tabchar)` | `putchar((unsigned char)tabchar[0])` | ASCII delimiter output |
| join.c | 576 | (none) | `fieldno = 0, filenum = 0` init | Silence GCC uninitialized warning |
| join.c | 681-682 | (none) | `obsolete_allocs[i++] = t` | Track malloc'd argv rewrites |
| join.c | all | `err(1,...)`/`errx(1,...)` (20+ sites) | `err(10,...)`/`errx(10,...)` | RETURN_ERROR |
| join.c | 705 | `exit(1)` | `exit(10)` | RETURN_ERROR |

## Shim Functions Exercised

- `amiport_err()`, `amiport_errx()`, `amiport_warnx()`
- `amiport_reallocarray()`
- `amiport_getline()` (via macro)
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
| Source files | join.c (1 file) |

## Test Results

39 tests via FS-UAE. Pass rate: 39/39 (100%).

| Category | Count | Description |
|----------|-------|-------------|
| Functional (flags) | 12 | Default join, -1, -2, -j, -t, -o (2 variants), -a 1, -a 2, -v 1, -v 2, -e |
| Error paths | 9 | -a/-v mutual exclusion, field < 1, file number not 1/2, missing file, multi-char tab, one file |
| Exit codes | 1 | No-match files produce RC=0 |
| Edge cases | 5 | Empty files (both positions), single-line match, multi-key cross product, comma separator |
| Amiga-specific | 3 | WORK: volume paths, -o comma parsing, colon separator with WORK: paths |
| Real-world | 4 | Passwd-style colon join, full outer join, missing-record detection, field reorder report |
| Stress | 3 | 200-line join (first+last), -v 1 on fully matched, -e precision |

## Memory Safety

- `obsolete()` argv rewrites tracked in `obsolete_allocs[]` array (up to 8 entries) for atexit cleanup
- `atexit(cleanup)` registered immediately after `amiport_expand_argv()`
- `cleanup()` frees obsolete allocs, expanded argv, and flushes stdout
- LINE set/field allocations grow via reallocarray() (overflow-safe)
- getline() buffers owned by LINE structs persist for program lifetime
- Reviewed by memory-checker agent

## Performance Notes

- join reads both files into LINE set arrays, comparing by join field -- O(n+m) for sorted inputs
- Field splitting via mbssep() is now a simple byte scan (faster than original wchar_t version)
- No critical performance concerns for typical text processing

## Known Limitations

1. **No multibyte field separator** -- The `-t` flag accepts only a single ASCII byte as delimiter. Multibyte characters (e.g., UTF-8 encoded characters) are not supported.
2. **Tab as separator** -- Literal tab as a `-t` argument is difficult to pass through AmigaDOS command parsing. Use comma or colon separators in test scenarios.
3. **Both files cannot be stdin** -- `join - -` is explicitly rejected (same as upstream).

## Review

Reviewed with `/review-amiga` and `memory-checker` agent. Score: **READY**.
