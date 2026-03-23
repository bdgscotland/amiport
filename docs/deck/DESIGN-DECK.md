# Design System — The Hard Direction (Slide Deck)

## Product Context
- **What this is:** 16:9 HTML slide presentation about AI-powered code migration
- **Who it's for:** Enterprise technical leadership, engineering directors, AI strategy teams
- **Presenter:** Duncan Bowring, Accenture
- **Project type:** Conference presentation / internal pitch deck

## Relationship to DESIGN.md
This deck inherits the Amiga MUI DNA from the amiport website design system but **inverts the palette** for projection. The website uses warm gray surfaces with dark text. The deck uses dark surfaces with amber highlights — same Commodore warmth, dramatically better contrast on projectors.

## Aesthetic Direction
- **Direction:** Industrial Terminal — Amiga MUI personality through structure and warmth, not skeuomorphic mimicry
- **Decoration level:** Intentional — MUI-style 1px bevels, circuit trace textures, rectilinear panels
- **Mood:** An engineer's terminal at 3am. Technical, credible, warm. Not retro-kitsch.
- **Border radius:** 0px everywhere. MUI is rectilinear. No rounded corners.
- **Dark mode:** Always. The deck IS dark mode. No toggle.

## Typography
- **Display/Hero:** IBM Plex Sans, 700, 48-64px — industrial precision, strong at large sizes
- **Body/Narrative:** IBM Plex Sans, 400, 20-28px — same family, generous line-height (1.5)
- **Stat numbers:** IBM Plex Sans, 700, 72-96px — hero numbers that punch from the back of the room
- **Monospace:** IBM Plex Mono, 400, 14-22px — terminal output, agent names, code snippets, status bar
- **Loading:** Google Fonts CDN

## Color
- **Approach:** Inverted warm Commodore — dark surfaces, amber highlights
- **Background:** `#1C1A18` — warm near-black
- **Surface:** `#2A2724` — card panels, code blocks
- **Surface elevated:** `#3A3632` — highlighted panels
- **Border:** `#4A4540` — circuit traces, subtle borders
- **Border bright:** `#6A6358` — emphasized borders, bevel edges
- **Text primary:** `#E8E0D4` — warm cream headlines and body
- **Text secondary:** `#A09888` — subtitles, annotations
- **Accent amber:** `#D4A020` — hero color, highlights, Commodore gold
- **Accent amber bright:** `#F0C040` — stat numbers, emphasis
- **Accent red:** `#C04030` — gate bars, Tier 3, stop indicators
- **Accent green:** `#50A050` — Tier 1, pass indicators, allowed tools
- **Accent yellow:** `#C0A030` — Tier 2, warning indicators
- **Terminal bg:** `#181614` — code/terminal insets
- **Terminal text:** `#D0C8B8` — monospace content
- **Terminal prompt:** `#D4A020` — the >_ prompt

## Layout
- **Aspect ratio:** 16:9 (1920x1080 logical pixels)
- **Content area:** 1680px wide, 120px top padding, 80px side padding
- **Status bar:** 48px tall, full width, persistent across all slides
- **Slide layouts:** full-width, split (50/50 or 60/40), stat-grid

## Visual Elements
- **Circuit traces:** SVG background texture at 0.06-0.10 opacity
- **Beveled panels:** 1px MUI-style shine/shadow borders
- **Gate bars:** 6px wide red vertical bars between pipeline stages
- **Tier indicators:** 12px colored squares (green/yellow/red)
- **Tool tags:** Monospace inline tags with green (allowed) / red strikethrough (denied)

## Motion
- **Slide transitions:** Fade, 200ms
- **No swoops, no 3D, no parallax.** Content appears, content leaves.

## Decisions Log
| Date | Decision | Rationale |
|------|----------|-----------|
| 2026-03-23 | Dark background for deck | Website's #8C8C8C washes out on projectors |
| 2026-03-23 | IBM Plex Sans + Mono | Industrial precision, not overused, both variants pair |
| 2026-03-23 | Custom slide engine | No dependencies, single HTML+CSS+JS, keyboard nav |
| 2026-03-23 | Inverted palette from website | Same Commodore warmth, better projection contrast |
