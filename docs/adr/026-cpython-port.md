# ADR-026: CPython 3.11.12 Port Architecture

## Status

Accepted

## Date

2026-03-27

## Context

The amiport project needed a "cornerstone" port for LinkedIn positioning -- something that makes people stop scrolling. After researching Aminet, CPython 3 was identified as genuinely novel: no Python 3 has ever existed for classic 68k AmigaOS. The source-analyzer initially rated this as "infeasible" (6-12 months, 30% success), but an eng review and empirical testing revealed the key blockers were either already solved or tractable.

CPython 3.11.12 was chosen over 3.6 (too old for LinkedIn value) and 3.12+ (removed `--without-threads`, added free-threading complexity). Python 3.11 has the WASI port as a reference (same constraints: no fork, no threads, no mmap, no sockets) and built-in `pthread_stubs.h` for threadless platforms.

## Decision

### Target: Python 3.11.12 on AmigaOS 3.x, 68020+, 8MB+ Fast RAM

### Architecture

```
Host (macOS)                     Target (AmigaOS 3.x)
+------------------+             +---------------------------+
| Host Python 3.11 |--freeze-->  | python3 (3.8MB binary)    |
| (build tool)     |  bytecode   |  Python/ (eval loop)      |
|                  |             |  Objects/ (types)         |
| bebbo-gcc in     |--cross--->  |  deepfreeze.c (frozen     |
| Docker           |  compile    |   importlib bytecode)     |
+------------------+             |  amigaos_stubs.c          |
                                 |  libnix libc.a (POSIX)    |
                                 |  libamiport.a (shim)      |
                                 +---------------------------+
```

### Key Architectural Decisions

**1. Use libnix native POSIX fd table (EUREKA finding)**

The source-analyzer rated the fd namespace conflict (amiport_open vs libnix fds) as a Tier 3 blocker requiring 4-8 weeks of work. Empirical testing on vamos proved that bebbo-gcc's libc.a (newlib-based) provides `open()`, `read()`, `write()`, `fdopen()`, `fileno()` in a unified fd table. `open()` returns fd=3, `fdopen()` works on it, `fileno(stdin)=0`. CPython uses libnix's native POSIX layer directly -- no shim surgery needed.

**2. Disable PEP 659 bytecode quickening**

CPython 3.11's specializing adaptive interpreter rewrites bytecode in-place after 128 executions via `_PyCode_Quicken()`. The `_Py_SET_OPCODE` macro writes the opcode byte at an offset that causes corruption on big-endian 68k, resulting in ACPU_InstErr (Guru Meditation #80000004). Disabled via `#ifdef __AMIGA__` early return in `specialize.c`. Performance impact: ~15% slower eval loop.

**3. pymalloc MUST be enabled**

With `WITH_PYMALLOC` disabled (all allocations through libnix malloc), CPython crashes with AN_MemCorrupt (Guru Meditation #81000005) during frozen module execution. pymalloc's arena-based allocator handles CPython's allocation patterns correctly on 68k. pymalloc is load-bearing on AmigaOS.

**4. Host-built deepfreeze bytecode**

The frozen stdlib (importlib._bootstrap, codecs, io, abc, etc.) is compiled to bytecode by a host Python 3.11 build, then baked into the binary as C data arrays via `deepfreeze.c` (154K lines). The cross-compiler resolves struct layouts for 68k correctly because deepfreeze.c uses C designated initializers (`.field = value`), not pre-serialized binary data.

**5. C11 via local stdatomic.h**

CPython 3.11 requires C11 for `<stdatomic.h>`. bebbo-gcc 6.5.0b supports C11 via `-std=gnu11` but doesn't ship the header. A 90-line `stdatomic.h` wraps GCC's `__atomic_*` builtins. AmigaOS is single-threaded, so atomics compile to plain loads/stores.

**6. pthread types: block system, use CPython stubs**

bebbo-gcc's `sys/_pthreadtypes.h` forward-declares pthread types as incomplete structs (can't be used as struct fields). CPython's `pthread_stubs.h` provides concrete stub types for the GIL structs. We block the system header via `#define _SYS__PTHREADTYPES_H_` in pyconfig.h.

**7. Skip encodings module import**

`_PyUnicode_InitEncodings()` tries to import the `encodings` Python module from disk. Without stdlib `.py` files deployed, this crashes. Skipped via `#ifndef __AMIGA__` guard in `pylifecycle.c`. stdout/stderr use ASCII codec (already registered as builtin). Will be restored when stdlib is deployed (Phase 8).

**8. Custom entropy source**

AmigaOS has no `/dev/urandom`. Python's `_Py_HashRandomization_Init` crashes without entropy. Patched `bootstrap_hash.c` with an `#ifdef __AMIGA__` path using a simple LCG seeded from `clock()`, `time()`, and stack address. Not cryptographically secure but sufficient for hash randomization.

### Disabled Features

- Networking (socket, ssl, http)
- Dynamic module loading (.so)
- Threading (stubs only via pthread_stubs.h)
- ctypes/FFI
- GUI (tkinter)
- Readline
- Doc strings (binary size savings)
- Bytecode quickening (PEP 659)
- encodings module (needs stdlib on disk)
- _tracemalloc, faulthandler (disabled in config.c)

### Built-in Modules

marshal, _imp, _ast, _tokenize, builtins, sys, gc, _warnings, _string, _io, posix, errno, _signal, time, _codecs, _weakref, _collections, _sre, _functools, _operator, itertools, _stat, _abc, _symtable, atexit, _thread, math, cmath, _struct, binascii, _json, _bisect, _heapq.

## Consequences

### Positive

- "Python 3.11 on a 1993 Commodore Amiga" -- headline no one expects
- Proves the amiport pipeline can handle 185K LOC projects
- The EUREKA finding (libnix unified fd table) benefits all future large ports
- 5 new crash patterns documented (#23 mbstowcs, #24 quickening)
- Established the phased approach (compile/link/boot/hello) as reusable for large ports

### Negative

- 3.8MB binary requires 8MB+ Fast RAM -- excludes stock A500/A600
- No stdlib on disk yet -- only frozen modules and `-c` inline expressions work
- AmigaDOS quoting mangles parentheses/brackets in `-c` arguments
- ~15% slower eval loop without quickening
- pymalloc cannot be disabled (libnix malloc incompatible)

### Neutral

- PYLONG_BITS_IN_DIGIT=15 (32-bit bignum digits, standard for 32-bit platforms)
- HAVE_BROKEN_MBSTOWCS (same as Android -- well-tested fallback path)
- vamos cannot run the binary (RC=252, libnix locale init failure) -- FS-UAE only
