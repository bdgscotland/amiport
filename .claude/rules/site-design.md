Paths: site/**/*.css, site/**/*.html, site/**/*.php, site/**/*.js

# Site Design System Enforcement

**Always read `DESIGN.md` before making any visual changes to the website.**

## Design System: Amiga MUI on Warm Gray

The site uses an Amiga MUI (Magic User Interface) aesthetic — NOT Workbench 3.x skeuomorphism.

### DO:
- Use warm gray base (`#8C8C8C`), dark gray title bars (`#505050`)
- Use Commodore warm palette: amber (`#CC9933`), brown (`#776644`), red (`#BB4444`)
- Use 1px MUI-style bevels (shine/shadow)
- Use system sans-serif font stack for body, monospace for code only
- Use group frames (`.group-frame`) as content containers
- Use 0px border-radius everywhere (rectilinear)
- Ensure all components match DESIGN.md specs

### DO NOT:
- Use blue anywhere (no `#0055AA`, no blue accents, no blue title bars)
- Use rounded corners (no `border-radius` > 0)
- Use gradients, drop shadows, or decorative effects
- Use fake window gadgets (close/resize dots on title bars)
- Use checkered desktop background pattern
- Use Topaz or all-monospace typography for body text
- Use CSS frameworks (hand-crafted CSS only)
- Use CDN-hosted JS libraries or web fonts

### Amiga Browser Compatibility
- `/amiga.html` must be HTML 3.2 compatible, under 30KB, 640x480
- Main site pages should degrade gracefully without CSS/JS
- No CSS custom properties in critical layout paths
