# lib/libnsgif

NetSurf progressive animated GIF image decoder.

Upstream: https://github.com/netsurf-browser/libnsgif @ commit `5d5d750`
(v1.0.x). Copyright 2003-2023 NetSurf Browser Project, MIT-licensed
(see `COPYING`).

## What it is

LibNSGIF is a 2-translation-unit (gif.c + lzw.c, 2689 LOC total)
animated GIF decoder. It uses caller-supplied bitmap-allocation
callbacks, supports the full GIF87a + GIF89a spec, animation looping,
8 different output pixel formats (R8G8B8A8, B8G8R8A8, etc.), and
progressive (chunked) data feeds suitable for network fetches.

It is the seventh library shipped in the NetSurf Vampire Phase 1 dep
stack (Phase D-prime). Standalone -- no NetSurf-internal dependencies.

## Public API

See `include/nsgif.h`. Core entry points:

- `nsgif_create(callbacks, format, &gif)` -- create a decoder instance
- `nsgif_data_scan(gif, size, data)` -- feed raw GIF bytes (callable
  multiple times with growing `size` for progressive decode)
- `nsgif_data_complete(gif)` -- mark data feed as final
- `nsgif_frame_prepare(gif, &area, &delay_cs, &frame_new)` -- query
  next frame to display + animation timing
- `nsgif_frame_decode(gif, frame_num, &bitmap)` -- decode the frame's
  pixels into the caller's bitmap
- `nsgif_reset(gif)` -- rewind the animation
- `nsgif_destroy(gif)` -- free decoder state
- `nsgif_strerror(err)` -- human-readable error string

The bitmap callback vtable (`nsgif_bitmap_cb_vt`) requires:
- `create(width, height)` -> `nsgif_bitmap_t *`
- `destroy(bitmap)` -> void
- `get_buffer(bitmap)` -> `uint8_t *` (32-bit pixel buffer)
- Optional: `set_opaque`, `test_opaque`, `modified`, `get_rowspan`

## Build

```bash
make -C lib/libnsgif
```

Produces `libnsgif.a` (~10 KB).

**CPU target:** `-m68040 -m68881`. Same NetSurf-Vampire dep stack
convention.

**Defines:** `-DNDEBUG -std=c99`.

**Optimization:** whole-archive `-O1 -fno-strict-aliasing` after audit
2026-05-02. Both TUs are -O1-safe per crash-patterns #16: only one
struct-by-value return (`nsgif_colour_layout`, 4 bytes -- well under
the 8-byte threshold), zero soft-float pulls (verified via
`m68k-amigaos-nm`), no large stack arrays, no 64-bit math in the hot
path. See `lib/libnsgif/PERF-REPORT.md`.

**Depends on:** nothing (only libc/libnix). Standalone.

## Test

```bash
make -C tests/libnsgif run
```

Runs the 16-test suite via `vamos -C 68040 -s 1024 -m 4096 ./test_libnsgif`.
Coverage:

- 5 functional (create+destroy, data_scan, frame_decode, strerror,
  frame_prepare)
- 4 error path (bad magic, truncated, bad frame number, scan after
  complete)
- 3 edge case (chunked scan, reset replays, all 8 pixel format
  variants)
- 2 Amiga-specific (little-endian LSD parse on big-endian 68k, no
  soft-float during 50 LZW decodes)
- 2 stress (50-iter create+destroy, 5 parallel instances)

Tests use a synthetic 1x1 GIF87a fixture committed in the test source
(no external test data files).

## Memory audit findings

Memory-checker APPROVED with no findings. All 12 malloc/free pairs are
properly balanced; both realloc patterns use the safe intermediate-
pointer idiom (the libdom-style realloc bug is NOT present); bitmap
callbacks are correctly paired at create/destroy sites; LZW context has
clean lifecycle; frame array growth correctly initialises new entries;
all error paths flow through `nsgif_destroy()` cleanup.

See `lib/libnsgif/MEMORY-AUDIT.md` for the full report.

## AmigaOS exit cleanup

libnsgif does not maintain process-wide globals. All state lives in
caller-owned `nsgif_t` instances. The downstream consumer MUST call
`nsgif_destroy(gif)` for every successfully-created decoder at exit
(or earlier when done).

AmigaOS `-noixemul` does NOT reclaim process memory on exit.

## Test ASSERT-failure leak caveat

Same as the prior dep-stack libs: the test framework's `ASSERT_*` macros
return early from a failing test without running cleanup. Acceptable for
unit-test purposes (vamos host process exit reclaims memory) but not
representative of a real-world consumer leak.

## Consumers

- `ports/netsurf/` (Phase 1 final consumer) -- decodes inline `<img
  src="X.gif">` resources, including animated banners (NetSurf throttles
  animation playback per its own scheduling).
