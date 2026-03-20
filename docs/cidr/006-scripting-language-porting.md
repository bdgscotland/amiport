# CIDR-006: Scripting Language Interpreter Porting

## Status

Exploring

## Date

2026-03-20

## Context

Scripting language interpreters (Lua, bc, awk, Tcl) represent Category 2 in our port target taxonomy (ADR-011). They're interesting because they're typically written in portable C89, have minimal OS dependencies, and provide enormous utility on the Amiga — enabling scripting, automation, and extension of other ported software.

The key question: **Can the existing posix-shim/emu pipeline port interpreters without new infrastructure?**

## Exploration

### Lua as proof-of-concept

Lua is the ideal first target:
- Pure ANSI C89, designed for portability
- ~30 source files, ~25K lines — non-trivial but manageable
- Minimal POSIX surface: `popen()`, `tmpnam()`, `system()`, `clock()`, `setlocale()`
- Already ported to many embedded/retro platforms
- Has existing (ancient) Amiga ports on Aminet — validates feasibility
- The Amiga community actively wants a modern Lua for ARexx scripting replacement

### POSIX dependencies in typical interpreters

| Function | Lua | bc | awk | Status in amiport |
|----------|-----|-----|-----|-------------------|
| `malloc`/`realloc`/`free` | Yes | Yes | Yes | Native (libnix) |
| `stdio` (fopen, fprintf, etc.) | Yes | Yes | Yes | Native (libnix) |
| `string.h` (strlen, memcpy, etc.) | Yes | Yes | Yes | Native (libnix) |
| `setjmp`/`longjmp` | Yes | Yes | No | Native (libnix) |
| `popen`/`pclose` | Optional | No | Optional | Tier 2 emu |
| `system()` | Optional | No | Yes | Tier 1 shim |
| `tmpnam`/`tmpfile` | Optional | No | No | Tier 1 shim (mkstemp) |
| `setlocale` | Stub ok | No | Stub ok | Tier 1 stub |
| `clock()`/`time()` | Yes | No | No | Tier 1 shim |
| `dlopen`/`dlsym` | Optional | No | No | Not available — stub |
| `signal()` | Optional | No | No | Tier 1 stub |
| `getenv`/`setenv` | Yes | No | Yes | Tier 1 shim |
| `exit`/`atexit` | Yes | Yes | Yes | Native (libnix) |

### Challenges specific to interpreters

1. **Dynamic loading** — Lua's `require()` with C modules uses `dlopen()`. On Amiga, this maps to `OpenLibrary()` but the calling convention is completely different. Solution: stub dlopen, document that C modules must be statically linked.

2. **Large stack usage** — Interpreters with recursive descent parsers can use significant stack. Solution: `long __stack = 65536;` or higher.

3. **Interactive line editing** — REPLs want readline/editline. On Amiga, raw console mode provides basic editing. Solution: minimal line editing via console.device, or just use basic fgets() for first port.

4. **Filesystem paths** — Lua's `require()` searches paths like `/usr/local/lib/lua/5.4/?.so`. Solution: remap to Amiga assigns (`LUA:`, `LIBS:`) in the path translator.

5. **Build system** — Most interpreters use autoconf/cmake. Solution: extract the source list and create a direct Makefile (the build-manager agent already handles this).

### What new infrastructure is needed?

Surprisingly little:
- **No new shim library** — existing posix-shim/emu covers the surface
- **dlopen stubs** — add to posix-emu as `amiport_emu_dlopen()` returning NULL
- **Path remapping** — the planned `amiport_path_translate()` from TODO.md would help
- **Stack size guidance** — document in porting guide

### Proof-of-concept plan

1. Download Lua 5.4 source
2. Run `/analyze-source` — expect mostly green (Tier 1/native)
3. Run `/transform-source` — replace the few POSIX calls
4. Add `long __stack = 65536;` and `$VER:` string
5. Build with `/build-amiga` — should compile with minor fixups
6. Test with `/test-amiga` — run `lua -e "print('Hello from Amiga!')"` in vamos
7. Test interactive REPL if possible

## Questions to Resolve

- Should we ship a default `LUA:` assign setup script, or rely on the user's `S:User-Startup`?
- Should C module support be completely stubbed, or should we provide a static-linking mechanism?
- Is Lua 5.4 the right version, or should we target 5.1 (smaller, used by more software)?
- Should bc/awk be attempted before Lua, since they're simpler?

## Related

- ADR-008: Tiered POSIX Compatibility
- ADR-011: Beyond CLI — Port Categories
- TODO.md: dlopen stubs, path translation shim
