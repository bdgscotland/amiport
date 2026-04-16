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
