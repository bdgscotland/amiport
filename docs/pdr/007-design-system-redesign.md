# PDR-007: Website Design System Redesign — MUI on Warm Gray

## Status

Accepted

## Date

2026-03-22

## Problem

The amiport website used a full Workbench 3.x skeuomorphic design (fake window chrome, checkered background, bevel borders, all-monospace Topaz font, blue/orange palette). While authentic, it read as a novelty — fun for 5 seconds, then felt like a theme park rather than a tool people trust. The all-monospace typography was hard to read. The gray-on-checkered palette was oppressive. The fake window gadgets (close/resize dots) were decorative clutter.

Additionally, the site needed to be usable on actual classic Amiga browsers (IBrowse, AWeb), which the complex skeuomorphic CSS actually degraded badly on.

## Target Users

1. **Amiga hobbyists** — need to discover and download ports. Want something that feels Amiga but is usable.
2. **Retro computing enthusiasts** — visiting to see the AI porting pipeline. First impression matters.
3. **Classic Amiga browser users** — browsing from actual hardware. Need a lightweight, compatible page.

## Decision

Replace the WB3.x skeuomorphic design with an **Amiga MUI (Magic User Interface) aesthetic on a warm gray base**:

- **Warm gray base** (#8C8C8C) instead of checkered WB desktop
- **Commodore warm palette** (amber #CC9933, brown #776644, red #BB4444) — no blue
- **Dark gray title bars** (#505050) — no fake gadgets, no gradient
- **System sans-serif fonts** for body, monospace for code only
- **MUI-style 1px bevels** — subtle, not chunky 2px WB3.x bevels
- **Group frames** instead of fake windows
- **Dedicated `/amiga.html`** for classic Amiga browsers (HTML 3.2, <30KB)

Additionally accepted 6 scope expansions: rich port detail pages, typing animation hero, enhanced stats dashboard, port request form (→ GitHub Issues), keyboard shortcuts, RSS feed.

## Rationale

The redesign was driven by iterative design consultation with 6+ visual previews:

1. **WB3.x skeuomorphism** — rejected (theme park feel)
2. **Clean modern dark** (brew.sh style) — rejected (felt too Unix, not Amiga)
3. **Dark + MUI widgets** — close but blue title bars felt wrong
4. **Dark + Commodore palette** — better but dark base still felt Unix
5. **MUI on warm gray, no blue** — accepted (feels like a real MUI app)

MUI was chosen because it's what a well-configured Amiga actually looked like day-to-day — not raw Workbench, but the polished MUI experience that Amiga power users knew. The warm Commodore palette (amber/brown/red) references heritage through color, not mimicry.

## Success Criteria

- Site visitors don't think "this is a novelty" — they think "this is a real tool"
- Recognizably Amiga to anyone who used MUI, without being a cosplay
- /amiga.html works on IBrowse 2.5 in FS-UAE
- Design system documented in DESIGN.md and enforced by rules

## Alternatives Considered

- **Keep WB3.x** — Rejected. Owner explicitly said "hate the font, kinda hate the design."
- **Clean modern (no retro)** — Rejected. Lost all Amiga identity. Could be any developer tool.
- **Dark theme** — Rejected. Dark backgrounds make everything feel Unix/terminal regardless of accent colors.
- **Blue accents** — Rejected through iteration. Blue title bars "don't feel very Amiga" regardless of shade.
- **Brown title bars** — Rejected. "Don't like the brown." Neutral dark gray was the sweet spot.
