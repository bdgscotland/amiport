# lib/codesets-sdk

`codesets.library` SDK headers -- vendored from Aminet for NetSurf-MUI
and any future port that needs character-set conversion / detection /
UTF-8 handling on AmigaOS (the canonical AmigaOS i18n library).

## Source

- Upstream: codesets-6.22.lha (2024-02-26)
- Aminet path: `util/libs/codesets.lha`
- Direct URL: https://aminet.net/util/libs/codesets-6.22.lha
- Authors: Alfonso "alfie" Ranieri (2001-2005); codesets.library Open
  Source Team (2005-2021); maintainer Jens Maus.
- Project page: https://github.com/jens-maus/codesetslib
- License: LGPL-2.1 (see `COPYING`). Headers are part of the public
  SDK distribution, freely usable as build-time .h includes.

## Headers

| Path | Purpose |
|---|---|
| `include/libraries/codesets.h` | Main header: `struct codeset`, `CSA_*` tags, error codes |
| `include/clib/codesets_protos.h` | C function prototypes |
| `include/proto/codesets.h` | Standard NDK-style dispatcher |
| `include/inline/codesets.h` | bebbo-gcc inline LP* macros (JSR through `CodesetsBase`) |
| `include/inline/codesets_protos.h` | Extended protos |
| `include/pragmas/codesets_pragmas.h` | SAS/C / VBCC pragma forms |
| `include/defines/codesets.h` | AROS dispatch define tables |
| `include/inline4/codesets.h` | OS4 inline calls |
| `include/interfaces/codesets.h` | OS4 IFace struct |
| `codesets_lib.fd` | Function descriptor (LVO offsets) |
| `UPSTREAM-codesets.doc` | Autodoc API reference |

## Build

No build needed -- headers-only set. `ports/netsurf/Makefile` adds
`-I/work/lib/codesets-sdk/include` to the docker run env.

## Linking

`codesets.library` calls do NOT need a stub `.a` library. The inline
header expands every call to a JSR through `CodesetsBase`. Every
consumer must:

1. Open the library at startup:
   ```c
   struct Library *CodesetsBase = OpenLibrary("codesets.library", 6);
   ```
2. Close at exit.
3. Make `CodesetsBase` visible to the inline expansion (global or
   `#define CODESETS_BASE_NAME my_base_name`).

NetSurf-MUI handles this in `frontends/mui/gui.c`.

## Used by

- `ports/netsurf/` -- NetSurf-MUI uses codesets for character-set
  conversion in HTML / HTTP charset handling. References:
  `frontends/mui/gui.c`, `frontends/mui/browserclass.c`.

## Vendored 2026-05-02

Discovered as a forecasted Stage 11 blocker by the forward-scan
(task #6). 2 NetSurf-MUI .c file consumers. SDK headers come from
Aminet's official codesets-6.22 distribution.

## Cross-references

- Same vendor pattern as `lib/cybergraphx-sdk/` and
  `lib/muimaster-sdk/` (extracted from amigadev image) -- this one
  is sourced from Aminet because amigadev/crosstools doesn't ship
  codesets headers.
- Vendor-list status: `~/.claude/projects/-Users-duncan-Developer-amiport/memory/project_netsurf_stage11_progress.md`
