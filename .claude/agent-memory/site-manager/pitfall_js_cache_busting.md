---
name: JS files missing cache-buster version strings
description: packages.js, stats.js, changelog.js served with max-age=2592000 but no ?v= query strings
type: feedback
---

JS files on Dreamhost are served with `Cache-Control: max-age=2592000` (30 days). Without a version query string in the `<script src>`, browsers cache the JS for a month. If the JS is ever updated, browsers serve stale code and pages break silently.

**Why:** The CSS had `style.css?v=20260325c` and activity.js / catalog.js had version strings, but packages.js, stats.js, and changelog.js were added without them.

**How to apply:** Whenever editing an HTML file's `<script src>`, check that the referenced JS has a `?v=YYYYMMDDX` query string. When updating JS, bump the version string in the HTML. Pattern: `?v=20260327a`.

Fixed 2026-03-27: added `?v=20260327a` to packages.js, stats.js, changelog.js.
