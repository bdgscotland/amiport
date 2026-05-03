# lib/libjpeg

JPEG image decoder + encoder. The second EXTERNAL library in the
NetSurf-Vampire Phase D-prime dep stack.

Upstream: https://www.ijg.org/files/jpegsrc.v9f.tar.gz (libjpeg 9f),
IJG license (essentially zlib-style; see UPSTREAM-README and
UPSTREAM-INSTALL.txt). Copyright 1991-2024 Independent JPEG Group.

## What it is

libjpeg is the canonical reference implementation of the JPEG image
file format. We vendor 44 hand-written .c files (37 LIBSOURCES from
upstream's Makefile.unix minus the FLOAT DCT variants, plus
jmemnobs.c -- the no-backing-store memory manager).

Total compiled archive: ~272 KB at -O0 (expected ~220-240 KB after
Stage 7 -O1 hot-file promotion).

## Public API

See `include/jpeglib.h`. Core entry points:

- **Decompress side:**
  - `jpeg_create_decompress(&cinfo)` -- init cinfo struct
  - `jpeg_mem_src(&cinfo, buffer, size)` -- set memory data source
  - `jpeg_read_header(&cinfo, TRUE)` -- parse JPEG header chunks
  - `cinfo.dct_method = JDCT_ISLOW;` -- (optional) select integer DCT
  - `jpeg_start_decompress(&cinfo)` -- begin decode pipeline
  - `jpeg_read_scanlines(&cinfo, rowptrs, n)` -- decode N rows
  - `jpeg_finish_decompress(&cinfo)` -- complete the decode
  - `jpeg_destroy_decompress(&cinfo)` -- free internal pool
- **Compress side:** symmetric via `jpeg_create_compress` etc.
- **Error handling:** consumer wraps every entry point in
  `if (setjmp(err.setjmp_buffer)) { ... }` and provides an
  `error_exit` callback that calls longjmp.

## Build

```bash
make -C lib/libjpeg
```

Produces `libjpeg.a` (~272 KB at -O0).

**CPU target:** `-m68040 -m68881`. Same NetSurf-Vampire dep stack
convention. The `-m68881` ensures GCC inlines FPU instructions for
any future float math -- but our config disables float DCT entirely.

**Defines:** `-DNDEBUG -std=c99`.

**Optimization:** `-O0 -fno-strict-aliasing` whole-archive default
per crash-patterns #16. Stage 7 perf-optimizer audit will promote
specific hot-path files to `-O1`.

**Depends on:** nothing (no zlib, no external deps; pure C).

## CRITICAL config: float DCT disabled

The vendored `src/jmorecfg.h` and `include/jmorecfg.h` (kept identical
between the two locations) gate the `DCT_FLOAT_SUPPORTED` define
behind `#ifdef JPEG_AMIPORT_FLOAT_DCT` -- which we don't define. This
excludes the entire floating-point DCT/IDCT code path from compilation.

**Why:** The float DCT methods (`jfdctflt.c`, `jidctflt.c`) use
`double` arithmetic. With our `-m68881` CPU target, GCC inlines FPU
instructions for these -- which works on Vampire / 68040 / 68060
hardware. But:

- On 68000-only configurations, double math would pull libm symbols
  routing through ROM `mathieee*.library`, which crashes on FS-UAE
  (the documented mathieeesingbas crash family, crash-patterns #2)
- On FS-UAE 68882 emulation, transcendental FPU instructions trigger
  the documented FS-UAE 68882 transcendental gap

The integer DCT methods (`JDCT_ISLOW` and `JDCT_IFAST`) provide
equivalent functionality:
- `JDCT_ISLOW` -- "slow but accurate" integer DCT (the libjpeg default)
- `JDCT_IFAST` -- "fast" integer DCT (slightly less accurate but
  noticeably faster on 68k)

NetSurf's typical JPEG decode workload uses `JDCT_ISLOW` (the
default) and never requests `JDCT_FLOAT`. If consumer code does
request `JDCT_FLOAT` at runtime, libjpeg's `jddctmgr.c` dispatch
will hit a JERR_NOT_COMPILED case and call `error_exit` (which
longjmps via the consumer's setjmp).

**Verified post-build:** `m68k-amigaos-nm libjpeg.a | grep
'__divsf3\|__divdf3\|__floatunsisf'` returns empty.

## Memory manager: jmemnobs.c

We use `jmemnobs.c` -- the no-backing-store memory manager. This
holds the entire decompressed image in heap memory (no temp files).
Appropriate for in-memory decode without filesystem touchpoints.

`DEFAULT_MAX_MEM` is set to 16 MB in `src/jconfig.h` -- plenty for
typical web JPEGs (rare to decode >2 MB compressed at once on AmigaOS).

The other memory managers (`jmemansi.c`, `jmemname.c`, `jmemmac.c`,
`jmemdos.c`) are NOT vendored.

## Test

```bash
make -C tests/libjpeg run
```

Runs the 18-test suite via `vamos -C 68040 -s 1024 -m 4096 ./test_libjpeg`.

The test fixture is the upstream IJG `testimg.jpg` (227x149 RGB JPEG,
5770 bytes) embedded inline as a `static const` array via `xxd -i`.
This avoids any filesystem touchpoints during the test.

Coverage:

- 8 functional (version string; create+destroy decompress; mem_src;
  read_header; inspect dimensions; start_decompress; read all
  scanlines via JDCT_ISLOW; read all scanlines via JDCT_IFAST)
- 3 error path (truncated input -> setjmp recovery; wrong magic ->
  setjmp recovery; JDCT_FLOAT requested -> safe handling)
- 3 edge case (jpeg_abort_decompress mid-stream; decode-then-recreate
  cycle; multiple concurrent decompress structs)
- 1 Amiga-specific (decode works without stdio, callback I/O only)
- 3 stress (50 create/destroy cycles, 50 full decode cycles, 50
  setjmp-recovery cycles)

## CRITICAL: vamos resource sizing

Test binaries linking libjpeg need `__stack >= 524288` AND
`__MEMORY_STEP >= 524288` (libpng-class). 256 KB defaults that work
for standalone NetSurf utility libs fail with no stdout output.

For `ports/netsurf` consumer linking the FULL dep stack: 1 MB cookies
recommended (libdom-class threshold).

## Memory model + consumer cleanup discipline

- **Every `jpeg_create_decompress` MUST be paired with `jpeg_destroy_decompress`**
- Same pair for compress
- **Every setjmp recovery path MUST also call destroy** (libjpeg's pool
  manager is robust to half-built cinfo and frees everything)
- I/O sources/destinations: caller-owned (libjpeg never frees buffers)
- Decoded scanline buffers: caller-allocated (libjpeg writes into them)

On AmigaOS `-noixemul`: leaks are permanent until reboot.

## Memory audit findings

See `lib/libjpeg/MEMORY-AUDIT.md` (Stage 6 report).

## Performance audit findings

See `lib/libjpeg/PERF-REPORT.md` (Stage 7 report).

## Test ASSERT-failure leak caveat

Same as the prior dep-stack libs.

## Consumers

- `ports/netsurf/` (Phase 1 final consumer) -- inline `<img src="X.jpg">`
  decoding (read side; consumer never encodes JPEGs)
- (potentially) future image-viewer ports
