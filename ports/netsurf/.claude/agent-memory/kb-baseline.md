# amiga-kb Baseline — NetSurf Vampire Phase 1

Captured 2026-05-02 at the start of Phase 1 implementation, per
`docs/superpowers/plans/2026-05-02-netsurf-vampire-text-rendering.md`
"Pre-implementation: mandatory amiga-kb queries" section.

## Q1 — `amiga_pitfalls_for("vampire.resource V_EnableAMMX context switch")`

**Result:** No known pitfalls found. Gap logged.

**Implication for plan:** Task 5 (`ammx_init.c`) is greenfield from amiga-kb's
perspective. The implementing subagent should use the spec's pseudocode (which
draws from `lib/posix-shim` source patterns, not from amiga-kb pitfalls) and
report any discoveries via `/capture-learning` so future ports get them.

## Q2 — `amiga_pitfalls_for("LockBitMapTags Picasso96 ARGB framebuffer pixel access")`

**Result:** No known pitfalls found. Gap logged.

**Implication for plan:** Task 21 (`font_freetype.c` text() hot path —
LockBitMap → AMMX kernel → UnlockBitMap) is greenfield. The spec's data flow
diagram is the design source. Capture every surprise to amiga-kb.

## Q3 — `amiga_pitfalls_for("FT_Load_Char FreeType bullet glyph rendering")`

**Result:** Returns the captured pitfall (medium severity):

> **NetSurf 68k font_bullet.c OT_GlyphMap8Bit AA path is `#ifdef __amigaos4__`
> only — no AA on AmigaOS3**
>
> In NetSurf's `frontends/amiga/font_bullet.c` (arczi84/NetSurf-68k fork and
> mainline), the anti-aliased glyph rendering path is gated by
> `#ifdef __amigaos4__`. Specifically lines 559-569 set `glyphmaptag =
> OT_GlyphMap8Bit` and `template_type = BLITT_ALPHATEMPLATE` only on
> AmigaOS 4; on AmigaOS 3 the code unconditionally uses `OT_GlyphMap`
> (1-bit bitmap glyph) and ignores the caller's `aa` flag.
>
> Furthermore, the OS3 blit path at lines 602-614 uses `AllocVec(MEMF_CHIP)
> + CopyMem + BltTemplate + FreeVec` per glyph — no glyph cache, chip-RAM
> bounce per character, no SIMD.

**Implication for plan:** Confirms the design problem the spec sets out to
solve. Task 22 (font.c dispatch patch) must preserve `#ifdef __amigaos4__`
branches verbatim — the OS4 build keeps using bullet.library AA. Only the
AmigaOS 3 path gets the new FreeType + AMMX route.

## Q4 — `amiga_pitfalls_for("AMMX PMULA STOREm3 alpha blend")`

**Result:** No known pitfalls found. Gap logged.

**Implication for plan:** The canonical AMMX2 reference is
`.claude/rules/known-pitfalls.md` "Apollo AMMX: Authoritative Instruction
Reference" (~lines 1043-1180), captured during the brainstorm session from
`AC68080PRM.pdf`. Task 18 (`font_freetype_ammx.asm`) MUST follow that
reference exactly — `PCMP src vs 0 → mask`, `PMULA dst, alpha → blended`,
`STOREm3 #1 → write only non-zero`. Three-instruction inner loop, V2 idiom.
Reading the rule file is not optional for the implementing subagent.

## Q5 — `amiga_search("RastPort BitMap Picasso96 LockBitMap pixel format")`

**Result:** Low confidence. Returns generic graphics.library `WritePixelArray8`
/ `WritePixelLine8` / `WriteChunkyPixels` autodocs (none directly about
Picasso96 or LockBitMap).

**Implication for plan:** Task 21 (`font_freetype.c`) needs to determine the
RastPort BitMap pixel format at runtime. The graphics.library autodocs in
the KB are about chunky/planar conversion, not Picasso96 ARGB32. The
implementing subagent should:

1. Query `LockBitMapTags` + `BMA_DEPTH` + `BMA_PIXELFORMAT` (Picasso96 V3+
   tags) to discover the surface format
2. If `PIXFMT_ARGB32` (or `PIXFMT_BGRA32`) → call AMMX kernel directly
3. If `PIXFMT_RGB16` → fall back to `WritePixelArray` per-glyph (slower
   path; OK for Phase 1 since AGA isn't an explicit target anyway)
4. Capture the actual Picasso96 tag values + struct field offsets to
   amiga-kb when discovered

## Q6 — `amiga_recipe_lookup("ammx_alpha_blit")`

**Result:** No recipe found.

**Implication for plan:** Phase 1's Task 18 + Task 19 ESTABLISH the
`ammx_alpha_blit` recipe. Task 27 (knowledge corpus contributions) should
register it via `amiga_add_*` so Phase 2 (PNG row composite) and future
text-rendering ports find it.

## Q7 (bonus) — `amiga_search("FT_Load_Char alpha bitmap glyph rendering")`

**Result:** Low confidence. Returns Amiga Mail "Rasterizing a Glyph" docs
(about Amiga's native bullet.library, not FreeType) and graphics.library
font docs.

**Implication for plan:** FreeType's API surface is not in amiga-kb. The
implementing subagent should consult FreeType's own docs (the `lib/freetype/`
header files installed at `lib/freetype/include/freetype2/freetype/`). The
key APIs for Phase 1: `FT_Init_FreeType`, `FT_New_Face`, `FT_Set_Pixel_Sizes`,
`FT_Load_Char(face, cp, FT_LOAD_RENDER | FT_LOAD_TARGET_NORMAL)` — produces
8-bit alpha bitmap in `face->glyph->bitmap.buffer`, dimensions in
`bitmap.width` / `bitmap.rows`, stride in `bitmap.pitch`, advance in
`face->glyph->advance.x` (26.6 fixed-point).

## Cross-cutting note for the implementing subagents

Every gap above (Q1, Q2, Q4, Q5, Q6, Q7) becomes a Phase 1 deliverable per
the spec — the corpus growth IS part of the work. Throughout implementation,
when a new finding emerges (Picasso96 tag value, AMMX context-switch gotcha,
FreeType integer-vs-fixed quirk, glyph cache eviction edge), the implementer
MUST invoke `/capture-learning` to route it to the right enforcement
mechanism (project-local rule for amiport-specific lessons, `amiga_add_*` for
universal AmigaOS knowledge).

This file is the BEFORE snapshot. After Phase 1 lands, re-run all 7 queries
and diff — that diff is the Phase 1 corpus contribution.
