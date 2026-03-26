---
name: rs_analysis
description: OpenBSD rs v1.30 portability analysis: MODERATE verdict, missing mbsavis() from vis.c is key blocker, getline shim, ssize_t/%zd C99 issue, setlocale MB_CUR_MAX risk, exit code fixes
type: project
---

# rs v1.30 Portability Analysis

**Verdict:** MODERATE

**Date:** 2026-03-26

## Key Findings

### Missing Source File (Linker Blocker)
- `mbsavis()` is declared in rs.c (line 84) and called at lines 154, 170 but is NOT defined in rs.c
- It is an OpenBSD internal function from `lib/vis.c` (vis(3) API) — converts multibyte strings to visual representation and returns display width
- Must provide a local implementation: for ASCII-only Amiga target, can be simplified to strdup() + strlen() since no multibyte support
- Signature: `int mbsavis(char **outp, const char *inp)` — allocates a visual representation of `inp` into `*outp`, returns display width

### POSIX Issues
- `getline()` — Tier 1 shim: `amiport_getline()` (line 319)
- `reallocarray()` — available in BOTH libnix AND shim (line 337) — use as-is or via shim
- `strtonum()` — Tier 1 shim via `amiport/err.h` (multiple lines in getargs())
- `pledge()` — stub macro (line 105)
- `strsep()` — available natively in libnix (line 170) — no action needed
- `err()`/`errx()`/`warnx()` — via `amiport/err.h`
- `setlocale(LC_CTYPE, "")` — returns "C" on AmigaOS; shim is correct

### C99 Issues
- `ssize_t` for `curlen` (line 314) — needs `long` replacement
- `%zd` printf format (line 317) — needs `%ld` with cast
- `static ssize_t curlen` declaration at line 314 — C89 issue (type only, not ssize_t availability per se, but `ssize_t` is POSIX)

### Exit Code Issues
- `exit(1)` at lines 113, 116 (in main) — change to `exit(0)` (these are success paths)
- `exit(1)` in usage() at line 235 — change to `exit(10)` (RETURN_ERROR)
- `errx(1, ...)` at line 280 — change to `errx(10, ...)`

### MB_CUR_MAX / Locale Risk
- `setlocale(LC_CTYPE, "")` at line 103 may cause MB_CUR_MAX to return > 1 at runtime
- `mbsavis()` replacement must not use MB_CUR_MAX or wchar functions
- The local implementation should be ASCII-only with `#ifdef __AMIGA__` guard

### Stack Issues
- `get_line()` has `static size_t cursz` and `static ssize_t curlen` — already static, no stack risk
- No large local arrays identified
- `__stack = 32768` should be sufficient

### __progname
- `extern char *__progname` used in usage() (line 230) — provided by `amiport_expand_argv()` per current strong-symbol convention. No local definition needed.

## Shims Required
- `#include <amiport/unistd.h>` (replaces `<unistd.h>`)
- `#include <amiport/err.h>` (provides err, errx, warnx, strtonum)
- `#define pledge(p, e) (0)` stub
- `amiport_getline()` for `getline()`
- Local `mbsavis()` implementation (ASCII-only, compile with `#ifdef __AMIGA__`)
- `ssize_t` → `long` in get_line()
- `%zd` → `%ld` with `(long)` cast

## Build Notes
- Single-file port — only rs.c
- `-std=gnu99` NOT required (only one C99 issue: ssize_t, easily fixed with typedef/replacement)
- Category 1 CLI tool
- Test strategy: vamos
