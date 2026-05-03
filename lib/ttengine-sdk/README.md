# lib/ttengine-sdk

`ttengine.library` SDK headers -- vendored from Aminet for NetSurf-MUI's
TrueType text rendering path. ttengine is a TrueType-rendering shared
library widely used by classic Amiga apps before lib/freetype/ became
the cross-platform standard.

## Source

- Upstream: `util/libs/ttengine-68k.lha` (Aminet, ttengine 7.2 release).
- Direct URL: https://aminet.net/util/libs/ttengine-68k.lha
- Author: Grzegorz Kraszewski (2002-2005).
- License: see `UPSTREAM-ttengine.doc` (free for non-commercial use).

## Headers

| Path | Purpose |
|---|---|
| `include/libraries/ttengine.h` | `TT_*` tag definitions (TT_OpenFont, TT_GetPixmap, TT_DoneFont) |
| `include/clib/ttengine_protos.h` | C function prototypes |
| `include/proto/ttengine.h` | Standard NDK-style dispatcher |
| `include/inline/ttengine.h` | bebbo-gcc inline LP* macros (JSR through `TTEngineBase`) |
| `include/inline/ttengine_protos.h` | Extended protos |
| `include/pragma/ttengine_lib.h` | Pragma form |
| `include/pragmas/ttengine_pragmas.h` | Alt pragma name |
| `include/lvo/ttengine_lib.i` | Asm LVO labels |
| `include/ppcinline/ttengine.h` | MorphOS PPC inline |
| `ttengine_lib.fd` | Function descriptor (LVO offsets) |
| `UPSTREAM-ttengine.doc` | Autodoc API reference |

## Build

No build needed -- headers-only set. `ports/netsurf/Makefile` adds
`-I/work/lib/ttengine-sdk/include` to the docker run env.

## Linking

`ttengine.library` calls do NOT need a stub `.a` library. The inline
header expands every call to a JSR through `TTEngineBase`. Consumer
must:

1. Open at startup: `OpenLibrary("ttengine.library", 6);`
2. Close at exit.
3. Make `TTEngineBase` visible to inline expansion.

NetSurf-MUI handles this in `frontends/mui/font_tte.c` and `gui.c`.

## Used by

- `ports/netsurf/` -- 5 .c files (`font_tte.c`, `plotters.c`,
  `browserclass.c`, `gui.c`, `extrasrc.h` + `os3.h`). The `font_tte.c`
  is the TT-engine font path (alternative to FreeType-based rendering;
  NetSurf chooses one at compile time based on FB_FONTLIB).

## Why TTEngine and not FreeType?

NetSurf-MUI's `frontends/mui/` was written for the classic Amiga
TTEngine library -- the de-facto TrueType rendering API on AmigaOS 3.x
before bebbo-gcc made FreeType practical. Phase E of NetSurf-Vampire
plans to add an AMMX-accelerated FreeType path, but for Phase 1 we
preserve NetSurf-MUI's existing TTEngine-based rendering.

## Vendored 2026-05-02

Per Stage 11 forward-scan (task #6) -- compile + link side covered.
ttengine 7.2 was the last 68k release.
