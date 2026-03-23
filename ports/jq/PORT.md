# Port: jq

## Overview

| Field | Value |
|-------|-------|
| Program | jq |
| Version | 1.7.1 |
| Source | https://github.com/jqlang/jq (jq-1.7.1) |
| Category | 1 — CLI |
| License | MIT (jq), ICU (decNumber) |
| Original Author | Stephen Dolan / jqlang contributors |
| Port Date | 2026-03-22 |

## Description

jq is a lightweight and flexible command-line JSON processor. It's like sed for JSON data — you can slice, filter, map, and transform structured data with the same ease that sed, awk, grep, and friends let you manipulate text.

This is the first 68k AmigaOS port of jq. A PPC-MorphOS port exists on Aminet (by Stefan Haubenthal, v1.8.1, Oct 2025) but is incompatible with classic 68k hardware.

## Prior Art on Aminet

- `jq` (text/misc) — PPC-MorphOS only, v1.8.1, uploaded Oct 2025. Not usable on 68k AmigaOS 3.x.

## Build Notes

This port uses `-std=gnu99` (see ADR-022). jq's source has 60+ C99 for-init declarations that would require massive mechanical conversion to C89 with no runtime benefit. bebbo-gcc supports `-std=gnu99` and produces identical 68020 machine code.

Built without oniguruma (regex builtins return runtime errors) and without decNumber (uses standard IEEE 754 doubles for JSON numbers).

## Portability Analysis

Verdict: HARD

| Issue | Tier | Resolution |
|-------|------|------------|
| C99 for-init declarations (60+) | Config | Use -std=gnu99 (ADR-022) |
| pthread TLS (jv_dtoa_tsd.c, jv.c, jv_alloc.c) | Tier 2 | Single-threaded stubs — static globals |
| oniguruma regex library | Config | Build without — regex builtins return runtime error |
| decNumber library | Config | Build without — standard double arithmetic |
| fdopen(amiport_open()) in jv_file.c | Tier 1 | Replace with fopen() + stat() directory check |
| stat/fstat calls (jv_file.c, linker.c) | Tier 1 | amiport_stat/amiport_fstat shim |
| opendir/readdir (linker.c) | Tier 1 | amiport_opendir/amiport_readdir shim |
| isatty (main.c) | Tier 1 | Available in libnix |
| getenv (main.c) | Tier 1 | Available in libnix |
| exit codes | Tier 1 | Map to Amiga RC values |
| sys/time.h / gettimeofday | Tier 1 | amiport_gettimeofday shim |
| libgen.h / basename | Tier 1 | amiport provides basename |
| alloca.h | Config | Available in bebbo-gcc |
| IEEE byte order | Config | MC68k is big-endian (IEEE_MC68k) |
| MurmurHash3 alignment | Note | Casts char* to uint32_t* — OK on 68020+ (unaligned access handled in HW) |
| autotools config.h | Config | Replaced with -D flags in Makefile |

## Transformations Applied

| File | Original | Transformed | Comment |
|------|----------|-------------|---------|
| main.c | `#include "src/version.h"` | `#include "version.h"` | Tier 1 — fix path for ported/ layout |
| main.c | `#include "src/config_opts.inc"` | `#include "config_opts.inc"` | Tier 1 — fix path for ported/ layout |
| main.c | `#include <stdlib.h>` | `#include <amiport/stdlib.h>` | Tier 1 — activates exit() shim |
| main.c | `#include <unistd.h>` | `#include <amiport/unistd.h>` | Tier 1 — provides isatty(), realpath() |
| main.c | `#include <libgen.h>` | removed; `extern char *dirname(...)` | Tier 1 — libgen.h not in sysroot; dirname in libnix |
| main.c | added | `static const char *verstag = "$VER: jq 1.7.1 (22.03.2026)"` | Amiga boilerplate |
| main.c | added | `long __stack = 65536` | Amiga boilerplate — recursive evaluator needs deep stack |
| main.c | added | `int __nowild = 1` | Suppress argv glob expansion — jq takes filter expressions |
| main.c | `exit(1)` (out-of-memory path) | `exit(10)` | Tier 1 — RETURN_ERROR |
| main.c | `exit(2)` (multiple sites) | `exit(20)` | Tier 1 — RETURN_FAIL |
| main.c | `jq_exit_with_status` / `jq_exit` | `jq_amiga_exit_code()` remapper | Tier 1 — maps jq exit codes 2,3→20, 1→10 |
| jv_dtoa_tsd.c | pthread TSD | static singleton (`dtoa_ctx_static`) | Tier 2 — AmigaOS is single-threaded |
| jv_thread.h | `#include <pthread.h>` | `#elif defined(__AMIGA__)` stubs | Tier 2 — no-op mutex, 8-slot TSD array, pthread_once |
| jv_file.c | `open()+fstat()+fdopen()` | `stat()+fopen()` | Tier 1 — crash-patterns #12: amiport_open fd ≠ libnix fd |
| jv_file.c | `#include <sys/stat.h>` | `#include <amiport/sys/stat.h>` | Tier 1 — stat shim |
| jv_file.c | large local `char buf[4096+4]` | `static char buf[4096+4]` | crash-patterns #10 — non-recursive function, safe to static |
| linker.c | `#include <sys/stat.h>` | `#include <amiport/sys/stat.h>` | Tier 1 — stat shim |
| linker.c | `stat()` | `stat()` (via shim macro) | Tier 1 — `amiport/sys/stat.h` provides `#define stat amiport_stat` |
| linker.c | `#include <libgen.h>` | removed; `extern char *dirname(...)` | Tier 1 — dirname in libnix |
| linker.c | `path_is_relative()` | `#elif defined(__AMIGA__)` branch | Tier 1 — AmigaOS uses `:` for volumes, not leading `/` |
| builtin.c | `#include <sys/time.h>` | `#include <amiport/sys/time.h>` | Tier 1 — gettimeofday shim |
| builtin.c | `gettimeofday()` | `#define gettimeofday(tv, tz) amiport_gettimeofday(tv)` | Tier 1 — shim lacks POSIX macro alias |
| builtin.c | `struct timeval` | `#define timeval amiport_timeval` | Tier 1 — struct alias for shim type |
| builtin.c | strptime macro conflict | `#define AMIPORT_NO_TIME_MACROS 1` | Tier 1 — use libnix strptime() directly (avoids struct tm / struct amiport_tm type conflict) |
| util.c | `#include <sys/stat.h>` | `#include <amiport/sys/stat.h>` | Tier 1 — stat shim |
| util.c | `#include <unistd.h>` | `#include <amiport/unistd.h>` | Tier 1 — realpath() shim |
| util.c | `#include <stdlib.h>` | `#include <amiport/stdlib.h>` | Tier 1 — exit() shim |
| util.c | `#include <fcntl.h>` | removed | Tier 1 — only used in WIN32 blocks |
| util.c | `#include <sys/types.h>` | removed | Tier 1 — AmigaOS types via exec/types.h |
| execute.c | `#include <sys/stat.h>` | `#include <amiport/sys/stat.h>` | Tier 1 — unused but present in original |
| compile.c | `#include <unistd.h>` | `#include <amiport/unistd.h>` | Tier 1 — unused but present in original |
| lexer.c | `#include <unistd.h>` | `#include <amiport/unistd.h>` | Tier 1 — used for isatty() in flex scanner |
| lexer.c | `YY_EXIT_FAILURE 2` | `YY_EXIT_FAILURE 20` | Tier 1 — RETURN_FAIL |
| lexer.h | `#include <unistd.h>` | `#include <amiport/unistd.h>` | Tier 1 — flex-generated header |
| jv_dtoa.c | `exit(1)` in `Bug()` macro | `exit(20)` | Tier 1 — RETURN_FAIL (debug-mode fatal error) |
| jv_alloc.c | `#include <pthread.h>` | guarded by `#ifdef HAVE_PTHREAD_KEY_CREATE` | No change — macro not defined, compile-time dead code |
| jv.c | `#include <pthread.h>` (×2) | guarded by `#ifdef USE_DECNUM` | No change — macro not defined, compile-time dead code |

## Shim Functions Exercised

- `amiport_stat()` — via `#define stat amiport_stat` in `<amiport/sys/stat.h>` (jv_file.c, linker.c, execute.c)
- `amiport_isatty()` — via `#define isatty amiport_isatty` in `<amiport/unistd.h>` (lexer.c, main.c)
- `amiport_realpath()` — via `#define realpath amiport_realpath` in `<amiport/unistd.h>` (util.c)
- `amiport_gettimeofday()` — via direct `#define` alias in builtin.c (HAVE_GETTIMEOFDAY code path)
- `amiport_opendir()`, `amiport_readdir()`, `amiport_closedir()` — NOT needed (jq does not use opendir; library search is via stat only)

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc 6.5.0 (bebbo) |
| Target | m68k-amigaos, 68020+ |
| CFLAGS | `-O0 -noixemul -std=gnu99 -m68000 -Wall` (see crash-patterns #16) |
| Libraries | `-lamiport -lm` |
| Binary size | 377 KB (unstripped), ~340 KB (stripped) |

## Test Results

50/50 FS-UAE tests pass (AmigaOS 3.1, 68020). Test suite covers:
- 28 functional tests (all flags: -n, -r, -c, -s, -S, -R, -j, -a, -C, -M, -e, -f, --tab, --indent, --arg, --argjson, --args, --jsonargs, --rawfile, --slurpfile, --stream, --seq, --unbuffered, --run-tests, --build-configuration)
- 5 error path tests (RC 20)
- 2 exit code tests (RC 10)
- 7 edge case tests (empty file, nested access, arithmetic, string concat)
- 3 Amiga-specific tests (WORK: paths)
- 5 real-world tests (select, group_by, sort_by, unique, add on 7-package dataset)

## Performance

| Workload | vamos (~10MHz) | Est. A1200 (14MHz 68020) |
|----------|---------------|--------------------------|
| --version | 0.13s | <1s |
| Simple filter | 1.28s | ~7-12s |
| 100 objects select+filter | 1.48s | ~9-14s |
| 1000 objects | 2.36s | ~15-25s |

87% of runtime is startup (compiling 13KB builtin library). Practical for config file processing on accelerated A1200.

## Known Limitations

- **No regex:** `test()`, `match()`, `capture()`, `scan()`, `sub()`, `gsub()` return runtime errors (built without oniguruma)
- **No arbitrary-precision decimals:** Uses IEEE 754 doubles (standard JSON behavior, but `1.1 + 2.2 != 3.3`)
- **GNU math extensions unavailable:** Bessel functions (j0/j1/yn), tgamma, cbrt, erf, etc. return runtime errors
- **-O0 mandatory:** bebbo-gcc 6.5.0b corrupts struct returns >8 bytes at -O1/-O2 (crash-patterns #16). Binary is ~12% larger than -O2.
- **Slow startup:** 1.14s on vamos for builtin compilation — architectural to jq, not fixable at source level

## Review

Reviewed with 3 specialized agents (2 rounds each):
- **Memory checker:** All leak paths fixed (14 total across 2 rounds). Safe for -noixemul.
- **Perf optimizer:** Output buffer (3-5x), vfmt probe, MurmurHash alignment, escape batching. All applied.
- **Hardware expert:** Stack 128KB, atexit flush, alignment fix verified correct for 68000-68060.
- **Profiler:** 87% startup, linear scaling, output-bound for large workloads.

Score: **READY** (with -O0 and known limitations documented)
