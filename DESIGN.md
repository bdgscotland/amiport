# Design System — amiport

## Product Context
- **What this is:** Website + package index + CLI installer for porting POSIX tools to AmigaOS 3.x
- **Who it's for:** Amiga hobbyists, retro computing enthusiasts, Amiga developers
- **Space/industry:** Retro computing, Amiga community, developer tools
- **Project type:** Developer tool website with package browser + Amiga-side CLI

## Aesthetic Direction
- **Direction:** MUI (Magic User Interface) on warm gray — Amiga personality through structure and color, not skeuomorphic mimicry
- **Decoration level:** Intentional — MUI-style 1px bevels, group frames, rectilinear widgets. No fake window gadgets, no checkered desktop pattern, no decorative chrome.
- **Mood:** Like a well-configured A1200 running MUI apps. Clean, functional, warm. Recognizably Amiga to anyone who used one, but not a theme park.
- **Reference sites:** Aminet (aminet.net), OS4Depot (os4depot.net) — functional but dated. amiport should feel like a modern MUI app, not a 2003 web page.
- **Boot sequence:** Optional easter egg. CSS-only animation on first visit. Skipped on repeat via localStorage. Replay available via UI control.

## Typography
- **Display/Hero:** System sans-serif stack: `-apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Oxygen, Ubuntu, Cantarell, sans-serif`
- **Body:** Same as Display — clean, readable, fast. No web font dependency.
- **UI/Labels:** Same as body
- **Data/Tables:** Same as body
- **Code/Terminal:** Monospace only for code: `"SF Mono", "Fira Code", "JetBrains Mono", Consolas, monospace`
- **Loading:** System fonts — zero network requests, instant render
- **Scale:**
  - 3xl: 30px (page title, hero)
  - 2xl: 22px (section headings, package name)
  - xl: 18px (subheadings)
  - lg: 16px (large body)
  - base: 15px (body text)
  - sm: 14px (table text)
  - xs: 12px (metadata, badges, labels)
  - 2xs: 10px (gauge text, fine print)

## Color
- **Approach:** Warm Commodore palette — amber, brown, red accents on neutral gray. No blue.
- **Background:** `#8C8C8C` — warm medium gray, like an actual MUI application background
- **Surface:** `#9A9A9A` — group frame interior, card background
- **Elevated:** `#A8A8A8` — table headers, tab background, step counters
- **Light surface:** `#B4B4B4` — hover highlight, alternate row
- **Title bar:** `#505050` — dark neutral gray for screen bar and window titles
- **Title bar shine:** `#6A6A6A` — bevel highlight on title bars
- **Title bar shadow:** `#383838` — bevel shadow on title bars
- **Bevel shine:** `#C8C8C8` — light edge of raised elements
- **Bevel shadow:** `#606060` — dark edge of raised elements
- **Bevel dark:** `#505050` — dark edge of inset elements
- **Text:** `#1A1A1A` — primary text on gray backgrounds
- **Text muted:** `#4A4A4A` — secondary text, metadata, descriptions
- **Accent amber:** `#CC9933` — highlights, stat values, prompts, gauge fill
- **Accent amber light:** `#DDAA44` — badges (new), hover accents
- **Accent amber dark:** `#997722` — primary buttons
- **Accent brown:** `#776644` — links, section labels, group frame titles
- **Accent red:** `#BB4444` — accent/CTA buttons, destructive actions
- **Accent salmon:** `#CC7755` — hover state for links
- **Accent cream:** `#E0D0B8` — title bar text, screen bar text
- **Terminal bg:** `#222222` — shell window body
- **Terminal fg:** `#D0CCC4` — shell text
- **Terminal prompt:** `#CC9933` — amber prompt in shell
- **Terminal output:** `#888880` — dimmer output text
- **Semantic:**
  - Success: border `#448844`, bg `#88BB88`
  - Warning: border `#886633`, bg `#CCAA66`
  - Error: border `#884444`, bg `#CC8888`
  - Info: border `#446688`, bg `#88AACC`
- **Dark mode:** None. Warm gray IS the mode. Matches how MUI apps actually looked.

## Spacing
- **Base unit:** 8px
- **Density:** Comfortable — MUI was well-spaced but not wasteful
- **Scale:**
  - 2xs: 2px
  - xs: 4px
  - sm: 8px (base)
  - md: 16px
  - lg: 24px
  - xl: 32px
  - 2xl: 48px

## Layout
- **Approach:** Grid-disciplined — single column, centered
- **Max content width:** 780px
- **Border radius:** 0px everywhere. MUI is rectilinear. No rounded corners.
- **Background:** Solid warm gray. No checkered pattern, no texture.

## UI Components

### Screen Bar
Top navigation bar:
- Dark gray background (`#505050`), cream text (`#E0D0B8`)
- Brand name left (amber `#DDAA44`), nav links right
- 44px height, 1px bevel (shine top, shadow bottom)
- No depth gadget, no decorative elements

### MUI Window
Container for terminal/code display:
- Dark gray title bar (`#505050`), centered cream text, no gadgets
- 22px title bar height, 1px bevels
- Body area inset with dark bevel, 3px margin inside window frame
- Terminal variant: black body (`#222222`), monospace text

### Group Frame
Primary content container (replaces the old "Window" component):
- Raised 1px bevel (shine top-left, shadow bottom-right)
- Gray surface background (`#9A9A9A`)
- Labeled section header: uppercase, brown (`#776644`), letterspaced, bottom border
- 14px padding

### Buttons (MUI XEN Style)
- 1px raised bevel (shine top-left, shadow bottom-right)
- On hover: inverted bevel (depressed)
- 26px min-height, 13px font, 600 weight
- Variants:
  - Default: elevated gray bg, dark text
  - Primary: dark amber bg (`#997722`), white text
  - Accent: red bg (`#BB4444`), white text
  - Ghost: transparent, muted text, 1px border
- Border radius: 0

### Tabs (MUI Cycle Gadget)
- Row of tabs above content area
- Active tab: raised bevel, surface bg, dark text, bottom border matches content bg
- Inactive tab: inset bevel, elevated bg, muted text
- No gap between tabs, flush with tab body

### Table
- Elevated gray header row, dark text, uppercase labels
- Alternating row colors (surface / elevated)
- Hover: light surface highlight
- 1px bottom borders between rows
- Package names in brown, versions in monospace muted

### Gauge (Progress Bar)
- Inset bevel container on gray bg
- Amber fill (`#CC9933`)
- Centered percentage text (10px, bold)
- 16px height

### Listview (Drawer/Collapsible)
- Inset bevel container
- Expandable headers: elevated bg, bold text, `+`/`-` prefix in brown
- Content area: surface bg, indented, muted text
- No animation — instant show/hide

### Badges
- 10px uppercase text, 600 weight
- Variants:
  - New: amber light bg (`#DDAA44`), dark text
  - Published: green-tinted bg, green text, green border
  - Popular: amber-tinted bg, dark text, amber border
  - Archived: elevated bg, muted text, gray border

### Form Inputs
- White background
- Inset bevel (shadow top-left, shine bottom-right)
- Focus ring: amber border
- System font, 15px

### Alerts
- Colored border + tinted background
- No icons — text only
- 1px border, 10px padding

### Install Block
- Terminal-style: dark bg (`#222222`), inset bevel
- Prompt in muted gray, command in amber
- Copy button (ghost style) right-aligned

### Steps (Numbered)
- Counter squares: elevated bg, amber number, raised bevel
- 26x26px squares, left-aligned
- Title bold, description muted

## Motion
- **Approach:** Minimal-functional — MUI had no animations
- **Transitions:** Hover state changes only (bevel inversion on buttons, row highlight on tables)
- **Duration:** Instant (no easing, no delay)
- **Boot sequence:** Optional easter egg exception. CSS-only, first visit only.
- **Blinking cursor:** Exception — shell window cursor blinks (1s step-end). Adds life to the hero.
- **Prohibited:** No scroll effects, no entrance animations, no parallax, no loading spinners (use text: "Loading...")

## Anti-Slop Rules
- No gradients (exception: subtle title bar gradient if needed for polish)
- No drop shadows (use 1px bevel borders)
- No rounded corners (border-radius: 0 always)
- No hero images, stock photos, or illustrations
- No CSS framework (hand-crafted CSS only)
- No emoji in the UI
- No blue anywhere — use amber/brown/red accent palette
- No purple/violet accents
- No card grids (use tables, lists, or group frames)
- No fake window gadgets (close/resize dots are decorative clutter)
- No checkered desktop pattern
- No generic SaaS/developer-tool patterns

## Amiga Browser Compatibility

Two-tier approach: the main site uses progressive enhancement, plus a dedicated Amiga-optimized page.

### Main Site (Progressive Enhancement)
- Site must degrade gracefully on IBrowse, AWeb, and NetSurf-Amiga
- No CSS custom properties in critical layout (use as progressive enhancement)
- No flexbox/grid for essential layout (use as progressive enhancement)
- No web fonts — system font stack works everywhere
- Simple HTML structure that renders as readable text without CSS
- Modern browsers get bevels, hover states, boot animation. Amiga browsers get clean text on plain background.

### Dedicated Amiga Page (`/amiga.html`)
A purpose-built page optimized for classic Amiga browsers. Linked from the main site nav ("Amiga" link).

**Technical constraints:**
- HTML 3.2 compatible — no CSS required (inline styles only for minor tweaks)
- `<table>` layout — no flexbox, no grid, no floats
- 640x480 target resolution (fits PAL hi-res and most Amiga monitor configs)
- Max page width: 620px (with 10px margin)
- No JavaScript
- No images larger than 16 colors / 320px wide (if any images at all)
- Total page size under 30KB (Amiga users may be on serial/PLIP connections)
- No `<style>` block — IBrowse 2.x has limited CSS support, inline only
- UTF-8 with ASCII-safe content (no special characters beyond Latin-1)

**Content:**
- Package listing table: name, version, category, size, download link (direct .lha)
- Brief intro text (2-3 lines)
- Install instructions for `amiget`
- Link back to full site for modern browsers
- Footer with Aminet link

**Visual style:**
- Amiga system font (Topaz) renders naturally on real hardware — no font specification needed
- `bgcolor` on `<body>` and `<table>` cells for basic color (use WB3.x gray `#AAAAAA` here — it's what the Amiga user expects)
- Table headers: dark background, light text
- Links: default browser blue is fine (don't fight the browser)
- Keep it simple and functional — this page should feel like an Aminet listing, not a design showcase

**Serving:**
- Same domain: `amiport.platesteel.net/amiga.html`
- Can be server-rendered from the same package manifest API that powers the main site
- Consider `amiget` using this page as its package index endpoint (lightweight, parseable)

## Responsive
- 780px max-width is naturally mobile-friendly
- Package table: horizontal scroll on screens < 600px
- Minimum font size: 14px body text
- Touch targets: 44px minimum height on interactive elements
- Nav remains horizontal (4 items fit at 320px)
- Stats row wraps on narrow screens

## Accessibility
- Keyboard navigation: all interactive elements focusable, visible focus ring (amber outline)
- ARIA landmarks: `<nav>`, `<main>`, `<footer>` semantic elements
- Color contrast: `#1A1A1A` on `#8C8C8C` = 4.6:1 (AA for normal text), `#1A1A1A` on `#9A9A9A` = 5.2:1 (AA)
- Screen reader: tables with `<caption>`, `<th scope="col">`
- Skip-to-content link (hidden until focused)
- Alt text on all images/GIFs
- Collapsible sections use `<details>`/`<summary>` for native accessibility

## Decisions Log
| Date | Decision | Rationale |
|------|----------|-----------|
| 2026-03-22 | Initial design system created | Created by /design-consultation based on Workbench 3.x aesthetic |
| 2026-03-22 | Redesigned: MUI on warm gray | Full WB3.x skeuomorphism felt like a theme park. MUI-style widgets on warm gray base looks like a real Amiga app without cosplay. |
| 2026-03-22 | Dropped all blue | Blue title bars felt too Unix/corporate. Neutral dark gray title bars with warm amber/brown/red accents feel more Commodore. |
| 2026-03-22 | System sans-serif fonts | All-monospace was hard to read. System fonts are fast, readable, and degrade well on Amiga browsers. Monospace reserved for code. |
| 2026-03-22 | No fake window gadgets | Decorative close/resize dots on title bars are pointless clutter. Clean centered text only. |
| 2026-03-22 | No checkered background | The checkered WB desktop pattern is charming for 5 seconds then distracting. Solid gray is cleaner. |
| 2026-03-22 | Warm Commodore palette | Amber, brown, red accents reference Commodore heritage through color, not through surface-level UI mimicry. |
| 2026-03-22 | Amiga browser compatibility | Progressive enhancement ensures the site works on IBrowse/AWeb. Simple HTML + minimal CSS degrades better than complex skeuomorphic styles. |
| 2026-03-22 | Boot animation kept as easter egg | Still charming, still shareable on Amiga forums, but now optional rather than central to the aesthetic. |
| 2026-03-22 | Dedicated Amiga page (/amiga.html) | Main site uses progressive enhancement but looks bland on IBrowse. A dedicated HTML 3.2 page gives Amiga users a proper experience — table layout, <30KB, 640x480, Aminet-style listing. |
