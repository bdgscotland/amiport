---
name: fold_analysis
description: OpenBSD fold v1.18 portability analysis: EASY verdict, Category 1 CLI, MB_CUR_MAX runtime pitfall in isu8cont(), wcwidth/mbtowc available in libnix, pledge stub, exit codes, __dead macro
type: project
---

## fold v1.18 (OpenBSD)

**Verdict:** EASY
**Category:** 1 (CLI tool, stdin/stdout/file I/O)
**Required shims:** posix-shim
**Test strategy:** vamos

## Key Findings

1. **MB_CUR_MAX runtime call in isu8cont()** (crash-patterns #11): `isu8cont()` at line 265 checks `MB_CUR_MAX > 1`. On libnix, this is a runtime function call that returns > 1 even without real locale support. Result: high-bit continuation bytes will be detected as UTF-8 continuation bytes, causing extra read-ahead. Mitigated by mbtowc() returning -1 and len=1 fallback -- no crash, but unnecessary overhead. Fix: guard with `#ifndef __AMIGA__`. Or: replace entire isu8cont() with `return 0;` since AmigaOS has no multibyte support.

2. **setlocale() shim**: amiport_setlocale() returns "C" only. The call `setlocale(LC_CTYPE, "")` is harmless since the shim ignores it, but should use `amiport_setlocale()`.

3. **wcwidth() + mbtowc()**: Both are available in libnix wchar.h -- no shim needed. No compile issue.

4. **pledge() stubs**: Two calls at lines 64-65 and 111-112. Stub both as `#define pledge(p, e) (0)`.

5. **exit codes**: err(1,...)/errx(1,...) need → err(10,...)/errx(10,...).

6. **__dead macro**: Used on usage() at line 50/268. Strip it or `#define __dead __attribute__((noreturn))`.

7. **strtonum()**: Available via amiport/err.h.

8. **reallocarray()**: Available natively in libnix.

9. **freopen()**: Available in libnix stdio.

10. **No fork/exec, no pthreads, no sockets** -- clean CLI.
