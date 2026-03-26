---
name: paste_analysis
description: OpenBSD paste v1.27 portability analysis: EASY verdict, Category 1 CLI, sys/queue.h vendor SIMPLEQ macros, getline shim, __dead strip, pledge stub, exit code fixes
type: project
---

# paste 1.27 Portability Analysis

**Verdict:** EASY
**Category:** 1 (CLI tool)
**Date:** 2026-03-26

## Key Issues

1. `sys/queue.h` — not available on AmigaOS. Vendor SIMPLEQ macros from mg/patch ports' queue.h. Only needs: SIMPLEQ_HEAD, SIMPLEQ_ENTRY, SIMPLEQ_HEAD_INITIALIZER, SIMPLEQ_INSERT_TAIL, SIMPLEQ_FOREACH.

2. `getline()` — not in libnix. Shim via `amiport/stdio_ext.h` (already implements `amiport_getline`). Return type is `long` not `ssize_t` on shim — declare `long len` or cast. Actually `ssize_t` is defined in `amiport/unistd.h` as `long`, so compatible.

3. `__dead` attribute — OpenBSD-specific, not available on bebbo-gcc. Strip or replace with `__attribute__((noreturn))`.

4. `pledge()` — stub as `#define pledge(p, e) (0)`.

5. `err(1, ...)` / `errx(1, ...)` — exit code 1 must become exit code 10 for Amiga shells.

6. `exit(1)` in usage() — must become `exit(10)`.

7. `__progname` — provided by `amiport_expand_argv()`; `extern char *__progname` declaration is fine.

8. `<err.h>` — replace with `<amiport/err.h>`.

9. `<unistd.h>` — replace with `<amiport/unistd.h>` (provides ssize_t, getopt).

10. `getopt()` — libnix getopt (short options only) works for `-d` and `-s`. No getopt_long needed.

## Exit Code Notes

- `usage()` calls `exit(1)` — change to `exit(10)`
- `err(1, ...)` — change exit code to `err(10, ...)`
- `errx(1, ...)` — change to `errx(10, ...)`

## Memory Notes

- `getline` buffer is allocated once per function and freed at end. Passes `&line` / `&linesize` to getline. No leak on normal path.
- On error exit via `err()`, `line` buffer is leaked. Should track via atexit or accept as minor leak (single invocation).
- `lp` structs from `parallel()` malloc are never freed (program exits after function returns). Accept as minor leak.

## SIMPLEQ Usage Pattern

paste.c only uses: SIMPLEQ_HEAD, SIMPLEQ_HEAD_INITIALIZER, SIMPLEQ_ENTRY, SIMPLEQ_INSERT_TAIL, SIMPLEQ_FOREACH. The mg/patch vendored queue.h contains all of these. Copy queue.h to ported/ and change `#include <sys/queue.h>` to `#include "queue.h"`.

## fclose(stdin) Risk

The code correctly guards all fclose calls: `if (lp->fp != stdin) fclose(lp->fp)` and `if (fp != stdin) fclose(fp)`. No risk here.

## Stack Safety

No large local buffers. getline buffer is heap-allocated. SIMPLEQ list nodes are heap-allocated. Stack usage is minimal.
