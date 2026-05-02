# NetSurf 68k Phase 1 — FreeType Font Path + AMMX Glyph Compositor

**Status:** Design — awaiting implementation plan
**Date:** 2026-05-02
**Phase:** 1 of 3 (subsequent phases: AMMX PNG decode, Duktape JavaScript)

## Summary

Ship NetSurf 3.11 on Apollo A6000 (Vampire V4) with anti-aliased text rendering at higher visual quality and a target ~90× faster text throughput than the current 68k build. As a side-effect, land reusable amiport infrastructure — an AMMX abstraction layer with runtime CPU dispatch, a generic glyph cache library, and a Vampire-native build target — that future ports can adopt without each one re-inventing the same primitives.

Deliverable: a `ports/netsurf/` amiport port plus a planned upstream PR after the FreeType+AMMX path has proven itself over weeks of real use on V4 hardware.

## Problem

NetSurf 68k as it ships today (arczi84/NetSurf-68k fork and mainline) has two compounding problems on AmigaOS 3 / Vampire:

1. **No anti-aliasing.** `frontends/amiga/font_bullet.c` lines 559-569 request `OT_GlyphMap8Bit` + `BLITT_ALPHATEMPLATE` only inside `#ifdef __amigaos4__`. On AmigaOS 3 the code unconditionally requests `OT_GlyphMap` (1-bit bitmap glyph) and ignores the caller's `aa` flag. Every NetSurf user on Vampire is staring at jagged Topaz-quality glyphs on every page. Press claims about "AA on classic Amiga via freetype2.library" conflate system-wide TypeManager rendering with NetSurf's content area — they are not the same path.

2. **The glyph hot path is pathological.** Lines 602-614: per glyph, `AllocVec(MEMF_CHIP) + CopyMem + BltTemplate + FreeVec`. Chip-RAM bounce per character, no cache, no SIMD. There is a `font_cache.c` in the same frontend but it caches `struct OutlineFont *` (font handles), not glyph bitmaps.

The current Vampire user experience on text-heavy pages is dominated by these two costs. Text is what users look at on every page; fixing it is the highest-leverage user-visible improvement available.

## Goal

Replace the broken `font_bullet.c` AmigaOS 3 path with a parallel `font_freetype.c` peer that:

- Renders 8-bit alpha glyphs via amiport's existing `lib/freetype/` (integer-only fixed-point, no soft-float crash exposure)
- Caches glyph bitmaps in a new generic `lib/glyph-cache/` (LRU, slab-allocated arena)
- Composites via a new `amiport_ammx_t` function-pointer interface that dispatches to either Apollo AMMX2 instructions on V4 or a portable scalar fallback on V2 / stock 68k

## Success criteria

1. **Visual:** NetSurf renders the Aminet front page, NetSurf's own homepage, and `en.wikipedia.org`'s main article body with visibly anti-aliased glyphs on A6000. No jagged edges in body text.
2. **Performance:** Text-render frame time drops by ≥4× vs current `font_bullet.c` path on A6000, measured with `ReadEClock` probes around `ami_font_*_text` entry points. Cycle-budget analysis projects ~90× on a typical line of 80 chars at 16 px; ≥4× is a conservative ship gate.
3. **Compatibility:** Binary runs unchanged on stock 68040/60 (no Apollo) and on Vampire V2, falling back to the scalar glyph compositor with no regressions or crashes.
4. **CI:** FS-UAE regression suite passes — rendering correctness on five reference HTML fixtures, no-crash on a 50-page browsing scenario, perf canary within 20% of baseline.
5. **Reuse:** `lib/posix-shim/include/amiport/ammx.h` is consumable by at least one other amiport port (the existing Julius/SDL game ports get `amiport_ammx->blend_alpha8` available even if they don't switch yet). `lib/glyph-cache/` is consumable by future text-rendering ports (mg, less, PDF viewer, terminal).

## Non-goals

- JavaScript engine integration (Phase 3 — separate brainstorm)
- AMMX-accelerated PNG / JPEG decode (Phase 2 — separate brainstorm)
- HTTP/2, Brotli, WebP, AVIF (deferred indefinitely — wrong audience for the cost)
- Upstream PR submission (deferred until the FreeType+AMMX path has proven itself on V4 over weeks of real use)
- AmigaOS 4 path changes — `#ifdef __amigaos4__` branches preserved as-is

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│ ports/netsurf/                  (the application port)          │
│   ├── original/                 (mainline NetSurf source, RO)   │
│   ├── ported/                                                   │
│   │     └── frontends/amiga/                                    │
│   │           ├── font_freetype.c     NEW — FreeType backend    │
│   │           ├── font.c              PATCH — add 3rd dispatch  │
│   │           └── gui_options.c       PATCH — new nsoption      │
│   ├── Makefile                                                  │
│   ├── PORT.md / .readme / test-fsemu-cases.txt                  │
│   └── netsurf-3.11-1.lha                                        │
└─────────────────────────────────────────────────────────────────┘
                  │ links against ▼
                  ▼
┌─────────────────────────────────────────────────────────────────┐
│ amiport reusable infrastructure                                 │
│                                                                 │
│  lib/freetype/                       EXISTING (486 KB)          │
│    └── libfreetype.a                                            │
│                                                                 │
│  lib/glyph-cache/                    NEW                        │
│    ├── include/amiport/glyph_cache.h                            │
│    ├── src/glyph_cache.c             LRU + slab arena           │
│    └── libglyphcache.a               ~8 KB                      │
│                                                                 │
│  lib/posix-shim/include/amiport/                                │
│    └── ammx.h                        NEW — function-table API   │
│                                                                 │
│  lib/posix-shim/src/                                            │
│    ├── ammx_init.c                   NEW — AttnFlags + V_Enable │
│    ├── ammx_scalar.c                 NEW — portable C fallback  │
│    └── ammx_v4.s                     NEW — vasm AMMX2 asm       │
│                                                                 │
│  toolchain/scripts/                                             │
│    └── m68k-amigaos-gcc-v4           NEW — wrapper, -m68040 +881│
│                                                                 │
│  tests/glyph-cache/                  NEW                        │
│  tests/ammx/                         NEW                        │
└─────────────────────────────────────────────────────────────────┘
```

### Component contracts

**`lib/posix-shim/include/amiport/ammx.h`** — public API:

```c
typedef struct amiport_ammx {
    void (*blend_alpha8)(uint8_t *dst, const uint8_t *src,
                         const uint8_t *alpha, size_t count);
    void (*compose_glyph_argb32)(uint32_t *dst, ptrdiff_t dst_stride,
                                 const uint8_t *alpha, ptrdiff_t alpha_stride,
                                 int w, int h, uint32_t color);
    bool (*has_ammx2)(void);
} amiport_ammx_t;

extern amiport_ammx_t *amiport_ammx;       /* populated at init */
void amiport_ammx_init(void);              /* call once at startup */
```

`amiport_ammx_init()` runs once at process startup, detects Apollo via `ExecBase->AttnFlags & (1 << AFB_68080)` plus `vampire.resource` `V_EnableAMMX(V_AMMX_V2)` to unlock AMMX2 context-switch handling. Populates the global function table with either the AMMX2 backend (Apollo + AMMX2 unlocked) or the scalar backend (everything else). Branch-free in the hot loop — callers indirect-call through the table without checking which backend is active.

Two backends:
- `ammx_v4.s` — vasm-built (`vasmm68k_mot -m68080 -Fhunk`), uses Apollo AMMX2 instructions: `PCMP` (per-byte compare), `STOREm3 #1` (fused compare+masked store, write-without-read), `PMULA` (per-byte alpha multiply), `PACK3216` (ARGB→RGB565 packing), `VPERM` (byte permute)
- `ammx_scalar.c` — portable C fallback with the same per-pixel arithmetic

**`lib/glyph-cache/`** — generic LRU cache:

- Key: `(face_id, codepoint, px_size, hint_flags)` packed into a 64-bit int
- Value: `(alpha_bitmap_offset_in_arena, advance_x_q16, bearing_x, bearing_y, w, h)`
- Storage: pre-sized slab arena (caller specifies cap, e.g., 512 KB ≈ 800-1500 glyphs depending on size). Bitmaps allocated bump-pointer style with LRU eviction reclaiming the oldest entries when full.
- Hash: open-addressing linear probe, sized to ~2× expected entry count
- Eviction: LRU via doubly-linked list overlaid on slab entries
- Thread-safety: none (AmigaOS is single-threaded; this is fine)
- Reusable by any text-rendering port — `mg`, `less`, future PDF viewer, terminal emulator

**`ports/netsurf/ported/frontends/amiga/font_freetype.c`** — implements NetSurf's font_funcs interface:

- `init()` — open `lib/freetype/`, register a fontconfig-lite path resolver pointing to `FONTS:Truetype/`, allocate glyph cache (default 512 KB cap, override via env var)
- `width()` — measure string width using FT_Glyph_Get_CBox sums (cache-aware: uses cached metrics when present)
- `position_in_string()` / `split()` — text break iteration, walks codepoints accumulating advances
- `text()` — the hot path: per glyph, glyph_cache_lookup → on miss `FT_Load_Char(face, cp, FT_LOAD_RENDER | FT_LOAD_TARGET_NORMAL)` → cache insert → `amiport_ammx->compose_glyph_argb32()` onto the locked RastPort BitMap

**`ports/netsurf/ported/frontends/amiga/font.c`** — patch only. Adds a third branch to the existing `nsoption_bool(bitmap_fonts)` dispatch: when `nsoption_bool(freetype_fonts)` is true, route to `ami_font_freetype_init()`. Default true on AmigaOS 3, false on AmigaOS 4 (preserves existing OS4 behavior).

**`toolchain/scripts/m68k-amigaos-gcc-v4`** — compiler wrapper that pins `-m68040 -m68881`. Bebbo-gcc does not natively emit AMMX from C; AMMX comes from the asm files. The 040+881 baseline lets us emit hardware FPU instructions where useful. AMMX init checks `AttnFlags` at runtime; the binary runs on stock 68040/60 (scalar path) and on V2/V4 (V4 takes the AMMX2 path).

## Data flow (text-render hot path)

```
ami_plot_text(rp, x, y, "Hello", style)
   │
   ├─→ ami_font_freetype_text(rp, x, y, "Hello", style)
   │     │
   │     ├─→ resolve face: hash(family, weight, italic) → FT_Face*
   │     │     ├─ HIT in face cache: return cached
   │     │     └─ MISS: FT_New_Face from FONTS:Truetype/<name>.ttf,
   │     │             FT_Set_Pixel_Sizes, insert in face cache
   │     │
   │     └─→ for each codepoint in "Hello":
   │           │
   │           ├─→ key = (face_id, codepoint, px_size, hint_flags)
   │           ├─→ glyph_cache_lookup(key)
   │           │     ├─ HIT (~95% expected): return cached bitmap+metrics
   │           │     │     cost: ~50 cycles (hash + ptr chase)
   │           │     │
   │           │     └─ MISS (~5%):
   │           │           FT_Load_Char(face, cp, FT_LOAD_RENDER | FT_LOAD_TARGET_NORMAL)
   │           │           → 8-bit alpha bitmap in face->glyph->bitmap
   │           │           glyph_cache_insert(key, bitmap, advance, bearings)
   │           │           cost: ~50,000 cycles (FreeType rasterize)
   │           │
   │           └─→ amiport_ammx->compose_glyph_argb32(
   │                   fb + (x+bearing_x)*4 + (y+bearing_y)*pitch, pitch,
   │                   alpha_bitmap, alpha_stride, w, h, color_argb)
   │                 │
   │                 ├─ AMMX2 backend (V4/A6000): ~1 cycle/pixel
   │                 │    inner loop, 8 pixels per iteration:
   │                 │       PCMP src vs 0 → mask
   │                 │       PMULA dst, alpha → blended bytes
   │                 │       STOREm3 #1 → write only non-zero
   │                 │
   │                 └─ Scalar backend (V2 / stock 68k): ~6 cycles/pixel
   │                      per-pixel: dst = (src*a + dst*(255-a) + 128) >> 8
   │
   └─ x += advance
```

**Per-line cycle budget (80 chars at 16 px, A6000 ~133 MIPS):**
- Cache lookups: 80 × 50 + 4 misses × 50K = ~204K cycles
- AMMX composite: 80 × ~10×16 px × 1 cycle = ~12.8K cycles
- **Total: ~217K cycles ≈ 1.6 ms per line**
- Today's path: 80 × `AllocVec+CopyMem+BltTemplate+FreeVec` ≈ 80 × ~250K = ~20M cycles ≈ **150 ms per line**
- **Projected improvement: ~90×** (dominated by chip-RAM round-trip elimination + cache hits + AMMX)

## Error handling

| Failure | Response |
|---|---|
| `FT_New_Face` returns non-zero | Log via NSLOG, fall back to `font_diskfont` (bitmap path) for that face. Single graceful fallback. |
| `FT_Load_Char` returns non-zero for codepoint | Render `?` glyph at same position. Page renders with substitution. |
| Glyph cache cap reached | LRU eviction until insert fits. No malloc churn (slab arena). |
| Glyph larger than cache cap (96 px headline) | Bypass cache: render fresh, free after compose. Logged. |
| `amiport_ammx_init` Apollo not detected / V_EnableAMMX errors | Function table populated with scalar implementations. `has_ammx2() == false`. Hot loop calls work unchanged. Startup logs "AMMX2: ENABLED" or "AMMX2: not available, using scalar fallback". |
| `LockBitMapTags` returns NULL | Fall back to `WritePixelArray`. Slower but always works. Log. |
| Memory exhaustion during cache insert | Trigger LRU eviction earlier (50% cap) → retry. Final fallback: skip caching, render uncached. |

## Testing

**Unit (vamos, CI):**
- `tests/glyph-cache/` — LRU correctness (insert/lookup/evict, hit-rate, slab arena bounds, LRU ordering invariant under random workload)
- `tests/ammx/` — both backends produce identical output for ~50 reference patterns (alpha=0/128/255 ramps, transparent/opaque sprites, edge widths, alignment cases). Scalar is ground truth; AMMX2 must match byte-for-byte. AMMX2 path itself can't run on vamos — only the scalar backend exercises in CI; AMMX2 verification on real hardware.

**Integration (FS-UAE, CI):**
- `ports/netsurf/test-fsemu-cases.txt` — boot NetSurf, load 5 reference HTML fixtures (plain para, mixed headlines, italic+bold mix, Unicode block, 200-char paragraph). Screenshot each, compare against committed PNG baselines via existing pyte/visual-test infra. Catches regressions in glyph positioning, missing chars, baseline drift.
- Perf canary: render the 200-char paragraph 10 times, log average cycle count via `ReadEClock`. CI fails if regresses >20% from baseline.

**Manual ship gate (A6000, not CI):**
- `make install-a6000 TARGET=ports/netsurf` pushes via `amigactl` (Channel A from `project_real_hardware_loop.md`)
- Browse 3 sites: Aminet front, NetSurf homepage, en.wikipedia.org main article
- Verify: visibly anti-aliased text, no jaggies, no missing glyphs, page paint <2 s for non-image-heavy pages
- Capture serial-debug log via `toolchain/debug-tools/serial-monitor/` — confirm "AMMX2: ENABLED" line and `ReadEClock` cycles per glyph composite (target: ~1 cycle/pixel, single-digit µs per typical 10×16 glyph)

**Regression protection (after port lands):**
- `regression-checker` agent dispatched after any change to `lib/posix-shim/`, `lib/freetype/`, or `lib/glyph-cache/` — rebuild + re-test the NetSurf binary alongside other affected ports

## Knowledge corpus interactions

amiga-kb usage is mandatory throughout, per project conventions and the user's specific direction:

- **Pre-implementation queries** (capture into the implementation plan):
  - `amiga_pitfalls_for("LockBitMapTags")`, `amiga_pitfalls_for("FreeType bullet")`, `amiga_pitfalls_for("vampire.resource V_EnableAMMX")`
  - `amiga_search("RastPort BitMap pixel format Picasso96")`, `amiga_search("AMMX PMULA STOREm3")`, `amiga_search("FT_Load_Char alpha bitmap")`
  - `amiga_recipe_lookup` for any Apollo AMMX recipe that already exists
- **During implementation**: every new pitfall encountered (FreeType integration, glyph cache eviction edge case, AMMX init order, OS3 vs OS4 RastPort difference) routed via `/capture-learning` to `amiga_add_pitfall` so the corpus grows
- **Post-implementation**: `amiga_coverage` to confirm new entries indexed; addendum to the AMMX section of `pc-game-porting-cookbook.md` documenting glyph compositing as a pattern future ports can copy

## Open architectural questions (for the implementation plan to resolve)

1. **Per-glyph vs pre-resolved face/size dispatch** — recommendation is per-glyph for v1 (matches NetSurf's existing API, glyph cache absorbs the cost), revisit if Aminet front-page hit-rate drops below 95%
2. **Glyph cache size default** — 512 KB starting point; tunable via env var; collect hit-rate telemetry on real workloads to refine
3. **Font path resolution** — `FONTS:Truetype/<family>-<weight>.ttf` naming convention vs a config file mapping CSS family names to filenames; defer to implementation plan after surveying what NetSurf already does on OS4
4. **Glyph-cache key bit packing** — 64 bits is tight if we want to support all FT hint flags; may need 128-bit key (two 64-bit ints) which doubles the hash entry size

## Phasing within Phase 1

The implementation plan should sequence the work so each step is independently testable:

1. `lib/glyph-cache/` standalone (no NetSurf dep) — write, unit-test on vamos
2. `lib/posix-shim/include/amiport/ammx.h` + scalar backend + AMMX2 backend — write, unit-test scalar path on vamos, AMMX2 path on A6000 manually
3. `toolchain/scripts/m68k-amigaos-gcc-v4` wrapper — write, smoke-test by building a hello-world
4. `ports/netsurf/` skeleton — clone NetSurf 3.11, get the existing build working under bebbo-gcc + libnix on V4 (no font changes yet)
5. `font_freetype.c` + `font.c` patch — integrate, add nsoption, build, FS-UAE smoke test
6. Connect to AMMX compositor via `amiport_ammx->compose_glyph_argb32` — A6000 hardware test
7. Performance instrumentation + ship gate

Each step ends in a working artifact that the next step builds on. If we hit a blocker mid-stream, prior steps are still useful contributions.

## Phase 2 / Phase 3 cross-references (out of scope here)

- Phase 2: AMMX-accelerated PNG / JPEG row composite. Builds on Phase 1's `amiport_ammx` API by adding `unpack_rgb565_from_argb`, `swap_argb_to_bgra` primitives. Targets NetSurf's image handlers.
- Phase 3: Duktape-on-libnix port. New `lib/duktape/`. NetSurf already has `NETSURF_USE_DUKTAPE = YES, NETSURF_USE_JS = NO` in its Makefile.defaults — flipping JS=YES once Duktape works on libnix unlocks minimal JavaScript without further NetSurf core work.

Both phases will get their own brainstorm → spec → plan cycle.
