# PDR-014: Fold SDL Game Ports into amiport as Category 5

## Status

Accepted

## Date

2026-04-15

## Supersedes

- [PDR-008](008-sdl2-amigaos3-vision.md) "Relationship to amiport" section — the "separate repo, consumed as dependency" clause only. The rest of PDR-008 (scope, phases, port candidate inventory) stands.

## Problem

PDR-008 (proposed 2026-03-27) decided libSDL2-amigaos3 should live in a sibling repo (`bdgscotland/libSDL2-amigaos3`) and amiport would "gain it as a dependency for graphical port candidates." That decision was sound while the library was unproven.

Between 2026-03-27 and 2026-04-15, the library shipped to v0.7.0 with four working game ports — 1oom, Chocolate Doom, Julius, Celeste Classic — currently living in a second sibling repo (`bdgscotland/amiga-game-ports`). The proof is in. What's missing is a single integrated pipeline, catalog, and publishing surface for *all* amiport outputs — CLI tools AND SDL games — so that a user visiting amiport.platesteel.net sees the full picture in one place, and so that the amiport pipeline (source-analyzer → code-transformer → build-manager → test-runner → memory-checker → perf-optimizer → aminet-publisher → amiport-publisher) is reused rather than reinvented for every graphical port.

Leaving the two surfaces split means: two catalogs to maintain, two publishing flows, two sets of release notes, duplicate shim logic, and the ergonomic friction of "is this project a CLI port or a game — which repo do I look at?"

## Target Users

- **Amiga game enthusiasts** get a single browsable catalog covering every ported program — games next to CLI tools.
- **amiport contributors** get one pipeline to learn, one catalog to update, one publishing flow to invoke. No mental model split between "SDL tier" and "POSIX tier".
- **libSDL2-amigaos3 maintainer (Duncan)** keeps the library in a focused repo with its own release cycle, but stops maintaining a parallel `amiga-game-ports` repo and parallel catalog.
- **The site (amiport.platesteel.net)** gains Category 5 as a first-class filter alongside the existing CLI / Scripting / Console UI / Network categories.

## Decision

**SDL-based game ports become amiport Category 5 "SDL Graphical", living in `amiport/ports/` under the existing pipeline. libSDL2-amigaos3 remains a separate sibling repo with its own release cycle and is consumed as a build-time dependency (analogous to how bebbo-gcc is an external toolchain).**

Concretely:

1. **libSDL2-amigaos3 stays external.** It is *not* folded into `amiport/lib/`. Its release cadence, test suite, contribution model, and zlib license remain independent. amiport references it by version (e.g. `LIBSDL2_AMIGAOS3_VERSION = 0.7.0`) and the build-manager knows how to locate/install the SDK.
2. **SDL game ports move into `amiport/ports/`.** The four games currently in `bdgscotland/amiga-game-ports` (1oom, chocolate-doom, julius, ccleste) migrate into `amiport/ports/` under their existing names. The `amiga-game-ports` repo is archived (not deleted — the history is valuable) with a README pointing at amiport.
3. **Category 5 is added.** `data/catalog.json` gains a `sdl` or `sdl-graphical` category value. ADR-011 (port category taxonomy) is amended to define Category 5: "Graphical SDL program requiring libSDL2-amigaos3 + RTG or AGA video hardware."
4. **Pipeline stages are reused, not rewritten.** source-analyzer, code-transformer, build-manager, test-runner, memory-checker, perf-optimizer, aminet-publisher, and amiport-publisher continue to own their stages. Two of them gain minor extensions:
   - **build-manager** learns to link `-lSDL2` and point include paths at the external SDK.
   - **test-designer** gains a "visual" mode for SDL ports: FS-UAE screenshot capture + PNG reference diffing, instead of TAP/stdout comparison. (This extension is partially in place already — see `test-fsemu-visual-cases.txt` and the ADR-024/025 visual test infrastructure — and can be re-purposed.)
5. **Shared shims become available to SDL ports.** Every shim amiport already maintains (`lib/posix-shim/`, `lib/console-shim/`, `lib/bsdsocket-shim/`, `lib/zlib/`, `lib/libtommath/`, `lib/libtomcrypt/`, `lib/libgit2/`, `lib/oniguruma/`) is reusable by SDL ports that happen to need file I/O, compression, networking, crypto, or VCS. Julius almost certainly wants zlib. Any future SDL netplay port wants bsdsocket-shim. The amiport/ports/ layout automatically exposes these without duplication.

## Rationale

### Why fold the games in (vs keep them in `amiga-game-ports`)

- **One catalog, one pipeline, one publishing surface.** The cost of maintaining a parallel sibling repo scales with every new port; the cost of adding a port to amiport is bounded by the existing pipeline.
- **Shim reuse is automatic.** Games that need zlib, bsdsocket, or libgit2 get them for free. In a sibling repo, the shim dependency would have to be vendored or shipped as yet another release artifact.
- **Pipeline discipline is already right for games.** The source-analyzer/code-transformer/build-manager/test-runner cycle was designed for POSIX ports but is not POSIX-specific — every stage applies to SDL programs too. The only truly new piece is visual test capture, and the ADR-024/025 infrastructure has already started on that problem for console UI ports.
- **Single narrative for users.** amiport.platesteel.net becomes the one place to browse every program. No split between "CLI ports here, games over there."

### Why keep libSDL2-amigaos3 external (vs fold the library in too)

- **PDR-008's core argument still holds for the library.** libSDL2-amigaos3 is a *platform library* — it binds to CyberGraphX, AHI, Intuition, Exec — not a POSIX port. amiport's `lib/` pattern is for bundled shim libraries that wrap POSIX over AmigaOS. SDL2 is the opposite direction.
- **Release cadence independence.** libSDL2-amigaos3 ships on its own schedule (already on 0.7.0 with weekly releases). Folding it into amiport would couple its release to amiport's release cycle and force every SDL bug fix through amiport's commit gates.
- **Contribution model.** libSDL2-amigaos3 is upstream-able to SDL2 proper someday. Keeping it as a standalone repo preserves that option. Folding it in would bury it under amiport's structure.
- **Size.** libSDL2-amigaos3 is a 130-ish-file C codebase with its own test suite, examples, docs, and build system. Merging it into amiport doubles the repository's C footprint for a library that amiport's pipeline doesn't actually transform.

### Why Category 5 (and not Category 6, or a new numbering scheme)

- ADR-011 currently defines Categories 1-4 (CLI / Scripting / Console UI / Network). Adding Category 5 at the next integer is the obvious extension.
- PDR-008's prose mentioned "Category 5/6: SDL graphical" ambiguously. This PDR settles it: **5**.
- Category numbers reflect the *testing and runtime profile*, not difficulty. SDL graphical programs need RTG or AGA + visual test capture — distinct enough from Category 4 (network) to warrant its own number.

## Success Criteria

1. **Catalog unity.** `data/catalog.json` contains all four migrated SDL games with `category: "sdl-graphical"` and `site/data/packages/<name>.json` exists for each. The four game LHAs are staged under `site/packages/` per the site-mirror-discipline rule.
2. **Site browsability.** amiport.platesteel.net's package browser (`site/packages.html`) filters Category 5 correctly and shows the four games.
3. **Pipeline dispatches.** A fifth SDL game port (candidate TBD — Crimson Fields is a natural choice since it was identified as a viable new port earlier in planning) can be taken from candidate → published using only amiport pipeline commands, with no detours into `amiga-game-ports`.
4. **No shim duplication.** Whichever migrated game needs zlib (likely Julius) links against `lib/zlib/libz.a` from amiport, not a duplicate copy.
5. **libSDL2-amigaos3 release cycle unchanged.** A v0.8.0 release of the library triggers a version bump in amiport's build configuration, but the library repo is untouched by the fold-in.
6. **amiga-game-ports repo archived.** Its README points at amiport and at libSDL2-amigaos3. Its git history remains accessible on GitHub.

## Implementation Outline (not binding — subject to its own plan)

This PDR records the decision. The actual implementation should be planned via `/brainstorm` → `/plan` → `/execute-plan` or equivalent, and will likely need an ADR amendment on top. Rough sketch:

- **Step 1:** Amend ADR-011 to define Category 5 ("SDL Graphical"). Update `data/catalog.json` schema to accept the new category value. Update `scripts/catalog-score.py` and site JS if they enumerate categories explicitly.
- **Step 2:** Extend `build-manager` agent: when a port Makefile declares `LIBS_SDL2 = 1` (or similar), the agent knows to locate libSDL2-amigaos3's SDK, pass `-I<sdk>/include`, `-L<sdk>/lib -lSDL2`, and ensure the SDK version matches `LIBSDL2_AMIGAOS3_VERSION` in the project-level config.
- **Step 3:** Extend `test-designer` agent: new "visual" mode that produces FS-UAE screenshot capture tests against a reference PNG. Builds on the existing ADR-024 visual test infrastructure but uses RTG framebuffer capture instead of ConUnit trap. Where the existing infra doesn't fit, write ADR-027 for the SDL visual test approach.
- **Step 4:** Pick one of the four existing SDL games as the migration pilot. ccleste is the smallest — good first migration. Run it through the full amiport pipeline: copy source into `ports/ccleste/`, fill in `PORT.md` per the template, run build-manager + test-runner + memory-checker + perf-optimizer, publish via amiport-publisher. Any pipeline gaps discovered get fixed before the next migration.
- **Step 5:** Migrate the remaining three games (1oom, chocolate-doom, julius) using the pilot's patterns.
- **Step 6:** Archive `amiga-game-ports` with a README redirect to amiport.
- **Step 7:** Add Crimson Fields (or another net-new SDL port candidate) as proof that net-new SDL ports can be taken through the amiport pipeline cleanly.

Each of these is its own chunk of work and will be planned separately. This PDR exists so that the *direction* is recorded before implementation churn begins.

## Alternatives Considered

### Keep the status quo (PDR-008's original decision)

- **Pros:** No migration cost. `amiga-game-ports` stays where it is. No pipeline extensions needed in amiport. Duncan keeps maintaining two sibling repos.
- **Cons:** Parallel catalogs, parallel publishing flows, duplicated shim knowledge. Users have to know to look in two places. Every new SDL port pays the cost of maintaining the second repo's bespoke pipeline. The shim library amiport has already built (zlib, bsdsocket, libgit2, etc.) is harder to reuse across the boundary.
- **Verdict:** Rejected. The proof exists; the separation cost exceeds the separation benefit.

### Fold libSDL2-amigaos3 into amiport too (full merge)

- **Pros:** Single repo contains everything. Single git history. No external dependency to manage.
- **Cons:** libSDL2-amigaos3 is a platform library, not a POSIX port — it doesn't belong under amiport's POSIX-shim philosophy. Its release cadence would be coupled to amiport's. Contribution-from-upstream would be harder. The repository size would roughly double for a library amiport doesn't actually transform. Loses optionality of upstreaming the backend to SDL2 proper.
- **Verdict:** Rejected. The games belong in amiport; the library belongs in its own home.

### Create a third sibling repo that holds both the library and the games

- **Pros:** Separation of concerns if you want game-related work distinct from CLI work.
- **Cons:** Triples the repository surface. Doesn't solve the parallel-catalog problem. Cross-repo shim reuse gets harder, not easier.
- **Verdict:** Rejected. Adding repositories to solve a "where does X live" question is almost always wrong.

### Make amiport consume `amiga-game-ports` as a submodule / subtree

- **Pros:** Keeps `amiga-game-ports` as a thing while integrating it into amiport.
- **Cons:** Submodules are fragile. Subtrees duplicate history. Neither avoids the parallel-catalog cost; they just hide it behind a git mechanism. Contributors would still have to know which tree they're editing.
- **Verdict:** Rejected. Git mechanism is the wrong layer to solve a logical-boundary problem.

## Risks

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| Visual test infrastructure (FS-UAE screenshot + PNG diff) is harder than the ADR-024 console-based version | Medium | Medium | Start with exit-code-only tests for SDL ports; add visual diffing as a follow-up ADR-027. Four games already run, so exit-code gate is viable today. |
| libSDL2-amigaos3 version skew — amiport pins v0.7.0 but library ships v0.8.0 with API break | Low | Medium | Pin by exact version in build-manager config. Surface mismatches at build time with a loud error. Library follows semver so breaks are signposted. |
| Migration introduces regressions in already-working games (1oom/Doom/Julius/ccleste) | Medium | High | Migrate ccleste first as pilot; full FS-UAE test pass required before migrating the next. Keep the `amiga-game-ports` repo readable (not deleted) for reference. |
| Category 5 visual test authoring is more labor per port than CLI ports | Medium | Medium | Accept this. SDL ports are inherently higher effort than `yes` or `basename`. The unification benefit still dominates. |
| Shim version drift — games link `lib/zlib/libz.a` but the library was built for CLI ports' usage profile | Low | Low | zlib is shared today between libgit2 consumers and CLI consumers; SDL consumers are just another caller. Same rules apply. |
| amiport repo grows significantly in size from SDL game sources | Medium | Low | Acceptable. The repo already hosts libgit2 (1.44 MB) and CPython candidate source. Git handles it. |
| Contributors get confused about "which SDL backend are we editing" | Low | Low | Clear boundary: amiport edits *game* ports; libSDL2-amigaos3 edits the *library*. The repo split is the boundary. |

## References

- [PDR-008: SDL2 for AmigaOS 3.x — Vision and Feasibility](008-sdl2-amigaos3-vision.md) — the original "separate repo" decision; partially superseded here
- [PDR-009: Hardware expansion, SDL, and WHDLoad — exploration notes](009-hardware-expansion-and-sdl.md) — related context on SDL + hardware targets
- [libSDL2-amigaos3 sibling repo](https://github.com/bdgscotland/libSDL2-amigaos3) — the delivered library
- [amiga-game-ports sibling repo](https://github.com/bdgscotland/amiga-game-ports) — the four existing game ports to be migrated
- [ADR-011: Port category taxonomy](../adr/011-port-categories.md) — to be amended to add Category 5
- [ADR-024: Visual verification testing](../adr/024-visual-verification-testing.md) — basis for SDL visual test extension
- [ADR-025: Screen read trap for interactive cursor verification](../adr/025-screen-read-trap.md) — adjacent visual test infrastructure
