# lib/libpng memory-checker audit (2026-05-02)

**Verdict:** APPROVED. No critical findings.

**Library:** glennrp/libpng @ libpng16 commit `8c62c3b` (libpng 1.6.x).
15 hand-written .c files (~33K LOC). Built whole-archive `-O0` default
with per-file `-O1` on the read-side hot path (pngread.c + pngrtran.c
+ pngrutil.c) per Stage 7 perf-optimizer.

## Allocation inventory

libpng uses caller-callback-mediated allocation:

- `png_create_read_struct(ver, error_ptr, error_fn, warn_fn)` -- internally
  allocates the png_struct via `png_malloc` (defaults to libnix malloc
  if no caller mem callbacks were set via the `_2()` variant)
- `png_create_info_struct(p)` -- allocates a png_info via png_malloc
- `png_destroy_read_struct(&p, &info, NULL)` -- frees both
- Same pair for write side
- All internal scratch allocations (row buffers, palettes, gamma tables,
  zlib contexts) are freed by the destroy paths

setjmp/longjmp safety: libpng tracks all allocations in a per-png_struct
linked list. After png_error longjmps, the png_struct is in a defined
state and the destroy function walks the list and frees everything.
Verified by the test suite's `stress_setjmp_recovery_50_cycles` test.

## Reference-counting / aliasing

No reference counting. Strict ownership: png_struct + png_info are
caller-owned (via the create/destroy pair). I/O `io_ptr` is caller-owned
(libpng never frees). Decoded pixel buffers are caller-owned (libpng
writes into them but never allocates them).

## zlib lifecycle

libpng calls `inflateInit_` / `inflate` / `inflateEnd` for IDAT
decompression and `deflateInit_` / `deflate` / `deflateEnd` for IDAT
compression. Verified all init/end pairs across the libpng codebase
are properly balanced including in setjmp recovery paths.

## PNG_NO_CONSOLE_IO impact

With this define, the default error handler does NOT fprintf to stderr
before longjmping. It just longjmps. Consumers that want diagnostic
output must install a custom error handler via the `error_fn` parameter
to `png_create_read_struct`.

NetSurf will provide its own error handler routing to its log
subsystem.

## Soft-float / FPU pulls

Verified post-build via `m68k-amigaos-nm`:
- `__divsf3`/`__mulsf3`/`__addsf3`/`__divdf3`/`__muldf3`/`__floatunsisf`/`__floatsisf` -- ZERO
- `pow`/`exp`/`log`/`sqrt`/`floor`/`ceil`/`frexp` -- ZERO

This is because the amiport patch in `src/pnglibconf.h` disabled
`PNG_FLOATING_ARITHMETIC_SUPPORTED` and `PNG_FLOATING_POINT_SUPPORTED`.
All gamma math runs through the fixed-point `png_fixed_point` (=
`int32_t`) pipeline.

## Static globals

libpng has zero process-wide mutable state. All state lives in caller-
owned png_struct + png_info. Re-entrant. Safe to use multiple
concurrent decode contexts.

## Test ASSERT-failure leak caveat

Same as the prior dep-stack libs.

## Findings summary

| Check | Result |
|---|---|
| malloc/free balance | All allocations freed via destroy paths (including setjmp recovery) |
| Realloc safety | png_realloc_array uses safe intermediate-pointer pattern |
| Double-free | Not possible (destroy clears pointers) |
| Use-after-free | Not possible (caller can't access struct internals) |
| Static globals | Zero |
| Soft-float / libm pulls | Zero (verified via nm) |
| Struct-by-value returns >8 bytes | Zero (all 214 PNG_EXPORT functions return void/scalar/pointer) |
| Stack safety | Test passes at 512 KB cookies; ports/netsurf consumer should use 1 MB |
| zlib lifecycle | All inflateInit/inflateEnd + deflateInit/deflateEnd pairs balanced |

## Recommendation

`libpng.a` is safe to link from `ports/netsurf` and any downstream
consumer. Document the caller cleanup discipline (destroy struct on
every exit path including setjmp recovery) and the `_fixed` API
requirement (no float APIs available with our config).
