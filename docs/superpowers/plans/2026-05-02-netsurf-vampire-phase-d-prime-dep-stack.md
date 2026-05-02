# NetSurf Vampire Phase D-prime: Cross-compile the netsurf-buildsystem dep stack

**Status:** Plan addendum — inserted between Tasks 14 and 15 of the original Phase 1 plan after Task 15's first build attempt revealed the dep-stack blocker.
**Date:** 2026-05-02
**Reason:** Original spec assumed NetSurf "just builds". Reality: NetSurf needs ~10 NetSurf-specific libraries (libcss/libdom/libhubbub/...) cross-compiled before main build can proceed. Plus external libs (libcurl, libpng, libjpeg, libwebp).
**Decision:** User chose cross-compile path (vs binary-patch / pivot / pause). Estimated +2-4 weeks of focused work. Highest reusability — becomes the foundation for future browser/parser/XML/RSS ports.

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

## Recommended next session opening

```
Read these in order:
1. docs/superpowers/handoffs/2026-05-02-netsurf-vampire-resume-bridge.md (original session 1 handoff)
2. docs/superpowers/specs/2026-05-02-netsurf-vampire-text-rendering-design.md (original spec)
3. docs/superpowers/plans/2026-05-02-netsurf-vampire-phase-d-prime-dep-stack.md (this file)
4. ~/.claude/projects/-Users-duncan-Developer-amiport/memory/project_netsurf_dep_stack_blocker.md

Then:
- Pick up at Phase D-prime Task A (libwapcaplet) — smallest, no deps, clean validation
- After A lands cleanly, reassess: is the per-library effort estimate accurate?
- If yes, tackle B-J in order. If much slower, revisit option 3 (pivot) before sinking
  another week.
```
