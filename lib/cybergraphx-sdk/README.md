# lib/cybergraphx-sdk

CyberGraphX RTG (Retargetable Graphics) SDK headers — vendored for
NetSurf-MUI and any future port that needs to write graphics code
against `cybergraphics.library` (the dominant pre-OS4 RTG API on
classic Amiga, used by Picasso96, CyberVision64, and most modern
graphics cards via emulation layers).

## Source

- Upstream: phase5 digital products CyberGraphX SDK 41.18 (1998-02-21)
- Distribution path used here: extracted from
  `amigadev/crosstools:m68k-amigaos` Docker image
  (`/opt/m68k-amigaos/m68k-amigaos/include/{cybergraphx,cybergraphics,
  clib,proto,inline,pragmas}/...`), which has bundled this SDK in every
  bebbo-gcc 6.5 cross-toolchain build for years.
- Authors: Frank Mariak, Carsten Magerkurth (phase5 digital products).
- Copyright: phase5 digital products, 1996-1998.
- License: phase5's CyberGraphX SDK has been freely redistributed
  alongside every Amiga gcc cross-toolchain since the late 1990s. The
  headers are publicly distributed as part of `cybergraphx.lha` on
  Aminet (`gfx/board/`) and bundled in `bebbo/amiga-gcc`'s prebuilt NDK
  archive. Using them as build-time headers (no runtime redistribution
  of the upstream SDK package) follows the same convention as every
  other AmigaOS gcc port.
- Companion files retained:
  - `cybergraphics.fd` -- function descriptor (LVO offsets per call,
    used to regenerate `inline/cybergraphics.h` if needed via
    `m68k-amigaos-fd2pragma` or sfdc).
  - `cybergraphics.sfd` -- structured fd format (sfdc input).
  - `UPSTREAM-cybergraphics.doc` -- upstream API documentation.

## Headers

| Path | Purpose |
|---|---|
| `include/cybergraphx/cybergraphics.h` | Constants, structs (PIXFMT_*, RECTFMT_*, struct CyberModeNode, LBMI_*, UBMI_*) |
| `include/cybergraphx/cybergraphics.i` | Asm-side constants (.i format, ASM consumers only) |
| `include/cybergraphics/cybergraphics.h` | Identical to `cybergraphx/cybergraphics.h` -- alternate spelling some consumers use |
| `include/cybergraphics/cybergraphics.i` | Asm-side, alternate spelling |
| `include/clib/cybergraphics_protos.h` | C function prototypes (sfdc 1.11f generated) |
| `include/proto/cybergraphics.h` | Standard NDK-style dispatcher (pulls in clib/inline OR pragmas) |
| `include/inline/cybergraphics.h` | bebbo-gcc inline LP* macros that JSR through `CyberGfxBase` LVOs |
| `include/inline/cybergraphics_protos.h` | sfdc-generated extended protos |
| `include/pragmas/cybergraphics_pragmas.h` | SAS/C / VBCC pragma forms (non-gcc compilers) |

## Build

No build needed -- headers-only set. Consumers add
`-I/work/lib/cybergraphx-sdk/include` to CFLAGS before any `-I` paths
that point at the NDK (the NDK has no cybergraphics headers, so order
isn't critical, but consistency with the SDI / AmiSSL pattern means
this -I goes alongside other vendored SDK -I flags).

`ports/netsurf/Makefile` adds it to the docker run env.

## Linking

`cybergraphics.library` calls do NOT need a stub `.a` library. The
inline header expands every call to:

```
LP10(0x66, ULONG, WritePixelArray, ..., CYBERGFX_BASE_NAME)
```

which is a JSR through `CyberGfxBase` at LVO offset `0x66`. Every
consumer must:

1. Open the library at startup:
   ```c
   struct Library *CyberGfxBase = OpenLibrary("cybergraphics.library", 39);
   if (!CyberGfxBase) { /* fallback path -- no RTG */ }
   ```
2. Close the library at exit:
   ```c
   if (CyberGfxBase) CloseLibrary(CyberGfxBase);
   ```
3. Make `CyberGfxBase` visible to the inline expansion -- either as a
   global or via `#define CYBERGFX_BASE_NAME my_base_name` before
   `#include <proto/cybergraphics.h>`.

NetSurf-MUI's `frontends/mui/gui.c` already does this correctly (lines
147 / 367-369 / 442-445 / 639). Future consumers should follow the same
pattern.

## Used by

- `ports/netsurf/` -- NetSurf-MUI 3.11 frontend uses `WritePixelArray`,
  `ReadPixelArray`, `FillPixelArray` (with RECTFMT_RGB / RECTFMT_RGBA /
  RECTFMT_ARGB) for browser canvas rendering on RTG screens. The
  consumer surface is small (~5 functions) but spans 8 .c files
  (`browserclass.c`, `gui.c`, `print.c`, `plotters.c`, `plot.c`,
  `extrasrc.c`, `toolbuttonclass.c`, `applicationclass.c`,
  `transferanimclass.c`).
- (potentially) future AmigaOS application ports that need to render
  to RTG screens (Picasso96, CyberVision64, Vampire SAGA chunky modes
  via Picasso96 emulation, etc.).

## Vendored 2026-05-02

Discovered as a Stage 11 second blocker in NetSurf-MUI compilation
(after `lib/sdi-headers/` cleared the SDI block). Symptom:
`frontends/mui/applicationclass.c:22:10: fatal error:
cybergraphx/cybergraphics.h: No such file or directory`. Vendored to
clear the blocker; build advances past the cybergraphx-using TUs to
the next class (likely MUI library bindings -- `libraries/mui.h` is
NetSurf-MUI's local copy at `frontends/mui/include/libraries/mui.h`,
but `mui/muimaster.library` may need separate vendoring).

## Why we picked this source over alternatives

Considered four sources during vendoring:

1. **Aminet `gfx/board/CyberGraphX_4.3rc6.lha`** -- runtime driver
   only, no SDK headers. Ruled out.
2. **Aminet `dev/c/P96Develop.lha`** -- ships Picasso96-NATIVE headers
   (`libraries/Picasso96.h`) but no `cybergraphx/cybergraphics.h`
   compatibility layer. Wrong API surface for NetSurf-MUI.
3. **AROS `workbench/devs/diskimage/include/amigaos/cybergraphx/`** --
   has the .h and clib protos, but lacks bebbo-gcc inline LP* header
   (AROS uses a different LVO dispatch via `defines/cybergraphics.h`).
   Would have required hand-generating the inline header from the .fd.
4. **bebbo `amigadev/crosstools:m68k-amigaos` image** -- ships the
   complete sfdc-generated SDK (proto + inline + clib + pragmas + fd
   + sfd) already configured for bebbo-gcc. **PICKED -- option (d)
   from the session brief.** ABI-neutral (the inline LP* macros
   resolve through `<inline/macros.h>` which is identical between
   bebbo-gcc 6.5 and 13.3). Clean import.

## Cross-references

- Vendor pattern: `lib/sdi-headers/` (smaller header-only Aminet vendor)
- Stub library pattern (for future muimaster.library / Picasso96API.library):
  `lib/amissl-sdk/`
- Status memo: `~/.claude/projects/-Users-duncan-Developer-amiport/memory/project_netsurf_stage11_progress.md`
