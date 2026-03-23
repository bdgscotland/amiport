---
name: bc_analysis
description: GNU bc 1.07.1 portability analysis: EASY verdict, bundled getopt, signal shim, write(1,...) in signal handler, YY_BUF_SIZE stack issue, readline/libedit must be disabled
type: project
---

GNU bc 1.07.1 portability analysis — bc only (not dc)

**Verdict: EASY**

Key findings:
- Bundled getopt/getopt_long in lib/getopt.c and lib/getopt1.c — use as-is, do NOT use libnix's broken getopt_long
- signal(SIGINT, handler) — Tier 1 via amiport_signal()
- isatty() — Tier 1 via amiport_isatty()
- getenv() — Tier 1 via amiport_getenv()
- write(1, buf, n) in signal handler (main.c:351,355) — replace with puts() or printf() (signal handlers on AmigaOS cannot use stdio but amiport Ctrl-C is not truly async)
- read(fileno(yyin), ...) in scan.c YY_INPUT — must replace fileno() + read() with fread() (standard YY_INPUT path)
- ssize_t in scan.c:768 — typedef to long for bebbo-gcc C89
- __attribute__((__noreturn__)) in scan.c:356 — harmless with bebbo-gcc (GCC macro fallback already present)
- __builtin_alloca / __builtin_memcpy in bc.c (yacc parser) — YYSTACK_USE_ALLOCA not defined so falls through to YYMALLOC=malloc; __builtin_memcpy has GCC fallback too
- random() in execute.c:296 — available in libnix, no action needed
- assert() in number.c — available in libnix, no action needed
- READLINE / LIBEDIT — must be disabled (#undef); not available on AmigaOS
- Bundled getopt resets optind=0 (GNU extension) — works because bundled implementation handles it
- inttypes.h in scan.c — guarded by C99 check, falls back to manual typedefs, safe
- sys/types.h in bcdefs.h — available in libnix but rarely causes issues; may need stub
- <errno.h> — available in libnix
- Exit codes: bc_exit(1) used for errors — change to bc_exit(10) per Amiga convention
- Stack: arithmetic and recursive identifier lookup; recommend __stack = 65536
- No fork(), no threads, no mmap, no sockets — clean architecture

**Why:** Needed for Amiga calculator port
**How to apply:** Use bundled getopt, disable readline/libedit, shim signal+isatty+getenv, fix write() in signal handler, fix YY_INPUT to use fread not read+fileno, add ssize_t typedef
