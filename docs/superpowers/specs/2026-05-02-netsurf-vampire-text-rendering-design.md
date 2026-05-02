# NetSurf Vampire Text Rendering — Phase 1 Design

**Status:** Design — awaiting implementation plan
**Date:** 2026-05-02 (revised)
**Phase:** 1 of 3 (subsequent phases: AMMX PNG decode, Duktape JavaScript)

## Summary

Ship NetSurf 3.11 on Apollo A6000 (Vampire V4) with Vampire-native anti-aliased text rendering — FreeType for glyph rasterization, Apollo AMMX2 for the alpha glyph compositor, no scalar fallback path. The binary is for Vampire users; stock 68040/60 support is explicitly out of scope.

As a side effect, land reusable amiport infrastructure that future Vampire-native ports can adopt: a generic glyph cache, a vendored Vampire SDK (flype44 headers), an AMMX assembly toolchain (vasm 1.8b in our Docker image), and an `amiport_ammx` API surface that consumers call without ceremony.

Deliverable: a `ports/netsurf/` amiport port. Upstream PR is out of scope until the path proves itself on V4 over weeks of real use.

## Problem

NetSurf 68k as it ships today (arczi84/NetSurf-68k fork, mainline NetSurf, the MUI fork) renders body text on AmigaOS 3 / Vampire as **1-bit bitmap glyphs blitted via `BltTemplate` with a `MEMF_CHIP` bounce buffer per character**:

- `frontends/amiga/font_bullet.c:559-569` gates the AA path (`OT_GlyphMap8Bit` + `BLITT_ALPHATEMPLATE`) inside `#ifdef __amigaos4__`. The OS3 build silently downgrades to bitmap regardless of the caller's `aa` flag.
- `frontends/amiga/font_bullet.c:602-614` does `AllocVec(MEMF_CHIP) + CopyMem + BltTemplate + FreeVec` per glyph — chip-RAM round-trip per character, no cache, no SIMD.
- `font_cache.c` exists in the same frontend but caches `struct OutlineFont *` (font handles), not glyph bitmaps.

Press claims about "AA on classic Amiga via freetype2.library" describe TypeManager system rendering, not NetSurf's content area — they are different code paths and bullet.library V51 (OS3 baseline) does not honour `OT_GlyphMap8Bit` regardless of which freetype backend is installed.

The Vampire user experience on text-heavy pages is dominated by these two costs. Text is what users read on every page; fixing it is the highest-leverage user-visible improvement available, and the Vampire 68080 has the AMMX2 instruction set sitting unused while we do it the slow way.

## Goal

Replace the broken `font_bullet.c` AmigaOS 3 path with a parallel `font_freetype.c` peer that:

- Renders 8-bit alpha glyphs via amiport's existing `lib/freetype/` (integer-only fixed-point, no soft-float crash exposure)
- Caches glyph bitmaps in a new generic `lib/glyph-cache/` (LRU, slab-allocated)
- Composites via vasm-assembled Apollo AMMX2 kernels — `PCMP` + `STOREm3 #1` for write-without-read masked store, `PMULA` for per-byte alpha multiply

The binary requires Apollo 68080 silicon at runtime — `V_EnableAMMX(V_AMMX_V2)` is called once at startup; if it fails the port refuses to launch with a clear error message. No scalar fallback, no runtime CPU dispatch, no portability complexity.

## Success criteria

1. **Visual:** NetSurf renders the Aminet front page, NetSurf's own homepage, and `en.wikipedia.org`'s main article body with visibly anti-aliased glyphs on A6000. No jagged edges in body text.
2. **Correctness:** No glyph-position regressions vs the bitmap path on a five-page reference fixture (paragraph, mixed headlines, italic+bold mix, Unicode block, 200-char line). Screenshots checked into git as visual baselines.
3. **No-crash:** 50-page browsing scenario completes without Guru Meditation. Glyph cache eviction under pressure does not corrupt or leak.
4. **Reusable infra ships alongside:**
   - `lib/glyph-cache/` consumable by future text-rendering ports (mg, less, future PDF viewer, terminal emulator)
   - `lib/vampire-sdk/` (vendored flype44 headers, MPL 2.0) consumable by any future amiport port that wants AMMX
   - Updated Docker image (`ghcr.io/bdgscotland/amiport-toolchain-gcc13`) ships with vasm 1.8b for AMMX `.asm` assembly
   - `amiport_ammx` API surface (header + linkable kernels) so subsequent AMMX work doesn't re-discover the wiring

## Non-goals

- **Stock 68040 / 68060 / 68030 support.** Vampire-native means Vampire-required. The binary checks for AMMX2 at startup and refuses to launch otherwise.
- **Runtime CPU dispatch / function-pointer table / scalar fallback path.** Single code path, AMMX2 always.
- **Performance gates / cycle budgets.** "Optimize later" — we'll measure what we get and tune in Phase 1.5+ if hot paths show up.
- **JavaScript engine integration** (Phase 3 — separate brainstorm)
- **AMMX-accelerated PNG / JPEG decode** (Phase 2 — separate brainstorm; arczi84's NetSurf-MUI already has JPEG AMMX patterns we can crib from when we get there)
- **HTTP/2, Brotli, WebP, AVIF** (deferred indefinitely — wrong audience for the cost)
- **Upstream PR submission** (deferred until proven on V4 over weeks of real use)
- **AmigaOS 4 path changes** — `#ifdef __amigaos4__` branches preserved as-is; OS4 keeps its bullet.library AA path

## Toolchain landscape & precedent

**Apollo cross-compiler reality** (verified 2026-05-02):

- There is no integrated "Apollo SDK" product. The de-facto Vampire toolchain is **bebbo's amiga-gcc + vasm + a third-party Vampire SDK**.
- **bebbo-gcc 13.3** (our current `:gcc13` Docker image) does NOT support `-m68080` from C. Only the experimental `68080regs` branch does, and that branch is gcc 6.5 only — mutually exclusive with our C++17 toolchain.
- **AMMX from C must be done via vasm-assembled `.asm` files**, linked alongside C objects. This is the canonical pattern.
- **vasm 1.8b** (`vasmm68k_mot -m68080 -Fhunk`) is the AMMX assembler. Free, redistributable, emits HUNK objects compatible with `m68k-amigaos-ld`.
- **flype44/Vampire** (github, MPL 2.0, last updated 2025-10-10) is the de-facto Vampire SDK — provides `vampire/vampire.h`, `proto/vampire.h`, `V_VAMPIRENAME`, `V_AMMX_V2`, `V_EnableAMMX()`. Apollo Team does not publish these as a package.

**Working precedent:** [arczi84/NetSurf-3.11-MUI](https://github.com/arczi84/NetSurf-3.11-MUI) (last updated 2025-11-29, same author as the NetSurf-68k base port) **already ships AMMX2-accelerated JPEG SIMD inside NetSurf** for V4/A6000. The pattern:

- AMMX kernels live in `.asm` files: `jdcolor-ammx.asm`, `jdmerge-ammx.asm`, `jidctfst-ammx.asm`, `jmemset-ammx.asm`
- C glue (`jsimd_ammx.c`) calls `V_EnableAMMX(V_AMMX_V2)` once at init and dispatches to the asm kernels via extern declarations
- C objects + ASM objects + bebbo `m68k-amigaos-ld` → working NetSurf binary

This means the toolchain story is **proven, not theoretical**. Our font work follows the same pattern: vasm `.asm` AMMX kernels + C glue + V_EnableAMMX gate. We can crib boilerplate from arczi84's `jsimd_ammx.c` for `font_freetype.c`'s AMMX dispatch.

**Source tree decision:** fork from **arczi84/NetSurf-3.11-MUI**, not arczi84/NetSurf-68k. Rationale:
- MUI fork is more recently active (2025-11-29 vs older)
- MUI fork already has the V_EnableAMMX init wired up
- MUI fork has the vasm `.asm` build infrastructure in its Makefile
- MUI is the AmigaOS 3 native widget toolkit; fits Vampire/A6000 user expectations
- We get the JPEG-AMMX precedent code visible in the same tree as our font-AMMX additions

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│ ports/netsurf/                  (the application port)          │
│   ├── original/                 (NetSurf-MUI source, RO)        │
│   ├── ported/                                                   │
│   │     └── frontends/amiga/                                    │
│   │           ├── font_freetype.c     NEW — FreeType + AMMX glue│
│   │           ├── font_freetype_ammx.asm  NEW — vasm kernels   │
│   │           ├── font.c              PATCH — add 3rd dispatch  │
│   │           └── gui_options.c       PATCH — new nsoption     │
│   ├── Makefile                  -m68040 -m68881, links vasm .o  │
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
│  lib/vampire-sdk/                    NEW (vendored flype44)     │
│    ├── include/vampire/vampire.h                                │
│    ├── include/proto/vampire.h                                  │
│    └── LICENSE  (MPL 2.0)                                       │
│                                                                 │
│  lib/posix-shim/include/amiport/                                │
│    └── ammx.h                        NEW — minimal header       │
│                                                                 │
│  lib/posix-shim/src/                                            │
│    └── ammx_init.c                   NEW — V_EnableAMMX wrapper │
│                                                                 │
│  toolchain/docker/Dockerfile.bebbo-gcc13   PATCH                │
│    + vasm 1.8b                                                  │
│                                                                 │
│  tests/glyph-cache/                  NEW (vamos)                │
└─────────────────────────────────────────────────────────────────┘
```

### Component contracts

**`lib/posix-shim/include/amiport/ammx.h`** — minimal Vampire-init header:

```c
/* Initialize AMMX2 context-switch handling for the current task.
 * MUST be called once at startup before any AMMX kernel runs.
 * Returns 0 on success, non-zero on failure (no Apollo, vampire.resource
 * missing, V_EnableAMMX rejected).
 *
 * Callers that fail this check should print a friendly error and exit —
 * AMMX kernels invoked without successful init will Line-F trap and
 * Guru the system. There is no scalar fallback in this build.
 */
int amiport_ammx_init(void);
```

`amiport_ammx_init()` opens `vampire.resource`, calls `V_EnableAMMX(V_AMMX_V2)`, returns 0 on success. NetSurf's `gui.c` early-init calls it; on failure prints `"This NetSurf build requires Apollo 68080 with AMMX2 (Vampire V4 / A6000). Use the standard NetSurf 68k build for stock 68k systems."` and exits cleanly.

**`lib/glyph-cache/`** — generic LRU cache:

- Key: `(face_id, codepoint, px_size, hint_flags)` packed into a 64-bit int
- Value: `(alpha_bitmap_offset_in_arena, advance_x_q16, bearing_x, bearing_y, w, h)`
- Storage: pre-sized slab arena (caller specifies cap, e.g., 512 KB ≈ 800-1500 glyphs depending on size). Bitmaps allocated bump-pointer style with LRU eviction reclaiming the oldest entries when full.
- Hash: open-addressing linear probe, sized to ~2× expected entry count
- Eviction: LRU via doubly-linked list overlaid on slab entries
- Thread-safety: none (AmigaOS is single-threaded; this is fine)
- Reusable by any text-rendering port — `mg`, `less`, future PDF viewer, terminal emulator

**`lib/vampire-sdk/`** — vendored from flype44/Vampire (MPL 2.0). We pick only the headers we need (`vampire/vampire.h`, `proto/vampire.h`, the lvo/inline glue). Pinned to a specific commit; documented in `lib/vampire-sdk/README.md`. Updates handled via `make update-vampire-sdk` script that re-pulls the upstream tag.

**`ports/netsurf/ported/frontends/amiga/font_freetype.c`** — implements NetSurf's `font_funcs` interface for the AmigaOS 3 build:

- `init()` — open `lib/freetype/`, register a font path resolver pointing to `FONTS:Truetype/`, allocate glyph cache (default 512 KB cap, override via env var)
- `width()` — measure string width using FT_Glyph metrics (cache-aware: uses cached metrics when present)
- `position_in_string()` / `split()` — text break iteration, walks codepoints accumulating advances
- `text()` — the hot path: per glyph, glyph_cache_lookup → on miss `FT_Load_Char(face, cp, FT_LOAD_RENDER | FT_LOAD_TARGET_NORMAL)` → cache insert → call AMMX kernel `font_compose_glyph_argb32_ammx()` to composite onto the locked RastPort BitMap

**`ports/netsurf/ported/frontends/amiga/font_freetype_ammx.asm`** — vasm-assembled AMMX2 kernels:

- `font_compose_glyph_argb32_ammx` — composite an 8-bit alpha bitmap onto an ARGB32 framebuffer at a given color
  - Inner loop, 8 pixels per iteration: `PCMP` source vs 0 → mask, `PMULA` for per-byte alpha multiply, `STOREm3 #1` for write-without-read
  - Direct port of the recipe in `.claude/rules/known-pitfalls.md` AMMX section, lines 1050-1180
- Boilerplate cribbed from arczi84/NetSurf-MUI `j*-ammx.asm` files

**`ports/netsurf/ported/frontends/amiga/font.c`** — patch only. Adds a third branch to the existing `nsoption_bool(bitmap_fonts)` dispatch: when `nsoption_bool(freetype_fonts)` is true, route to `ami_font_freetype_init()`. Default true on AmigaOS 3, false on AmigaOS 4 (preserves existing OS4 behavior).

**`toolchain/docker/Dockerfile.bebbo-gcc13`** — patch to add vasm 1.8b:

```dockerfile
RUN cd /tmp && \
    wget -O vasm.tar.gz http://sun.hasenbraten.de/vasm/release/vasm.tar.gz && \
    tar xzf vasm.tar.gz && cd vasm && \
    make CPU=m68k SYNTAX=mot && \
    install -m 755 vasmm68k_mot /usr/local/bin/ && \
    cd / && rm -rf /tmp/vasm*
```

The amiport build pipeline gets vasm available alongside `m68k-amigaos-gcc`. Port Makefiles can `vasmm68k_mot -m68080 -Fhunk -o foo.o foo.asm` then link with the rest of the C objects.

## Data flow (text-render hot path)

```
ami_plot_text(rp, x, y, "Hello", style)
   │
   ├─→ font dispatch in font.c → ami_font_freetype_text(rp, x, y, "Hello", style)
   │     │
   │     ├─→ resolve face: hash(family, weight, italic) → FT_Face*
   │     │     ├─ HIT in face cache: return cached
   │     │     └─ MISS: FT_New_Face from FONTS:Truetype/<name>.ttf,
   │     │             FT_Set_Pixel_Sizes, insert in face cache
   │     │
   │     └─→ for each codepoint in "Hello":
   │           │
   │           ├─→ glyph_cache_lookup(face_id, cp, px_size, hint_flags)
   │           │     ├─ HIT: return cached alpha bitmap + metrics
   │           │     └─ MISS: FT_Load_Char(face, cp, FT_LOAD_RENDER | FT_LOAD_TARGET_NORMAL)
   │           │              → 8-bit alpha bitmap in face->glyph->bitmap
   │           │              glyph_cache_insert(...)
   │           │
   │           └─→ font_compose_glyph_argb32_ammx(
   │                   fb + (x+bearing_x)*4 + (y+bearing_y)*pitch, pitch,
   │                   alpha_bitmap, alpha_stride, w, h, color_argb)
   │                 │
   │                 └─ AMMX2 inner loop, 8 pixels per iteration:
   │                      PCMP src vs 0 → mask
   │                      PMULA dst, alpha → blended bytes
   │                      STOREm3 #1 → write only non-zero
   │
   └─ x += advance
```

No dispatch step, no fallback branch — the `font_compose_glyph_argb32_ammx` symbol resolves directly to vasm-assembled AMMX2 code at link time.

## Error handling

| Failure | Response |
|---|---|
| `amiport_ammx_init` fails (no Apollo, vampire.resource missing) | Print friendly error to console + log, exit cleanly (RC=10). No fallback. Documented in PORT.md and `.readme` as a hardware requirement. |
| `FT_New_Face` returns non-zero (font file missing, corrupted) | Log via NSLOG, fall back to `font_diskfont` (bitmap path) for that face only. Other faces still use FreeType. |
| `FT_Load_Char` returns non-zero for codepoint | Render `?` glyph at same position. Page renders with substitution. |
| Glyph cache cap reached | LRU eviction until insert fits. No malloc churn (slab arena). |
| Glyph larger than cache cap (96 px headline) | Bypass cache: render fresh, free after compose. Logged. |
| `LockBitMapTags` returns NULL | Fall back to `WritePixelArray`. Slower but always works. Log. |
| Memory exhaustion during cache insert | Trigger LRU eviction earlier (50% cap) → retry. Final fallback: skip caching, render uncached. |

## Testing

**Unit (vamos, CI):**
- `tests/glyph-cache/` — LRU correctness (insert/lookup/evict, hit-rate measurement, slab arena bounds, LRU ordering invariant under random workload). Pure-C, no AMMX, runs on vamos 68000 baseline fine.
- AMMX kernels themselves are not unit-tested in CI — they require Apollo silicon to execute. Manual hardware test on A6000 covers them.

**Integration (FS-UAE, CI):**
- `ports/netsurf/test-fsemu-cases.txt` — boot NetSurf, load 5 reference HTML fixtures (plain para, mixed headlines, italic+bold mix, Unicode block, 200-char paragraph). Screenshot each, compare against committed PNG baselines via the existing pyte/visual-test infra. Catches regressions in glyph positioning, missing chars, baseline drift.
- Note: FS-UAE does not emulate Apollo AMMX2. CI cannot exercise the AMMX kernels — only confirms the NetSurf binary boots, font dispatch reaches `font_freetype.c`, and the build is self-consistent. Visual correctness of the AMMX-rendered output requires real-hardware verification.

**Manual ship gate (A6000, not CI):**
- `make install-a6000 TARGET=ports/netsurf` pushes via `amigactl` (Channel A from `project_real_hardware_loop.md`)
- Browse 3 sites: Aminet front, NetSurf homepage, en.wikipedia.org main article
- Verify: visibly anti-aliased text, no jaggies, no missing glyphs, no Guru Meditation
- Capture serial-debug log via `toolchain/debug-tools/serial-monitor/` — confirm `amiport_ammx_init` succeeded and no fallback paths fired

**Regression protection (after port lands):**
- `regression-checker` agent dispatched after any change to `lib/posix-shim/`, `lib/freetype/`, `lib/glyph-cache/`, or `lib/vampire-sdk/` — rebuild + re-test the NetSurf binary alongside other affected ports

## Knowledge corpus interactions

amiga-kb usage is mandatory throughout, per project conventions and the user's specific direction:

- **Pre-implementation queries** (capture into the implementation plan):
  - `amiga_pitfalls_for("LockBitMapTags")`, `amiga_pitfalls_for("FreeType bullet")`, `amiga_pitfalls_for("vampire.resource V_EnableAMMX")`, `amiga_pitfalls_for("AMMX PMULA STOREm3")`
  - `amiga_search("RastPort BitMap pixel format Picasso96")`, `amiga_search("FT_Load_Char alpha bitmap")`
  - `amiga_recipe_lookup` for any Apollo AMMX recipe that already exists
- **During implementation**: every new pitfall encountered (FreeType integration, glyph cache eviction edge case, AMMX init order, OS3 vs OS4 RastPort difference, vasm linkage subtleties) routed via `/capture-learning` to `amiga_add_pitfall` so the corpus grows
- **Post-implementation**: `amiga_coverage` to confirm new entries indexed; addendum to the AMMX section of `pc-game-porting-cookbook.md` documenting glyph compositing as a pattern future ports can copy

## Open architectural questions (for the implementation plan to resolve)

1. **Per-glyph vs pre-resolved face/size dispatch** — recommendation is per-glyph for v1 (matches NetSurf's existing API, glyph cache absorbs the cost). Revisit if hit-rate measurements show otherwise.
2. **Glyph cache size default** — 512 KB starting point; tunable via env var; collect hit-rate telemetry on real workloads to refine.
3. **Font path resolution** — `FONTS:Truetype/<family>-<weight>.ttf` naming convention vs a config file mapping CSS family names to filenames; defer to implementation plan after surveying what NetSurf-MUI already does.
4. **Glyph-cache key bit packing** — 64 bits is tight if we want to support all FT hint flags; may need 128-bit key (two 64-bit ints) which doubles the hash entry size.
5. **vasm install path** — system-wide in `/usr/local/bin/` vs amiport-private under `toolchain/scripts/`. Recommend system-wide (matches how `m68k-amigaos-gcc` is exposed) but worth confirming.

## Phasing within Phase 1

The implementation plan should sequence the work so each step is independently testable and one bad surprise doesn't stall everything:

1. **Toolchain prep** — patch `Dockerfile.bebbo-gcc13` to add vasm 1.8b; vendor flype44/Vampire SDK to `lib/vampire-sdk/`; rebuild image; smoke-test by assembling a trivial AMMX hello asm and linking with a C main.
2. **`lib/posix-shim/include/amiport/ammx.h` + `ammx_init.c`** — write `amiport_ammx_init()`; unit test on A6000 (returns 0). On vamos / FS-UAE / non-Apollo it should return non-zero.
3. **`lib/glyph-cache/`** standalone — write LRU cache, unit-test on vamos. Pure-C, no AMMX dependency.
4. **`ports/netsurf/` skeleton** — clone arczi84/NetSurf-3.11-MUI, get the existing build working under bebbo-gcc 13.3 + libnix on V4 with no font changes yet. This step surfaces toolchain compat issues; might require Makefile patches.
5. **`font_freetype_ammx.asm`** — write the AMMX2 glyph compositor kernel. Standalone unit test that fills a small ARGB buffer with synthetic alpha glyph data, run on A6000, eyeball-verify.
6. **`font_freetype.c` + `font.c` patch** — wire FreeType + glyph cache + AMMX kernel into NetSurf's font dispatch. FS-UAE smoke test (binary boots, scalar paths still work for OS4, OS3 path reaches font_freetype.c).
7. **A6000 hardware test** — install, browse Aminet/NetSurf/Wikipedia, verify AA glyphs, capture serial debug log.
8. **Visual regression baselines** — generate the 5 reference PNG screenshots, commit them, wire into CI.

Each step ends in a working artifact that the next step builds on. If we hit a blocker mid-stream, prior steps are still useful contributions.

## Phase 2 / Phase 3 cross-references (out of scope here)

- **Phase 2:** AMMX-accelerated PNG / JPEG row composite. Builds on Phase 1's vasm + flype44 SDK + glyph-cache slab pattern. Targets NetSurf's image handlers. arczi84's NetSurf-MUI already has the JPEG-AMMX kernels — we may be able to merge them rather than rewrite.
- **Phase 3:** Duktape-on-libnix port. New `lib/duktape/`. NetSurf already has `NETSURF_USE_DUKTAPE = YES, NETSURF_USE_JS = NO` in its Makefile.defaults — flipping JS=YES once Duktape works on libnix unlocks minimal JavaScript without further NetSurf core work.

Both phases will get their own brainstorm → spec → plan cycle.
