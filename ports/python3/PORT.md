# CPython 3.11.12 — AmigaOS 3.x Port

| Field | Value |
|-------|-------|
| Upstream | CPython 3.11.12 (Python Software Foundation) |
| Source | https://www.python.org/ftp/python/3.11.12/Python-3.11.12.tgz |
| Category | 2 — Scripting interpreter |
| Target | AmigaOS 3.x, 68030+, 16MB Fast RAM |
| License | PSF License |

## Status

**Phase 1: Compilation** — In progress.

## Architecture Decisions

### fd Namespace — Use libnix Native POSIX fds

**EUREKA (2026-03-26):** The feared fd namespace conflict between `amiport_open()` and libnix does NOT exist. bebbo-gcc's libc.a (newlib-based) provides `open()`, `read()`, `write()`, `close()`, `lseek()`, `fdopen()`, `fileno()` in a unified fd table. Empirically verified on vamos — `open()` returns fd=3, `fdopen()` works on it, `fileno(stdin)=0`.

CPython uses libnix's native POSIX fd layer directly. No amiport_open() involvement. Only use amiport shims for functions libnix doesn't provide (stat, signal, time, directory ops).

### C11 — Local stdatomic.h

bebbo-gcc 6.5.0b supports C11 via `-std=gnu11`. We provide a local `<stdatomic.h>` wrapping GCC's `__atomic_*` builtins. AmigaOS is single-threaded so atomics are plain loads/stores.

### pthread Stubs — Block System Types

bebbo-gcc's `sys/_pthreadtypes.h` forward-declares pthread types as incomplete structs. CPython's `pthread_stubs.h` needs concrete types for GIL structs. We block the system header via `#define _SYS__PTHREADTYPES_H_` in pyconfig.h and use CPython's stub definitions.

### Threading — CPython's Built-in Stubs

Using `HAVE_PTHREAD_STUBS` (designed for WASI). Thread creation fails with EAGAIN. GIL operations are no-ops. Single-threaded execution only.

### Performance — 15-bit Bignum Digits

`PYLONG_BITS_IN_DIGIT=15` avoids 64-bit multiply in the bignum hot path. Standard approach for 32-bit platforms without hardware 64-bit arithmetic.

### Build — Hermetic Host Python

Build host Python 3.11 from the same source for the freeze tool. Cross-compile Amiga binary with frozen bytecode embedded.

### pyconfig.h Interdependencies

**HAVE_NANOSLEEP requires HAVE_CLOCK_GETTIME.** CPython's `timemodule.c` nanosleep path calls `_PyTime_AsTimespec()`, which is only compiled when `HAVE_CLOCK_GETTIME` is defined in `pytime.c`. Enabling one without the other causes a link error at `_PyTime_AsTimespec`. Both must be enabled together, and `clock_gettime()` must be provided (via `amigaos_stubs.c` using `DateStamp()`).

### sys.executable Required for site Module

The `site` module (imported on startup unless `-S` is passed) accesses `sys.executable`. If `getpath_amiga.c` doesn't set `config->executable`, the site import crashes with `AttributeError: module 'sys' has no attribute 'executable'`. Fixed by setting it to `L"WORK:python3"` in getpath_amiga.c.

### pymalloc MUST Be Enabled

`WITH_PYMALLOC` must be defined. Without it, all allocations go through libnix `malloc()`/`free()` which causes `AN_MemCorrupt` (Guru Meditation 8100 0005) during frozen module execution. pymalloc's arena-based allocator handles CPython's allocation patterns correctly on 68k. Never disable pymalloc on AmigaOS.

### CPython Bytecode Quickening Disabled (PEP 659)

CPython 3.11's specializing adaptive interpreter (`_PyCode_Quicken`) rewrites bytecode in-place after 128 executions. The `_Py_SET_OPCODE` macro writes the opcode byte at the wrong offset on big-endian 68k, corrupting the bytecode and causing `ACPU_InstErr`. Disabled via `#ifdef __AMIGA__` early return in `specialize.c`. Performance impact: ~15% slower eval loop (no adaptive specialization).

### Disabled Features

- Networking (socket, ssl, http)
- Dynamic module loading (.so)
- Threading (stubs only)
- ctypes/FFI
- GUI (tkinter)
- Readline (basic input only)
- Doc strings (binary size savings)

### Built-in Modules

marshal, _imp, _ast, _tokenize, builtins, sys, gc, _warnings, _string, _io, posix, errno, _signal, time, _codecs, _weakref, _collections, _sre, _functools, _operator, itertools, _stat, _abc, _symtable, atexit, _thread, _tracemalloc, faulthandler, math, cmath, _struct, binascii, _json, _bisect, _heapq.

## Phased Approach

| Phase | Gate | Status |
|-------|------|--------|
| 1. Compile | .o files build without errors | **DONE** (153/153) |
| 2. Link | Binary links (all symbols resolved) | **DONE** (3.8 MB with frozen modules) |
| 3. Boot | Py_Initialize() completes | **DONE** — Fixed mbstowcs, entropy, quickening, sys.executable, encodings skip |
| 4. REPL | `>>>` prompt appears | Pending |
| 5. Hello | `print("hello")` works | **DONE** — 9/13 tests pass: int, arithmetic, power, bool, list, dict, sorted |
| 6. Math | `for/def/class` work | Partial — semicolons in -c args split by AmigaDOS, need .py files |
| 7. Files | `open/read/write` work | Blocked — WORK: path syntax not recognized by Python open() |
| 8. Stdlib | `import os/json/re/math` work | Pending — needs stdlib on disk |

## Porting Notes

CPython 3.11.12 is the largest port in the amiport catalog at ~185K LOC core. The port was motivated by LinkedIn positioning — "Python 3.11 on a 1993 Commodore Amiga" is a headline no one expects.

The key discovery was that libnix's POSIX fd table is unified (EUREKA finding), eliminating what the source-analyzer rated as a Tier 3 architectural blocker requiring 4-8 weeks of work. This downgraded the hardest problem from "novel redesign" to "already solved by the toolchain."

The WASI (WebAssembly System Interface) port of CPython 3.11 serves as the reference — it targets the same constraints: no fork, no threads, no mmap, no sockets. CPython's `pthread_stubs.h` was designed for exactly this use case.
