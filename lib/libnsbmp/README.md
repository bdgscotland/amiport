# lib/libnsbmp

NetSurf BMP / ICO image decoder.

Upstream: https://github.com/netsurf-browser/libnsbmp @ commit `ea063c9`
(v0.1.7). Copyright 2006 Richard Wilson + 2008 Sean Fox, MIT-licensed
(see `COPYING`).

## What it is

LibNSBMP is a small (1388 LOC), single-translation-unit BMP and ICO
image decoder. It uses caller-supplied bitmap-allocation callbacks to
decouple from any specific platform bitmap format, supports the full
range of BMP encodings (RGB 1/4/8/16/24/32 bpp + RLE4 + RLE8 + bitfield),
top-down and bottom-up scan orders, and ICO collections with mask alpha.

It is the sixth library shipped in the NetSurf Vampire Phase 1 dep
stack (Phase D-prime). Standalone -- no NetSurf-internal dependencies.

## Public API

See `include/libnsbmp.h`. Core entry points:

- `bmp_create(bmp, callbacks)` -- initialise a `bmp_image` struct
- `bmp_analyse(bmp, size, data)` -- parse headers, set `width`/`height`
- `bmp_decode(bmp)` -- decode pixels into the caller's bitmap buffer
- `bmp_decode_trans(bmp, transparent_colour)` -- decode with the BMP
  "limited transparency" trick
- `bmp_finalise(bmp)` -- free internal state (palette, etc.)
- `ico_collection_create` / `ico_analyse` / `ico_finalise` -- ICO variants

The bitmap callback vtable (`bmp_bitmap_callback_vt`) requires:
- `bitmap_create(width, height, state)` -> `void *`
- `bitmap_destroy(void *)` -> void
- `bitmap_get_buffer(void *)` -> `unsigned char *` (32-bit RGBA pixel buffer)

## Build

```bash
make -C lib/libnsbmp
```

Produces `libnsbmp.a` (~7 KB).

**CPU target:** `-m68040 -m68881`. Same NetSurf-Vampire dep stack
convention as the prior libs.

**Defines:** `-DNDEBUG -std=c99`.

**Optimization:** whole-archive `-O1 -fno-strict-aliasing` after audit
2026-05-02. Single TU is trivially -O1-safe per crash-patterns #16:
zero struct returns >8 bytes (every API returns enum or pointer), zero
soft-float pulls (verified via `m68k-amigaos-nm`), no large stack
arrays. `-fno-strict-aliasing` is required because the decoder uses
type-punning for endian-swapped pixel reads. See
`lib/libnsbmp/PERF-REPORT.md`.

**Depends on:** nothing (only libc/libnix). Standalone.

## Test

```bash
make -C tests/libnsbmp run
```

Runs the 18-test suite via `vamos -C 68040 -s 1024 -m 4096 ./test_libnsbmp`.
Coverage:

- 5 functional (bmp_create + finalise, analyse, decode, ICO create,
  decode_trans)
- 4 error path (bad magic, truncated, empty, decode without analyse)
- 3 edge case (1x1 BMP, top-down orientation, oversized dimensions
  rejected)
- 3 Amiga-specific (callback dispatch alignment, little-endian header
  parse on big-endian 68k, no soft-float during 50 decode cycles)
- 3 stress (50-iter create+destroy, 5 parallel BMP instances, 20-iter
  reuse)

Tests use a synthetic 2x2 24-bit BMP fixture committed in the test
source (no external test data files).

## Memory audit findings

Memory-checker APPROVED with **one CAVEAT**:

**ICO multi-image partial-allocation leak**: `ico_header_parse` walks
the linked list of ICO sub-images and allocates each `ico_image` node.
If `bmp_info_header_parse` fails partway through the loop (e.g., on a
malformed sub-image), the function returns immediately without freeing
the partially-allocated `ico_image *first` chain. On AmigaOS with
`-noixemul`, the leaked nodes are permanent until reboot.

**Practical risk: low**. ICOs typically have 1-6 sub-images;
`bmp_info_header_parse` only fails on truly malformed data; malloc
rarely fails for the small (~64-byte) allocations involved. NetSurf's
real-world risk is bounded by malformed favicons -- a few KB leak
worst-case per malformed file.

**Workaround for paranoid consumers**: validate the ICO header byte
range before calling `ico_analyse`. If you must accept untrusted
ICOs, periodically destroy and recreate the parser process.

**Long-term fix**: upstream patch to add cleanup-on-error in
`ico_header_parse`. Tracked for future contribution upstream.

See `lib/libnsbmp/MEMORY-AUDIT.md` for the full report.

## AmigaOS exit cleanup

libnsbmp does not maintain process-wide globals. All state lives in
caller-owned `bmp_image` / `ico_collection` instances. The downstream
consumer MUST call `bmp_finalise(bmp)` for every successfully-created
BMP and `ico_finalise(ico)` for every ICO at exit (or earlier when
done).

AmigaOS `-noixemul` does NOT reclaim process memory on exit.

## Test ASSERT-failure leak caveat

Same as the prior dep-stack libs: the test framework's `ASSERT_*` macros
return early from a failing test without running cleanup. Acceptable for
unit-test purposes (vamos host process exit reclaims memory) but not
representative of a real-world consumer leak.

## Consumers

- `ports/netsurf/` (Phase 1 final consumer) -- decodes inline `<img
  src="X.bmp">` and `<link rel="icon" href="X.ico">` resources.
