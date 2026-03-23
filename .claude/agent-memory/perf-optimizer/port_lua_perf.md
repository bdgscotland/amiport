---
name: port_lua_perf
description: Performance findings for ports/lua 5.4.7 — LUA_32BITS, localeconv override, match_class tolower optimization
type: project
---

# Lua 5.4.7 Performance Analysis Summary
Reviewed: 2026-03-22

## Prior fixes confirmed in place
- `read_line` uses `fgets` chunked read (not getc loop)
- `io_tmpfile` uses `amiport_tmpfile()`
- `LUA_USE_C89` active — integer type is `long` (32-bit), no long long emulation

## Top recommendations (not yet applied)

1. **LUA_32BITS=1 in luaconf.h under #ifdef __AMIGA__** — switches lua_Number from
   double (8 bytes, 64-bit soft-float) to float (4 bytes, 32-bit soft-float).
   Halves soft-float operation cost on 68000. Also shrinks TValue struct, reducing
   memory pressure across the whole VM. Trade-off: precision ~7 vs ~15 sig digits.
   INTEGER type is unaffected (stays 32-bit long). HIGH impact for float scripts.

2. **Override lua_getlocaledecpoint() to '.'** in luaconf.h under #ifdef __AMIGA__.
   AmigaOS CLI locale is always C. Eliminates localeconv() call on every number
   format/parse (lobject.c x3, lstrlib.c x2). TRIVIAL change, MEDIUM impact.

3. **lstrlib.c:429 — change `tolower(cl)` to `cl | 0x20`** in match_class().
   Saves ~18 cycles per character class test in pattern engine (JSR vs ORI.B).
   Safe for ASCII pattern characters. MEDIUM impact on string-heavy scripts.

## What is already optimal
- VM dispatch: jump table (goto *disptab[x]) active via LUA_USE_JUMPTABLE=1
- String hash lookup: hashpow2 uses AND, not division
- Stack: 256KB (__stack=262144) validated on FS-UAE

**Why:** Lua is CPU-bound on 68000. The main lever is reducing per-operation
cost in the inner VM loop — float type width and localeconv overhead are
the best available targets without restructuring the VM.
