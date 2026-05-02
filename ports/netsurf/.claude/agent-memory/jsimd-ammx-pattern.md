---
name: jsimd_ammx_pattern
description: AMMX init pattern from NetSurf-MUI's JPEG decoder (precedent for font compositor)
type: reference
---

# AMMX Init Pattern from NetSurf-MUI (jsimd_ammx.c)

Source: `original/include/simd/jsimd_ammx.c` + `jsimd_ammx.h` + `jdcolor-ammx.asm`

Upstream: arczi84/NetSurf-3.11-MUI commit 4a5176ce02c2c4d0a3aae493a458cf34f172d18d

This is the **working AMMX precedent** from the same author (arczi84) as NetSurf-68k. It shows how to wire up V_EnableAMMX init in a real application. Phase E (Tasks 17-19) will mirror this pattern for font_freetype.c.

## 1. Headers and Includes

```c
#include <vampire/vampire.h>
#include <proto/vampire.h>
#include <proto/exec.h>
#include <exec/execbase.h>

/* defined in exec (differently), invalidate before starting with jpeglib stuff */
#undef GLOBAL
```

**Key:** `#undef GLOBAL` before jpeglib headers — the exec headers define GLOBAL, jpeglib redefines it. This pattern will NOT apply to FreeType (no conflicting GLOBAL), but it's a reminder to check for header collisions.

## 2. Global VampireBase Storage

```c
struct Library *gVampireBase; /* we can overwrite this several times, no problem -> it's a resource */
```

**Pattern:** Global storage for VampireBase, marked safe to overwrite (it's a resource, not a library — no OpenLibrary/CloseLibrary lifecycle).

## 3. Init Function (ammx_init — called at library/startup init)

```c
LOCAL(int) ammx_init ( long privdata[] )
{
    struct Library *SysBase = *((struct Library **)(0x4));
    struct Library *VampireBase;
    int res;

    privdata[ AMMX_PRIV_ONOFF ] = 0;
    privdata[ AMMX_PRIV_VRES ] = (0);

    /* check whether we have Vampire.resource */
    VampireBase = OpenResource( "vampire.resource" );
    if( !VampireBase )
    {
        gVampireBase = (0); /* sorry, no SIMD */
        return 0;
    }

    if( VampireBase->lib_Version >= 45 )
    {
      res = V_EnableAMMX( V_AMMX_V2 );
      if( res != VRES_ERROR )
      {
        /* set Marker only when we actually need to disable AMMX (i.e. it was off before) */
        if( res == VRES_OK )
            privdata[ AMMX_PRIV_ONOFF ] = AMMX_PRIV_ONOFF_MARKER | VRES_OK;

        jsimd_memset_switch_ammx( 1 );

        privdata[ AMMX_PRIV_VRES ]  = (long)VampireBase;
        gVampireBase = VampireBase;
        return 1;
      }
    }

    gVampireBase = (0); /* sorry, no SIMD */
    return 0;
}
```

**Key points:**

1. **OpenResource, not OpenLibrary** — `vampire.resource` is a resource (no Close).
2. **Check lib_Version >= 45** — AMMX2 requires v45+. Our `amiport_ammx_init()` already does this.
3. **V_EnableAMMX(V_AMMX_V2)** — the actual enable call. Returns VRES_OK if it was off and is now on, or a previous-state code if already on.
4. **privdata state tracking** — the AMMX_PRIV_ONOFF_MARKER pattern lets cleanup know if we need to call `V_EnableAMMX(V_AMMX_DISABLE)` at shutdown. This is library-specific state; our font compositor can simplify (we'll call disable at shutdown unconditionally).
5. **Return 1 on success, 0 on failure** — simple boolean.

**For font_freetype.c:** We can call `amiport_ammx_init()` directly (it already wraps this), OR we can inline a simplified version like above. The plan says inline for clarity — this pattern shows it's ~20 lines.

## 4. Exit Function (ammx_exit — called at library/cleanup)

```c
LOCAL(int) ammx_exit ( long privdata[] )
{
    long t=privdata[AMMX_PRIV_ONOFF];
    struct Library *VampireBase;

    if( ( t & 0xffff0000 ) != AMMX_PRIV_ONOFF_MARKER )
        return 0;
    t &= 0xffff;    /* keep lower 16 Bit only */
    
    /* previous init was unsuccessful ? */
    if( !gVampireBase )
        return 0;
    if( gVampireBase != (void*)privdata[AMMX_PRIV_VRES] )
        return 0;

    VampireBase = (void*)privdata[AMMX_PRIV_VRES];

    if( t == VRES_OK )
    {
        if( VampireBase )
        {
            V_EnableAMMX( V_AMMX_DISABLE );
            privdata[AMMX_PRIV_ONOFF] = 0;
            return 0;
        }
    }
    return 0;
}
```

**Key:** Cleanup only disables AMMX if we were the ones who turned it on (`res == VRES_OK` at init). If AMMX was already on when we started, we leave it on at exit.

**For font_freetype.c:** We can unconditionally call `V_EnableAMMX(V_AMMX_DISABLE)` at cleanup — simpler pattern, our use case is single-binary not library.

## 5. Runtime Check (cinit_simd — called before SIMD paths)

```c
LOCAL(int)
cinit_simd (void)
{
 struct Library *VampireBase = gVampireBase;

 /* init wasn't called ? */
 if( !VampireBase )
     return 0;

 if( VRES_ERROR != V_EnableAMMX( V_AMMX_V2 ) )
     return 1;
 else
    return 0;
}
```

**Key:** Lightweight re-check that VampireBase is valid and AMMX is still enabled. This is called before dispatching to SIMD paths in the decoder hot loops.

**For font_freetype.c:** Our pattern will be simpler — a static `ammx_available` flag set at init, checked in `render_glyph_ammx()`.

## 6. Extern Declarations (jsimd_ammx.h — asm function prototypes)

```c
void jsimd_ycc_rgb_convert_ammx( register int img_width __asm("d2"),
                            register JSAMPIMAGE iptr    __asm("a2"),
                            register JSAMPARRAY optr    __asm("a3"),
                            register int img_row        __asm("d1"),
                            register int img_nrows      __asm("d0") );
```

**Pattern:** 68k register-based calling convention:
- `__asm("dN")` for data registers
- `__asm("aN")` for address registers
- No stack args in these examples (all args in registers)

**For font_freetype.c:** Our AMMX glyph compositor prototype will use the same register-convention pattern for the asm function.

## 7. vasm Assembly File Header (jdcolor-ammx.asm)

```asm
; jdcolor-ammx.asm - fast YCbCr to RGB conversion
;
; Copyright 2018 Henryk Richter <henryk.richter@gmx.net>
;
; This file should be assembled with VASM.
; VASM is available from http://sun.hasenbraten.de/vasm/
;
    machine ac68080

    xdef    _jsimd_ycc_rgb_convert_ammx
    xdef    _jsimd_ycc_rgbx_convert_ammx
```

**Key:**
- `machine ac68080` — tells vasm this is 68080-targeted code (AMMX instructions available)
- `xdef _function_name` — exports symbol with C-compatible name (underscore prefix)

**For font_freetype.c:** Our `glyph-blit-ammx.asm` will use the same header pattern.

## 8. Makefile Integration (inferred — not found in tree)

The .asm files exist in `original/include/simd/` but I could NOT find the actual vasm invocation in any Makefile in the tree (searched `original/frontends/amiga/Makefile`, `original/Makefile`, etc.). Possible reasons:

1. The build uses a higher-level build system (cmake / autoconf) that wasn't cloned with `--depth 1`
2. The .asm files are assembled manually and the .o files checked in
3. The build rule is in a non-Makefile file (shell script, etc.)

**For font_freetype.c:** Task 15 (writing the NetSurf port Makefile) will define the vasm rule. The canonical pattern from the plan is:

```make
glyph-blit-ammx.o: glyph-blit-ammx.asm
    vasmm68k_mot -Fhunk -m68080 -o $@ $<
```

## 9. No Error Reporting at Init Failure

The `ammx_init()` function returns 0 on failure but does NOT call `printf`, `fprintf(stderr)`, or any AmigaDOS alert. It's silent-fail with fallback to non-AMMX paths.

**For font_freetype.c:** Task 17 spec says we should emit a friendly "AMMX not available, using scalar path" message to stdout if init fails. This is DIFFERENT from the NetSurf-MUI pattern (which is silent). Our pattern is more user-friendly.

## 10. Comments in asm files are `;` (assembler syntax)

Standard 68k assembler comment syntax. No `//` or `/* */`.

## Summary for Phase E (Tasks 17-19)

**What to mirror:**
1. OpenResource("vampire.resource") + version check >= 45
2. V_EnableAMMX(V_AMMX_V2) return-code checking
3. Global VampireBase storage
4. Register-based calling convention for asm functions (`__asm("dN")`)
5. vasm with `machine ac68080` + `xdef _function_name`
6. Disable AMMX at cleanup if we enabled it

**What to do differently:**
1. Emit friendly error message if init fails (NetSurf-MUI is silent)
2. Simpler state tracking (static flag, not privdata array — we're a binary not a library)
3. Simpler cleanup (unconditional disable — we're single-binary not shared lib)

**Open question:** The vasm Makefile invocation. Will answer in Task 15 when writing `ports/netsurf/Makefile`.
