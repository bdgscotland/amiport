# Profiler Checkpoint 002: Fix stderr Issue and Rebuild

**Date:** 2026-04-17  
**Status:** Binary staged, ready for FS-UAE test

## Root Cause of Empty run.log

The previous session's profile dump **went to console, not to the log file**. The issue was:

1. `amiport_profile_summary()` wrote to **stderr** (`fprintf(stderr, ...)`), not stdout
2. AmigaDOS `>>WORK:OpenTTD-SDL2/run.log` only captures **stdout**
3. Profile dump happened but was invisible (went to the non-captured console window)

## Fixes Applied

### 1. Changed profile.c dump destination to stdout
- Replaced all `fprintf(stderr, ...)` with `printf(...)` + `fflush(stdout)` after every line
- Added explicit `fflush(stdout)` after every row to defeat libnix's 8KB internal buffer
- Affects both `amiport_profile_summary()` and the new `amiport_profile_dump()`

### 2. Added periodic dump function
- `amiport_profile_dump()` — non-destructive snapshot that can be called from main loop
- Dumps every 100 frames in `GameLoop()` via `if ((gl_count % 100) == 0 && gl_count > 0)`
- Same format as summary but does NOT clean up timer device resources

### 3. Instrumented three OpenTTD SDL2 files

Created profiled versions in `ports/openttd/debug-wip/`:
- `openttd_profiled.cpp` — instruments `GameLoop()` with BEGIN/END + periodic dump every 100 frames, adds init + atexit in `openttd_main()`
- `sdl2_v_profiled.cpp` — instruments `LoopOnce()` video driver main loop with BEGIN/END
- `genworld_profiled.cpp` — instruments `GenerateWorld()` with BEGIN/END (if world gen is triggered)

### 4. Added startup confirmation
- `printf("AMIPORT_PROFILE active: profiler initialized\n")` at first entry point in `openttd_main()`
- `printf("AMIPORT_PROFILE initialized in GameLoop\n")` on first GameLoop iteration
- These prove `-DAMIPORT_PROFILE` compiled in and the profiler is live

### 5. Updated recompile script
- `recompile_openttd_sdl2.sh` now compiles the profiled versions with `-DAMIPORT_PROFILE`
- Links `lib/posix-shim/src/profile.o` into the final binary
- Separate loop for profiled files vs non-profiled files

## Binary Details

- **Path:** `/Users/duncan/Developer/amiport/build/amiga/OpenTTD-SDL2/openttd-sdl2`
- **Size:** 21 MB (stripped)
- **Instrumentation:** 3 functions (GameLoop, SDL_LoopOnce, GenerateWorld)
- **Dump trigger:** Every 100 frames (approx every 3-5 seconds at 20-30 FPS)

## Next Steps

**User should launch FS-UAE:**
1. Launch `fs-uae ports/openttd/openttd-sdl2.fs-uae`
2. Let it run for 30-60 seconds at title menu or in-game
3. Close FS-UAE normally
4. Check `build/amiga/OpenTTD-SDL2/run.log` for profile snapshots

Expected output:
```
OPENTTD-SDL2 START
AMIPORT_PROFILE active: profiler initialized
... (lots of diaglog lines)
AMIPORT_PROFILE initialized in GameLoop
... (more diaglog lines)

=== amiport profiler SNAPSHOT (709379 Hz) ===
Function                Calls  Total(ms)  Avg(us)  Max(us)     %
GameLoop                  100       ...       ...      ...    ...
SDL_LoopOnce              100       ...       ...      ...    ...
...
Total measured: N ms

=== amiport profiler SNAPSHOT (709379 Hz) ===
...
```

If run.log still only contains "OPENTTD-SDL2 START", fallback diagnosis:
- libnix stdio buffer not flushed before close → add explicit `fflush(NULL)` in cleanup
- `gl_count` never reaches 100 (frozen before first dump) → lower threshold to 10 frames
- AMIPORT_PROFILE define not propagating → verify with grep of .o files

## Files Modified

- `lib/posix-shim/src/profile.c` — stderr→stdout + fflush after every line
- `lib/posix-shim/include/amiport/profile.h` — added `amiport_profile_dump()` declaration
- `ports/openttd/debug-wip/openttd_profiled.cpp` — instrumented GameLoop + init
- `ports/openttd/debug-wip/sdl2_v_profiled.cpp` — instrumented LoopOnce
- `ports/openttd/debug-wip/genworld_profiled.cpp` — instrumented GenerateWorld
- `ports/openttd/debug-wip/recompile_openttd_sdl2.sh` — compile profiled versions with -DAMIPORT_PROFILE + link profile.o

Turn count: 38/80
