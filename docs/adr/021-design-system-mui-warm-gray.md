# ADR-021: Website Design System — Amiga MUI on Warm Gray

## Status

Accepted

## Date

2026-03-22

## Context

The website design system defined the visual language for amiport.platesteel.net. The original design (DESIGN.md v1) used full Workbench 3.x skeuomorphism — fake window chrome, checkered desktop, bevel borders, all-monospace Topaz font, blue/orange palette. This was replaced after design consultation revealed it felt like a novelty rather than a trustworthy tool.

The site must also work on classic Amiga browsers (IBrowse, AWeb), which handle complex CSS poorly. A dual approach — modern main site + dedicated classic page — was chosen.

## Decision

### Visual System
- **Aesthetic:** Amiga MUI (Magic User Interface) — not WB3.x skeuomorphism, not generic modern
- **Base:** Warm gray (#8C8C8C) — like an actual MUI application background
- **Palette:** Commodore warm tones — amber (#CC9933), brown (#776644), red (#BB4444). No blue anywhere.
- **Title bars:** Dark gray (#505050), centered text, no decorative gadgets
- **Bevels:** 1px MUI-style (shine #C8C8C8, shadow #606060) — not 2px WB3.x chunky bevels
- **Typography:** System sans-serif for body, monospace for code/terminal only. No web fonts.
- **Corners:** 0px border-radius everywhere (MUI is rectilinear)
- **Components:** Group frames, MUI tabs, MUI gauge, listview/drawers, beveled buttons

### Amiga Browser Compatibility
- Main site uses progressive enhancement (degrades to plain text without CSS)
- Dedicated `/amiga.html` page: HTML 3.2, table layout, <30KB, 640x480 target
- `/amiga.html` is PHP-generated from the same package data as the API

### Enforcement
- `DESIGN.md` is the source of truth for all visual decisions
- `.claude/rules/site-design.md` enforces the design system for any edits to `site/`
- Site-manager agent references DESIGN.md before making visual changes
- CLAUDE.md directs all agents to read DESIGN.md before UI work

## Consequences

### Positive

- Site looks like a real tool, not a novelty/theme park
- Warm Commodore palette is recognizably Amiga without cosplay
- System fonts = zero network requests, instant render, works on any browser
- Progressive enhancement + /amiga.html = actual classic Amiga compatibility
- Design system is documented, enforced, and consistent

### Negative

- Existing CSS (1181 lines) must be fully rewritten
- HTML structure must be refactored (window → group-frame components)
- JS class references must be updated
- The charming WB3.x skeuomorphism (boot animation, checkered desktop, window gadgets) is largely removed — boot animation survives as optional easter egg

### Neutral

- PHP API and backend logic are completely unaffected
- The design change does not affect any ported programs or the porting pipeline
- The /amiga.html page could serve as the future `amiget` CLI's package index endpoint
