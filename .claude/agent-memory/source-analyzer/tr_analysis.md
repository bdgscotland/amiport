---
name: tr_analysis
description: OpenBSD tr v1.22 portability analysis: EASY verdict, Category 1 CLI, pledge stub, all err(1)/errx(1) exit codes, no locale/wchar, bzero in libnix, u_char via sys/types.h
type: project
---

OpenBSD tr v1.22 (str.c v1.15) — 3 files, ~340 lines. Category 1 CLI.

**Verdict:** EASY

**Tier counts:** 4 Tier 1 (green), 0 Tier 2, 0 Tier 3, 1 minor arch issue

## Key findings

1. **pledge stub** — `#define pledge(p, e) (0)`. Delete the 2-line err() block (lines 91-92 in tr.c).
2. **Exit codes** — 11 occurrences of `err(1,...)` / `errx(1,...)` / `exit(1)` all need → exit code 10. `exit(0)` calls are correct.
3. **err.h** — Both tr.c and str.c include `<err.h>` → replace with `<amiport/err.h>`.
4. **getopt** — Short options only (-C, -c, -d, -s). Use `<amiport/getopt.h>`. No getopt_long used.
5. **bzero()** — Used in tr.c:224. Already in libnix string.h — no shim needed.
6. **u_char** — Used in str.c:239 as cast. Defined in `<sys/types.h>` which libnix provides.
7. **reallocarray()** — Used in str.c:181,193. Available in libnix directly (not just via amiport shim).

## No concerns

- Zero locale/wchar/MB_CUR_MAX surface. All character classes use ctype.h predicates over 0-255.
- No stat/inode comparison, no stub value risk.
- No large local arrays (all big arrays are globals in BSS).
- No struct-by-value returns > 8 bytes.
- No offsetof alignment risk.
- Fully C89-clean (0 C99 features).
- NCHARS/OOBCH macros expand to compile-time constants from UCHAR_MAX.
- No getopt_long (no broken libnix symbol risk).
- No __progname definition needed.

## Transformation checklist

1. `#define pledge(p, e) (0)` + remove err() block
2. All `err(1,...)` → `err(10,...)`; all `errx(1,...)` → `errx(10,...)`; `exit(1)` in usage() → `exit(10)`
3. `<err.h>` → `<amiport/err.h>` in both files
4. `<stdlib.h>` → `<amiport/stdlib.h>`
5. Add `#include <amiport/getopt.h>`
6. Add `$VER:` string, `long __stack = 16384`
7. Add `amiport_expand_argv()` + `atexit(cleanup)` in main
