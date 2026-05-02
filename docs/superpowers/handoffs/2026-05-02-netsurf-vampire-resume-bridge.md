# Resume Bridge — NetSurf Vampire Phase 1

**Session date:** 2026-05-02
**Status:** Spec + Plan + KB hydration COMPLETE. Execution NOT started.
**Next action:** Start subagent-driven execution of Phase A Task 1 in a new worktree.

---

## What this session accomplished

A complete brainstorming → spec → plan → KB-hydration cycle for **NetSurf Vampire Phase 1** — a FreeType-backed font path with Apollo AMMX2 alpha glyph compositor for the user's Apollo A6000.

**No implementation code written yet.** Everything below is design + reference material in `docs/`.

## Project goal in one sentence

Ship NetSurf 3.11 on Apollo A6000 with anti-aliased text rendering via FreeType + Apollo AMMX2 alpha compositor, plus reusable amiport infrastructure (vasm in toolchain, vendored Vampire SDK, glyph cache library, `amiport_ammx_init` API).

## Critical artifacts (read these first)

| Path | What it is |
|---|---|
| `docs/superpowers/specs/2026-05-02-netsurf-vampire-text-rendering-design.md` | The design spec (commit `ed3a3344`). Approved by user. |
| `docs/superpowers/plans/2026-05-02-netsurf-vampire-text-rendering.md` | The implementation plan, 27 tasks across 8 phases (commit `af988079`). |
| `docs/references/ammx/instruction-reference.md` | Authoritative AMMX reference from AC68080PRM.pdf. |
| `docs/references/saga/sprite-hardware.md` | SAGA sprite hardware (Phase 1 doesn't use it but future game ports will). |
| `docs/references/saga/chunky-video.md` | SAGA chunky video registers — relevant for NetSurf framebuffer. |
| `docs/references/vampire-sdk/headers.md` | flype44/Vampire SDK header reference. |
| `docs/references/netsurf-mui/ammx-pattern.md` | Working precedent — arczi84/NetSurf-MUI's `jsimd_ammx.c` integration pattern. |
| `docs/references/apollo/toolchain-and-driver-references.md` | ApolloCrossDev + SAGA driver + browser-UA workaround. |

## Key architectural decisions (locked in via brainstorming)

1. **Deliverable shape:** amiport port (`ports/netsurf/`) + later upstream PR after proven on V4.
2. **Infrastructure scope:** Full reusable suite — `lib/glyph-cache/`, `lib/vampire-sdk/`, `lib/posix-shim/include/amiport/ammx.h`, vasm in Docker image.
3. **AMMX target:** V4/A6000 only (user's daily driver). Stock 68k / V2 users use mainline NetSurf 68k.
4. **Hardware-required, no scalar fallback** — `amiport_ammx_init()` failure exits cleanly with friendly error pointing user to mainline NetSurf 68k.
5. **Source tree:** Fork **arczi84/NetSurf-3.11-MUI** (NOT arczi84/NetSurf-68k base). Same author, more recently active, has V_EnableAMMX wired and JPEG-AMMX precedent.
6. **No perf gates / cycle budgets in v1** — "we can optimize later." v1 ships when AA text renders correctly on A6000. Phase 1.5+ does perf tuning.
7. **Test gate:** FS-UAE for CI (renders correctly path) + A6000 manual ship gate (AMMX2 path actually exercised).

## Phase 1 has been DECOMPOSED — Phases 2 & 3 are separate brainstorms

- **Phase 1 (this plan):** FreeType font + AMMX glyph compositor + reusable infrastructure
- **Phase 2 (later brainstorm):** AMMX-accelerated PNG row composite
- **Phase 3 (later brainstorm):** Duktape-on-libnix + NetSurf JS wiring

Don't expand Phase 1 scope mid-execution.

## The implementation plan in 8 phases

1. **A** — Toolchain prep (vasm 1.8b in Docker, flype44/Vampire SDK vendored)
2. **B** — `amiport_ammx_init` API in `lib/posix-shim`
3. **C** — `lib/glyph-cache/` standalone with LRU
4. **D** — NetSurf-MUI port skeleton (build unchanged)
5. **E** — AMMX glyph compositor kernel (vasm `.asm` + standalone test)
6. **F** — `font_freetype.c` + nsoption + dispatch wiring
7. **G** — A6000 hardware verification (Aminet, NetSurf homepage, en.wikipedia.org)
8. **H** — CI baselines, catalog, site mirror, ship via `amiport-publisher`
9. **I** — Knowledge corpus contributions (capture learnings as plan executes)

## amiga-kb hydration summary

Run `amiga_search` for any of these terms before writing implementation code — they all return primary-source content from this session's ingestion (~86 new vectors total):

| Query | Returns |
|---|---|
| `PMULA alpha blend` | The 1-instruction premultiplied alpha blend primitive |
| `STOREILM PCMP transparent sprite` | The 3-instruction V2/V4 masked-store idiom |
| `STOREM3 #1 byte palette` | The 2-instruction V4 fast-path masked store |
| `register __asm vasm bebbo-gcc` | Calling convention for AMMX kernels (NOT stack-based) |
| `V_EnableAMMX vampire.resource init` | Vampire SDK init pattern with VRES_OK / VRES_AMMX_WAS_ON nuance |
| `SAGA chunky $DFF1EC framebuffer` | SAGA chunky video registers and ARGB32 mode |
| `machine ac68080 vasm AMMX` | vasm syntax conventions |
| `font_bullet OT_GlyphMap8Bit AmigaOS3` | Why NetSurf 68k has no AA today |

## Pitfalls captured this session (queryable via `amiga_pitfalls_for`)

1. **NetSurf 68k font_bullet.c OT_GlyphMap8Bit AA path is `#ifdef __amigaos4__` only — no AA on AmigaOS3** (medium severity) — confirms the design problem
2. **STOREM3 EXISTS in Apollo AC68080 PRM page 73 — earlier amiport notes saying it doesn't are wrong** (medium) — corrects an earlier secondary-source mistake
3. **apollo-core.com filters non-browser User-Agents — use curl with Mozilla UA, not WebFetch** (low) — saves future sessions hours of "site is down" misdiagnosis

## Local resources (in `/tmp`, may be GC'd by macOS)

These were cloned/extracted during this session and the implementation plan references them. Re-clone if missing:

| Path | Source | Plan tasks that use it |
|---|---|---|
| `/tmp/ApolloCrossDev/` | `git clone github.com/WDrijver/ApolloCrossDev` | Crib VASM install + Makefile templates (Task 1, 4) |
| `/tmp/flype44-vampire/` | `git clone github.com/flype44/Vampire` | Vendor headers (Task 2) |
| `/tmp/arczi84-netsurf-mui/` | `git clone github.com/arczi84/NetSurf-3.11-MUI` | Reference jsimd_ammx.c + j*-ammx.asm (Task 14, 17) |
| `/tmp/saga-driver/` | Extract `~/Downloads/SAGADriver_3.5b1.lha` | Runtime driver reference (Task 16 .readme requirements) |

User-supplied PDFs (in `~/Downloads`):
- `AC68080PRM.pdf` — already ingested
- `Amiga_Sprites.pdf` — already ingested (Chapter 1 of SAGA TRM)
- `APOLLO68080_blockdiagram.pdf` — architecture confirmed in head; not separately ingested

## Mandatory rules for the next session

Per project CLAUDE.md and user's explicit direction this session:

1. **`amiga-kb` queries are mandatory** before writing AmigaOS/AMMX/SAGA code. The corpus is hydrated; use it. Specifically the pre-implementation queries listed in the plan's "Pre-implementation: mandatory amiga-kb queries" section.
2. **Pipeline agents are mandatory** — see `.claude/rules/use-pipeline-agents.md`. The plan dispatches `build-manager`, `test-runner`, `memory-checker`, `perf-optimizer`, etc. Do NOT do their work manually.
3. **`/capture-learning`** any new pitfall encountered. Both project-local AND `amiga_add_pitfall` for universal AmigaOS knowledge.
4. **Spec is locked.** Don't re-litigate the architectural decisions in the "Key architectural decisions" section above. If something is genuinely wrong, do a NEW brainstorm cycle to revise it — don't drift mid-execution.

## Honest gotchas the next session needs to know

1. **STOREM3 IS valid.** An earlier (now-corrected) amiport note in `.claude/rules/known-pitfalls.md` AMMX section said "STOREM3 doesn't exist." The plan's Task 18 uses `storem3 #1` and was correct all along. The KB has been updated; query `STOREM3 Apollo AMMX` for the corrected reference.
2. **Calling convention for AMMX kernels is REGISTER-BASED**, not stack-based. The plan's Task 18 example shows stack-based as a fallback option — prefer the register-based pattern via `register __asm("d2")` clauses (cribbed from arczi84's NetSurf-MUI). The KB doc `references/netsurf-mui/ammx-pattern.md` has the canonical example.
3. **Use `machine ac68080`** in vasm files (NOT `m68080`). vasm uses the `ac68080` machine name.
4. **apollo-core.com is reachable** — earlier session reports of "ECONNREFUSED" were due to bot filtering. Use `curl -A "Mozilla/5.0..."` via Bash, not WebFetch, when reaching apollo-core.com.
5. **Implementation plan said "8 steps within Phase 1"** but the actual task count is 27 numbered tasks across 8 phases (A-I). The plan's "Phasing within Phase 1" section is the high-level phase grouping; the per-task numbering is the granular execution plan.

## Recommended next session opening

```
Read these in order:
1. docs/superpowers/handoffs/2026-05-02-netsurf-vampire-resume-bridge.md  (this doc)
2. docs/superpowers/specs/2026-05-02-netsurf-vampire-text-rendering-design.md
3. docs/superpowers/plans/2026-05-02-netsurf-vampire-text-rendering.md

Then:
- Run the "Pre-implementation: mandatory amiga-kb queries" from the plan
- Create a worktree: git worktree add ../amiport-netsurf-phase1 -b feature/netsurf-vampire-phase1
- Invoke the superpowers:subagent-driven-development skill with the plan path
- Execute Task 1: patch toolchain/docker/Dockerfile.bebbo-gcc13 to add vasm 1.8b
```

## Stop hook compliance for this session

- ✅ Tests: no code was changed (only docs/references + plan). Nothing to test.
- ✅ Stray root files: clean — `git status --short` shows only the unrelated pre-session files.
- ✅ Docs updated: all new content committed. CLAUDE.md doesn't need updating (no new agent/skill/library shipped).
- ✅ Bugs/process failures captured: 3 pitfalls added via `amiga_add_pitfall`. The "STOREM3 doesn't exist" mistake from a prior session is now contradicted in KB.

## Session commits in order

| Commit | Subject |
|---|---|
| `b31c07ca` | docs: brainstorm spec for NetSurf 68k FreeType + AMMX glyph compositor (Phase 1) |
| `ed3a3344` | docs: revise NetSurf Vampire spec — Vampire-native, no fallback, MUI base |
| `af988079` | docs: NetSurf Vampire Phase 1 implementation plan (27 tasks across 8 phases) |
| `15491770` | docs(references): authoritative AMMX reference from AC68080PRM.pdf |
| `07584ee8` | docs(references): hydrate amiga-kb with Vampire SDK + NetSurf-MUI AMMX pattern + SAGA sprite reference |
| `5d8c53a3` | docs(references): SAGA chunky video mode register reference |
| `d166d59c` | docs(references): catalog ApolloCrossDev + SAGA driver + apollo-core.com browser-UA workaround |

## Estimated effort remaining for Phase 1

Based on the 27-task plan: **~3-4 weeks of focused work** (single developer, sequential subagent-driven execution). Toolchain prep (Phase A) + lib/glyph-cache (Phase C) are the smallest. NetSurf-MUI build adaptation (Phase D) is the highest-risk unknown — could surface compat issues that change downstream timeline.

Phase 2 (AMMX PNG decode) and Phase 3 (Duktape JS) are separate efforts — total project across all 3 phases is probably **~3 months** if executed sequentially.

---

*Bridge written 2026-05-02 at the end of a long brainstorm + KB hydration session. Read this first when resuming.*
