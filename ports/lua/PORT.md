# Port: lua

## Overview

| Field | Value |
|-------|-------|
| Program | lua |
| Version | 5.4.7 |
| Source | lua.org (Lua 5.4.7) |
| Category | 2 — Scripting interpreter |
| License | MIT |
| Original Author | Roberto Ierusalimschy, Luiz Henrique de Figueiredo, Waldemar Celes (PUC-Rio) |
| Port Date | 2026-03-21 |

## Description

Lua is a lightweight, high-performance scripting language designed for embedding. Lua 5.4 adds native integers, bitwise operators, generational GC, to-be-closed variables, and the utf8 library. This is the first port of Lua 5.4 to any Amiga platform.

## Prior Art on Aminet

| Package | Version | Year | Notes |
|---------|---------|------|-------|
| lua_68k.lha | 5.2.1 | 2012 | Geek Gadgets build, likely requires ixemul.library |
| lua-5.1.4-68k.lha | 5.1.4 | 2008 | Cubic IDE / GCC 2.95.3 build |
| amilua_m68k-amigaos.lha | 5.0.2 | 2005 | Adds Intuition bindings |

No Lua 5.3 or 5.4 port exists anywhere in the Amiga ecosystem. The most recent 68k port is 14 years old and two major versions behind.

## Portability Analysis

Verdict: **MODERATE** — 55 of 61 source files need zero changes. All issues are isolated to 5 files.

| Issue | Tier | Resolution |
|-------|------|------------|
| `long long` default integer type | Config | Defined `LUA_USE_C89` → selects `long`/`double` |
| POSIX path defaults (`/usr/local/...`) | Config | Overridden to `PROGDIR:lua/` |
| `tmpnam()` | Redesign | Custom `lua_tmpnam` in loslib.c generates `T:lua_XXXX_N` names |
| `tmpfile()` | Tier 1 | Routed to `amiport_tmpfile()` (uses `T:`) |
| `signal(SIGINT, ...)` | Tier 1 | Uses libnix `signal()` — works without shim |
| `dlopen()` / `dlsym()` | N/A | Dead code — `LUA_USE_DLOPEN` not defined |
| `popen()` / `pclose()` | N/A | Dead code — falls through to Lua's built-in error stub |
| `sigaction()` | N/A | Dead code — only under `LUA_USE_POSIX` |
| `system()` | Passthrough | Uses libnix `system()` → AmigaDOS `Execute()` |
| `exit(EXIT_FAILURE)` → `exit(10)` | Exit codes | Fixed in `lua.c`, `luac.c`, `loslib.c` |
| `isatty()` | N/A | ISO C fallback `(1)` used — always assumes tty |

## Transformations Applied

| File | Original | Transformed | Comment |
|------|----------|-------------|---------|
| `luaconf.h:44` | `/* #define LUA_USE_C89 */` | `#define LUA_USE_C89` (under `__AMIGA__`) | Cascades: disables POSIX, selects C89 numbers |
| `luaconf.h:807-820` | POSIX paths, no Amiga config | Amiga path config block | `PROGDIR:` paths, no C modules |
| `lua.c:17-20` | (none) | `$VER` string, `__stack = 65536` | AmigaOS version/stack |
| `lua.c:693` | `return EXIT_FAILURE` | `return 10` | RETURN_ERROR |
| `loslib.c:104-119` | ISO C `tmpnam()` | Custom `lua_tmpnam` for `__AMIGA__` | Generates `T:lua_XXXX_N` names |
| `loslib.c:397` | `EXIT_FAILURE` | `10` | RETURN_ERROR for `os.exit(false)` |
| `liolib.c:19-22` | (none) | `#include <amiport/unistd.h>` | For `amiport_tmpfile()` |
| `liolib.c:309` | `tmpfile()` | `amiport_tmpfile()` | Uses `T:` on AmigaOS |
| `luac.c:10-14` | (none) | `$VER` string, `__stack = 65536` | AmigaOS version/stack |
| `luac.c:45,51,71` | `exit(EXIT_FAILURE)` | `exit(10)` | RETURN_ERROR (3 occurrences) |

## Shim Functions Exercised

- `amiport_tmpfile()` — anonymous temp file using `T:` assign (for `io.tmpfile()`)

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68000+ |
| CFLAGS | `-O2 -noixemul -m68000 -Wall` |
| Libraries | `-lamiport` (posix-shim), `-lm` |
| Binary size | ~258KB |
| Stack | 65536 bytes (via `__stack` cookie) |

## Test Results

Tested via vamos (Category 2 — scripting interpreter).

| Test | Command | Expected | Result |
|------|---------|----------|--------|
| Version | `lua -v` | Prints "Lua 5.4.7" | PASS |
| Expression | `lua -e "print(2+3)"` | `5` | PASS |
| String ops | `lua -e "print(string.upper('hello amiga'))"` | `HELLO AMIGA` | PASS |
| Table ops | `lua -e "t={1,2,3}; print(#t)"` | `3` | PASS |
| Math | `lua -e "print(math.sqrt(144))"` | `12.0` | PASS |
| Loop | `lua -e "s=0; for i=1,100 do s=s+i end; print(s)"` | `5050` | PASS |
| Recursion | `lua -e "function fib(n) if n<2 then return n else return fib(n-1)+fib(n-2) end end; print(fib(10))"` | `55` | PASS |
| os.clock | `lua -e "print(type(os.clock()))"` | `number` | PASS |
| io.write | `lua -e "io.write('Hello from Lua on AmigaOS!\n')"` | `Hello from Lua on AmigaOS!` | PASS |
| Integer div | `lua -e "print(7 // 2)"` | `3` | PASS |
| File script | `lua test_hello.lua` | `Hello from Lua on Amiga!` | PASS |

### Deep Test Suite (16 categories, all PASS)

| Category | Tests |
|----------|-------|
| Integer arithmetic | 32-bit boundaries, overflow wrapping, floor division |
| Float precision | math.type, trig functions, float/int distinction |
| String operations | Patterns, captures, gsub, format, 100KB strings |
| Tables | 10K entries, sort, concat, move, pack/unpack |
| Coroutines | Create, resume, yield, status, value passing |
| Closures/upvalues | Independent counters, upvalue isolation |
| Error handling | pcall, xpcall, table errors, tracebacks |
| Metatables | __index, __add, __tostring, OOP patterns |
| GC/weak tables | Weak ref collection, __gc finalizers |
| File I/O | Write/read roundtrip, io.lines, read modes, 2000-char lines |
| Bitwise (5.4) | AND, OR, NOT, XOR, shifts |
| To-be-closed (5.4) | `<close>` scope finalizers |
| UTF-8 library | Character vs byte length |
| Dynamic code | load(), syntax error detection, environments |
| Stress | fib(25), deep recursion, large tables |
| Varargs | select, ipairs, multiple returns |
| os.tmpname | Returns T: path, unique names, file create/remove |
| io.tmpfile | Write/seek/read roundtrip via amiport_tmpfile |

## Known Limitations

- **`io.popen()` not supported** — Returns a Lua error: "'popen' not supported". This is a graceful degradation, not a crash.
- **No dynamic C module loading** — `package.loadlib()` returns "dynamic libraries not enabled". Pure Lua `require()` works normally.
- **32-bit integers** — Lua integers are `long` (32-bit) instead of the default `long long` (64-bit). Range: -2147483648 to 2147483647. Sufficient for most scripting but cannot represent 64-bit values.
- **`os.execute()` limited** — Uses libnix `system()` which maps to AmigaDOS `Execute()`. Return code semantics differ from POSIX.
- **`isatty()` always returns true** — Interactive mode detection assumes stdin is always a terminal (ISO C fallback).
- **No readline support** — Line editing uses basic `fgets()`. No command history or cursor movement.

## Performance Optimizations

- **`read_line` in liolib.c** — Replaced per-character `getc()` loop with `fgets()` for line reading. On a 7 MHz 68000, each `getc()` call costs ~20+ cycles in function-call overhead; `fgets` collapses this to one library call per buffer chunk. Uses `memchr()` instead of `strlen()` for NUL-safe newline detection. Affects `io.lines()`, `file:read("*l")`, and stdin line reading.
- **`os.tmpname()` in loslib.c** — Custom Amiga implementation generates unique names under `T:` (RAM:T/) instead of libnix `tmpnam()` which generates broken `/tmp/` paths.

## Review

Reviewed with `/review-amiga`. Score: **READY**.

| Category | Score |
|----------|-------|
| Stack safety | OK — `__stack = 65536`, no large stack buffers |
| Memory handling | OK — Lua GC + `lua_close()` frees all memory |
| Path handling | OK — Active paths use `PROGDIR:` and `T:` |
| Library cleanup | OK — No direct AmigaOS library usage |
| Conventions | OK — All exit codes use Amiga values |

Notes:
- Hash seed (`luai_makeseed`) is deterministic on AmigaOS due to no ASLR — acceptable for a scripting language.
- `isatty()` always returns true (ISO C fallback) — interactive mode assumed when no args given.
- Perf-optimizer (two passes) found no memory leaks on any exit path. Fixed `os.tmpname()` broken path and `read_line` NUL-byte regression.
