# lib/muimaster-sdk

MUI (Magic User Interface) `muimaster.library` SDK headers -- vendored
for NetSurf-MUI and any future port that builds an MUI-based GUI.

## Source

- Upstream: MUI 5.0 SDK / Open MUI (Stefan Stuntz original 1992-2006;
  Thore Boeckelmann + Jens Maus continuation 2006-2020).
- Distribution path used here: extracted from
  `amigadev/crosstools:m68k-amigaos` Docker image
  (`/opt/m68k-amigaos/m68k-amigaos/include/{clib,proto,inline,pragmas,lvo}/...`),
  same source as `lib/cybergraphx-sdk/`. ABI-neutral: the inline header
  uses `<inline/macros.h>` LP* macros, identical between bebbo-gcc 6.5
  and 13.3.
- Authors: Stefan Stuntz, Thore Boeckelmann, Jens Maus.
- License: MUI 5.0 source is GPL2; the SDK headers (proto/clib/inline/
  pragmas/lvo) are part of MUI 5's open-source release on GitHub
  (`adtools/MUI`) and have always been freely redistributable as
  build-time SDK headers (every Amiga gcc cross-toolchain bundles
  them).

## What we vendored

| Path | Purpose |
|---|---|
| `include/clib/muimaster_protos.h` | C function prototypes (sfdc 1.11f generated) |
| `include/proto/muimaster.h` | Standard NDK-style dispatcher (pulls in clib + inline OR pragmas) |
| `include/proto/muimaster_lib.h` | Identical alternate-name dispatcher |
| `include/inline/muimaster_lib.h` | bebbo-gcc inline LP* macros that JSR through `MUIMasterBase` |
| `include/inline/muimaster_lib_protos.h` | sfdc-generated extended protos |
| `include/pragmas/muimaster_pragmas.h` | SAS/C / VBCC pragma forms |
| `include/pragmas/muimaster_lib.h` | Alternate name |
| `include/lvo/muimaster_lib.i` | Asm-side LVO labels |

## What we did NOT vendor

NetSurf-MUI ships its OWN copies of these MUI headers and the `-I`
compiler flag picks them up first:

- `<libraries/mui.h>` -- NetSurf-MUI ships its own at
  `frontends/mui/include/libraries/mui.h` (specific MUI version pinned
  for the browser's class hierarchy).
- `<mui/Aboutbox_mcc.h>`, `<mui/Listtree_mcc.h>` and 19 other MCC
  (MUI Custom Class) headers -- all in `frontends/mui/include/mui/`.

Forward-scan confirmed NetSurf-MUI only references 2 MCC subclasses
(Aboutbox + Listtree) but its include/mui/ ships 21 headers in case
optional features are enabled.

## Build

No build needed -- headers-only set. `ports/netsurf/Makefile` adds
`-I/work/lib/muimaster-sdk/include` to the docker run env.

## Linking

`muimaster.library` calls do NOT need a stub `.a` library. The inline
header expands every call to:

```
LP3(0x36, struct IClass *, MUI_GetClass, ..., MUIMASTER_BASE_NAME)
```

which is a JSR through `MUIMasterBase` at the listed LVO offset. Every
consumer must:

1. Open the library at startup:
   ```c
   struct Library *MUIMasterBase = OpenLibrary("muimaster.library", 19);
   ```
2. Close at exit.
3. Make `MUIMasterBase` visible to the inline expansion -- either as
   a global or via `#define MUIMASTER_BASE_NAME my_base_name` before
   including `<proto/muimaster.h>`.

NetSurf-MUI's `frontends/mui/gui.c` already does this.

## Used by

- `ports/netsurf/` -- NetSurf-MUI 3.11 frontend uses muimaster
  extensively for window/object/notification dispatch (`MUI_GetClass`,
  `MUI_NewObject`, `DoMethod`, etc.). Found in 5+ .c files via
  `grep -l 'proto/muimaster.h' frontends/mui/*.c`.
- (potentially) future MUI-based application ports.

## Vendored 2026-05-02

Discovered as a forecasted Stage 11 blocker (per session 7 brief).
The forward-scan task #6 confirmed `proto/muimaster.h` is referenced
by 5+ NetSurf-MUI source files and is NOT in our amiport-toolchain-gcc13
NDK -- but IS in the canonical bebbo amigadev/crosstools image. Same
free win as `lib/cybergraphx-sdk/`.

## Cross-references

- Same vendor pattern: `lib/cybergraphx-sdk/` (extraction script in its
  UPSTREAM-source.txt is the reusable template).
- Vendor-list status: `~/.claude/projects/-Users-duncan-Developer-amiport/memory/project_netsurf_stage11_progress.md`
