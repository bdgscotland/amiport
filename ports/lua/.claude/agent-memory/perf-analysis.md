# Perf Analysis: Lua 5.4.7 (2026-03-22)

## Prior Fixes Confirmed
- read_line fgets optimization in liolib.c — still in place
- amiport_tmpfile() in loslib.c — still in place
- LUA_USE_C89 integer mode — still in place

## Actionable Optimizations

### 1. ~~HIGH: Enable LUA_32BITS~~ **REJECTED — Guru #8000000B**
LUA_32BITS switches lua_Number to float. On 68000 without FPU, float math
functions (sqrtf, powf, fmodf) generate Line F traps → Guru Meditation
#8000000B (ACPU_LineF). AmigaOS IEEE math libraries (mathieeedoubbas,
mathieeedoubtrans) only provide double-precision functions. The soft-float
fallback for float still hits Line F emulator traps.
**DO NOT USE LUA_32BITS on AmigaOS 68000 targets.**

### 2. MEDIUM: Override lua_getlocaledecpoint (luaconf.h)
Replace localeconv()->decimal_point[0] call with constant '.'.
Eliminates 5 call sites in lobject.c and lstrlib.c.
AmigaOS CLI always uses C locale.

### 3. MEDIUM: tolower → bitwise OR in match_class (lstrlib.c:429)
Replace tolower(cl) with (cl | 0x20) — saves ~18 cycles per pattern char test.
Pattern chars are always ASCII, making bit-OR safe.
