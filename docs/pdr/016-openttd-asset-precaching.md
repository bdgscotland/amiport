# PDR-016: OpenTTD Asset Precaching (.agf Cache Format)

## Status

Proposed

## Date

2026-04-20

## Problem

OpenTTD's GRF-based baseset architecture assumes a modern CPU. On Vampire V2 (68060-equivalent ~90-150 MIPS) the boot sequence takes **5-10 minutes** before the title screen paints:

- OpenGFX baseset = ~5 MB of compressed GRF data across 6 files
- OpenTTD parses GRFs in 6 sequential passes (SkipAct stages + real load)
- Sprite decompression is RLE-per-sprite, done TU-by-TU in generic C++
- Action resolution (Action 1/2/3 sprite group trees) is done at parse time, not pre-resolved
- Sound effects load from OpenSFX.cat (13 MB) adds another pass
- Title screen fallback (`GenerateWorld(GWM_EMPTY, 64, 64)`) triggers a SECOND full GRF parse because NewGRF state is reset on new game

On a modern x86 laptop this whole sequence is ~2 seconds. On 68k with 68882-class performance it is 5-10 minutes. This makes every iteration (test a patch, test a settings change, test a blitter tweak) cost 5-10 wall-clock minutes. This blocks serious development on real Amiga hardware.

The parsing cost is intrinsic to the GRF format — not an OpenTTD bug, not fixable with better optimization flags alone. We need to **precompute the parsed state** once, save it to disk in an amiport-native binary format, and mmap/load it at boot.

## Target Users

1. **Real-hardware testers** — anyone running amiport openttd on a physical Amiga (A2000+Vampire, A4000+Cyberstorm, etc.) who wants to develop, not wait
2. **End users on classic accelerated Amigas** (68030, 68040, 68060) where 5-10 min boot is simply unacceptable — the port is effectively unusable for casual play without this
3. **FS-UAE testers iterating on game logic or blitter** — even with JIT, GRF parse is the dominant cost
4. **CI / automated test runs** on the Amiga hardware channel — the whole test loop budget is consumed by GRF parse

## Decision

**Build `ottd-precache` — a host-side (x86 Linux/macOS) tool that:**

1. Links against OpenTTD's GRF loader + sprite cache code directly (same source, just compiled for host)
2. Loads the standard OpenGFX baseset (or any NewGRF combination) the exact way OpenTTD does
3. Runs the full GRF parse to completion, building the full in-memory `SpriteCache` + resolved action tables + glyph atlases
4. Serializes the resulting in-memory structures to a binary blob: `opengfx-v13.4.agf` (Amiga GRF cache)
5. Ship the `.agf` files alongside the amiport openttd LHA for each supported baseset

**And patch OpenTTD runtime to:**

6. Check for `data/opengfx-v13.4.agf` at boot
7. If present: `mmap()`/`Read()` the blob directly into the sprite cache + action tables, skip all GRF parsing
8. If absent: fall back to the slow original GRF parse (preserves compat)

Format version (`v13.4`) pins the blob to a specific OpenTTD version — any upstream change to internal data layout invalidates the cache. Tool regenerates on request.

**Scope out:**
- NewGRF dynamic/runtime addition (users adding custom GRFs via UI) stays slow — it falls back to parse path
- Localization files stay dynamic (small cost)
- Savegame parsing stays unchanged (different code path)

## Rationale

**Why precaching over alternative optimizations:**

- **`.agf` cache**: 30 sec → 2 sec boot. Permanent structural win. One-shot engineering cost.
- **-O2 per-file promotion on parser**: 2-3x speedup on parser, 5 min → ~2 min. Incremental, still slow.
- **Amiga-native sprite atlas (IFF/ILBM)**: loses NewGRF fidelity; complex UI/runtime coupling; harder to maintain.
- **Hand-tuned 68k RLE decoder**: 2-3x on decode hot path. Real win. Independent of `.agf` — can layer on top.
- **Profile-guided optimization**: requires data collection + recompile cycle; narrow impact.
- **CHIP RAM sprite residency**: runtime concern (blitter speedups during play), orthogonal to boot time.
- **Smaller baseset (strip unused GRFs)**: proportional win — cut 60% of GRFs → cut 60% of parse time. Still 2-4 min. And loses fidelity.

The precache approach wins because:
1. **It moves the cost to build-time, not runtime.** Host x86 CPU can parse in under a second; parse once, ship result.
2. **It keeps upstream OpenTTD pristine.** Only the runtime loader path gets a new branch (`if (.agf exists) load; else parse`).
3. **It's the same solution modern games ship shader caches with.** Well-understood pattern; users expect it; debuggable.
4. **It composes with every other optimization.** Once `.agf` loads in seconds, any RLE/blitter/CHIP RAM work applies at runtime only — no parse-path bottleneck.

**Tradeoffs:**

- **Cache invalidation**: `.agf` format pins to OpenTTD version + baseset version. Upstream bumps force us to regen + reship the cache blob alongside the binary. Acceptable — we're already shipping the binary per-version.
- **Development cycle**: porters iterating on OpenTTD internals that touch GRF state have to regen the cache per iteration. Slow. Mitigation: a `--no-cache` runtime flag to force parse path during porter sessions.
- **Binary size**: `.agf` is bigger on disk than the .grf source (expanded sprites, resolved pointer tables). Estimate ~15-25 MB for OpenGFX-v13.4. Fits in LHA; users with <32 MB Fast might need to use a reduced baseset.
- **Engineering cost**: ~1-2 weeks of focused work. Need to trace every in-memory pointer OpenTTD's GRF loader produces and either serialize it directly or rebuild from serializable state at load time. OpenTTD is in active development upstream; tests need to cover the round-trip.

## Success Criteria

- **First boot of amiport openttd-sdl2 on Vampire V2 reaches the title screen in under 10 seconds** (currently 5-10 minutes). Tested on Coffin R65 at 192.168.1.215.
- Second boot is equally fast (cache persists).
- No regression in ANY observable gameplay behavior — sprite IDs, coordinates, colors, palette, UI fidelity all identical between `.agf` boot and legacy parse boot on identical config.
- `ottd-precache` tool is reproducible: given the same input GRFs + OpenTTD source tree, produces byte-identical `.agf` output across platforms/runs.
- Works with at least: OpenGFX v7.1 (current ogfxi_logos / ogfx1_base / etc. set)
- Gracefully falls back to legacy parse if `.agf` is missing, corrupted, or version-mismatched.

## Alternatives Considered

1. **Strip baseset to minimum viable (MVP grfs)** — discussed in session. 5x speedup on parse, but still 1-2 min boot. Still painfully slow. Rejected as sole solution; may ship as supplemental option (`ottd-minimal-gfx` flag).

2. **Upstream OpenTTD caching patch** — propose cache serialization to openttd mainline. Better long-term but gated on upstream maintainer acceptance, review cycles, compatibility requirements for all target platforms. Not realistic for amiport timeline. Can revisit if upstream ever adds similar.

3. **Pre-compile to 68k-native binary data** — flat atlas with 68k byte order, palette lookup tables pre-computed. Lose NewGRF runtime replacement. More radical departure. Revisit if `.agf` proves insufficient.

4. **Rewrite GRF parser in 68k asm** — 2-3x speedup on parse. Still minutes. Same engineering cost as precache but much less leverage.

5. **Skip GRF load entirely, use bundled TTD 1995 data** — legal gray area, user must supply original TTD files, loses NewGRF support. Feasible as optional path for users who have original TTD, rejected as default.

6. **Shader-cache style "first run slow, then fast"** — run the full parse once on Amiga, save the .agf, subsequent boots fast. User-friendly (no host tool) but the first run is still 5-10 min. Best if paired with the host-side tool as the primary shipping path.

## Layered optimization menu (all compose with the .agf cache)

The `.agf` precache is the ground-truth answer to boot time. Beyond it, these optimizations target remaining runtime cost:

### Asset-side (pre-work, generated host-side, shipped in LHA)

- **Pre-resolved Action 2/3 sprite group trees** — GRFs describe sprite selection as nested "if vehicle-type-is-X, render sprite-Y, else ...". Currently resolved at runtime per draw. Pre-resolve host-side into lookup tables. Wins during gameplay too.
- **Pre-flattened sprite replacements** — Action 5 (GraphicsNew) + Action A (SpriteReplace) operate as patch layers on top of base sprites. Host-side tool resolves all layers, bakes result. Zero runtime replacement cost.
- **Pre-decoded sprite bitmaps** — RLE decompress every sprite host-side, ship flat 8-bit indexed bitmaps. Runtime: just memcpy to chip or fast RAM. Trade disk size for zero decode cost.
- **Pre-computed palette tables** — palette animation cycles baked into per-frame lookup tables. Instead of runtime palette transforms, index into the right row. 32× faster palette animation.
- **Pre-optimized sprite format** — sprites in a format that matches 68k blitter stride/alignment requirements (16-pixel-aligned, pre-rotated, mask+data interleaved). Blitter can draw directly without CPU pre-processing.
- **Pre-sorted sprite atlas** — group sprites by drawing frequency + type. Frequently-drawn UI chrome and terrain base tiles in CHIP RAM, everything else in FAST. Runtime picks source based on ID range, no sorting decisions.

### Build-side (host-side tooling we write once)

- **`ottd-precache`** — the main host tool. Consumes GRFs, emits `.agf`. (Core of this PDR.)
- **`ottd-spriteopt`** — host-side analyzer that takes a profile dump + .agf and generates a reordered .agf with CHIP RAM hot-set marked. One-shot calibration.
- **`ottd-baseset-strip`** — remove sprites for scenarios the user won't use (e.g., remove arctic climate if user plays only temperate). 5-10 MB data reduction. Same .agf format, smaller.
- **`ottd-patch-apply`** — Host-side NewGRF patch-applier. User drops new.grf on it → regenerated .agf with the new sprites baked in. Run once per GRF change.

### Runtime-side (68k-specific wins on top of .agf)

- **68k RLE decoder fallback** — if .agf is missing and we fall back to parse path, use a 68k asm decoder instead of generic C++. Still slow but 2-3x faster.
- **Blitter-assisted sprite blits** — identify sprites small enough for AGA/SAGA blitter direct path. Drop CPU rendering for those.
- **CHIP RAM locking for UI chrome + base terrain** — pin ~500 KB of hot sprites in CHIP RAM at load, fast blitter access thereafter.
- **Dirty-rect-aware Paint** — only update the actual changed region each frame, not the whole window. Already partly implemented in libSDL2-amigaos3.
- **Fixed-point map-coordinate arithmetic** — avoid the std::ostream<<int libstdc++ bugs by using snprintf-backed formatters everywhere. Already a known pitfall.
- **Frame-limiter that doesn't spin** — ensure SDL_Delay on AmigaOS is honored by scheduler (sleep yielding to other tasks). Already partly addressed.

### Aggressive options (break upstream compat for more speed)

- **Classic-TTD mode** — use original Transport Tycoon Deluxe 1995 .grfs (trg1r, trgcr, trgir, trgtr, trghr, signalsw) instead of OpenGFX. 500 KB total vs 5 MB for OpenGFX. User must supply TTD95 files (legal gray). Parse time: ~15-30 seconds instead of 5-10 min even WITHOUT .agf.
- **Strip Squirrel scripting** — the embedded Squirrel VM + AI framework adds significant cold-init cost. Remove entirely if users don't want AI opponents. Big binary size reduction too.
- **Strip NewGRF dynamic replacement** — once .agf is loaded, user cannot add custom GRFs without regenerating .agf. Trade flexibility for speed. Default: keep dynamic; opt-out with `ottd -b fast`.

### Order of implementation (proposed)

1. `ottd-precache` core tool + runtime loader → single biggest win, enables everything else.
2. `ottd-baseset-strip` for reducing .agf size when user doesn't need all climates.
3. CHIP RAM hot-set identification via profile dump → `ottd-spriteopt`.
4. 68k RLE decoder (fallback path improvement, not critical).
5. Classic TTD mode as opt-in for users who have the files and want fastest possible boot.

## Follow-ups

- Once grey-window rendering bug (current session) is resolved, this PDR is the single most impactful feature for amiport openttd's UX.
- Related optimizations that layer on top: 68k RLE decoder (cell.cpp, spriteloader), CHIP RAM residency, blitter-assisted blits, frame-limiter tuning (per existing perf work).
- Cross-reference: `project_openttd_perf_baseline.md`, `session_real_hardware_2026_04_19.md`, the current session (2026-04-20) confirming 5-10 min boot on Vampire.
