# NetSurf-MUI AMMX Integration Pattern

**Source:** [arczi84/NetSurf-3.11-MUI](https://github.com/arczi84/NetSurf-3.11-MUI), `include/simd/`. Working precedent for AMMX2-accelerated JPEG SIMD inside NetSurf 3.11 on AmigaOS 3 / Vampire. Same author as the NetSurf-68k base port. License: GPL v2 (NetSurf), copyright Henryk Richter 2018.

## File inventory in include/simd/

```
jsimd_ammx.c          — C glue: V_EnableAMMX init, dispatch to asm kernels
jsimd_ammx.h          — extern decls for asm-defined symbols
jmemset_ammx.h        — memset interface
jdcolor-ammx.asm      — vasm: JPEG YCbCr -> RGB color conversion
jdmerge-ammx.asm      — vasm: merged upsample (h2v1, h2v2)
jidctfst-ammx.asm     — vasm: integer fast IDCT
jmemset-ammx.asm      — vasm: memset variants
jsimd.h               — shared SIMD interface
jsimd68k.c            — non-AMMX 68k SIMD fallback (no Apollo)
```

This is the canonical integration template for ANY amiport port wanting to use AMMX from C. The text-rendering / NetSurf font compositor work in Phase 1 follows this exact shape.

## C-side init pattern (from jsimd_ammx.c)

```c
#include <vampire/vampire.h>
#include <proto/vampire.h>
#include <proto/exec.h>
#include <exec/execbase.h>

struct Library *gVampireBase;  /* global; can be re-set safely */

LOCAL(int) cinit_simd(void)
{
    struct Library *VampireBase = gVampireBase;
    if (!VampireBase) return 0;
    if (V_EnableAMMX(V_AMMX_V2) != VRES_ERROR) return 1;
    return 0;
}

LOCAL(int) ammx_init(long privdata[])
{
    struct Library *VampireBase = OpenResource("vampire.resource");
    if (!VampireBase) { gVampireBase = 0; return 0; }
    if (VampireBase->lib_Version < 45) { gVampireBase = 0; return 0; }

    int res = V_EnableAMMX(V_AMMX_V2);
    if (res == VRES_ERROR) { gVampireBase = 0; return 0; }

    /* Mark "we turned it on" only when res == VRES_OK
     * (so we know whether to disable it at shutdown) */
    if (res == VRES_OK)
        privdata[AMMX_PRIV_ONOFF] = AMMX_PRIV_ONOFF_MARKER | VRES_OK;

    privdata[AMMX_PRIV_VRES] = (long)VampireBase;
    gVampireBase = VampireBase;
    return 1;
}

LOCAL(int) ammx_exit(long privdata[])
{
    if (!gVampireBase) return 0;
    long t = privdata[AMMX_PRIV_ONOFF];
    if ((t & 0xffff0000) != AMMX_PRIV_ONOFF_MARKER) return 0;
    t &= 0xffff;
    /* Shut down AMMX only if we enabled it (not if it was already on) */
    if (t == VRES_OK)
        V_EnableAMMX(V_AMMX_DISABLE);
    privdata[AMMX_PRIV_ONOFF] = 0;
    return 1;
}
```

**Key insight on init/exit:** the shutdown logic checks `VRES_OK` (we enabled it) vs `VRES_AMMX_WAS_ON` (someone else already had it on). Only disable in the first case — never disable AMMX someone else needed.

## C-side dispatch pattern

```c
GLOBAL(void)
jsimd_ycc_rgb_convert(j_decompress_ptr cinfo, JSAMPIMAGE input_buf,
                       JDIMENSION input_row, JSAMPARRAY output_buf, int num_rows)
{
    AMMX_YCC_RGB_CONVERT_TYPE(ammxfct);
    ammxfct = jsimd_ycc_rgb_convert_ammx;        /* extern symbol from .asm file */

    if (cinfo->out_color_space != JCS_RGB) {
        if ((cinfo->out_color_space != JCS_EXT_RGB) &&
            (cinfo->out_color_space != JCS_EXT_BGR))
            ammxfct = jsimd_ycc_rgbx_convert_ammx;  /* alternate kernel */
    }
    ammxfct(cinfo->output_width, input_buf, output_buf, input_row, num_rows);
}
```

C calls the asm kernel as if it were a normal C function. The vasm file declares `xdef _jsimd_ycc_rgb_convert_ammx` and bebbo-gcc adds the leading underscore on the C side automatically.

## Calling convention — REGISTER-BASED (not stack-based)

**Critical:** the asm kernels use **bebbo-gcc's register-parameter syntax** rather than the standard m68k SysV stack convention:

```c
/* C-side declaration in jdcolor-ammx.asm comment block: */
void jsimd_ycc_rgb_convert_ammx(
    register int img_width      __asm("d2"),
    register JSAMPIMAGE iptr    __asm("a2"),
    register JSAMPARRAY optr    __asm("a3"),
    register int img_row        __asm("d1"),
    register int img_nrows      __asm("d0"));
```

This forces specific arguments into specific registers via the `register __asm("dN")` GCC extension. The asm file then reads directly from those registers without `movem.l n(sp)` decoding.

**Implications for the NetSurf font compositor (Task 18 in the implementation plan):**
- The plan's example showed stack-based parameter passing (`move.l 4+44(sp), a0`). That's still valid, but the cleaner pattern matching arczi84's precedent uses register-passing.
- For the font glyph compositor: declare the C signature with `register __asm()` clauses, drop the `movem.l` and stack-offset reads.

## vasm syntax conventions (from jdcolor-ammx.asm)

```asm
    machine ac68080            ; not "m68080" — use "ac68080" per vasm convention

    xdef    _jsimd_ycc_rgb_convert_ammx    ; export with leading underscore

_jsimd_ycc_rgb_convert_ammx:
    movem.l d3-d5/a3-a6,-(sp)  ; save callee-saved (skip d0-d2/a0-a2 if used as params)

    ; ... AMMX inner loop using d0-d7, e0-e23, a0-a7 ...

    movem.l (sp)+,d3-d5/a3-a6
    rts
```

## Key AMMX patterns from jdcolor-ammx.asm

```asm
; Zero a register (cheap)
peor    E16,E16,E16            ; XOR self → 0

; Widen 4 bytes to 4 words (for arithmetic without overflow)
vperm   #$84858687,d4,E16,E18  ; bytes 4,5,6,7 from d4 paired with byte from E16 (=0)

; DC offset for JPEG chroma (subtract 128)
psubw.w #128,E18,E18

; Fixed-point scaled multiply
pmul88.w #FIX_1_3711,E18,E18   ; E18 = (E18 * 359) >> 8 per word

; Pack words to unsigned bytes with saturation
packuswb E22,E23,E5

; Counted store (handles partial last group)
storec   E5,d3,(a0)+           ; write min(d3, 8) bytes from E5, post-increment
```

The 11-cycle-per-4-pixels chroma block in `RGB24_CHROMABLOCK` macro is a textbook example of dual-issue scheduling — paired peor/move.l on the same cycle line.

## How this maps to NetSurf font compositor (Phase 1 Task 18)

The font compositor's inner loop has the same shape as `jdcolor-ammx.asm`:

1. Load 8 alpha bytes from glyph bitmap → D0
2. Load 2 destination ARGB pixels from framebuffer → E0
3. PMULA against precomputed `(255-A | premul_R | premul_G | premul_B)` → blended pixels
4. STORE/STOREC to framebuffer
5. Loop with dbra (pairs with STORE for free in second pipe)

Use the same `register __asm("dN")` calling convention so the C glue can call the asm kernel without setup overhead.

## Why crib from this fork specifically

- Same author as the NetSurf-68k base port we're forking from
- Already wired up V_EnableAMMX correctly (with the VRES_OK / VRES_AMMX_WAS_ON nuance)
- Already has the vasm build infrastructure in its Makefile
- Has working AMMX2 JPEG SIMD shipping today on Vampire V4 / A6000
- License-compatible (GPL v2)

amiport's NetSurf Phase 1 fork should adopt the same `include/simd/` directory layout and naming convention to keep the code readable next to the JPEG-AMMX precedent.

## Tooling

The Makefile in this repo invokes vasm via something like:
```
vasmm68k_mot -m68080 -Fhunk -quiet -o out.o input.asm
```
(exact form depends on the upstream Makefile — read it before adapting).

## Cross-references

- `docs/references/ammx/instruction-reference.md` — Apollo PRM AMMX reference
- `docs/references/vampire-sdk/headers.md` — Vampire SDK header reference
- `lib/posix-shim/include/amiport/ammx.h` — amiport_ammx_init() wrapper (planned)
- `ports/netsurf/` — first amiport consumer

## Discovery context

Cloned and indexed 2026-05-02 during the NetSurf Vampire Phase 1 KB hydration.
