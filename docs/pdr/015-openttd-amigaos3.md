# PDR-015: OpenTTD for AmigaOS 3.x

## Status

Proposed

## Date

2026-04-15

## Problem

OpenTTD is the most-requested game port for classic Amiga hardware. The last 68k port was OpenTTD 0.3.3 in 2007 (Aminet: game/strat/openttd-68k.lha). The game has evolved through 22 years of development since then -- modern OpenTTD (12.x) has a vastly improved AI framework, NewGRF support, pathfinding, and UI. No one has attempted a modern 68k port.

The amiport project now has the infrastructure to attempt it: libSDL2-amigaos3 (video/audio/input), posix-shim, bsdsocket-shim, zlib, and a mature build/test pipeline. The Vampire V2/V4 68080 accelerators provide hardware capable of running it (128 MB RAM, ~153 MIPS, SAGA RTG, hardware FPU).

## Target Users

- Vampire V2/V4 owners (primary -- 68080 at ~153 MIPS with 128 MB RAM)
- 68040/060 accelerator owners with RTG cards (secondary -- playable on small maps)
- The broader retro Amiga community (aspirational -- "OpenTTD on real Amiga hardware")

## Decision

Port OpenTTD 13.4 to AmigaOS 3.x targeting 68040+ with hardware FPU. This is the last release requiring C++17 before the C++20 jump in 14.0.

### Target Configuration

| Parameter | Value |
|-----------|-------|
| OpenTTD version | 13.4 (last C++17, released July 2023) |
| Compiler | bebbo-gcc 13.3+ (amiga13.3 branch, full C++17) |
| CPU flags | `-m68040 -m68881` (Vampire optimized) |
| Optimization | `-O0` default, per-file `-O1` after audit |
| Runtime | `-noixemul` (libnix, static linking) |
| Video/Audio/Input | libSDL2-amigaos3 |
| Threading | Disabled (`-DNO_THREADS`) |
| Exceptions | Disabled (`-fno-exceptions`) -- replace 35 try/catch sites |
| RTTI | Disabled (`-fno-rtti`) -- replace 35 dynamic_cast sites |
| Music | Null driver (no MIDI initially) |
| Network | Optional via bsdsocket-shim (Roadshow) |
| Minimum RAM | 16 MB (64x64 map), 32 MB recommended (256x256 map) |
| Display | RTG required (CyberGraphX/Picasso96/SAGA), 640x480x8 |

### Why Not an Older Version?

OpenTTD 0.7-1.0 (C++03) would avoid the C++17 toolchain upgrade but:
- The build system is worse at disabling optional features
- Missing 12 years of gameplay improvements (AI framework, cargo distribution, improved pathfinding)
- Save game incompatibility with modern versions
- The 2007 port already proved 0.3.x works -- there is no new ground broken

OpenTTD 13.4 with bebbo-gcc 13.3 (full C++17) is the right balance of modern gameplay and toolchain feasibility.

### Why Not C++20 / OpenTTD 14+?

bebbo-gcc 15.2 (amiga15.2 branch) supports C++20 but is less tested on AmigaOS. The 13.3 branch is more mature. If 13.3 proves stable, a future version bump to OpenTTD 14+ with bebbo-gcc 15.2 is straightforward.

## Rationale

### What Makes This Feasible Now

1. **SDL2 is done.** libSDL2-amigaos3 handles video (RTG), audio (AHI), input (IDCMP), and timing. Chocolate Doom already proved this path with zero patches to game source.

2. **Threading is optional.** OpenTTD's `-DNO_THREADS` flag cleanly disables all multithreading. Link graph runs synchronously, save compression runs synchronously. Slower but functional.

3. **Big-endian is first-class.** OpenTTD has a mature endianness layer (MorphOS/OS X heritage). Save games are endian-safe by design. No byte-swapping surprises.

4. **The rendering is software-only.** OpenTTD's 8bpp blitter composites sprites entirely in software onto an SDL surface. No OpenGL, no GPU. This maps directly to our SDL2 RTG backend.

5. **The GUI is self-contained.** OpenTTD draws its own widgets (buttons, scrollbars, text input) -- no native toolkit needed. No Intuition/MUI integration required.

6. **Key dependencies are solved.** zlib (already ported), SDL2 (already ported), network (bsdsocket-shim). Remaining: libpng (screenshots, optional), liblzma (save compression, optional).

### What Makes This Hard

1. **Scale.** 358K LOC of C++17 across 447 .cpp files. Largest port attempted by ~4x.

2. **C++ toolchain.** Requires upgrading from bebbo-gcc 6.5 (default Docker image) to 13.3+ branch. The Docker image `amigadev/crosstools:m68k-amigaos-gcc10` may work, or we build from the Codeberg amiga13.3 branch.

3. **Binary size.** Estimated 5-7 MB. Requires accelerated Amiga with large RAM. No stock A500/A1200.

4. **Rendering performance.** Software blitter at 640x480: 307,200 pixels/frame. At 15 fps = 4.6M pixels/second. On 68030@50MHz that is ~10 cycles/pixel -- marginal. On Vampire 68080@85MHz, comfortable. AMMX assembly for the blitter inner loop is the performance unlock for Vampire.

5. **Pathfinding CPU cost.** YAPF A* search with priority queue. ~50K iterations per train pathfind on 256x256 map. Integer math, cache-friendly. Feasible on 68040+ but will limit vehicle count.

### Compiler Flag Selection

Per cross-referencing the KB against Apollo Team primary sources (apollo-core.com, wiki.apollo-computer.com):

- **`-m68040 -m68881`** is the correct flag for Vampire (not `-m68020` as some sources suggest)
- The 68080 presents as a 68040 to AmigaOS and executes the full 68040 ISA natively
- `-m68060` should be avoided -- it removes `muls.l`/`mulu.l` which the 68080 executes at full speed
- `-m68080` is available on the bebbo 68080regs branch for direct 68080 targeting (future optimization)
- The 68080 FPU is 64-bit double precision only (no 80-bit extended) -- fine for OpenTTD which uses `double` throughout

## Implementation Phases

### Phase 1: Toolchain and C++17 Compat (1-2 weeks)

- Build or obtain bebbo-gcc 13.3 Docker image with C++17 support
- Validate C++ exceptions, RTTI, libstdc++ on AmigaOS with a test binary
- Create C++17 compat header if needed (`string_view`, `optional` aliases)
- Validate `-m68040 -m68881` codegen produces correct binaries

### Phase 2: Library Dependencies (1-2 weeks)

- Port libpng to `lib/libpng/` (pure C, well-structured, depends on zlib)
- Evaluate liblzma -- port if straightforward, or stub with LZO fallback
- Verify libSDL2-amigaos3 API coverage for OpenTTD's SDL2 usage

### Phase 3: Core Build (2-3 weeks)

- Get the OpenTTD source tree compiling with bebbo-gcc 13.3
- Start with `-DDEDICATED` (no video) to isolate engine from rendering
- Iterate on compilation errors: template instantiation, missing stdlib, codegen bugs
- Target: `openttd` binary that runs the simulation headless

### Phase 4: SDL2 Video Integration (1-2 weeks)

- Wire up the SDL2 video driver to our libSDL2-amigaos3
- Create `src/os/amigaos3/` platform layer (~150 LOC)
- Get first frame on screen (title screen rendering)
- Test input handling (mouse, keyboard via SDL2 events)

### Phase 5: Gameplay Testing (2-3 weeks)

- Load a saved game, verify simulation runs
- Test map generation (small maps first: 64x64, then 128x128)
- Fix crashes (expect alignment, stack, codegen issues)
- Verify save/load round-trip (endian safety)
- Test basic gameplay: build track, run trains, manage finances

### Phase 6: Performance Optimization (2-3 weeks)

- Profile hot paths with ReadEClock-based instrumentation
- Enable per-file `-O1` on audited scalar-only hot files
- Write AMMX assembly blitter for Vampire (sprite compositing inner loop)
- Implement dirty-rect optimization if not already in OpenTTD's SDL path
- Cap frame rate to sustainable level per hardware tier

### Phase 7: Polish and Packaging (1-2 weeks)

- Save/load configuration (OpenTTD config file, paths)
- Aminet readme, PORT.md, test suite
- Memory-checker and perf-optimizer mandatory audits
- LHA packaging, amiport site publishing
- Real hardware testing on Vampire V2 (A2000)

## Performance Expectations

| Hardware | Display | Map Size | Expected FPS | Vehicles | Verdict |
|----------|---------|----------|-------------|----------|---------|
| 68030@50MHz + RTG | 640x480x8 | 128x128 | 5-10 | ~100 | Barely playable |
| 68040@40MHz + RTG | 640x480x8 | 256x256 | 8-15 | ~200 | Playable |
| 68060@50MHz + RTG | 640x480x8 | 256x256 | 12-20 | ~300 | Good |
| Vampire V2 (68080) + SAGA | 640x480x8 | 512x512 | 15-30 | ~500 | Comfortable |
| Vampire V2 + AMMX blitter | 640x480x8 | 512x512 | 20-40 | ~500 | Target scenario |

## Memory Budget (256x256 Map)

| Component | Estimated |
|-----------|-----------|
| Binary (code + static data) | 5-7 MB |
| libstdc++ (partial) | 300-500 KB |
| libSDL2.a | 1.3 MB |
| Map tiles (65,536 x 12 bytes) | 768 KB |
| Vehicle pool (~500 vehicles x 400 bytes) | 200 KB |
| Station/Town/Industry pools | 500 KB |
| Pathfinder working set | 1-2 MB |
| Sprite cache | 2-4 MB |
| SDL surface (640x480x1) | 300 KB |
| Overhead (heap fragmentation, stack) | 2-4 MB |
| **Total** | **~14-20 MB** |

Fits comfortably in 32 MB. Vampire's 128 MB allows 1024x1024 maps.

## Dependencies

| Library | Required? | Status | Effort |
|---------|-----------|--------|--------|
| zlib | Yes | Already ported (`lib/zlib/`) | Done |
| libSDL2 | Yes | Already ported (sibling repo) | Done |
| libpng | Optional (screenshots) | Needs porting | 1 week |
| liblzma | Optional (save compression) | Needs porting or stub | 1 week |
| LZO | Optional (legacy saves) | Needs porting | 3 days |
| FreeType | No | Sprite font fallback exists | Skip |
| ICU | No | English-only acceptable | Skip |
| FluidSynth | No | Null music driver | Skip |
| Squirrel | Bundled | In OpenTTD source tree | Included |
| fmt | Bundled | In OpenTTD source tree | Included |

## Risks

1. **bebbo-gcc 13.3 codegen stability.** 447 C++ compilation units with heavy templates. Unknown how many codegen bugs lurk at `-O0` vs `-O1`. Mitigation: start at `-O0`, promote files individually.

2. **libstdc++ on AmigaOS.** Standard containers (vector, string, map) allocate aggressively from heap. No process memory cleanup on exit with `-noixemul`. Mitigation: atexit cleanup, memory-checker audit.

3. **Exception removal.** 35 try/catch sites need replacement with error-code returns or setjmp/longjmp. Some are in critical save/load paths. Mitigation: systematic audit before building.

4. **RTTI removal.** 35 dynamic_cast sites need replacement with manual type tags. Mitigation: OpenTTD's window system already has type enums.

5. **Rendering on 68030.** May be too slow for enjoyable gameplay. Mitigation: document 68040+ as minimum, Vampire as recommended.

## Success Criteria

1. OpenTTD 13.4 boots to title screen on Vampire V2 via FS-UAE
2. 256x256 map generates and plays at >= 15 fps on Vampire
3. Save/load round-trip works (big-endian safe)
4. Basic gameplay functional: build track, run trains, manage finances, place stations
5. Runs on real Vampire V2 hardware (A2000)
6. Published on amiport.platesteel.net and Aminet

## Alternatives Considered

1. **Port OpenTTD 0.7-1.0 (C++03).** Avoids toolchain upgrade but misses 12 years of gameplay. The 2007 port already proved this works. Rejected -- not ambitious enough.

2. **Port OpenTTD 13+ (C++20).** More modern but requires bebbo-gcc 15.2 which is less tested. Rejected -- too much toolchain risk for the first attempt.

3. **Write a Transport Tycoon clone from scratch.** Maximum control but years of work. Rejected -- OpenTTD exists and is excellent.

4. **Use the MorphOS port as a starting point.** MorphOS is PPC, not 68k. Different toolchain, different OS APIs. The code structure is informative but not directly reusable. Rejected as a fork base -- but worth studying for the OS abstraction layer design.
