# SAGA Chunky Video Mode Reference

**Source:** Apollo Team SAGA documentation (canonical text supplied by user 2026-05-02 from primary Apollo source). The original wiki at `wiki.apollo-accelerators.com/doku.php/saga:video` has been replaced by a generic gaming blog (`apollogames.info`) and the technical content is no longer reachable from the public web. This document captures the authoritative register reference for SAGA's chunky-pixel display modes.

**Status:** Authoritative for Apollo Core (Vampire V2 / V4 / A6000) chunky-pixel framebuffer programming.

## Overview

SAGA can display a **chunky plane** alongside the classic Amiga planar bitplanes (up to 8 planar planes), or by itself. Chunky modes are the modern path: pixels are stored linearly, one byte/word/long per pixel — no chunky-to-planar conversion required.

For amiport ports targeting Vampire (NetSurf, future PDF viewer, SDL games), the chunky framebuffer is the modern rendering target. Picasso96 wraps these registers; direct hardware access is faster but bypasses the OS abstraction.

## Chunky display registers

| Address | Width | R/W | Name | Default | Description |
|---------|-------|-----|------|---------|-------------|
| `$DFF1EC` | 32-bit | W | Chunky-Data-Address-Register | `$0FB00000` | Plane pointer address to display |
| `$DFF1F0` | 16-bit | W | Chunky-Width-Register | 640 | Pixels per row |
| `$DFF1F2` | 16-bit | W | Chunky-Height-Register | 480 | Number of rows |
| `$DFF1F4` | 16-bit | W | Chunky-GFXMode-Register | 0 | Mode + scanline doubling |
| `$DFF400` + 4*N | 32-bit | W | Colour Register N | — | 256 entries, format `(- 8 | R 8 | G 8 | B 8)` |

**Key property:** the framebuffer can be at any address in onboard Fast RAM. No Chip RAM requirement. The framebuffer is readable AND writable directly — fast direct-hardware drawing is supported.

## GFXMode-Register layout (`$DFF1F4`)

### Low byte — pixel format

| Value | Format |
|-------|--------|
| `$00` | Chunky-DMA OFF (no chunky display) |
| `$01` | 8-bit CLUT (palette index, 256 colors via `$DFF400+`) |
| `$02` | 16-bit `R5 G6 B5` (5/6/5 RGB565) |
| `$03` | 15-bit `- R5 G5 B5` (5/5/5 RGB555 with high bit unused) |
| `$04` | 24-bit `R8 G8 B8` (3 bytes per pixel) |
| `$05` | 32-bit `- 8 R8 G8 B8` (4 bytes per pixel, top byte unused) |
| `$06` | 16-bit YUV422: `Y8 U8 Y8 V8` |

### High byte — scanline doubling

| Value | Effect |
|-------|--------|
| `$00` | Normal |
| `$01` | Double output each X-pixel (X-doublescan) |
| `$02` | Double output each row (Y-doublescan) |
| `$03` | X + Y doublescan |

## Palette format (`$DFF400`)

256 entries of 32 bits each, write-only:

```
[ unused 8 | R 8 | G 8 | B 8 ]
```

Used only when the chunky mode is set to CLUT8 (`$01`). For direct-color modes (15/16/24/32), the palette is unused.

## How to enable a chunky mode programmatically

```c
/* Example: set up 800x600 ARGB32 framebuffer in Fast RAM */
volatile UWORD *gfxmode  = (UWORD *)0xDFF1F4;
volatile UWORD *width    = (UWORD *)0xDFF1F0;
volatile UWORD *height   = (UWORD *)0xDFF1F2;
volatile ULONG *plane    = (ULONG *)0xDFF1EC;

ULONG *fb = AllocMem(800 * 600 * 4, MEMF_FAST);
*plane = (ULONG)fb;
*width = 800;
*height = 600;
*gfxmode = 0x0005;   /* low byte 5 = 32bit ARGB, high byte 0 = no doubling */

/* Now write ARGB pixels directly: */
fb[y * 800 + x] = 0x00FF8800;   /* unused, R, G, B */
```

To disable chunky display: write `$00` to the low byte of `$DFF1F4`.

## Direct-hardware vs Picasso96 trade-off

For amiport ports:

- **Picasso96 path** (recommended for compatibility): use `IGraphics->LockBitMap`, `WritePixelArray`, etc. Works on any RTG-equipped Amiga, not just Vampire. Some overhead per call.
- **Direct-hardware path** (Vampire-only): write `$DFF1EC` to the framebuffer pointer once, then write pixels directly. Bypasses Picasso96 entirely. Faster for tight inner loops, but Vampire-only and breaks if other software is also using the chunky display.

For NetSurf Phase 1 (text rendering), the Picasso96 path is the right default — NetSurf's plotter already calls `LockBitMapTags` / `WritePixelArray`, so we get chunky framebuffer access "for free" through the OS abstraction. Direct-hardware access would only matter if perf measurement shows Picasso96 overhead is the bottleneck.

For SDL games (Julius, OpenTTD on Vampire), the libSDL2-amigaos3 fast path already writes directly to the chunky framebuffer (see existing AMMX section of `known-pitfalls.md`).

## Implications for NetSurf font compositor (Phase 1)

The font compositor's output is ARGB32 pixels. The natural target is mode `$05` (32-bit `- R G B`). PMULA's output format (`0 R G B` per byte) maps directly — no conversion needed.

If the user runs NetSurf at 16-bit color depth (mode `$02` RGB565), we'd need PACK3216 (per AMMX reference) to convert ARGB → RGB565. The plan doesn't pre-commit; the implementation can detect the framebuffer format at glyph-render time.

## Picture-in-Picture (PIP) and other planes

The SAGA chunky plane is one of multiple display planes. It can coexist with:
- Up to 8 classic planar bitplanes (AGA-compatible games)
- PiP (picture-in-picture) overlay (separate palette D, see `docs/references/saga/sprite-hardware.md`)
- 16 SAGA sprites (see same doc)

Layering / priority is configurable but outside this document's scope.

## Other SAGA documentation gaps

The Apollo Core SAGA Technical Reference Manual was originally a multi-chapter document. We have:
- ✅ Chapter 1 — Sprite Hardware (`docs/references/saga/sprite-hardware.md`)
- ✅ Chunky video registers (this document)

Still missing:
- ❌ SAGA audio (Pamela 16-bit DACs, multi-channel audio mixer)
- ❌ Maggie 3D (texture/Z-buffer unit, hardware 3D acceleration)
- ❌ SAGA blitter enhancements (if any beyond AGA)
- ❌ Universal ModeLine Calculator (UMC) notes
- ❌ Video mode generation (custom resolutions)

Apollo Core wiki content has migrated/been removed; primary sources for these chapters would need to come from the Apollo Team Discord or community archives.

## Cross-references

- `docs/references/saga/sprite-hardware.md` — sister doc, sprite registers
- `docs/references/ammx/instruction-reference.md` — PACK3216 / UNPACK1632 for ARGB ↔ RGB565
- `docs/references/vampire-sdk/headers.md` — vampire.resource init (separate from SAGA)
- `docs/references/netsurf-mui/ammx-pattern.md` — vasm + V_EnableAMMX wiring

## Discovery context

User-provided text 2026-05-02. Wiki link `wiki.apollo-accelerators.com/doku.php/saga:video` no longer reachable as the domain has been repurposed; this document captures the canonical content for amiport's offline reference.
