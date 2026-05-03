# lib/libjpeg perf-optimizer audit (2026-05-02)

**Verdict:** Per-file -O1 promotion applied to 5 hot-path files.
Default -O0 for the remaining 39 TUs.

**Library:** IJG libjpeg 9f. 44 hand-written .c files. Built `-O0
-fno-strict-aliasing -m68040 -m68881 -DNDEBUG -std=c99` default with
**per-file `-O1` on hot path**: jdhuff.c + jidctint.c + jidctfst.c +
jdcolor.c + jdsample.c.

## crash-patterns #16 audit (44 TUs)

CLEAN across all 44 source files. Public API (jpeglib.h exports)
returns void / int / boolean / JDIMENSION / pointer types. The one
struct-typed return is `jpeg_std_error` which returns `struct
jpeg_error_mgr *` (pointer, not by value).

Internal functions also pointer-based -- the libjpeg API discipline
is `j_compress_ptr` / `j_decompress_ptr` everywhere. Zero struct
copies.

Hot-path 5 files explicitly audited: zero struct returns, zero
inline asm, conservative local sizes (largest is `int workspace[8*16]`
in jidctint.c = 512 bytes).

## Soft-float / libm pulls

Verified via `m68k-amigaos-nm libjpeg.a`:
- `__divsf3` / `__divdf3` / `__floatunsisf` -- ZERO
- `pow` / `exp` / `log` / `sqrt` / `floor` -- ZERO

DCT_FLOAT_SUPPORTED is disabled at compile time (src/jmorecfg.h
patch). All DCT/IDCT math uses fixed-point integer arithmetic.

## Hot path / per-call cost (decoding 256x256 JPEG)

Estimated cycles on 25 MHz 68040:

| Stage | Cost % | Location | Status |
|---|---|---|---|
| Huffman entropy decode | ~50% | jdhuff.c | -O1 promoted |
| Inverse DCT (ISLOW) | ~25% | jidctint.c | -O1 promoted |
| YCbCr -> RGB convert | ~10% | jdcolor.c | -O1 promoted |
| Chroma upsample | ~5% | jdsample.c | -O1 promoted |
| Header parse + misc | ~10% | jdmarker.c, jdinput.c, etc. | -O0 (rare) |

Wall time estimates:
- 256x256 JPEG @ 25 MHz 68040 + JDCT_ISLOW: **~280 ms (-O0) → ~175 ms (-O1)** = 1.6x speedup
- 256x256 JPEG @ 7 MHz 68000: ~1.0s -> ~630 ms

## Per-file -O1 promotion applied

5 files, 67 KB total at -O0:

| File | Bytes -O0 | Notes |
|---|---|---|
| jdhuff.c | 10,784 | Huffman entropy decoder; PRIMARY bottleneck |
| jidctint.c | 48,496 | Accurate integer IDCT (JDCT_ISLOW); LARGEST file |
| jidctfst.c | 2,000 | Fast integer IDCT (JDCT_IFAST) |
| jdcolor.c | 4,728 | YCbCr->RGB color conversion |
| jdsample.c | 1,728 | Chroma upsample (4:2:0) |

All 5 audited as -O1 safe (zero struct returns >8 bytes, zero soft-
float, zero 64-bit math, zero inline asm, conservative local sizes).

## Stack pressure

Test binary uses `__stack = 524288` (512 KB). Largest hot-path local
is 512 bytes (`int workspace[8*16]` in jidctint.c). Combined with
AmigaOS hidden ~4 KB DOS depth, ~5 KB peak. 512 KB cookies have
100x headroom. No change needed.

For ports/netsurf consumer linking the FULL dep stack: 1 MB cookies
recommended (libdom-class threshold).

## Code size

Before -O1 promotion (all -O0): 272 KB.
After -O1 promotion (5 hot files): **242 KB** (-11%).

The savings come from constant hoisting + register allocation in the
DCT and Huffman files. The other 39 TUs stay at -O0 (small or rare).

## Recommendation

Current Makefile is now optimal:

```
CFLAGS  = -O0 -fno-strict-aliasing -noixemul -m68040 -m68881 -std=c99
CFLAGS += -DNDEBUG -Isrc -Iinclude

HOTPATH_CFLAGS = -O1 -fno-strict-aliasing -noixemul -m68040 -m68881 -std=c99
HOTPATH_CFLAGS += -DNDEBUG -Isrc -Iinclude
```

with per-file `HOTPATH_CFLAGS` rules for the 5 hot files. 18/18 tests
still pass post-promotion. No further changes recommended.
