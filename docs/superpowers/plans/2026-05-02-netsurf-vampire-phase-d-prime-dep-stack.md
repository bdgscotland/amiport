# NetSurf Vampire Phase D-prime: Cross-compile the netsurf-buildsystem dep stack

**Status:** Plan addendum — inserted between Tasks 14 and 15 of the original Phase 1 plan after Task 15's first build attempt revealed the dep-stack blocker.
**Date:** 2026-05-02 (updated session 3 with Option B outcome + Option C confirmation)
**Reason:** Original spec assumed NetSurf "just builds". Reality: NetSurf needs ~10 NetSurf-specific libraries (libcss/libdom/libhubbub/...) cross-compiled before main build can proceed. Plus external libs (libcurl, libpng, libjpeg, libwebp).
**Decision:** User chose **Option C** (full per-lib library-pipeline) on 2026-05-02 after Option B was empirically ruled out. Estimated +2-4 weeks of focused work. Highest reusability — becomes the foundation for future browser/parser/XML/RSS ports. No `.claude/rules/library-pipeline.md` change needed; each lib follows the rule unchanged.

## Option B (vendored prebuilt env) — RULED OUT 2026-05-02

The session 2 handoff proposed testing the NetSurf project's prebuilt cross-toolchain tarball (`m68k-unknown-amigaos-73.tar.xz`, 6.7 MB) for ABI compatibility with our bebbo-gcc 13.3 + libnix. Session 3 ran the test. Result: **conclusively dead.** Three independent reasons:

1. **Tarball does not contain NetSurf-specific libraries.** It ships only EXTERNAL deps (`libcurl`, `libpng`, `libjpeg`, `libexpat`, `libcares`, `libutf8proc`, `libz`, `libiconv`, `libpbl`, `libtre`, `libcharset`). The 11 NetSurf libs (`libwapcaplet`, `libcss`, `libdom`, etc.) get built separately by `ns-pull-install` clone+build. The tarball alone could not have unblocked Task 15 even if ABI-compatible.

2. **GCC ABI mismatch — symbol naming.** Tarball is GCC **3.4.6** (the `73` in the filename is misleading; `m68k-unknown-amigaos-gcc-3.4.6` is the actual binary). bebbo-gcc 13.3 emits and expects `___divdf3` (TRIPLE underscore) for soft-float helpers; GCC 3.4.6 emits `__divdf3` (DOUBLE underscore). Linker reports them as different symbols.

3. **GCC ABI mismatch — float model + libnix runtime data.** The prebuilt `libpng.a` was compiled for software float (calls `__divdf3`/`__muldf3` etc.) and references `errno` and `__iob` as direct data symbols. bebbo-gcc 13.3 with `-m68040 -m68881` emits FPU instructions inline (no soft-float helpers in libgcc.a at all for the m68040+881 variant) and modern libnix exposes `___seterrno` (function indirection) instead of a direct `errno` data symbol. Neither break is repairable by linker flags or header tweaks.

Test artifact: `/tmp/netsurf-prebuilt/` (cleaned up after the test, `m68k-unknown-amigaos-73.tar.xz` re-fetchable from `https://ci.netsurf-browser.org/builds/toolchains/`). Test source `test-libpng.c` was a 12-line program calling `png_access_version_number()`. Link failure dumped 50+ undefined references across `__divdf3`/`__muldf3`/`__adddf3`/`__addsf3`/`__divsf3`/`__gtdf2`/`__ltdf2`/`__ledf2`/`__iob`/`errno` and missing `pow`/`floor`/`atof` (the math ones would have resolved with `-lm`; the others are ABI-fundamental).

**Implication for future amiport sessions:** any prebuilt m68k library compiled with GCC 3.x or older toolchain pinned to a different libnix snapshot will hit the same ABI walls. Vendor source-port (Option C) or write a build-script-inside-our-Docker (Option A, also viable but rejected for reusability reasons) are the only paths.

## Dep-stack ordering (build dependency DAG)

Each library is a separate `lib/<name>/` port following `.claude/rules/library-pipeline.md` (source-analyzer, build, test-designer, test-runner, memory-checker, perf-optimizer, docs). Order matters — later libs depend on earlier.

```
Wave 1 (no NetSurf deps):
  libwapcaplet     — string internment, ~500 LOC, pure C
  libparserutils   — parser primitives, ~3K LOC, pure C
  zlib             — ALREADY IN amiport (lib/zlib)
  libpng           — needs zlib
  libjpeg          — pure C, no deps
  libwebp          — pure C (or lib6lab variants)

Wave 2 (depends on wave 1):
  libhubbub        — HTML5 parser, depends on libparserutils
  libdom           — DOM impl, depends on libwapcaplet + libparserutils
  libcss           — CSS parser, depends on libwapcaplet + libparserutils
  libnsbmp         — BMP decoder, depends on (none from wave 1)
  libnsgif         — GIF decoder, depends on (none from wave 1)
  libsvgtiny       — SVG, depends on libdom + libwapcaplet

Wave 3 (depends on wave 1+2):
  libnsutils       — NetSurf utilities, depends on libwapcaplet
  libnslog         — logging, no NetSurf deps but project-style
  libnspsl         — public suffix list, depends on libwapcaplet
  libcurl          — HTTP, depends on AmiSSL (we have lib/amissl-sdk) or skip TLS
```

The non-NetSurf libs (libpng, libjpeg, libwebp, libcurl) may already exist on Aminet — dispatch `aminet-researcher` BEFORE porting from source. Same for MUI dev libs.

## Per-library task template

Each `lib/<name>/` port = these 8 mandatory steps per library-pipeline rule:

1. **Aminet research** — does it already exist on Aminet? If yes, vendor the binary + headers.
2. **KB query** — `amiga_pitfalls_for("<libname> ...")` + `amiga_search` for related concerns
3. **Source-analyzer** — portability audit, log all POSIX surface
4. **Build** — Makefile + iterate on errors with build-manager
5. **Test-designer + test-runner** — unit tests via tests/shim/test_framework.h pattern
6. **Memory-checker** — leak / UAF / double-free audit
7. **Perf-optimizer** — 68k tuning + per-file -O1 promotion if safe
8. **Docs** — README.md + CLAUDE.md codebase-map entry + top-level Makefile target

Estimated per-library cost: 1-2 hours for small (libwapcaplet ~500 LOC), 4-8 hours for medium (libcss, libdom several K LOC each), 1-2 days for libcurl with TLS bridge.

## Phase D-prime task list

| Task | Library | Estimated effort | Depends on |
|---|---|---|---|
| D-prime A | libwapcaplet | 1-2 hrs | (none — start here) |
| D-prime B | libparserutils | 2-3 hrs | (none) |
| D-prime C | libhubbub | 4-6 hrs | libparserutils |
| D-prime D | libdom | 6-10 hrs | libwapcaplet + libparserutils + libhubbub |
| D-prime E | libcss | 6-10 hrs | libwapcaplet + libparserutils |
| D-prime F | libnsutils + libnslog + libnspsl | 3-4 hrs total (small) | libwapcaplet |
| D-prime G | libnsbmp + libnsgif + libsvgtiny | 4-6 hrs total | libdom (svgtiny) |
| D-prime H | libpng + libjpeg + libwebp | 4-8 hrs total (mostly Aminet research first) | zlib (libpng) |
| D-prime I | libcurl with TLS bridge or skip | 1-2 days | libamiport-net (we have it) + AmiSSL stub |
| D-prime J | MUI dev libs (research Aminet first) | TBD | (none) |

After all D-prime tasks land, retry original Task 15 (`make -C ports/netsurf`). Should now reach a different blocker (compilation of NetSurf itself), which is what the original Task 15 was scoped for.

## Risk register

1. **Each new lib introduces its OWN unanticipated blockers.** Pure-C math libs are usually clean; libcurl + TLS is genuinely hard on AmigaOS. Build a pessimism budget.
2. **Test surface for these libs is moderate.** NetSurf's libs all have unit-test suites upstream, but most test against POSIX file IO that doesn't quite work the same on AmigaOS. Tests may need to be subset.
3. **MUI dev libs may not be cross-compilable** under bebbo-gcc 13.3. If they're old AmigaOS-native binaries-only, we may need to vendor as binaries with header stubs (lib/amissl-sdk pattern). Research before assuming source-port.
4. **Phase E (AMMX kernel) work blocks on this.** Tasks 17-19 of original plan can NOT start until Phase D-prime is done. Phase E is the perf-critical work; deferring it ~3 weeks delays the visible payoff.

## Execution order (locked 2026-05-02 session 3)

Execute in dependency-DAG order. Each lib follows `.claude/rules/library-pipeline.md` unchanged (KB query, source-analyzer, build, test-designer, test-runner, memory-checker, perf-optimizer, docs).

**Wave 1 — no NetSurf-internal deps, can run in any order or parallel:**

1. **libwapcaplet** (start here) — string interning, ~500 LOC, pure C, simplest lib. Clean validation that the per-lib pipeline works for this dep stack.
2. **libparserutils** — parser primitives, ~3K LOC, pure C.

**Wave 2 — depends on Wave 1:**

3. **libhubbub** — HTML5 tokeniser, depends on libparserutils.
4. **libdom** — DOM impl, depends on libwapcaplet + libparserutils + libhubbub.
5. **libcss** — CSS parser, depends on libwapcaplet + libparserutils.
6. **libnsbmp** — BMP decoder (no NetSurf deps).
7. **libnsgif** — GIF decoder (no NetSurf deps).
8. **libsvgtiny** — SVG, depends on libdom + libwapcaplet.

**Wave 3 — small NetSurf utilities:**

9. **libnsutils** — NetSurf utility funcs (monotonic time, base64, pwrite/pread). Standalone. **DONE 2026-05-02 session 5** (3 KB archive, 22/22 tests, KEEP-O1). Note: original plan said "depends on libwapcaplet" but the actual code does not.
10. **libnslog** — category logging + filter language (flex/bison). Standalone. **DONE 2026-05-02 session 5** (21 KB archive, 25/25 tests, KEEP-O1).
11. **libnspsl** — public suffix list lookup. Standalone. **DONE 2026-05-02 session 5** (67 KB archive, 18/18 tests, KEEP-O1). Note: original plan said "depends on libwapcaplet" but the actual API is just `const char *` in / `const char *` out.

## Wave 2 finisher: libsvgtiny — DEFERRED 2026-05-02 session 5

**Status:** Skipped. Phase D-prime is "10/11 NetSurf-internal libs landed" instead of 11/11.

**Reason:** libsvgtiny's main entry point `svgtiny_parse()` calls `dom_xml_parser_create` / `_parse_chunk` / `_completed` / `_destroy` from libdom's `bindings/xml/` subtree. We explicitly excluded `bindings/xml/` from our libdom build because it requires either **expat** OR **libxml2** -- neither is shipped in amiport. There is no way to use libsvgtiny without first porting expat (or libxml2) AND enabling libdom's XML binding.

**Implication for Phase D-prime:** acceptable. NetSurf renders HTML+CSS without inline SVG; SVG support is a polish item. ports/netsurf can ship without libsvgtiny initially. When a future phase ports expat (~30K LOC pure C, ~1 day), enable libdom's bindings/xml/ subtree and revisit libsvgtiny.

**Implication for amiga-kb:** captured as a pitfall ("libdom XML binding requires expat or libxml2 -- consumer libs that need DOM-XML APIs are blocked until expat lands").

**External libs (lib/<name>/ ports also, but later):** `libpng`, `libjpeg`, `libwebp`, `libcurl` (with `lib/amissl-sdk/` glue or HTTPS deferred). Aminet research first per `.claude/rules/use-pipeline-agents.md` "aminet-researcher / Check if a library is available on 68k" — many of these may be on Aminet already.

Each lib lives in `lib/<name>/` (NOT `lib/netsurf-deps/<name>/` — flat layout, consistent with `lib/zlib/`, `lib/libgit2/`, etc.). After all 11 NetSurf libs are present, retry original Task 15 (`make -C ports/netsurf`).

After libwapcaplet lands, reassess: is the per-library effort estimate accurate? If much slower, revisit Option 3 (pivot to a smaller browser like Lynx) or Option 4 (binary-patch the prebuilt NetSurf-gcc-7422.lha for AMMX font rendering — high-risk but might unblock the AA-text deliverable independent of the dep stack).

## Recommended next session opening

```
Read these in order:
1. docs/superpowers/handoffs/2026-05-02-netsurf-vampire-resume-bridge.md (original session 1 handoff)
2. docs/superpowers/handoffs/2026-05-02-netsurf-vampire-session-2-handoff.md (session 2 handoff)
3. docs/superpowers/plans/2026-05-02-netsurf-vampire-phase-d-prime-dep-stack.md (this file, current)
4. ~/.claude/projects/-Users-duncan-Developer-amiport/memory/project_netsurf_dep_stack_blocker.md

Then:
- Resume per-lib library-pipeline at the next Wave 1/2/3 lib not yet shipped
- Whichever lib is next, re-read the per-library task template above before dispatching agents
- Retry main NetSurf build (`make -C ports/netsurf`) only after all 11 NetSurf libs land cleanly
```
