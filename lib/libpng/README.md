# lib/libpng

PNG image decoder + encoder. The first EXTERNAL (non-NetSurf-internal)
library in the NetSurf-Vampire Phase D-prime dep stack.

Upstream: https://github.com/glennrp/libpng @ branch libpng16 commit
`8c62c3b` (libpng 1.6.x). zlib/libpng license (see `LICENSE`).
Copyright 1995-2024 contributors per upstream `AUTHORS` file.

## What it is

libpng is the canonical reference implementation of the PNG image
file format. Used by every modern browser, image viewer, and image
editor. We vendor 15 hand-written .c files (~33K LOC) that implement
the full read + write + transformation pipeline, plus 2 public
headers (png.h, pngconf.h), 4 internal headers (pngdebug.h, pnginfo.h,
pngpriv.h, pngstruct.h), and the generated build configuration
header (`pnglibconf.h`, derived from `scripts/pnglibconf.h.prebuilt`
with one critical amiport patch -- see "Critical config" below).

example.c and pngtest.c from upstream are NOT vendored -- they are
end-user demo programs, not part of the library.

## Public API

See `include/png.h`. Core entry points:

- **Read side:** `png_create_read_struct`, `png_create_info_struct`,
  `png_set_read_fn` (consumer supplies I/O callback), `png_read_info`,
  `png_get_image_width` / `_height` / `_bit_depth` / `_color_type` /
  `_channels` / `_rowbytes`, `png_read_image` (decode pixels into
  caller buffer), `png_destroy_read_struct`
- **Write side:** `png_create_write_struct`, `png_create_info_struct`,
  `png_set_write_fn`, `png_set_IHDR`, `png_set_compression_level`,
  `png_write_info`, `png_write_image`, `png_write_end`,
  `png_destroy_write_struct`
- **Helpers:** `png_sig_cmp` (verify 8-byte PNG signature),
  `png_get_libpng_ver`, `png_jmpbuf` (for setjmp error recovery)
- **Error model:** the consumer wraps every libpng entry point in
  `if (setjmp(png_jmpbuf(p))) { ... cleanup ... return; }`. When
  libpng encounters a fatal condition it calls `png_error()` which
  longjmps back to the caller's setjmp site.

## Build

```bash
make -C lib/libpng
```

Produces `libpng.a` (~215 KB at -O0, expected ~150-180 KB after
Stage 7 -O1 hot-file promotion).

**CPU target:** `-m68040 -m68881`. Same NetSurf-Vampire dep stack
convention. The `-m68881` ensures GCC inlines FPU instructions for
any future float math rather than emitting soft-float symbol calls
-- but with our floating-point-disabled config the library uses
zero float math anyway.

**Defines:**
- `-DNDEBUG` -- standard
- `-D_DEFAULT_SOURCE` -- standard for the dep stack
- `-DPNG_NO_CONSOLE_IO` -- disables the default fprintf-to-stderr
  error / warning handlers; consumer must install their own via
  `png_set_error_fn` if they want diagnostic output

**Optimization:** `-O0 -fno-strict-aliasing` whole-archive default
per crash-patterns #16 ("default to -O0 for new bundled libraries
until proven safe"). Stage 7 perf-optimizer audit will promote
specific hot files to `-O1` after struct-return audit.

**Depends on:** `lib/zlib/` (for IDAT inflate/deflate). Link order
on consumer side: `-lpng -lz` (-lpng FIRST -- libpng calls into zlib).

## CRITICAL config: floating-point disabled

The vendored `src/pnglibconf.h` is derived from upstream's
`scripts/pnglibconf.h.prebuilt` with **two `#define` lines commented
out**:

```c
/*#define PNG_FLOATING_ARITHMETIC_SUPPORTED*/
/*#define PNG_FLOATING_POINT_SUPPORTED*/
```

while `PNG_FIXED_POINT_SUPPORTED` is kept enabled.

**Why:** The floating-point gamma path uses `pow()`, `floor()`,
`frexp()`. With our `-m68881` CPU target, GCC normally inlines FPU
instructions for these -- which works on real Vampire / 68040 / 68060
hardware. But:

- On 68000-only configurations, `pow`/`exp`/`log` would pull libm
  symbols that route through ROM `mathieeesingbas.library`, which
  crashes on FS-UAE (the documented mathieeesingbas crash family,
  crash-patterns #2 variant)
- On FS-UAE 68882 emulation, transcendental FPU instructions
  (FPU `pow` doesn't exist; `exp` + `log` would be needed) trigger
  the documented FS-UAE 68882 transcendental gap

By disabling floating-point at config time, libpng compiles to use
ONLY the fixed-point gamma pipeline (`png_fixed_point` = `int32_t`,
1/100000 units). This eliminates ALL transcendental risk while
preserving the full PNG decode + encode API surface -- consumers
that need gamma correction call `png_set_gAMA_fixed` /
`png_get_gAMA_fixed` with integer values instead of `_set_gAMA` /
`_get_gAMA` with `double` values.

**Verified post-build:** `m68k-amigaos-nm libpng.a | grep
'__divsf3\|__divdf3\|__floatunsisf'` returns empty. `nm | grep ' U
_pow$\|_exp$\|_log$\|_sqrt$\|_floor$'` also returns empty.

## NetSurf compatibility

NetSurf's `frontends/amigaos3-art/loadpng.c` and the cross-platform
`content/handlers/image/png.c` use libpng for `<img src="X.png">`
decoding. Their typical use case is:

1. `png_create_read_struct` + `png_create_info_struct`
2. `png_set_read_fn` with a callback that pulls bytes from
   NetSurf's fetch layer
3. `png_read_info` to get dimensions
4. `png_set_*` transforms (typically `png_set_palette_to_rgb`,
   `png_set_strip_16`, `png_set_gray_to_rgb`, `png_set_filler` for
   alpha)
5. `png_read_image` into a caller-allocated bitmap buffer
6. `png_destroy_read_struct`

**None of these calls require the floating-point API.** sRGB images
without explicit gAMA chunks decode without any gamma math at all.
Images with gAMA chunks get integer-precision gamma correction (1
LSB tolerance vs floating-point), which is imperceptible at typical
8-bit-per-channel display depth.

## Test

```bash
make -C tests/libpng run
```

Runs the 18-test suite via `vamos -C 68040 -s 1024 -m 4096 ./test_libpng`.

The 1x1 RGBA test fixture is generated AT TEST INIT via libpng's
own write side -- we don't ship a hand-crafted PNG byte sequence
because getting CRCs / deflate streams right is fragile. This
also exercises the write path before any read tests run.

Coverage:

- 8 functional (version string; create+destroy read/write structs;
  decode 1x1 RGBA via callback; signature-cmp; full write+read
  round-trip of 4x4 RGBA; inspect IHDR; inspect channel count)
- 3 error path (wrong magic; truncated input -> setjmp recovery;
  zero-length input -> setjmp recovery)
- 3 edge case (rowbytes calculation; set_compression_level accepted;
  PNG_LIBPNG_VER >= 10600 sanity)
- 1 Amiga-specific (decode works without stdio -- callback-only I/O)
- 3 stress (50 create/destroy; 50 decode; 50 setjmp-recovery)

## CRITICAL: vamos resource sizing

Test binaries linking libpng + zlib need `__stack >= 524288` AND
`__MEMORY_STEP >= 524288`. The 256 KB defaults that work for
standalone libs (libnsbmp/libnsgif/libnslog/libnsutils/libnspsl)
fail with no stdout output and exit 20 -- libpng's larger code +
data footprint pushes libnix's startup-time allocation past 256 KB.

For ports/netsurf which links the full dep stack: 1 MB cookies
recommended (same as the libdom-class threshold).

## Memory model + consumer cleanup discipline

- **Every `png_create_read_struct` MUST be paired with `png_destroy_read_struct`**
- **Every `png_create_write_struct` MUST be paired with `png_destroy_write_struct`**
- **Every setjmp recovery path MUST also call destroy** (the destroy is
  re-entrant and safe to call after a longjmp -- libpng's allocator
  tracks all allocations)
- I/O callbacks: caller-owned `io_ptr`. libpng never frees it.
- Decoded pixel buffers: caller-allocated, libpng never frees them.

On AmigaOS `-noixemul`: leaks are permanent until reboot. NetSurf's
PNG decode loop must guarantee destroy on every exit path including
out-of-memory and decode-error.

## Memory audit findings

See `lib/libpng/MEMORY-AUDIT.md` (Stage 6 report).

## Performance audit findings

See `lib/libpng/PERF-REPORT.md` (Stage 7 report).

## Test ASSERT-failure leak caveat

Same as the prior dep-stack libs: ASSERT_* macros return early from
a failing test without running cleanup. Acceptable for unit-test
purposes (vamos host process exit reclaims memory) but not
representative of a real-world consumer leak.

## Consumers

- `ports/netsurf/` (Phase 1 final consumer) -- inline `<img>` PNG
  decoding, screenshot save (write side)
- (potentially) future image-viewer ports
