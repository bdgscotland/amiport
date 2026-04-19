# OpenTTD SDL2 Profiling - Checkpoint 1

## Status: READY FOR USER TESTING

Profiled binary built and staged at: `/Users/duncan/Developer/amiport/build/amiga/OpenTTD-SDL2/openttd-sdl2`

## Instrumented Functions

1. **VideoDriver::Tick()** - Full frame timing (entry to exit)
2. **VideoDriver_SDL_Default::Paint()** - SDL2 rendering backend
3. **UpdateWindows()** - Window system update pass

## Build Configuration

- `-DAMIPORT_PROFILE` added to CMake flags
- Profiled source files in `ports/openttd/debug-wip/`:
  - `video_driver_profiled.cpp`
  - `sdl2_default_v_profiled.cpp`
  - `window_profiled.cpp`
  - `openttd_profiled.cpp` (init + atexit summary)
- Linked with `/amiport/lib/posix-shim/src/profile.o`

## Profile Output

Will print to `build/amiga/OpenTTD-SDL2/run.log` via stdout redirect in User-Startup.

Expected format:
```
=== amiport profiler (709379 Hz) ===
Function                Calls  Total(ms)  Avg(us)  Max(us)     %
VideoDriver::Tick       120      12000     100000    150000   100%
...
```

## Next Steps

1. User launches FS-UAE via `make emu` (using `ports/openttd/openttd-sdl2.fs-uae`)
2. User holds on title menu for 30+ seconds
3. User closes OpenTTD
4. Read `build/amiga/OpenTTD-SDL2/run.log` for profile summary
5. Identify hotspot
6. Instrument subfunctions if needed (2nd pass)
