---
name: join_analysis
description: OpenBSD join v1.34 portability analysis: EASY verdict, Category 1 CLI, wchar/mbtowc locale risk, putwchar needs shim check, getline shim, reallocarray shim, strsep libnix-native, MB_CUR_MAX runtime pitfall, exit code fixes, __progname strong symbol, obsolete() malloc leak
type: project
---

OpenBSD join v1.34 — portability analysis.

**Verdict:** EASY

**Why:** Single-file CLI, no fork/exec/threads/sockets/mmap. All blocking POSIX calls have Tier 1 shims or are in libnix. The only runtime risk is the wchar/mbtowc/putwchar locale path under `MB_CUR_MAX` and `putwchar` availability.

**Key findings:**
- `getline()` → `amiport_getline()` (Tier 1)
- `reallocarray()` → `amiport_reallocarray()` (Tier 1)
- `strsep()` — in libnix, no shim needed
- `pledge()`/`unveil()` — stub as macros
- `setlocale(LC_CTYPE, "")` — replace with `amiport_setlocale(LC_ALL, "C")` or no-op
- `mbtowc()` / `wctomb()` / `wcslen()` — in libnix wchar.h, available but locale-dependent
- `putwchar()` — in libnix, available
- `MB_CUR_MAX` — runtime function call pitfall; governs the `-t` tab character path
- `wchar_t tabchar[] = L" \t"` — wchar global, fine for C89 with libnix
- `err(1,...)` / `errx(1,...)` — exit codes need changing to 10
- `exit(1)` in usage() — needs → `exit(10)`
- `__progname` — use as extern (amiport_expand_argv provides strong symbol)
- `u_long` — BSD typedef, define as `unsigned long` or use `<sys/types.h>` stub
- `ssize_t` — needs `<sys/types.h>` or typedef in compat header
- `obsolete()` malloc leak — `malloc(len+3)` strings assigned to argv never freed
- `MB_CUR_MAX` in `-t` option processing — if returns >1 on libnix, mbtowc check mismatch
- `getopt()` short-options only, no `getopt_long` used — safe with `amiport_getopt()`

**How to apply:** Use as reference for similar single-file wchar-using OpenBSD CLI tools.
