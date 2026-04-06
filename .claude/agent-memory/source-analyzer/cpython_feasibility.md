---
name: cpython_feasibility
description: CPython 3.x feasibility analysis for AmigaOS 3.x port — verdict INFEASIBLE as full interpreter, but Python 3.6 stripped build is HARD/possible with 6+ month effort
type: project
---

# CPython 3.x Feasibility Analysis for AmigaOS 3.x

**Verdict: INFEASIBLE as general-purpose CPython. Python 3.6 with --without-threads + frozen stdlib is HARD (6+ months, senior effort).**

## Key findings

- CPython 3.11+ requires C11. CPython 3.6-3.10 requires C99 with long long + stdint — bebbo-gcc with -std=gnu99 can handle this but libnix lacks many C99 stdlib functions needed at runtime.
- CPython 3.6 has `--without-threads` which allows building without pthreads. 3.11+ requires HAVE_PTHREAD_STUBS.
- The GIL uses PyMUTEX/PyCOND abstractions; the stubs (thread_pthread_stubs.h) provide no-op mutex/condvar but prevent thread creation. Available from 3.11+; in 3.6 WITH_THREAD=no is the path.
- longobject.c supports 15-bit digit mode (no uint64_t required) via PYLONG_BITS_IN_DIGIT=15.
- obmalloc does NOT use mmap — pure malloc/free.
- GC does not use threads or mmap.
- posixmodule.c wraps all POSIX functions behind HAVE_ guards — fork/exec/mmap can be compiled out.
- _io module (built-in, mandatory) uses low-level fd calls (open/read/write/lseek/fstat/isatty). Needs amiport fd table integration — critical problem.
- _io module uses PyThread locks when WITH_THREAD is set; with --without-threads these compile out.
- importdl.c is guarded by HAVE_DYNAMIC_LOADING — can be fully disabled.
- signalmodule.c: individual signals conditionally compiled; PyErr_CheckSignals() always present.
- Random: can fall back to time()+pid if /dev/urandom not available.
- math module: software FP fine, uses libm.
- time module: only time() is absolutely required; rest guarded by HAVE_ macros.

## Blockers

1. C11 requirement for 3.11+. Bebbo-gcc 6.5.0b targets C11 but libnix does NOT have C11 atomics or _Thread_local. Use 3.6.
2. PyLong uses long long (uint64_t for twodigits in 30-bit mode). Must force 15-bit mode (PYLONG_BITS_IN_DIGIT=15, SIZEOF_LONG_LONG=0).
3. _io mandatory built-in uses fd-level open()/read()/write() — these are libnix fds, NOT amiport fd table. The two fd namespaces cannot be mixed (crash-patterns #12). This is the deepest architectural problem: Python's file object wraps raw OS fds, but our shim uses a separate fd namespace.
4. The posix module (also mandatory built-in) wraps OS stat/open/etc using raw fd numbers. Same namespace conflict.
5. _PyRandom_Init() calls _PyOS_URandomNonblock() — needs /dev/urandom shim (return time-based seed).
6. setlocale(LC_CTYPE, "") — libnix will load system locale, potentially breaking MB_CUR_MAX. Must setlocale("C") immediately after.
7. fileno() for stdin/stdout/stderr: Python calls fileno() on standard streams. libnix fileno() returns real fd numbers that libnix manages. amiport_isatty(fileno(stdin)) will fail (namespace mismatch).
8. Unicode: unicodeobject.c is enormous (~7000 lines) and requires the full Unicode tables. On 68020 with 2MB, the unicode tables alone are 200-400KB.
9. Frozen stdlib: even a "minimal" Python needs importlib, _collections_abc, abc, io, codecs all frozen. Freezing 40+ stdlib modules adds ~500KB of bytecode.
10. Stack depth: Python's eval loop recurses. Default Python recursion limit is 1000. Each frame is ~200 bytes. That's 200KB stack minimum for recursion-heavy code. __stack = 512*1024 would be needed.

## What CAN be shimmed (Tier 1)
- open/read/write/lseek/close (but fd namespace is the problem)
- stat/fstat
- getcwd/chdir/mkdir/unlink/rename
- getenv (amiport_getenv)
- time/gettimeofday
- signal (SIGINT only)
- sleep/nanosleep
- isatty
- getpid (returns task addr)

## What needs redesign (Tier 3)
- The fd namespace problem: Python needs unified fd table covering both libnix stdio AND raw file operations. Either (a) fork the entire Python _io layer to use amiport fds, or (b) make amiport_open() return libnix-compatible fds (extremely hard), or (c) patch Python's is_valid_fd() and _Py_open() to use amiport primitives with consistent fd numbering.
- /dev/urandom: provide T:python-entropy fallback using DateStamp() as seed
- fork/exec/subprocess: completely disable (no replacements)
- threading: --without-threads (3.6) or pthread_stubs (3.11)
- socket/ssl: disable completely
- mmap module: disable (Tier 2 emulation available but Python's mmap API needs MAP_SHARED too)
- ctypes: disable (requires dlopen)
- multiprocessing: disable (requires fork)

## Recommended version: Python 3.6
- Has --without-threads (compile-time disable, not just stubs)
- Uses C99 not C11 (pyport.h, longintrepr.h need C99 but not C11 atomics)
- More conservative codebase, fewer platform assumptions
- Known to have been ported to obscure platforms in the past
- 3.6 is EOL but that's fine for embedded/retro use

## Approximate source size
- Python/ : 60 C files, ~90,000 LOC
- Objects/ : 44 C files, ~80,000 LOC
- Parser/ : 9 C files, ~15,000 LOC
- Modules/ : ~120 C files, ~200,000 LOC (most can be disabled)
- Core total (no modules): ~120 C files, ~185,000 LOC
- Minimal build: 60-70 C files

## Binary size estimate
- MicroPython minimal: ~200-300KB
- CPython bare minimum (no stdlib): probably 800KB-1.2MB on 68k
- CPython with frozen minimal stdlib (os, sys, math, re, json): 2-4MB
- This exceeds typical Amiga RAM for meaningful use

## RAM requirements
- Interpreter startup: ~1-2MB minimum
- With stdlib loaded: ~3-4MB
- Running a real script: 4-8MB
- Recommendation: needs 8MB Fast RAM minimum for comfortable use

## Effort estimate
- Understanding CPython build system and porting hooks: 2-4 weeks
- Resolving fd namespace problem: 2-4 weeks (architectural, no easy path)
- Patching pyconfig.h and configure for AmigaOS: 1-2 weeks
- Getting it to link: 2-4 weeks
- Getting interpreter to start: 2-4 weeks
- Frozen stdlib subset that works: 4-8 weeks
- REPL functional: additional 2-4 weeks
- Total: 4-6 months minimum, likely 6-12 months for a solid result

## Why: The fd namespace problem is the core blocker
CPython's _io module is a built-in that cannot be disabled. It uses raw OS fd numbers from open()/read()/write(). Our posix-shim uses a completely separate fd table. Python also uses fileno() to get fds from FILE* streams (stdin/stdout/stderr). There is no clean way to make libnix fds and amiport fds share the same namespace without either (a) rewriting amiport_open to use real libnix fds (requires deep libnix internals), or (b) patching all of Python's _io to use a higher-level abstraction layer. Option (b) is ~2000 lines of surgery in the most complex file in CPython's core.
