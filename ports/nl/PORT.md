# Port: nl

## Overview

| Field | Value |
|-------|-------|
| Program | nl |
| Version | 1.8 |
| Source | OpenBSD usr.bin/nl (v1.8, Dec 2022) |
| Category | 1 -- CLI Filter |
| License | BSD 2-Clause (NetBSD Foundation) |
| Original Author | Klaus Klein, NetBSD Foundation |
| Port Date | 2026-03-26 |

## Description

nl numbers lines from files or standard input with configurable formatting. It supports logical page sections (header/body/footer with `\:\:\:`, `\:\:`, `\:` delimiters), multiple numbering types (-b a/t/n/p), format styles (-n ln/rn/rz), custom separators (-s), custom delimiters (-d), line number width (-w), increment (-i), and starting value (-v). This is the OpenBSD implementation with regex support for pattern-based numbering.

## Prior Art on Aminet

Researched via aminet-researcher agent. No modern standalone noixemul port found.

## Portability Analysis

Verdict: **MODERATE** -- Single file with regex dependency (Tier 2), wchar/multibyte paths, and getline() requirement.

| Issue | Tier | Resolution |
|-------|------|------------|
| `<regex.h>` (regcomp/regexec/regfree/regerror) | Tier 2 | Via `<amiport-emu/regex.h>` (posix-emu library) |
| `<stdlib.h>` | Tier 1 | Replaced with `<amiport/stdlib.h>` |
| `<unistd.h>` | Tier 1 | Replaced with `<amiport/unistd.h>` |
| `<err.h>` (err/errx) | Tier 1 | Replaced with `<amiport/err.h>` |
| `<getopt.h>` (getopt) | Tier 1 | Replaced with `<amiport/getopt.h>` |
| `<locale.h>` (setlocale) | Tier 1 | Removed; stubbed via `<amiport/unistd.h>` |
| `<wchar.h>` (mbrlen) | Tier 3 | Removed; delimiter parsing simplified to ASCII |
| `getline()` | Tier 1 | Via `<amiport/stdio_ext.h>` macro |
| `getprogname()` | Tier 1 | Macro to `__progname` from `<amiport/glob.h>` |
| `strtonum()` | Tier 1 | Via `<amiport/err.h>` |
| `NL_TEXTMAX` | Tier 1 | Defined as 256 fallback |
| `__dead` attribute | Tier 1 | Defined as empty (GCC handles noreturn separately) |
| `pledge()` | Stub | Macro stub |
| `ssize_t` | Tier 1 | Replaced with `long` |
| Exit codes | Tier 1 | `EXIT_FAILURE` -> `exit(10)` (RETURN_ERROR) |

## Transformations Applied

| File | Line(s) | Original | Transformed | Comment |
|------|----------|----------|-------------|---------|
| nl.c | 43 | `<err.h>` | `<amiport/err.h>` | Error shim |
| nl.c | 46 | `<locale.h>` | removed | setlocale no-op |
| nl.c | 48-49 | `<regex.h>` | `<amiport-emu/regex.h>` | Tier 2 regex emulation |
| nl.c | 51 | `<stdlib.h>` | `<amiport/stdlib.h>` | Exit macro |
| nl.c | 53 | `<unistd.h>` | `<amiport/unistd.h>` | POSIX shim |
| nl.c | 54 | `<wchar.h>` | removed | No wchar on AmigaOS |
| nl.c | 57-63 | (none) | `<amiport/stdio_ext.h>`, `<amiport/glob.h>`, `<amiport/getopt.h>` | getline, argv, getopt |
| nl.c | 66 | pledge() | `#define pledge(p, e) (0)` | Stub |
| nl.c | 76-78 | (none) | `NL_TEXTMAX` fallback | Define as 256 |
| nl.c | 82-83 | (none) | `getprogname()` macro | Maps to `__progname` |
| nl.c | 110-113 | regex_t init | `{ {0}, 0, 0, 0 }` | Silence "missing braces" warning for amiport_emu_regex_t |
| nl.c | 124 | `wchar_t delim[MB_LEN_MAX]` | `char delim[2]` | ASCII-only delimiters |
| nl.c | 153-163 | (none) | getline_buf tracking + cleanup() | atexit cleanup |
| nl.c | 196-209 | mbrlen() multibyte parsing | `strlen(optarg)` checks | ASCII-only delimiter handling |
| nl.c | 299 | `ssize_t linelen` | `long linelen` | C89 type |
| nl.c | 376 | (none) | `getline_buf = NULL;` | Prevent double-free |
| nl.c | all | `err(1,...)`/`errx(1,...)` (14 sites) | `err(10,...)`/`errx(10,...)` | RETURN_ERROR |
| nl.c | 432 | `exit(EXIT_FAILURE)` | `exit(10)` | RETURN_ERROR |

## Shim Functions Exercised

- `amiport_err()`, `amiport_errx()`
- `amiport_strtonum()`
- `amiport_getline()` (via macro)
- `amiport_expand_argv()`, `amiport_free_argv()`
- `amiport_exit()` (via macro)
- `amiport_emu_regcomp()`, `amiport_emu_regexec()`, `amiport_emu_regfree()`, `amiport_emu_regerror()` (Tier 2)

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68000+ |
| CFLAGS | `-O2 -noixemul -m68000 -Wall -I../../lib/posix-emu/include` |
| Libraries | `-lamiport-emu -lamiport` (Tier 2 regex + Tier 1 posix-shim) |
| Stack size | `__stack = 16384` (16 KB) |
| Source files | nl.c (1 file) |
| `__nowild` | `1` (suppress wildcard expansion for `-b p<regex>` args) |

## Test Results

42 tests via FS-UAE. Pass rate: 42/42 (100%).

| Category | Count | Description |
|----------|-------|-------------|
| Functional (flags) | 18 | -b a/t/n, -b p (regex), -h a, -f a, -n ln/rn/rz, -w, -v, -i, -s, -l, -p, -d |
| Logical page sections | 2 | Header/body/footer boundaries, multi-page reset |
| Error paths | 8 | Invalid flag, invalid -n/-b/-d/-i/-w values, missing file, two files |
| Edge cases | 5 | Empty file, all-blank, long line, special chars, WORK: paths |
| Amiga-specific | 2 | WORK: volume paths, -n rz format with WORK: |
| Real-world | 3 | Source listing format, diff annotation, multi-page header numbering |
| Stress | 3 | 500-line file default, 500-line -b a -n rz, precision at line 250 |

## Memory Safety

- `getline_buf` tracked at file scope for atexit cleanup -- prevents leak when err()/errx() exits
- `getline_buf = NULL` after free() in filter() -- prevents double-free in atexit
- `atexit(cleanup)` registered immediately after `amiport_expand_argv()`
- Regex patterns allocated by regcomp() are freed via regfree() before recompilation
- Reviewed by memory-checker agent

## Performance Notes

- nl processes input line-by-line via getline() -- efficient for typical text files
- No hot inner loops beyond the line-reading loop
- printf() formatting per line is the main cost; negligible for text processing

## Known Limitations

1. **Regex POSIX character classes not supported** -- `-b p` pattern matching uses Tier 2 regex emulation, which does not support `[:alpha:]`, `[:digit:]` etc. Use character ranges `[a-zA-Z]`, `[0-9]` instead.
2. **No multibyte delimiter support** -- The `-d` flag accepts at most 2 ASCII bytes as the delimiter pair. Multibyte characters are not supported.
3. **Maximum regex groups** -- Tier 2 regex supports up to 9 capture groups.

## Review

Reviewed with `/review-amiga` and `memory-checker` agent. Score: **READY**.
