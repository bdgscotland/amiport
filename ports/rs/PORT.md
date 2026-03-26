# Port: rs

## Overview

| Field | Value |
|-------|-------|
| Program | rs |
| Version | 1.30 (port revision: 1) |
| Source | OpenBSD (GitHub mirror) |
| Category | 1 -- CLI |
| License | BSD-3-Clause |
| Original Author | John Kunze, UC Berkeley |
| Port Date | 2026-03-26 |

## Description

rs reshapes a data array -- it reads whitespace-delimited or character-delimited input and reformats it into a specified number of rows and columns. It supports transposition (-t, -T), custom input/output separators (-s, -S, -c, -C), column squeezing (-z), right justification (-j), null padding (-n), shape reporting (-h, -H), and recycling of elements (-y). This is the OpenBSD implementation, useful for reformatting tabular data in shell pipelines.

## Prior Art on Aminet

No existing standalone 68k port of rs found on Aminet. No equivalent utility exists in AmigaDOS.

## Portability Analysis

Verdict: **MODERATE** -- Single-file, but requires multibyte stub (mbsavis), getline, reallocarray, strtonum, and locale removal.

| Issue | Tier | Resolution |
|-------|------|------------|
| `<err.h>` (err/errx/warn/warnx) | Tier 1 | Replaced with `<amiport/err.h>` |
| `<stdlib.h>` | Tier 1 | Replaced with `<amiport/stdlib.h>` (exit macro) |
| `<unistd.h>` | Tier 1 | Replaced with `<amiport/unistd.h>` (setlocale stub) |
| `<locale.h>` (setlocale) | Tier 1 | Removed -- setlocale stub in `<amiport/unistd.h>` |
| `getline()` | Tier 1 | Via `<amiport/stdio_ext.h>` macro |
| `reallocarray()` | Tier 1 | Via `<amiport/string.h>` macro |
| `strtonum()` | Tier 1 | Via `<amiport/err.h>` macro |
| `getopt()` | Tier 1 | Via `<amiport/getopt.h>` |
| `mbsavis()` | Stub | Local ASCII stub -- returns strlen as display width |
| `ssize_t` | Tier 1 | Replaced with `long` |
| `%zd` format | Tier 1 | Changed to `%ld` |
| `pledge()` | Stub | `#define pledge(p, e) (0)` |
| `__progname` | Tier 1 | Provided by `<amiport/glob.h>` |
| Wildcard expansion | Tier 1 | `amiport_expand_argv()` via `<amiport/glob.h>` |
| Exit codes | Tier 1 | `exit(1)` / `errx(1,...)` -> `exit(10)` / `errx(10,...)` |
| Floating-point (gutter proportion) | Tier 1 | Available in libnix; requires `-lm` |

## Transformations Applied

| File | Line(s) | Original | Transformed | Comment |
|------|----------|----------|-------------|---------|
| rs.c | 46 | `#include <err.h>` | `#include <amiport/err.h>` | amiport shim + strtonum |
| rs.c | 49 | `#include <locale.h>` | removed | setlocale stub elsewhere |
| rs.c | 52 | `#include <stdlib.h>` | `#include <amiport/stdlib.h>` | exit() macro |
| rs.c | 55 | `#include <unistd.h>` | `#include <amiport/unistd.h>` | setlocale stub |
| rs.c | 57 | (none) | `#include <amiport/getopt.h>` | getopt shim |
| rs.c | 59 | (none) | `#include <amiport/string.h>` | reallocarray |
| rs.c | 61 | (none) | `#include <amiport/stdio_ext.h>` | getline |
| rs.c | 63 | (none) | `#include <amiport/glob.h>` | Wildcard expansion, __progname |
| rs.c | 66 | pledge call | `#define pledge(p, e) (0)` | Stub |
| rs.c | 77-84 | `mbsavis()` (vis.h) | Local ASCII stub | No multibyte/vis on AmigaOS |
| rs.c | 137-142 | (none) | `cleanup()` function | atexit handler |
| rs.c | 148-150 | (none) | `amiport_expand_argv()` + `atexit(cleanup)` | Amiga shell glob |
| rs.c | 157 | `err(1, "pledge")` | `err(10, "pledge")` | RETURN_ERROR |
| rs.c | 286 | `exit(1)` | `exit(10)` | RETURN_ERROR |
| rs.c | 331 | `errx(1, ...)` | `errx(10, ...)` | RETURN_ERROR |
| rs.c | 366 | `ssize_t curlen` | `long curlen` | C89 on 68k |
| rs.c | 369 | `%zd` | `%ld` | libnix printf compatibility |
| rs.c | 374 | `err(1, NULL)` | `err(10, NULL)` | RETURN_ERROR |
| rs.c | 393 | `err(1, ...)` | `err(10, ...)` | RETURN_ERROR |

## Shim Functions Exercised

- `amiport_err()`, `amiport_errx()`, `amiport_warn()`, `amiport_warnx()`
- `amiport_strtonum()`
- `amiport_reallocarray()`
- `amiport_getline()` (via getline() macro)
- `amiport_expand_argv()`, `amiport_free_argv()`
- `amiport_exit()` (via exit() macro)

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68000+ |
| CFLAGS | `-O2 -noixemul -m68000 -Wall` |
| LDFLAGS | `-lamiport -lm` |
| Libraries | libamiport.a (posix-shim), libm.a (math) |
| Stack size | 16384 bytes |
| Binary size | ~51 KB |

## Test Results

No FS-UAE TEST-REPORT.md found. Testing status unknown -- rs requires FS-UAE test suite generation via test-designer agent.

## Memory Safety

No agent-memory from memory-checker found. Manual analysis: `getptrs()` uses `reallocarray()` with geometric growth for the element array -- allocated once, grows as needed, never explicitly freed (process-lifetime). `getline()` allocates/grows the `curline` buffer -- never freed (process-lifetime). `colwidths` allocated by `calloc()` in `prepfile()` -- never freed. These are all process-lifetime allocations acceptable for a short-lived CLI tool. `amiport_expand_argv()` freed via atexit handler.

## Performance Notes

The main performance concern is the `getptrs()` reallocation pattern -- it doubles the element array when full, which is standard geometric growth. The `mbsavis()` stub calls `strlen()` on every entry, adding O(n) cost per element. On 68k, the floating-point gutter proportion calculation (`maxwidth * propgutter / 100.0`) triggers soft-float -- acceptable since it runs once per invocation.

## Known Limitations

1. **No multibyte/wide character support** -- The `mbsavis()` function is stubbed to return `strlen()`. Display width equals byte count, which is correct for ASCII but wrong for multibyte encodings. No practical impact on AmigaOS (no locale support).
2. **No vis() escape processing** -- The OpenBSD version uses `vis()` to escape non-printable characters in the display. The stub passes strings through unmodified.
3. **Stack size 16 KB** -- Higher than other ports due to recursive element processing and `getline()` buffer growth.
