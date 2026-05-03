# lib/asyncio-sdk

`asyncio.library` SDK headers -- vendored from Aminet for NetSurf-MUI.
asyncio.library provides buffered, asynchronous file I/O with overlapping
read/write (one CPU runs while the next disk I/O is dispatched in
parallel). Originally Martin Taillefer's 1995 wrapper; widely used by
Amiga apps that do bulk file I/O.

## Source

- Upstream: `dev/c/AsyncIO.lha` (Aminet, 1997 release).
- Direct URL: https://aminet.net/dev/c/AsyncIO.lha
- Author: Martin Taillefer.
- License: see `UPSTREAM-asyncio.doc` (free redistribution clauses).

## Headers

| Path | Purpose |
|---|---|
| `include/libraries/asyncio.h` | `struct AsyncFile` + `MODE_*` constants |
| `include/clib/asyncio_protos.h` | C function prototypes |
| `include/proto/asyncio.h` | Standard NDK-style dispatcher |
| `include/pragmas/asyncio_pragmas.h` | SAS/C `#pragma libcall` -- LVO offset reference, NOT used by gcc |
| `fd/asyncio_lib.fd` | Function descriptor (LVO offsets per call) |

## ⚠️ Linking gap

Aminet's AsyncIO release pre-dates bebbo-gcc inline headers and does
NOT ship `inline/asyncio.h`. The `proto/asyncio.h` directly includes
`pragmas/asyncio_pragmas.h` whose `#pragma libcall` directives are
SAS/C-only -- bebbo-gcc ignores them.

**Compilation works** (gcc parses the function declarations from
`clib/asyncio_protos.h` normally), but **LINK fails** (no symbols for
OpenAsync / CloseAsync / ReadAsync / WriteAsync).

When we get to the link stage, we have three options:

1. **Hand-generate `inline/asyncio.h`** with bebbo-gcc LP* macros
   from the .fd file (~9 functions). The fd format is documented; LVO
   offsets are explicit (`OpenAsync 1e 10803`, etc.). Estimated 30 min
   of mechanical work.
2. **Build an asyncio stub `.a`** using `fd2pragma -i` (or sfdc) to
   emit asm stubs that JSR through `AsyncIOBase`. Same approach as
   `lib/amissl-sdk/`.
3. **Source-replace asyncio calls** in NetSurf-MUI -- asyncio is a
   thin wrapper over `Open()`/`Read()`/`Write()`; a 50-line source
   patch could replace it. This is what the dragonball-DOS / morphos
   forks do.

Option 1 is the cleanest. Deferred to the link stage of Stage 11.

## Used by

- `ports/netsurf/` -- 5 functions across 3 files:
  `OpenAsync` / `CloseAsync` / `ReadAsync` / `WriteAsync`. References:
  `frontends/mui/gui.c`, `frontends/mui/mui_fetch.c`,
  `frontends/mui/fetch_file_mui.c`.

## Vendored 2026-05-02

Per Stage 11 forward-scan (task #6) -- compile-side blocker for
`mui_fetch.c` and `fetch_file_mui.c`. Link-side will need follow-up
(see linking gap section above).
