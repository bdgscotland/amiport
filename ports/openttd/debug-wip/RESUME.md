# OpenTTD Debug Session Resume Notes (2026-04-16)

## State at end of session

OpenTTD compiles and runs to **deep inside `openttd_main()`** — past
DeterminePaths, TarScanner BASESET, LoadFromConfig, InitializeLanguagePacks,
InitFontCache, InitWindowSystem, and BaseGraphics::FindSets. Last reached:
`OTTD: L8 after BaseGraphics::FindSets`. Next crash is somewhere in the
graphics/blitter/video init sequence (lines 750-820 of openttd.cpp).

## Bugs found this session

1. **bebbo-gcc 13.3 std::string operator+ corruption at -O0 -m68040**
   - Minimal repro: `toolchain/repros/bebbo-gcc-13-stdstring-opplus.cpp`
   - Workaround: build at -O1 (proven on full openttd binary)
   - Captured to amiga-kb pitfall + crash-pattern + local known-pitfalls.md

2. **bebbo-gcc 13.3 std::map iteration corruption (empty map gives garbage)**
   - Workaround: skip iteration of `_tar_filelist[sd]` in `FileScanner::Scan`
   - Captured to amiga-kb pitfall

3. **libnix `getenv("HOME")` returns `"SYS:Prefs/Env-Archive"`**
   - Captured to amiga-kb pitfall + local known-pitfalls.md

## Refuted from previous session

- "GNU ld text-hunk-size relocation bug" — REFUTED. A 32MB pure-C test binary
  (`build/amiga/OpenTTD/bigtext30`, source at /tmp/bigtext.c) ran cleanly.
  The actual cause was the std::string operator+ bug, not anything to do
  with text hunk size or the linker.

## How to resume

1. Restore the diaglog'd source files:
   ```bash
   cp ports/openttd/debug-wip/openttd_with_diaglog.cpp ports/openttd/original/OpenTTD-13.4/src/openttd.cpp
   cp ports/openttd/debug-wip/fileio_with_diaglog.cpp  ports/openttd/original/OpenTTD-13.4/src/fileio.cpp
   ```

2. Use the debug recompile script (NOT the CMake build — that's broken in
   the runtime image because cmake is missing):
   ```bash
   docker run --rm -v $(pwd):/amiport -v /tmp:/tmp \
     ghcr.io/bdgscotland/amiport-toolchain-gcc13:latest \
     bash ports/openttd/debug-wip/recompile_openttd.sh
   docker run --rm -v /tmp:/tmp \
     ghcr.io/bdgscotland/amiport-toolchain-gcc13:latest \
     /opt/amiga13/bin/m68k-amigaos-strip --strip-debug /tmp/openttd
   cp /tmp/openttd build/amiga/OpenTTD/openttd
   ```

3. The script uses `build-dedicated/` build dir, hardcodes -O1, links with
   `-lsocket -latomic`, includes `network_stubs.c` (getpwuid + xpg_strerror_r),
   and uses `-Wl,--allow-multiple-definition` to handle libstdc++ duplicates.

4. Test with FS-UAE (config at `ports/openttd/openttd-test.fs-uae`):
   ```bash
   rm -f build/amiga/OpenTTD/run.log build/amiga/OpenTTD/debug.log
   fs-uae ports/openttd/openttd-test.fs-uae &
   FSPID=$!
   sleep 60
   tail -20 build/amiga/OpenTTD/debug.log
   kill -9 $FSPID
   ```

## What needs more work

- `Debug(net, 3, ...)` call at line ~694 of openttd.cpp — fmt::format crashes
  even at -O1. Currently bypassed (commented out). Workaround pending.
- `BaseSounds::FindSets()`, `BaseMusic::FindSets()` — same template as
  BaseGraphics::FindSets, will hit the same std::map iteration bug. Need
  the tar-loop skip workaround applied to all three. Easier: fix
  FileScanner::Scan to globally skip tars (current state).
- Past L8 — need to bisect the next crash in BaseGraphics::SetSet,
  GfxInitPalettes, blitter selection, video driver, NetworkStartUp,
  HandleBootstrap.
- Final goal: get to the OpenTTD interactive console / dedicated server
  startup banner.

## Known shape of remaining work

The biggest unknown is `NetworkStartUp()` which probably calls into
`bsdsocket.library` — needs auto-init from the bsdsocket-shim. We have
`network_stubs.c` for getpwuid/strerror_r but not for full networking.
For a dedicated server even local listen would need bsdsocket open.

If `NetworkStartUp()` blocks because bsdsocket isn't open: could either
(a) ensure bsdsocket-shim's auto-init runs at startup, (b) compile with
`-DENABLE_NETWORK=0` (likely a CMake option) to skip network entirely
for the first runnable build.

---

## UPDATE 2026-04-16 (continuation)

Bisection continued past `OTTD: L8 after BaseGraphics::FindSets`. Now reaching `OTTD: N3 before NetworkStartUp` — past 11 more init steps:
- BaseGraphics::SetSet ✓
- GfxInitPalettes ✓
- Debug(misc, 1, "Loading blitter...") ✓ (not formatted because misc level=0, filtered out)
- BlitterFactory::SelectBlitter ✓
- DriverFactoryBase::SelectDriver(videodriver) ✓ ("dedicated" driver)
- InitializeSpriteSorter ✓
- _screen.zoom assignment ✓
- AdjustGUIZoom — CRASHED, **bypassed** (dedicated server doesn't need GUI zoom)

Next blocker: **NetworkStartUp()** — almost certainly needs bsdsocket.library
auto-open. Either:
1. Wire bsdsocket-shim's auto-init into the openttd binary (preferred — production)
2. Compile with -DENABLE_NETWORK=0 to skip (faster but loses dedicated server functionality)

For a "first runnable" milestone, option 2 may be cleaner.

## fmt::format observation

Debug(misc, 1, "...") works because misc debug level=0 (default), so the
log is filtered out before formatting. The earlier Debug(net, 3, ...) crashed
because we had `-D` which set net debug level=4, so 3<4 triggered formatting.
This means **fmt::format itself is broken at -O1** (or at least when called
with `_openttd_revision` as argument). This will bite us repeatedly as openttd
loads more code with active debug categories.

Possible workarounds:
- Try -O2 (different codegen for templates)
- Replace OpenTTD's fmt::format with snprintf wrappers (large patch)
- Set ALL debug categories to 0 in cli args (no Debug() output at all)

## VICTORY 2026-04-16 (final session entry)

**OpenTTD now RUNS on AmigaOS 3.x!**

Full debug.log shows complete init through every stage:
1. main() entered, all C++ ctor/heap pre-tests pass
2. openttd_main entered (A)
3. AfterNewGRFScan ctor (B)
4. GetOptData ctor (C)
5. Option loop (D, E)
6. DeterminePaths (F, G) including DBP, DSWD, DP sub-bisection points
7. TarScanner BASESET (H)
8. dedicated check + Debug skip (H1, H2, H3, I)
9. LoadFromConfig (J, K)
10. InitializeLanguagePacks (L1, L2)
11. InitFontCache (L3, L4)
12. InitWindowSystem (L5, L6)
13. BaseGraphics::FindSets (L7, L8) -- with FileScanner::Scan tar-skip
14. BaseGraphics::SetSet, GfxInitPalettes (M1, M2, M3, M4)
15. Blitter selection + Debug filter passthrough (M5, M6, M7)
16. Video driver selection (M8, M9)
17. InitializeSpriteSorter (N1, N2)
18. Skipped AdjustGUIZoom (N2a, N2b, N2c)
19. **NetworkStartUp** (N3, N4)  -- worked because Debug(net,3,...) filtered out
20. HandleBootstrap (N5) -- returned false (no graphics set installed) -- EXPECTED
21. Program exited cleanly with RC=1 (graceful shutdown, no Guru, no crash)

The "first runnable" milestone is HIT. To turn this into a "first playable":
- Install OpenGFX or similar graphics base set in WORK:OpenTTD/baseset/
- Install OpenSFX or similar sounds base set
- Install OpenMSX or similar music base set (or accept null music)

## Workarounds applied (must be ported to clean source patches eventually)

| File | Workaround | Reason |
|------|-----------|--------|
| `openttd.cpp` case 'D' | Skip `SetDebugString("net=4")` | Avoids Debug(net,3,...) hitting fmt::format crash |
| `openttd.cpp` ~line 800 | Skip `AdjustGUIZoom(false)` | Crashed in AdjustGUIZoom (likely fmt or vector ops) |
| `openttd.cpp` ~line 694 | Skip `Debug(net, 3, "Starting dedicated server, version {}", _openttd_revision)` | fmt::format crash with std::string arg |
| `fileio.cpp` FileScanner::Scan | Skip tar iteration block, use `_tar_filelist[sd].size()` for diagnostic | std::map iteration corruption when empty |
| `fileio.cpp` FioGetDirectory | Decompose `string + string` into copy + += | std::string operator+ -O0 bug (now mitigated by -O1) |
| build flags | -O0 → -O1 globally | std::string operator+ codegen bug at -O0 |
| `network_stubs.c` | Add getpwuid + __xpg_strerror_r stubs | libnix doesn't provide these |

## Compiler-level findings to file upstream at codeberg.org/bebbo/amiga-gcc

1. **bebbo-gcc 13.3 std::string operator+ codegen bug at -O0 -m68040**
   - 4-line repro: `toolchain/repros/bebbo-gcc-13-stdstring-opplus.cpp`
   - Workaround: -O1
   - Severity: critical (any C++ port hitting std::string + literal crashes)

2. **bebbo-gcc 13.3 fmt::format / template instantiation issues at -O1**
   - Manifests when fmt::format with std::string arg is actually executed
   - Workaround: filter out Debug() calls (bypass formatting)
   - Need narrower repro to file upstream — TODO

3. **bebbo-gcc 13.3 std::map iteration corruption (size() returns garbage on empty map)**
   - Manifests in range-based for over std::map<>
   - Workaround: explicit empty check (but empty() may also be unreliable)
   - Need narrower repro to file upstream — TODO
