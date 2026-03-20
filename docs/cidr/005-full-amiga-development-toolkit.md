# CIDR-005: Expand amiport into a Full Amiga Development Toolkit

## Status

Exploring

## Date

2026-03-20

## Idea

Expand amiport from a POSIX-to-Amiga porting toolkit into a fully agentic Amiga development platform. Claude should be able to write native Amiga software from scratch, port GUI applications, work with custom hardware, and serve as a comprehensive Amiga programming assistant backed by authoritative reference material.

## Motivation

The current scope — porting CLI tools using POSIX shim wrappers — is a solid foundation but covers only a fraction of Amiga development. The Amiga community needs:

- Ports of GUI applications (text editors, image viewers, file managers)
- New native Amiga software written idiomatically (not just ported POSIX code)
- Hardware-aware programming (graphics, audio, input devices)
- Maintenance and modernisation of existing Amiga codebases
- A knowledgeable assistant that understands the full Amiga programming model

Claude already has general knowledge of Amiga programming, but it's shallow and sometimes wrong on specifics (register names, library call signatures, structure field offsets). Embedding authoritative reference material from the RKMs and NDK as skill references would make the agents precise rather than approximate.

## Expansion Phases

### Phase 1: Reference Foundation (no new skills yet)

Populate `docs/references/` with structured, agent-consumable reference material extracted from authoritative sources. This benefits the existing pipeline immediately (better shim implementations, more accurate analysis) and enables all future phases.

**Sources:**

| Source | Content | Format | Status |
|--------|---------|--------|--------|
| NDK 3.2 R4 Autodocs | Function-by-function API docs for every library | Already fetchable via `make fetch-ndk` | Parse into markdown |
| RKM: Libraries (3rd Ed.) | exec, dos, intuition, graphics, utility, etc. | Archive.org PDF | Extract key chapters |
| RKM: Devices (3rd Ed.) | timer, console, serial, parallel, input devices | Archive.org PDF | Extract key chapters |
| RKM: Includes & Autodocs | Complete header/struct definitions | Archive.org PDF | Cross-reference with NDK |
| AmigaDOS Manual (3rd Ed.) | Filesystem, CLI, DOS packet interface | Archive.org PDF | Extract for dos.library depth |
| Hardware Reference Manual | Custom chips, registers, DMA, copper lists | Archive.org PDF | Phase 3 only |
| Amiga Intern (1992) | Deep system internals | Archive.org PDF | Phase 3 only |

**Reference docs to create:**

| Document | Location | Covers |
|----------|----------|--------|
| `exec-library-reference.md` | `docs/references/` | Tasks, signals, memory, message ports, libraries, interrupts, semaphores |
| `dos-library-reference.md` | `docs/references/` | File I/O, locks, directory scanning, pattern matching, date handling, CLI/Shell |
| `intuition-reference.md` | `docs/references/` | Screens, windows, menus, gadgets, IDCMP messages, IntuiMessages |
| `graphics-reference.md` | `docs/references/` | RastPort, drawing primitives, layers, bitmaps, sprites, ViewPort, display modes |
| `gadtools-reference.md` | `docs/references/` | High-level gadgets (listview, cycle, slider, etc.), menus |
| `devices-reference.md` | `docs/references/` | timer.device, console.device, input.device, serial/parallel |
| `boopsi-reference.md` | `docs/references/` | Object-oriented gadget system, custom classes, method dispatch |
| `amigados-structures.md` | `docs/references/` | Key data structures (Task, Process, MsgPort, Message, FileInfoBlock, etc.) |
| `hardware-reference.md` | `docs/references/` | Custom chip registers, copper, blitter, audio hardware (Phase 3) |
| `coding-standards.md` | `docs/references/` | Amiga coding conventions, library usage patterns, resource tracking |

### Phase 2: GUI Porting (graduates CIDR-002)

**New skills:**
- `/port-gui` — Orchestrate GUI application porting
- `/design-gui` — Generate Intuition/GadTools/MUI interface from a description

**New agents:**
- `gui-designer` — Maps abstract UI concepts to Amiga Intuition/GadTools/MUI
- `console-mapper` — Maps ncurses/termcap to Amiga console.device ANSI sequences

**New shim/library:**
- `lib/console-shim/` — ncurses-compatible wrappers over console.device
- `lib/gui-bridge/` — Abstraction layer for Intuition/GadTools/MUI

**Reference material needed:**
- `intuition-reference.md` — Screens, windows, IDCMP event loop, menus, requesters
- `gadtools-reference.md` — High-level gadgets for forms and dialogs
- `graphics-reference.md` — Drawing, text rendering, bitmaps
- `console-device-reference.md` — ANSI escape sequences, raw/cooked modes

**Testing:**
- FS-UAE automated testing (graduates CIDR-004)
- Screenshot comparison for GUI output validation

### Phase 3: Native Amiga Development

**New skills:**
- `/write-amiga` — Write new native Amiga software from a specification
- `/explain-amiga` — Explain Amiga system concepts, debug Amiga code
- `/modernise-amiga` — Update old Amiga source code (pre-2.0 → 3.x, fix deprecated APIs)

**New agents:**
- `amiga-developer` — General Amiga programming assistant with deep system knowledge
- `hardware-programmer` — Copper lists, blitter operations, sprite multiplexing, audio programming

**Reference material needed:**
- `hardware-reference.md` — Custom chip registers, DMA timing, copper instructions
- `coding-standards.md` — Idiomatic Amiga programming patterns
- MUI reference (if targeting MUI applications)
- AHI audio reference (for modern audio programming)
- Picasso96/CyberGraphX reference (for RTG graphics)

### Phase 4: Full Ecosystem

- Agent teams for large projects (graduates CIDR-001)
- Community port registry (graduates CIDR-003)
- ADF/LHA generation for distribution
- Support for AmigaOS 4.x (PPC) and MorphOS as secondary targets
- Integration with real hardware via serial/network transfer

## Open Questions

- **Copyright**: The RKMs are freely readable on Archive.org but are copyrighted works. What level of extraction/summarisation is appropriate for reference docs? Autodocs from the NDK are freely distributable. Function signatures and struct definitions are factual. Prose descriptions should be rewritten, not copied.
- **Scope control**: How do we prevent this from becoming an unfocused "everything Amiga" project? Each phase should have clear graduation criteria.
- **MUI vs Intuition**: MUI is far more capable but is third-party (now open-source). GadTools is OS-native but limited. Which do we target first?
- **Which RKM chapters first?** Priority should follow what the pipeline needs: exec.library and dos.library depth first (helps existing shim work), then intuition and graphics (for Phase 2).
- **NDK autodoc parsing**: The NDK autodocs are in a structured text format. Can we write a parser to convert them into markdown reference docs automatically?

## Early Thinking

The key insight is that **reference material is the foundation** — every other expansion (GUI porting, native development, hardware programming) depends on Claude having precise, authoritative knowledge of AmigaOS APIs. The current `amiga-api-reference.md` covers ~30 functions. The full NDK autodocs cover thousands. Bridging this gap is the single highest-leverage investment.

The approach should be:
1. Start with NDK autodoc parsing — automated, comprehensive, already freely distributable
2. Supplement with hand-written reference docs for the areas where autodocs are insufficient (system concepts, patterns, examples)
3. Each new skill/agent added should reference specific docs, not rely on Claude's training data

This keeps amiport's core strength: Claude doesn't guess at Amiga APIs, it looks them up in authoritative references.

## Next Steps

1. **Parse NDK autodocs** into `docs/references/ndk/` — write a script that converts the structured autodoc format into per-library markdown files
2. **Write exec-library-reference.md** and **dos-library-reference.md** — supplement autodocs with patterns, examples, and gotchas from the RKMs
3. **Prototype `/explain-amiga`** — a simple skill that answers Amiga programming questions using the reference docs, to validate the reference material is sufficient
4. **Define Phase 2 graduation criteria** — what does "ready for GUI porting" look like?
5. **Evaluate MUI vs GadTools** as the primary GUI target
