# lib/openurl-sdk

`openurl.library` SDK headers -- vendored from Aminet for NetSurf-MUI
and any other Amiga app that needs to launch URLs in the user's
configured web browser (the canonical AmigaOS "Open in Browser" mechanism).

## Source

- Upstream: `comm/www/OpenURL-7.18.lha` (Aminet, 2018-01-16).
- Direct URL: https://aminet.net/comm/www/OpenURL-7.18.lha
- Authors: Troels Walsted Hansen et al. (1998-2005);
  openurl.library Open Source Team (2005-2018).
- Project page: https://sourceforge.net/projects/openurllib
- License: Public domain (the library itself is PD; some components
  may be LGPL or GPL -- see in-header comments).

## Headers

| Path | Purpose |
|---|---|
| `include/libraries/openurl.h` | Tag definitions (`URL_*` tags), error codes |
| `include/clib/openurl_protos.h` | C function prototypes |
| `include/proto/openurl.h` | Standard NDK-style dispatcher |
| `include/inline/openurl.h` | bebbo-gcc inline LP* macros (JSR through `OpenURLBase`) |
| `include/inline/openurl_protos.h` | Extended protos |
| `include/pragmas/openurl_pragmas.h` | SAS/C `#pragma libcall` |
| `include/defines/openurl.h` | AROS dispatch defines |
| `include/inline4/openurl.h` | OS4 inline calls |
| `include/interfaces/openurl.h` | OS4 IFace struct |
| `include/ppcinline/openurl.h` | MorphOS PPC inline |
| `openurl_lib.fd` | Function descriptor (LVO offsets) |

## Build

No build needed -- headers-only set. `ports/netsurf/Makefile` adds
`-I/work/lib/openurl-sdk/include` to the docker run env.

## Linking

`openurl.library` calls do NOT need a stub `.a` library. The inline
header expands every call to a JSR through `OpenURLBase`. Consumer
must:

1. Open at startup: `OpenLibrary("openurl.library", 7);`
2. Close at exit.
3. Make `OpenURLBase` visible to inline expansion.

NetSurf-MUI handles this in `frontends/mui/gui.c`.

## Used by

- `ports/netsurf/` -- 1 file (`frontends/mui/gui.c`) for the Help menu's
  "Open Homepage in System Browser" feature (typical fallback when the
  user wants to open a URL in their preferred Amiga browser).

## Vendored 2026-05-02

Per Stage 11 forward-scan (task #6) -- compile + link side covered.
Lower priority than codesets / asyncio (NetSurf-MUI usage is sparse).
