# amiga-kb New Content (Apollo / 68080 / SAGA / AMMX)

User added significant new content to the KB on 2026-05-02 mid-session.
Use these as primary references for upcoming tasks instead of relying on
project-local pitfalls notes.

## New documents in amiga-kb

| File | Lines | Chunks | Content |
|---|---|---|---|
| `saga-coding-examples.md` | 400 | 30 | 13 bare-metal SAGA code examples |
| `references/saga/saga-register-reference.md` | 3,050 | 122 | 445 registers, 88 detail pages |
| `references/saga/saga-driver-reference.md` | 110 | 8 | Driver tools, resolutions, SAGA audio from C |
| `references/ammx/AMMX-autodoc.md` | 2,620 | 76 | Full AMMX instruction docs + undocumented |
| `references/apollo/68080-instruction-reference.md` | 1,464 | 21 | Instruction lists, fusing, web scrape |
| `references/apollo/68080-features.md` | 70 | 3 | Feature comparison matrix |
| `references/apollo/AC68080-programmers-reference.md` | 1,371 | 129 | Full PRM: instructions, encodings, pipeline, timing, optimization |

## Mandatory queries by task

### Before Task 18 (write font_freetype_ammx.asm — the AMMX2 glyph compositor)

```
amiga_search "AMMX PMULA STOREm storeilm alpha blend" — primary instruction reference
amiga_search "AMMX register file E0-E23 calling convention vasm"
amiga_search "Apollo 68080 pipeline AMMX timing" — for perf design
amiga_search "AC68080 PRM addressing modes AMMX"
```

The AMMX-autodoc.md (2,620 lines) supersedes the spec's reference to
`.claude/rules/known-pitfalls.md` "Apollo AMMX: Authoritative Instruction
Reference" section. The known-pitfalls.md content was captured FROM
AC68080PRM.pdf during the brainstorm session; the new amiga-kb content
includes the FULL PRM + autodocs (more comprehensive). Query the KB first.

### Before Task 21 (write font_freetype.c — RastPort/Picasso96 hot path)

```
amiga_search "SAGA chunky video framebuffer DFF1EC ARGB32"
amiga_search "Picasso96 LockBitMapTags BMA_DEPTH PIXELFORMAT"
amiga_search "RastPort BitMap pixel format pen direct write"
```

The saga-register-reference.md may include Picasso96-side tags for the
SAGA chunky modes if those are documented register-side; saga-driver-reference.md
covers the runtime side.

### For perf-optimizer (after kernels land)

```
amiga_search "Apollo 68080 instruction fusing pipeline cycles"
amiga_search "AC68080 dual-issue retirement reorder"
amiga_search "AMMX cycle timing per instruction"
```

The 68080-instruction-reference.md + AC68080-programmers-reference.md
together give cycle-accurate timing for perf modeling.

## Update to kb-baseline.md (Q4)

The original baseline noted:
> Q4 -- amiga_pitfalls_for("AMMX PMULA STOREm3 alpha blend"): No known
> pitfalls found. The canonical AMMX2 reference is .claude/rules/known-pitfalls.md
> "Apollo AMMX: Authoritative Instruction Reference" section.

UPDATE: As of 2026-05-02 mid-session, the canonical reference is
`references/ammx/AMMX-autodoc.md` in amiga-kb, queryable via
`amiga_search "AMMX ..."`. The known-pitfalls.md content is still valid
but the KB version is more comprehensive and fully indexed.

## Implication for Task 18 dispatch prompt

When dispatching the Task 18 implementer:
1. Re-run the AMMX queries above and capture results into a fresh
   agent-memory file
2. Hand the implementer the AMMX-autodoc + AC68080-PRM excerpts directly
   (don't make them re-query — controllers curate context per the
   subagent-driven-development skill)
3. Cite the exact instruction encodings + cycle counts from the PRM, not
   the inferred-from-known-pitfalls patterns

This is the same discipline the spec called out as mandatory; the KB
hydration just made the source-of-truth richer.
