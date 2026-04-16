---
name: port_freetype_perf
description: Performance findings for lib/freetype 2.13.3 — soft-integer ops, -O1 promotion candidates, dead-module analysis, zero-init alloc, psnames table reviewed 2026-04-16
type: project
---

# FreeType 2.13.3 Performance Audit — 2026-04-16

## Key Findings

### Soft-float: CLEAR
No float/double soft-emulation symbols (__divsf3, __floatunsisf, etc.) anywhere in
libfreetype.a. FT_Angle = FT_Fixed (integer). Trig uses CORDIC. float/double in
FT_TRACE macros compile to `do {} while(0)` (FT_DEBUG_LEVEL_TRACE not defined).
float in CF2_IO_FAIL block is dead code. mathieeesingbas crash risk = ZERO.

### Soft-integer ops (libgcc, safe): ALL MODULES
___divsi3, ___mulsi3, ___udivsi3 in: ftbase, autofit, smooth, raster, sfnt, truetype,
psaux, pshinter, ftbitmap.
___muldi3, ___divdi3 in: ftbase (FT_MulDiv), smooth (FT_UDIV macro).
These go through libgcc, NOT mathieeesingbas. No crash risk. Performance cost on 68000:
___divsi3 = ~40-80 cycles, ___mulsi3 = ~20-40 cycles. Eliminated at -m68020 by using
native MULS.L/DIVS.L when -O1 enables scheduling.

### Struct-by-value returns: CLEAR
FT_Vector (8 bytes) is exactly at the crash-patterns #16 limit, not over it.
FT_BBox (16 bytes) never returned by value — always via pointer. FT_Matrix same.
All hot-path functions return void, int, FT_Error, or FT_Fixed scalars.
TT interpreter uses switch() dispatch, not function pointer table.

### -O1 promotion candidates
All hot-path files are scalar-return, no aliasing beyond `(unsigned char)` casts
(which -fno-strict-aliasing covers). FreeType already designed for strict ANSI C.

### psnames: 65KB object with static glyph name tables
The 268KB pstables.h compiles to 65KB of ROM-able static data. Required for sfnt
glyph name service even in TT-only builds. Not droppable.

### ft_mem_alloc zero-inits: performance note
ft_mem_alloc calls FT_MEM_ZERO (memset) on every allocation. ft_mem_qalloc skips it.
FreeType uses FT_MEM_QALLOC for allocations it will immediately overwrite (e.g. glyph
bitmap buffer). Pattern is correct; no redundant zeroing found.

### ftsystem.c: uses malloc/realloc (libnix heap), no MEMF_CHIP. CLEAN.

## Module sizes (text)
truetype.o: 91KB | sfnt.o: 84KB | psnames.o: 65KB | autofit.o: 61KB
psaux.o: 61KB | ftbase.o: 55KB | pshinter.o: 20KB | raster.o: 12KB
smooth.o: 12KB | ftbitmap.o: 7KB

## -O1 Promotion Table (applied in Stage 8)
smooth/smooth.c: -O1 -fno-strict-aliasing (AA rasterizer, pure scalar, static fns)
truetype/truetype.c: -O1 -fno-strict-aliasing (bytecode interp + glyph loader)
base/ftbase.c: -O1 -fno-strict-aliasing (FT_MulDiv/FT_MulFix/FT_DivFix)
autofit/autofit.c: -O1 -fno-strict-aliasing (hint calculation)
raster/raster.c: -O1 -fno-strict-aliasing (mono rasterizer, pure integer)
sfnt/sfnt.c: -O0 (large, complex, less hot — conservative)
psaux/psaux.c: -O0 (CFF/PS decoder, function complexity)
pshinter/pshinter.c: -O0 (PS hinter, not in TT glyph hot path)
psnames/psnames.c: -O0 (table lookups, not in render hot path)
base/ftbitmap.c: -O1 -fno-strict-aliasing (bitmap conversion, small)
base/ftbbox.c: -O1 -fno-strict-aliasing (bounding box, scalar only)
base/ftsystem.c: -O0 (startup/teardown only)
base/ftinit.c: -O0 (init only)
base/ftdebug.c: -O0 (debug only)
base/ftglyph.c: -O0 (glyph copy/transform utilities)
base/ftmm.c: -O0 (Multiple Master font support)

## Dead module analysis
psaux + pshinter: 81KB combined. Needed if SDL_ttf might open OTF/CFF fonts
(OpenType is CFF-based). Cannot safely drop without knowing SDL_ttf font list.
If guaranteed TTF-only: FT_USE_PSAUX + FT_USE_PSAUX removal saves 81KB.
psnames: needed for sfnt glyph name service even in TTF builds. Keep.
