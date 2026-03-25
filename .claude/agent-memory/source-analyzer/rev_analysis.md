---
name: rev_analysis
description: OpenBSD rev v1.16 portability analysis: EASY verdict, Category 1 CLI, MB_CUR_MAX runtime pitfall, fclose(stdin) guard needed, exit code fixes, __progname weak symbol
type: project
---

OpenBSD rev v1.16 — single-file (144 lines), Category 1 CLI. Portability verdict: EASY.

Key findings:
- MB_CUR_MAX: libnix expands to __locale_mb_cur_max() runtime call; can return >1 on AmigaOS even without locale support. The `multibyte` flag will be set TRUE, entering the UTF-8 multibyte path on every run. This is a logic bug, not a compile error. Fix: guard with `#ifndef __AMIGA__` around the MB_CUR_MAX check, hardcode `multibyte = 0`.
- getline(): NOT in libnix. Use amiport_getline() from <amiport/stdio_ext.h>. Shim exists and is Tier 1.
- pledge(): stub as `#define pledge(p, e) (0)`. Both calls (lines 57 and 71) need stubbing.
- err(1, ...) / warn(): Use <amiport/err.h>. Exit codes: err(1, ...) must become err(10, ...) for RETURN_ERROR.
- fclose(fp) at line 131: fp can be stdin (when path == NULL). amiport_fclose() does NOT guard against closing stdin. Must add `if (fp != stdin) fclose(fp)` guard. This is the known-pitfalls fclose(stdin) issue.
- exit(1) in usage(): must become exit(10) for Amiga shell scripts.
- setlocale(LC_CTYPE, ""): shim in <amiport/unistd.h> returns "C" always. Include order matters.
- putchar() in hot loop (lines 119, 122): per-character I/O, 68k perf concern. perf-optimizer should flag this.
- __progname at line 139: weak symbol known pitfall (crash-patterns known-pitfalls) — define char *__progname = "rev"; in ported source.
- ssize_t: defined in <amiport/unistd.h>. Must include it or <sys/types.h>.
- sys/types.h: libnix provides this, fine.
- No C99 features beyond ssize_t. No getopt_long. No 64-bit types. No fork/exec. Trivial port.

Required headers: <amiport/unistd.h>, <amiport/err.h>, <amiport/stdlib.h>, <amiport/stdio_ext.h>
Required shim macros: #define pledge(p,e) (0)
Exit code fix: err(1,...) -> err(10,...), exit(1) -> exit(10)
__Why:__ EASY — no Tier 2 or Tier 3 issues. All blockers are Tier 1 shims or known-pitfalls fixes.
__How to apply:__ code-transformer can handle all transforms mechanically except MB_CUR_MAX guard which requires source-level #ifdef.
