# Design System — amiport

## Product Context
- **What this is:** Website + package index + CLI installer for porting POSIX tools to AmigaOS 3.x
- **Who it's for:** Amiga hobbyists, retro computing enthusiasts, Amiga developers
- **Space/industry:** Retro computing, Amiga community, developer tools
- **Project type:** Developer tool website with package browser + Amiga-side CLI

## Aesthetic Direction
- **Direction:** Retro-Futuristic / Amiga Workbench 3.x
- **Decoration level:** Intentional — bevel borders, window title bars, checkered desktop pattern
- **Mood:** Like opening a Workbench window on a real Amiga. Not a pixel-perfect emulator, but an unmistakable tribute. Functional, honest, and deeply respectful of the platform.
- **Reference sites:** Aminet (aminet.net), OS4Depot (os4depot.net), Amigans.net — all functional but visually stuck in 2003. None actually evoke the Amiga aesthetic. amiport is the first Amiga site that looks like an Amiga.
- **Boot sequence:** On first visit, a 3-second CSS animation mimics an Amiga boot (black screen → ROM text → Workbench blue → site appears). Skipped on repeat visits via localStorage. Replay available via UI control.

## Typography
- **Display/Hero:** `Topaz New`, `Amiga Topaz`, `Courier New`, `Courier`, `monospace` — THE Amiga font, no substitute
- **Body:** Same as Display — everything is monospace, like a real Amiga
- **UI/Labels:** Same as body
- **Data/Tables:** Same — monospace IS tabular by nature
- **Code:** Same font stack
- **Loading:** Web font for Topaz New if available; falls back gracefully to Courier New → system monospace
- **Scale:**
  - 3xl: 32px (page title, hero)
  - 2xl: 24px (section headings)
  - xl: 20px (subheadings)
  - lg: 16px (large body, emphasis)
  - base: 14px (body text)
  - sm: 12px (metadata, captions, table text)

## Color
- **Approach:** Restrained — the classic Workbench 3.x 8-color palette
- **Primary:** `#0055AA` — Workbench blue. Title bars, links, active elements, table headers.
- **Secondary:** `#FF8800` — Workbench orange. Highlights, badges, CTAs, focus rings, accents.
- **Background:** `#AAAAAA` — Workbench gray. Page background (with checkered overlay).
- **Surface:** `#BBBBBB` — Lighter gray for raised panels and alternate table rows.
- **Text:** `#000000` on gray/surface; `#FFFFFF` on blue/black backgrounds.
- **Shadow:** `#555555` — Bevel dark edge (bottom-right of raised elements, top-left of inset elements).
- **Highlight:** `#FFFFFF` — Bevel light edge (top-left of raised elements, bottom-right of inset elements).
- **Dark Gray:** `#666666` — Secondary text, metadata, muted content.
- **Terminal:** `#000000` background, `#FFFFFF` text — Amiga Shell/CLI window.
- **Semantic:**
  - Success: border `#008800`, bg `#AADDAA`
  - Warning: border `#886600`, bg `#FFDD88`
  - Error: border `#AA0000`, bg `#FFAAAA`
  - Info: border `#0055AA`, bg `#AACCEE`
- **Dark mode:** None. Workbench 3.x was gray. A dark mode would break the aesthetic entirely.

## Spacing
- **Base unit:** 8px
- **Density:** Comfortable — Workbench wasn't cramped but wasn't spacious
- **Scale:**
  - 2xs: 2px
  - xs: 4px
  - sm: 8px (base)
  - md: 16px
  - lg: 24px
  - xl: 32px
  - 2xl: 48px
  - 3xl: 64px

## Layout
- **Approach:** Grid-disciplined — single column, centered
- **Max content width:** 720px — mirrors Amiga hi-res screen width (640-720px)
- **Border radius:** 0px everywhere. Amiga UI is rectilinear. No rounded corners.
- **Checkered background:** Classic Workbench checkered pattern (4px tiles, alternating #AAAAAA/#999999 at 45deg). Content sits in 'windows' floating on the desktop.

## UI Components

### Window
The primary container. Every content section is a "window" with:
- Blue title bar (`#0055AA`, white text, decorative close/resize gadgets)
- Bevel border (light top-left `#FFFFFF`, dark bottom-right `#555555`)
- Gray body (`#AAAAAA`)

### Shell Window
Variant of Window for terminal/code display:
- Same title bar (labeled "AmigaShell")
- Black body (`#000000`), white monospace text (`#FFFFFF`)

### Screen Bar
Sticky top navigation bar:
- Blue background (`#0055AA`), white text
- Logo/brand left, depth gadget right
- Navigation links inline

### Buttons (Gadgets)
- Raised bevel (light top-left, dark bottom-right)
- On hover/active: inverted bevel (depressed look)
- Variants: Default (gray bg), Primary (blue bg, white text), Accent (orange bg, black text)
- Minimum touch target: 44px height

### Table
- Blue header row (`#0055AA`, white text)
- Alternating row colors (`#AAAAAA` / `#BBBBBB`)
- Full-row hover highlight (blue bg, white text)
- Clickable rows (cursor: pointer)
- 1px borders (`#555555`)

### Form Inputs
- White background (`#FFFFFF`)
- Inset bevel (shadow top-left, highlight bottom-right — opposite of buttons)
- Focus ring: orange border (`#FF8800`)

### Badges
- Orange background (`#FF8800`), black text
- 1px border (`#555555`)
- Used for category labels

### Alerts
- Colored border + tinted background
- No icons — text only (Amiga style)

## Motion
- **Approach:** Minimal-functional — Amiga UI had no animations
- **Transitions:** Hover state changes only (bevel inversion on buttons, row highlight on tables)
- **Duration:** Instant (no easing, no delay) — like real Amiga UI
- **Boot sequence:** Exception to the no-animation rule. CSS-only 3.5-second boot animation on first visit. Keyframe sequence: black → ROM text → blue screen → fade to site.
- **Prohibited:** No scroll effects, no entrance animations, no parallax, no loading spinners (use text: "Loading...")

## Anti-Slop Rules
- No gradients anywhere
- No drop shadows (use bevel borders instead)
- No rounded corners (border-radius: 0 always)
- No hero images, stock photos, or illustrations
- No CSS framework (hand-crafted CSS only)
- No emoji in the UI
- No sans-serif fonts
- No purple/violet accents
- No card grids (use tables or lists)
- No generic SaaS/developer-tool patterns

## Responsive
- 720px max-width is naturally mobile-friendly (narrower than most phones in landscape)
- No layout breakpoints needed
- Package table: horizontal scroll (`overflow-x: auto`) on screens < 600px
- Minimum font size: 14px body text
- Touch targets: 44px minimum height
- Nav remains horizontal (4 items fit at 320px)

## Accessibility
- Keyboard navigation: all interactive elements focusable, visible focus ring (orange outline)
- ARIA landmarks: `<nav>`, `<main>`, `<footer>` semantic elements
- Color contrast: `#000000` on `#AAAAAA` = 5.3:1 (AA), `#FFFFFF` on `#0055AA` = 5.7:1 (AA)
- Screen reader: tables with `<caption>`, `<th scope="col">`
- Skip-to-content link (hidden until focused)
- Alt text on all images/GIFs

## Decisions Log
| Date | Decision | Rationale |
|------|----------|-----------|
| 2026-03-22 | Initial design system created | Created by /design-consultation based on Workbench 3.x aesthetic direction from /plan-design-review |
| 2026-03-22 | Retro Amiga aesthetic chosen | No Amiga community site actually looks like an Amiga. This is the first. Differentiator + respect for the platform. |
| 2026-03-22 | All-monospace typography | Authenticity over flexibility. Amiga was monospace. Developers expect monospace. |
| 2026-03-22 | No dark mode | Workbench 3.x was gray. Dark mode would break the aesthetic coherence. |
| 2026-03-22 | Boot sequence animation | First-visit-only CSS animation. Viral shareability for Amiga forums. localStorage skip on repeat. |
| 2026-03-22 | Checkered desktop background | Classic WB pattern. Content in windows floating on the desktop. Maximum authenticity. |
| 2026-03-22 | 720px max-width | Mirrors Amiga hi-res screen resolution. Naturally mobile-friendly as a side effect. |
| 2026-03-22 | Table layout for packages | Matches Aminet/OS4Depot conventions. Honest about catalog size (11 packages). Dense and scannable. |
