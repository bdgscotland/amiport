# ADR-018: ADCD Reference Knowledge Base as Agent Context

## Status

Accepted

## Date

2026-03-21

## Context

AmigaOS is a niche platform. LLM training data contains fragmented, often incorrect information about AmigaOS APIs — function signatures are wrong, register conventions are hallucinated, library base handling is confused with modern shared library conventions. When agents generate AmigaOS code from training data alone, the result compiles but crashes at runtime due to incorrect API usage.

Commodore's Amiga Developer CD (ADCD) 2.1 is the authoritative reference for AmigaOS programming. It contains complete Autodocs (API reference), Amiga Mail articles (usage patterns), library descriptions, device driver documentation, and hardware reference. However, it was published as AmigaGuide hypertext — not accessible to agents.

Three approaches were considered:

1. **Rely on training data** — no reference docs, let agents use what they know. Fast but unreliable for a niche platform.
2. **Web search at runtime** — agents fetch AmigaOS docs from the internet as needed. Slow, unreliable (sites go down), and sources vary in accuracy.
3. **Embed authoritative reference docs** — convert ADCD to markdown, mount into agent context at dispatch time. One-time cost, always available, always correct.

## Decision

Convert the complete ADCD 2.1 to agent-optimized markdown and store it at `docs/references/adcd/`. The conversion pipeline (`make scrape-adcd`) extracts all content from AmigaGuide format, preserves cross-references as markdown links, and organizes by topic:

- `FUNCTIONS.md` — All Autodocs, one entry per function with signature, description, inputs, outputs, and see-also
- `TYPES.md` — All type definitions and structures
- `Libraries/` — Per-library documentation (exec, dos, intuition, etc.)
- `Devices/` — Device driver documentation (trackdisk, serial, timer, etc.)
- `Hardware/` — Custom chip registers and hardware programming
- `AmigaMail/` — Technical articles with code examples

Agent definitions in `.claude/agents/` reference these paths directly. The code-transformer agent gets `FUNCTIONS.md` and `TYPES.md` for API correctness. The hardware-expert agent gets `Hardware/` for chipset validation. The build-manager agent gets library-specific docs when resolving linker errors.

This is supplemented by hand-written references for areas ADCD covers poorly:
- `docs/references/68k-hardware.md` — CPU variants, memory map, addressing modes (maintained by hardware-expert agent)
- `docs/references/crash-patterns.md` — Crash signatures and fix patterns (maintained by debug-agent)
- `docs/references/newlib-availability.md` — What libnix provides vs what needs shimming

## Consequences

### Positive

- Agents produce correct AmigaOS API calls on the first attempt — function signatures, register conventions, and library base handling are verified against Commodore's own documentation
- The reference is always available, never stale (ADCD 2.1 is the final version), and costs nothing per query
- Cross-references between functions, types, and libraries help agents understand API relationships
- Supplements training data rather than replacing it — agents still use general knowledge but verify specifics

### Negative

- The ADCD conversion is ~3,600 pages of markdown, adding to repo size (~15MB)
- `make scrape-adcd` takes ~20 minutes and requires the original ADCD AmigaGuide files
- ADCD 2.1 documents AmigaOS 3.1 — functions added in 3.5/3.9/4.x are not covered
- Agent context windows are consumed by reference material, reducing space for code and conversation

### Neutral

- The markdown format is human-readable — developers can browse `docs/references/adcd/` directly
- ADCD is Commodore's copyrighted work; the converted markdown is a derivative for internal agent use
- Hardware-specific gaps (CPU details, chipset quirks) are filled by the separate `68k-hardware.md` reference maintained by the hardware-expert agent
