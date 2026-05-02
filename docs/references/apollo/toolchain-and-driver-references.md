# Apollo Toolchain & Driver References

**Source:** Multiple Apollo Team and community resources, captured 2026-05-02 during NetSurf Vampire Phase 1 KB hydration.

This document catalogs:
1. **ApolloCrossDev** — community toolchain bundle (alternate path to amiport's Docker image)
2. **SAGA Driver Package** — runtime drivers for Vampire (Picasso96 RTG, AHI, SD, ethernet)
3. **apollo-core.com coding portal** — official AMMX/SAGA documentation links
4. **Browser-User-Agent workaround** for fetching apollo-core.com

## ApolloCrossDev (community toolchain bundle)

**Repo:** [github.com/WDrijver/ApolloCrossDev](https://github.com/WDrijver/ApolloCrossDev)
**Author:** Willem Drijver
**Target:** Ubuntu 24.04.3 LTS amd64 in a VM, with VS Code

### What it bundles

1. **BinUtils** (ar / as / ld / nm / objdump / ranlib)
2. **GCC compilers**: 6.50 Stable + Latest, **13.1, 13.2, 13.3, 15.2** Beta
3. **VASM Assembler** (Apollo-optimized, by Volker Barthelmann + Frank Wille)
4. **AmigaOS NDKs**: 1.3, 3.9, **3.2 (default)**
5. **Pre-built libraries**: MUI5, SDL-Base + Mixer + TTF + Image, **Vorbis, Ogg, GL, FreeType, Timidity, ZLib, JPeg, PNG**
6. **GDB-Server** running native on Apollo V4 (remote debug from VS Code on Linux)
7. C/C++/ASM **example projects** with Makefiles and remote debug enabled
8. **ApolloExplorer** file transfer utility (by ronybeck)

### Layout

```
ApolloCrossDev/
├── GCC-13.3.sh                # install script per GCC version
├── GCC-Install.sh
├── Compilers/                  # downloaded GCC builds
├── Docs/                       # Apollo and Amiga reference PDFs
├── Projects/
│   ├── ApolloDemo/             # sample project with build + remote debug
│   └── _makefiles/             # Makefile templates
└── README.md
```

### Why we don't switch amiport to ApolloCrossDev

amiport uses Docker for reproducible CI builds. ApolloCrossDev is a VM-based, VS-Code-centric developer workflow targeting Ubuntu 24.04 in a VM. Switching would mean rebuilding our pipeline, losing CI reproducibility, and forcing every contributor to install a VM.

### What we CAN crib from ApolloCrossDev

- **Their VASM install script** (`GCC-Install.sh`, etc.) — shows the exact build invocation, useful when patching `Dockerfile.bebbo-gcc13` (plan Task 1)
- **Their Makefile templates** in `Projects/_makefiles/` — likely have `register __asm()` calling convention examples
- **Their pre-built libraries** — confirm which libs (FreeType, SDL_TTF, etc.) are available; useful for cross-checking against amiport's `lib/freetype/` build
- **`Docs/` folder** — collection of Amiga/Apollo Reference Manuals (some PDFs already we have)
- **Sample asm/C projects** — concrete examples of C calling AMMX kernels via vasm-built `.o`

For Phase 1: clone the repo, copy useful asm patterns and Makefile snippets into our `ports/netsurf/` Makefile. Don't migrate our Docker workflow.

## SAGA Driver Package (`SAGADriver_3.5b1.lha`)

**Source:** Aminet `driver/other/SAGADriver_3.5b1.lha` (or Apollo Team distribution).
**Author:** Apollo Team (uploaded by Renaud Schweingruber).
**Version:** 3.5b1
**Requires:** Picasso96 (Aminet `driver/video/Picasso96.lha`) — install with uaegfx selected.

### Contents

| Path | Purpose |
|------|---------|
| `Devs/sagasd.device_2.8` | SAGA SD card device driver (V4) |
| `Devs/sagasd.device_2.5` | SAGA SD card device driver (V2) |
| `Devs/Picasso96Settings.V4` | Picasso96 video mode database for V4 |
| `Devs/Picasso96Settings.V2` | Picasso96 video mode database for V2 |
| `Devs/Monitors/vampiregfx` | Workbench-side RTG monitor activator (vampiregfx 1.57) |
| `Devs/AudioModes/ARNE` | AHI audio mode definition |
| `Libs/Maggie3D.library` | Maggie 3D unit library (texture/Z-buffer accelerator) |
| `Libs/Warp3D.library` | Warp3D wrapper |
| `Libs/maggie.library` | Lower-level Maggie unit library |
| `Libs/i2c.library` | I2C bus access (V4 RTC, etc.) |
| `L/fat95` | FAT filesystem handler |
| `C/ApolloControl` | Board info and settings utility |
| `C/ApolloFloppy` | Mount ADF as DF0/DF1 |
| `C/ApolloMap` / `C/ApolloFlash` | Kickstart map/flash |
| `C/I2Clock` | Set V4 RTC via DS3231 module |
| `C/SGDiag`, `C/SDDiag`, `C/sdcardtest`, `C/sdnettest`, `C/v2expethtest` | Diagnostics |
| `C/GiggleDisk` | Floppy emulator |
| `C/FlushSD` | Flush SD card buffers |

### Pre-configured Picasso96 video modes (V4)

From `Devs/Picasso96Settings.V4` (IFF `P96SANNO` format):

```
16:10  320x200  60Hz
 4:3   320x240  60Hz
16:10  640x400  60Hz
 4:3   640x480  60Hz
16:10  720x480  60Hz
 5:3   800x480  60Hz
16:9   960x540  50Hz
16:9   640x360  60Hz
 5:4   720x576  50Hz
16:9  1280x720  50Hz
16:9   848x480  60Hz
... (more)
```

These are the `BoardName: SAGA` modes. NetSurf will pick whichever the user has set as their default.

### Installation requirements (from .readme)

1. Flash Vampire with latest core: **GOLD2.16+** for V2, **r8900+** for Standalone/FireBird/IceDrake/Manticore
2. Install Picasso96 from Aminet (select uaegfx during install)
3. Install this driver

### Implications for amiport

The SAGA driver is the **runtime requirement** for any amiport port using Picasso96 RTG (NetSurf, future PDF viewer, SDL games on Vampire). amiport ports don't ship the driver — users install it once for their Vampire. Our `.readme` files should mention "Requires SAGADriver 3.5b1+ and Picasso96 installed on the Vampire."

The Maggie3D / Warp3D libraries open the door to a future hardware-3D port (OpenGL games on Vampire). Out of scope for NetSurf Phase 1.

## Apollo coding portal (apollo-core.com)

**Coding hub:** `http://apollo-core.com/index.htm?page=coding`

Sub-tabs:
- **Code Examples** (`tl=0`) — runnable demo projects
- **Code Snippets** (`tl=3`) — small inline samples
- **68080 CPU** (`tl=1`) — CPU-specific guide
- **SAGA Chipset** (`tl=2`) — SAGA hardware programming (this is what the user pointed me to)
- **Documentation** (`tl=4`) — links to AC68080PRM.pdf, AMMX.doc.txt, etc.

The SAGA tab embeds an iframe at `http://apollo-core.com/sagadoc/` which is a full register reference table (OCS/ECS/AGA/SAGA — DFF000 onwards), with per-register sub-pages.

### Browser-UA workaround for apollo-core.com

apollo-core.com **filters non-browser User-Agents** — Claude Code's WebFetch tool (default UA) gets ECONNREFUSED. Fetching with curl + browser UA works:

```bash
curl -sS -L --max-time 10 \
  -A "Mozilla/5.0 (Macintosh; Intel Mac OS X 14_0) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0 Safari/537.36" \
  "http://www.apollo-core.com/<page>"
```

This unblocks earlier amiport sessions where apollo-core.com was reported as down — it's not down, just bot-filtered. Worth capturing as an amiga-kb pitfall.

### Other useful URLs

- **AMMX.doc.txt:** `http://www.apollo-core.com/AMMX.doc.txt` (already ingested)
- **AC68080PRM.pdf:** `http://www.apollo-core.com/documentation/AC68080PRM.pdf` (already ingested)
- **Apollo Knowledge Forum:** `http://www.apollo-core.com/knowledge.php` (community Q&A; needs browser UA)
- **Apollo Computer downloads:** `https://www.apollo-computer.com/downloads.php` (ApolloOS distro image, etc.)

## Cross-references

- `docs/references/ammx/instruction-reference.md` — Apollo PRM AMMX (already ingested)
- `docs/references/saga/sprite-hardware.md` — SAGA sprite hardware
- `docs/references/saga/chunky-video.md` — SAGA chunky video registers
- `docs/references/vampire-sdk/headers.md` — flype44 Vampire SDK headers
- `docs/references/netsurf-mui/ammx-pattern.md` — NetSurf-MUI AMMX integration

## Discovery context

Captured 2026-05-02 during NetSurf Vampire Phase 1 KB hydration. ApolloCrossDev cloned to `/tmp/ApolloCrossDev`, SAGA driver LHA extracted to `/tmp/saga-driver/`. Browser-UA workaround discovered when user suggested "pretend you are a browser" — curl with Mozilla UA bypasses the filter.
