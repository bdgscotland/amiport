# Session 2 Handoff — NetSurf Vampire Phase 1

**Date:** 2026-05-02 (continuation of session 1 brainstorm/spec/plan)
**Worktree:** `.claude/worktrees/netsurf-vampire-phase1`
**Branch:** `feature/netsurf-vampire-phase1`
**Status:** Phases A + B (3/4) + C done. Phase D Task 14 done. Phase D Task 15 BLOCKED on dep stack.

---

## What this session delivered

**24 commits on `feature/netsurf-vampire-phase1`.** All built, all tested where applicable, all reviewed by spec + code-quality where applicable.

| Phase | Tasks | Status | Notes |
|---|---|---|---|
| **A — Toolchain** | 3/3 | ✅ done | vasm 2.0e in Docker image, Vampire SDK vendored, image published to GHCR (`ghcr.io/bdgscotland/amiport-toolchain-gcc13:latest` + `:vasm-2.0e`) |
| **B — `amiport_ammx_init` API** | 3/4 | ✅ except Task 7 | Header + impl + smoke test all pass on vamos. Task 7 (A6000 hardware test) deferred — needs your hardware. |
| **C — `lib/glyph-cache/`** | 6/6 | ✅ done with full library-pipeline | Header, scaffolding, create/destroy, hash+lookup, LRU, README, top-level Makefile, memory-checker APPROVED, perf-optimizer applied (-O1 + load-factor cap). 6/6 tests pass on vamos. |
| **D — NetSurf-MUI port skeleton** | 1/3 | ⚠️ Task 14 done, Task 15 BLOCKED | NetSurf-MUI source vendored as submodule. Wrapper Makefile written. Build halts at "Unable to find library for: CSS (libcss)". |
| **D-prime — NetSurf dep stack** | 0/10 | 🆕 PLANNED, not started | New phase inserted between original D Tasks 14 and 15. Plan addendum at `docs/superpowers/plans/2026-05-02-netsurf-vampire-phase-d-prime-dep-stack.md`. |
| **E-I** | 0/11 | ⏳ blocked on D-prime | AMMX kernel work + NetSurf integration + visual baselines + ship + corpus contribs |

**Side wins this session (separate from plan tasks):**
- All 20 amiport agent definitions repinned from bare `model: sonnet/haiku` to specific version IDs (`claude-sonnet-4-6`, `claude-haiku-4-5-20251001`) — was silently using older models
- New amiga-kb pitfall captured: "Vampire SDK V_EnableAMMX / V_*: VampireBase local-shadow is load-bearing, do not rename" — universal AMMX consumer warning
- 4 feedback memories captured (subagent_model_default, check_docs_before_inferring, ascii_check_pattern, netsurf_dep_stack_blocker)

---

## The strategic blocker (Task 15)

NetSurf 3.11 (MUI fork or upstream) requires ~13 NetSurf-specific libraries cross-compiled before its main build proceeds:
**libwapcaplet, libparserutils, libhubbub, libdom, libcss, libnsutils, libnslog, libnspsl, libnsgif, libnsbmp, libsvgtiny, libutf8proc** + **libcurl/libpng/libjpeg/libwebp** for external.

NetSurf-MUI does NOT bundle these — its `pkg/` dir has release LHA binaries
(`NetSurf-3.8_m68k.lha`, `NetSurf-ammx.lha`, `NetSurf-m68k.lha`) but no source libs.

The Phase 1 spec/plan didn't anticipate this. User decision needed.

---

## The revised options (after broader internet search)

A broad search beyond Aminet revealed three previously-unknown facts:

1. **NetSurf project ships an OFFICIAL m68k AmigaOS 3 nightly build** at
   `https://ci.netsurf-browser.org/builds/amigaos3/NetSurf-gcc-7422.lha`
   (3.1 MB, dated 2026-05-01). Proves the full dep stack DOES build cleanly on m68k.

2. **NetSurf project ships a prebuilt cross-toolchain tarball** at
   `https://ci.netsurf-browser.org/builds/toolchains/m68k-unknown-amigaos-73.tar.xz`
   (6.7 MB, dated 2026-01-09). GCC 7.x against an older libnix snapshot.
   Pair with `ns-pull-install` script to clone+build all 13 NetSurf libs.

3. **arczi84/NetSurf-3.11-MUI consumes this prebuilt env** — its
   `Makefile.config` sets `PKG_CONFIG_PATH = /opt/netsurf/m68k-unknown-amigaos/env/lib/pkgconfig`,
   exactly where ns-pull-install puts everything.

### Three revised options (A → B → C, recommended sequence)

**Option B — try the prebuilt tarball first (~30 min to know if it works)**
- Download `m68k-unknown-amigaos-73.tar.xz`
- Unpack into `/tmp/netsurf-prebuilt-env/`
- Write a 5-line C test that calls `lwc_string_data()` from libwapcaplet
- Try to link with bebbo-gcc 13.3 + libnix using the unpacked `env/lib/*.a`
- If link succeeds: the GCC 7.x archives are ABI-compatible with our 13.3 + libnix. **Skip 2-4 weeks of work.** Vendor the env into amiport's `lib/netsurf-deps/`, point our wrapper Makefile at it, retry Task 15.
- If link fails (likely some symbol mismatch / runtime model conflict): fall back to A.

**Option A — adapt ns-pull-install to bebbo-gcc 13.3 Docker (~1 week)**
- Write `toolchain/scripts/build-netsurf-deps.sh` that runs the equivalent of NetSurf's `ns-pull-install` script INSIDE our `ghcr.io/bdgscotland/amiport-toolchain-gcc13:latest` image
- Clones each lib from `git.netsurf-browser.org/<libname>.git`
- Cross-compiles with our toolchain, installs into `lib/netsurf-deps/env/lib/`
- Same end state as B but built fresh against our exact ABI

**Option C — original D-prime plan, per-lib library-pipeline (2-4 weeks)**
- Each of 10 libs gets its own `lib/<name>/` port
- Full source-analyzer / build / test-designer / test-runner / memory-checker / perf-optimizer per lib
- Highest reuse but the libs are essentially NetSurf-specific in practice
- Cleanest amiport-style work but the reuse argument is weaker than it sounds

**Recommendation: B → A → C in that order.** Don't sink afternoon on B if it's clearly not going to work; abandon early. Don't sink a week on A if B works.

---

## The Option B test recipe (do this first next session, ~30 min)

Self-contained. Run from project root in the worktree:

```bash
cd /Users/duncan/Developer/amiport/.claude/worktrees/netsurf-vampire-phase1

# Step 1: Download the prebuilt env (~7 MB, ~10 sec)
mkdir -p /tmp/netsurf-prebuilt
cd /tmp/netsurf-prebuilt
curl -L -o m68k-toolchain.tar.xz https://ci.netsurf-browser.org/builds/toolchains/m68k-unknown-amigaos-73.tar.xz
tar xJf m68k-toolchain.tar.xz
ls -la m68k-unknown-amigaos*/env/lib/ | head -10
# Expected: see libwapcaplet.a, libcss.a, libdom.a, etc.

# Step 2: Write a tiny C test
cat > /tmp/netsurf-prebuilt/test-libwapcaplet.c <<'EOF'
#include <stdio.h>
#include <libwapcaplet/libwapcaplet.h>
long __stack = 262144;
unsigned long __MEMORY_STEP = 262144;
int main(void) {
    lwc_string *s;
    if (lwc_intern_string("hello", 5, &s) != lwc_error_ok) return 10;
    printf("interned: %.*s len=%zu\n", (int)lwc_string_length(s), lwc_string_data(s), lwc_string_length(s));
    lwc_string_unref(s);
    return 0;
}
EOF

# Step 3: Try to link with bebbo-gcc 13.3 in our Docker image
ENV_DIR=$(ls -d /tmp/netsurf-prebuilt/m68k-unknown-amigaos*/env)
docker run --rm -v /tmp/netsurf-prebuilt:/work \
    -v $ENV_DIR:/env \
    ghcr.io/bdgscotland/amiport-toolchain-gcc13:latest \
    bash -c "m68k-amigaos-gcc -O0 -noixemul -m68040 -m68881 -std=gnu99 \
        -I/env/include /work/test-libwapcaplet.c \
        -L/env/lib -lwapcaplet \
        -o /work/test-libwapcaplet && echo LINK_OK"

# Step 4: If LINK_OK printed -> ABI compatible! Run on vamos:
docker run --rm -v /tmp/netsurf-prebuilt:/work \
    ghcr.io/bdgscotland/amiport-toolchain-gcc13:latest \
    bash -c "vamos -C 68040 -s 256 -m 4096 /work/test-libwapcaplet" || echo "RUN FAILED"
# Expected: "interned: hello len=5"
```

**Decision matrix from the result:**

| Result | Interpretation | Next action |
|---|---|---|
| LINK_OK + correct vamos output | ABI compatible. Vendor `env/` as `lib/netsurf-deps/`. Retry Task 15. | Do Option B vendoring (~2 hrs) |
| LINK_OK but vamos crash/wrong output | ABI link-compatible but runtime broken. | Investigate (could be -m68000 vs -m68040 mismatch, libnix version drift). Worth 1 hr; if no quick fix, abandon to Option A. |
| Link fails with undefined references | Symbol naming mismatch (different name mangling between GCC 7 and 13.3). | Abandon B, go to A. |
| Link fails with archive format error | File format incompatibility. | Abandon B, go to A. |

---

## Open TODOs from this session (not blocking but noted)

1. **Phase B Task 7** — A6000 hardware smoke-test for `amiport_ammx_init`. Needs your physical Apollo. Defer until you have time at the machine OR until other Phase B-dependent work needs verification.

2. **Test binaries in working tree** — `tests/ammx-init/test_ammx_init` and `tests/glyph-cache/test_glyph_cache` are checked into the worktree (this is per `.claude/rules/test-hygiene.md` — compiled binaries should be committed). They occasionally show up as "modified" in git status when rebuilt with the same flags but slightly different timestamps. Ignorable.

3. **Library-pipeline rule cross-reference** — the `.claude/rules/library-pipeline.md` mandates source-analyzer + memory-checker + perf-optimizer for libraries. Options A and B for D-prime BYPASS this discipline. Either: amend the rule to allow vendored prebuilt deps (bypass exception for `lib/netsurf-deps/` style), OR run the full pipeline on the unpacked archives (treating them as "imported source"). Worth a small policy discussion at the start of next session.

4. **Plan revision** — once D-prime path is confirmed (B works / B fails → A confirmed), the original Phase 1 plan needs revision to insert the D-prime task list. The plan addendum at `docs/superpowers/plans/2026-05-02-netsurf-vampire-phase-d-prime-dep-stack.md` is the basis.

---

## Recommended next-session opening prompt

```
Read these in order:
1. docs/superpowers/handoffs/2026-05-02-netsurf-vampire-session-2-handoff.md (this doc)
2. docs/superpowers/handoffs/2026-05-02-netsurf-vampire-resume-bridge.md (session 1 -> session 2 bridge)
3. ~/.claude/projects/-Users-duncan-Developer-amiport/memory/project_netsurf_dep_stack_blocker.md
4. docs/superpowers/plans/2026-05-02-netsurf-vampire-phase-d-prime-dep-stack.md

Then:
- Run the Option B test recipe (this handoff's section "The Option B test recipe").
  ~30 min budget; abandon early if link fails clearly.
- Based on result, either:
  A) Option B succeeded -> vendor lib/netsurf-deps/ + retry Task 15 (~half day total)
  B) Option B failed -> dispatch the build-netsurf-deps.sh script work for Option A (~1 week)
- Address the library-pipeline rule discussion (TODO #3 above) before committing
  the D-prime path -- it's a policy change worth a quick AskUserQuestion.
```

---

## Session commits (chronological)

```
66c079a9 docs(netsurf): capture amiga-kb baseline for Phase 1 implementation
73ecbbd5 build(toolchain): add vasm 1.8b to gcc13 Docker image for AMMX kernels
adeadbad chore(agents): pin specific model IDs (sonnet 4.6 / haiku 4.5)
02ab1599 chore(toolchain): vasm comment honesty + visible wget errors
32bf0ed2 build(vampire-sdk): vendor flype44/Vampire headers for AMMX init
cc219445 chore(vampire-sdk): update-script chdirs to repo root automatically
39f57f17 shim: add amiport/ammx.h public API for Vampire AMMX init
74b6d2c7 shim: implement amiport_ammx_init() for Apollo 68080 / Vampire
f21c1e2b docs(netsurf): note new amiga-kb Apollo/AMMX/SAGA content for Phase E+ tasks
28f37e21 shim(ammx): doc-comments to prevent foreseeable future regressions
c89f891b test: add amiport_ammx_init smoke test (vamos: expects rc!=0)
32ca2b04 test(ammx-init): bump __stack to 262144, drop dead vampire-sdk include
a30c444d lib(glyph-cache): public API header
c9e53ff8 test(glyph-cache): Task 9 -- test framework scaffolding
faaa0825 lib(glyph-cache): Task 10 -- stub create/destroy implementation
6a2dd09a lib(glyph-cache): Task 11 -- hash table + insert/lookup
7d4bad94 feat(glyph-cache): implement LRU eviction with flush-all strategy
242f1831 docs(glyph-cache): add README per Task 12 Step 5
5e6660aa build: wire lib/glyph-cache into top-level make targets
2879c655 perf(glyph-cache): per-file -O1 + 70% load factor cap
cdce02a0 feat(netsurf): import NetSurf-MUI 3.11 upstream source
fc585f37 fix(netsurf): convert ports/netsurf/original to proper submodule
86dc81a8 feat(netsurf): wrapper Makefile + Phase D dep-stack blocker captured
fd1220a6 docs: Phase D-prime plan (NetSurf dep stack cross-compile, ~10 libs)
```

---

*Bridge written 2026-05-02 at the end of session 2. Read this first when resuming.*
