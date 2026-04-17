# ADR-011: Beyond CLI — Expanding Port Target Categories

## Status

Accepted (amended 2026-04-15 per [PDR-014](../pdr/014-fold-sdl-games-into-amiport.md) — Category 5 reassigned to SDL Graphical, native GUI apps renumbered to Category 6)

## Date

2026-03-20 (original) / 2026-04-15 (amended)

## Context

amiport was built for porting POSIX CLI utilities (wc, head, diff, cal). The pipeline (analyze → transform → build → test) and the tiered shim model (ADR-008) work well for this class of software. But the Amiga community needs more than coreutils — they want interactive console programs (less, vim), network tools (curl, irc), and scripting interpreters (Lua, bc).

Each category has different porting challenges:

1. **CLI tools** (current scope) — Pure POSIX function mapping. Solved by posix-shim/emu.
2. **Scripting interpreters** — Mostly portable C, but may use dlopen, longjmp/setjmp, or platform-specific tricks. Usually achievable with the existing pipeline plus minor shim additions.
3. **Console UI apps** — Use ncurses/termcap for screen control. Requires a terminal abstraction layer mapping to Amiga console.device.
4. **Network apps** — Use BSD sockets. Requires bsdsocket.library integration (ADR-010).
5. **SDL graphical apps** (added 2026-04-15) — Use SDL2 for video/audio/input. Require libSDL2-amigaos3 (PDR-008, delivered) + RTG (CyberGraphX / Picasso96) or AGA (chunky-to-planar). The practical path to graphical programs on classic AmigaOS 3.x.
6. **Native GUI apps** — Use platform-specific toolkits (Intuition / GadTools / MUI). Fundamentally creative, not mechanical. Still future.

### Category 5 reassignment (2026-04-15)

The original 2026-03-20 ADR assigned "GUI apps" to Category 5 as a future placeholder. Between then and 2026-04-15, libSDL2-amigaos3 shipped to v0.7.0 with four working game ports (1oom, Chocolate Doom, Julius, Celeste Classic) — SDL2 became the practical route to graphical programs on 68k AmigaOS 3.x, ahead of any native-GUI porting work. PDR-014 folds SDL game ports into amiport, so the practically-delivered capability claims Category 5 and the native-GUI category bumps to 6. Note: the two are distinct in kind — SDL ports rely on a cross-platform abstraction layer; native GUI ports would use Amiga-specific toolkits directly — and nothing prevents both categories from coexisting in the long run.

## Decision

Expand amiport to support five port categories, in this priority order:

### Category 1: CLI tools (existing)
No changes needed. The posix-shim/emu pipeline handles this.

### Category 2: Scripting interpreters
Use the existing pipeline. Add shims as needed (e.g., `dlopen` stubs, `setlocale` stubs). Port Lua as the proof-of-concept to validate that the existing infrastructure works for larger, non-trivial codebases.

### Category 3: Console UI apps
Add `lib/console-shim/` (ADR-009) providing a subset of ncurses mapped to Amiga console.device ANSI escape sequences. The pipeline gains awareness of console UI programs and recommends FS-UAE testing instead of vamos-only.

### Category 4: Network apps
Add `lib/bsdsocket-shim/` (ADR-010) providing thin wrappers that handle bsdsocket.library open/close lifecycle and the close()→CloseSocket(), select()→WaitSelect() mappings. The analyzer gains awareness of socket dependencies.

### Category 5: SDL graphical apps (amended 2026-04-15, see PDR-014)

Consume libSDL2-amigaos3 (external sibling repo `bdgscotland/libSDL2-amigaos3`, currently v0.7.0) as a build-time dependency. Port candidates are 2D SDL2 games, emulators, image viewers, and any graphical program that uses SDL2's video/audio/input/timer/threading subsystems.

**Runtime requirements:** AmigaOS 3.x, 68020+, and either a CyberGraphX/Picasso96 RTG card (primary path) or the AGA chipset with chunky-to-planar conversion (fallback path, slower). Audio via Paula (8-bit mono) or AHI (when available).

**Pipeline changes from Categories 1-4:**
- `build-manager` gains an SDL-aware build profile that locates the libSDL2-amigaos3 SDK, adds `-I<sdk>/include`, `-L<sdk>/lib -lSDL2`, and pins the SDK version in the port Makefile.
- `test-designer` gains a "visual" mode that produces FS-UAE screenshot-capture tests against reference PNGs. Builds on the ADR-024/025 visual test infrastructure but uses the RTG framebuffer instead of ConUnit reads. Where the existing infra doesn't fit, a successor ADR (ADR-027) will specify the SDL visual test capture approach.
- `analyze-source` learns to detect `#include <SDL.h>` / `SDL_*` symbol usage and recommend Category 5.
- `catalog.json` gains `category: "sdl-graphical"` as a valid value.
- Existing amiport shims (`lib/posix-shim/`, `lib/bsdsocket-shim/`, `lib/zlib/`, `lib/libtommath/`, `lib/libtomcrypt/`, `lib/libgit2/`, etc.) remain available to SDL ports that need file I/O, compression, networking, crypto, or VCS.

**Non-scope:** libSDL2-amigaos3 itself is NOT amiport-managed. It lives in its own repo with its own release cycle. amiport consumes it; amiport does not transform or maintain it (per PDR-014's rationale).

### Category 6: Native GUI apps (future — see CIDR-002, CIDR-005)

Not yet in scope for implementation. Requires a gui-designer agent and fundamentally different transformation approach for programs that target Intuition / GadTools / MUI directly. Keep exploring via CIDRs. Distinct from Category 5 — SDL ports abstract over graphics via a cross-platform layer; native GUI ports would use Amiga-specific toolkits directly.

### Pipeline impact

The `/analyze-source` skill should classify the port target category as part of its report:

```json
{
  "category": "console-ui",
  "rationale": "Uses ncurses (initscr, endwin, mvaddch, getch)",
  "recommended_shims": ["console-shim"],
  "test_strategy": "fs-uae"
}
```

The `/port-project` orchestrator should adapt its strategy based on category.

## Consequences

### Positive

- Clear scope expansion path without losing focus on CLI tools
- Each category adds to the toolkit incrementally
- Scripting interpreters can be attempted immediately with existing infrastructure
- Console and network shims are self-contained libraries that don't affect existing ports

### Negative

- More shim libraries to maintain (console-shim, bsdsocket-shim)
- Console UI testing requires FS-UAE (heavier than vamos)
- Network testing requires a TCP/IP stack in the emulator environment
- Risk of scope creep — must resist jumping to GUI (Category 5) before Categories 2-4 are solid

### Neutral

- The tiered model (ADR-008) still applies: each new shim library is Tier 1 or Tier 2 depending on fidelity
- The existing agents (source-analyzer, code-transformer, build-manager) need minor updates to recognize new patterns, not wholesale replacement
