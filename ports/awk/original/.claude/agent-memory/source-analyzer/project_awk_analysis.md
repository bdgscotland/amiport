---
name: awk portability analysis
description: One True Awk (Brian Kernighan, version 20251225) portability analysis for AmigaOS 3.x — key issues and decisions
type: project
---

Analyzed One True Awk version 20251225. Port category: Category 2 (Scripting Interpreter).

**Files for final binary:** main.c, run.c, lex.c, lib.c, b.c, tran.c, parse.c, awkgram.tab.c, proctab.c
**Build-time tools (NOT compiled into binary):** maketab.c, awkgram.y

**Key issues:**
1. Requires -std=gnu99 (C99): stdbool.h, stdint.h, bool/true/false, // comments (41+ occurrences), noreturn, intmax_t/uintmax_t, wchar_t
2. MB_CUR_MAX runtime check: awk_mb_cur_max = MB_CUR_MAX at startup -- will be non-zero on libnix, enabling wchar/mbtowc/wctomb paths that likely don't work
3. popen/pclose: 4 calls in run.c -- amiport_emu_popen available (AmigaDOS PIPE: device)
4. system(): 1 call in run.c line 2110, with WIFEXITED/WEXITSTATUS macros -- needs amiport_emu_system + W* macros stub
5. signal(SIGFPE, fpecatch) in main.c line 158: #ifdef SA_SIGINFO path (sigaction) is safe (won't compile), the #else path uses signal(SIGFPE) which libnix ignores -- functionally OK (SIGFPE silently ignored)
6. /dev/null in run.c line 2356: freopen("/dev/null", "r+", fp) for flushing stdin/stdout/stderr on close -- needs replacement with NIL: device
7. /dev/stdin, /dev/stdout, /dev/stderr in run.c stdinit() lines 2245/2248/2251: used as fname strings for file table lookup, NOT actually opened -- can be replaced with "*", ">", ">" or similar Amiga-safe strings
8. fcntl(fileno(fp), F_SETFD, FD_CLOEXEC) in run.c line 2318: no-op stub needed (no exec on AmigaOS)
9. stat() in run.c line 2294: stat(s, &sbuf) + S_ISDIR -- amiport_stat() available, S_ISDIR shim available
10. sys/types.h, sys/stat.h, sys/wait.h, wctype.h, fcntl.h, locale.h, signal.h, strings.h: all need removal/replacement
11. random()/srandom() in run.c/main.c: NOT in libnix -- replace with rand()/srand() per known-pitfalls
12. intmax_t/uintmax_t casts in run.c line 1197-1198: used with snprintf %d/%u -- on 68k, these are long, OK with %ld/%lu but may need cast adjustment
13. %zu format specifiers in run.c lines 2243/2284: FATAL() prints size_t with %zu -- must replace with %lu + cast
14. wchar_t/mbtowc/wctomb/towupper/towlower in run.c: guarded by awk_mb_cur_max > 1 check; if MB_CUR_MAX stub returns 1 on AmigaOS (it may not), this code is bypassed
15. strings.h in lib.c: provides strcasecmp/strncasecmp -- available in libnix, but include name may vary

**Why:** Analysis completed 2026-03-25 for AmigaOS 3.x porting pipeline.
**How to apply:** Use this as the reference for code-transformer stage; C99 is acceptable via -std=gnu99 (ADR-022). The MB_CUR_MAX / wchar path is the biggest runtime risk.
