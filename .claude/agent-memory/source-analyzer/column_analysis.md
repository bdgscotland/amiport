---
name: column_analysis
description: OpenBSD column v1.27 portability analysis: EASY verdict, Category 1 CLI, wchar/locale loop uses MB_CUR_MAX runtime pitfall, ioctl/TIOCGWINSZ shim, getline shim, reallocarray shim, strtonum shim, pledge stub, exit code fixes, __dead strip, ssize_t/%zd C99 fix
type: project
---

## OpenBSD column v1.27 — Portability Analysis

**Verdict:** EASY
**Category:** 1 — CLI tool (stdin/stdout/file I/O, getopt)
**Test strategy:** vamos

### Key findings

1. `#include <sys/ioctl.h>` + `ioctl(STDOUT_FILENO, TIOCGWINSZ, &win)` — Tier 1 via `amiport_ioctl()`. Already implemented. Falls back to 80x24.
2. `getline()` — Tier 1 via `amiport_getline()`. Header-only `#include <amiport/unistd.h>`.
3. `reallocarray()` — already in libnix, no shim needed. But amiport's version is also fine.
4. `strtonum()` — Tier 1 via `amiport/err.h` header-only macro.
5. `pledge()` — stub as `#define pledge(p, e) (0)`.
6. `__dead` macro — define as `__attribute__((noreturn))` or `#define __dead`.
7. `ssize_t` used as local var and in getline return — fine with `#include <amiport/unistd.h>`.
8. `err(1,...)` / `errx(1,...)` — change exit codes to 10 (RETURN_ERROR).
9. `setlocale(LC_CTYPE, "")` — returns "C" from amiport shim; benign.
10. `MB_CUR_MAX` in inner loop (lines 250, 259) — STUB VALUE WARNING: MB_CUR_MAX is a runtime function call in libnix. It may return >1 on AmigaOS. However, `mbtowc()` in the "C" locale always returns 1 per char, so the loop is correct but may call a function instead of a constant. Guard with `#ifndef __AMIGA__` if performance is critical, but not a correctness blocker.
11. `wchar_t`, `wcschr()`, `wcwidth()`, `mbtowc()`, `mbstowcs()`, `iswspace()` — ALL present in libnix. No shim needed.
12. `L"\t "` wide string literal — C89 extension, accepted by bebbo-gcc.
13. `%*s` printf format — fine (C89).
14. `ssize_t llen` used with `getline` — `%zd` NOT used (compared directly to -1, fine).

### No Tier 2 or Tier 3 issues.

### Exit code fix needed
- `err(1,...)`, `errx(1,...)`, `exit(1)` in usage() — change to 10.
- `eval = 1` (error flag returned via `return eval`) — change to 10.

### Memory pattern
- All allocations via `ereallocarray()` wrapper — calls `reallocarray()` then `err()` on failure.
- No complex allocations. `strdup()` used for field content.
- buf from getline() is `free()`'d implicitly (left as NULL out of loop). Need atexit cleanup guard.
- No double-fopen issue, no dirname() issue.
