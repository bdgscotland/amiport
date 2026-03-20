# CIDR-002: GUI Application Porting via Intuition/MUI

## Status

Exploring

## Date

2026-03-19

## Idea

Extend the porting pipeline beyond CLI utilities to support GUI applications by mapping common UI frameworks (ncurses, GTK concepts) to Amiga Intuition or MUI (Magic User Interface).

## Motivation

The most impactful ports for the Amiga community would be GUI applications — text editors, file managers, image viewers. But GUI porting is fundamentally harder than CLI because there's no simple function-level mapping.

## Open Questions

- Is there a meaningful subset of ncurses that maps to Amiga console?
- Could we generate MUI interfaces from abstract UI descriptions?
- How do we handle event loops (Amiga uses message ports, not poll/select)?
- What about screen modes, fonts, and Amiga-specific display constraints?

## Early Thinking

Start with console UI (ncurses → Amiga console device). This is tractable because Amiga's console.device supports ANSI escape codes. Full GUI porting would need a higher-level abstraction — possibly Claude generates MUI layouts from semantic descriptions of what the UI should do.

## Next Steps

- Research existing ncurses-to-Amiga ports (there are some)
- Investigate MUI's programmatic API for auto-generation potential
- Consider this only after CLI porting is solid
