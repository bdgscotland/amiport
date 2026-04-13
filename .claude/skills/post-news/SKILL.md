---
name: post-news
description: Publish a news entry to the amiport site. Appends to site/data/news.json, validates, deploys via site-manager, and clears the activity cache. Use when announcing a release, project update, milestone, or behind-the-scenes note.
user_invocable: true
---

# /post-news — Publish News Entry

Adds a news entry to `site/data/news.json`, deploys the site, and refreshes caches so the post appears on the homepage activity feed, the News archive page (`news.html`), and the RSS feed (`feed.php`).

## Usage

```
/post-news                    # Interactive — prompt for title + body
/post-news <title>            # Title on command line, prompt for body
```

If the user has not provided a title + body in the current conversation, ask for:

1. **Title** — short headline, aim for <70 characters
2. **Body** — one or more paragraphs. Supports a tiny markdown-lite subset rendered by `site/js/news.js`:
   - blank lines split paragraphs
   - `[label](url)` — links (http(s) or site-relative only)
   - `**bold**`
   - `` `code` ``
3. **Tags** (optional) — short keyword array, e.g. `["release", "amigit"]`
4. **URL** (optional) — a canonical link for the entry (defaults to `/news.html#<id>`)

Do not fabricate any of these. Ask.

## Steps

1. **Load** `site/data/news.json` (JSON array). If missing or malformed, stop and report — do not overwrite.
2. **Build the new entry:**
   ```json
   {
     "id": "<YYYY-MM-DD>-<slug>",
     "date": "<YYYY-MM-DDTHH:MM:SSZ>",
     "title": "<title>",
     "body": "<body>",
     "tags": ["..."],
     "url": "<optional canonical url>"
   }
   ```
   - `id` — `<today>-<kebab-case slug>` derived from the title. Slug max 40 chars. Must be unique within the file — if it collides, append `-2`, `-3`, etc.
   - `date` — current UTC timestamp (ISO 8601, `Z` suffix).
   - `title` — ASCII only. Reject em-dashes and smart quotes — replace with `--` and straight quotes before writing. amiga.html and feed.php consume this.
   - `body` — ASCII only, same rule. Preserve paragraph breaks.
3. **Prepend** the new entry to the array (newest-first makes diffs tidy, though `news.js` re-sorts by date on render).
4. **Write** `site/data/news.json` back. Validate that `python3 -c "import json; json.load(open('site/data/news.json'))"` still parses.
5. **Deploy** — dispatch the `site-manager` agent with this prompt:
   > "Deploy the site to Dreamhost. A new news entry has been added to `site/data/news.json`. Run the standard rsync deploy, then clear the activity cache on the server (`/tmp/amiport-activity-cache.json`), then verify `/feed.php` and `/news.html` return 200."
6. **Verify** — after site-manager returns, fetch these URLs and confirm each contains the new title:
   - `https://amiport.platesteel.net/news.html`
   - `https://amiport.platesteel.net/feed.php`
   - `https://amiport.platesteel.net/api/v1/activity.php`
7. **Report** — print a short confirmation with the entry id, title, and URLs.

## Validation Rules

- **ASCII only** in `title` and `body`. Reject em-dash (U+2014), smart quotes (U+2018-201D), arrows (U+2192). Replace with ASCII equivalents (`--`, `'`, `"`, `->`) before writing. This is the same rule as `.claude/rules/aminet-publishing.md` and `.claude/rules/amiga-coding.md` — amiga.html renders news too and ARexx tools may consume the feed.
- **Unique id** within `news.json`. Grep for the slug before writing.
- **Valid JSON** after write. If `python3 -c "import json; json.load(...)"` fails, restore from the backup the skill writes first (`site/data/news.json.bak`).
- **No placeholder content.** Never write "TODO", "TBD", "(fill in)", or lorem ipsum. If the user has not provided real content, ask for it.

## What Not To Do

- Do not edit `news.json` manually through Edit when this skill exists — use the skill so validation + deploy + cache clear happen atomically.
- Do not commit the change before the site-manager deploy confirms. The catalog-sync hook is git-gated but `news.json` is not in the hook, so local and server can drift if you commit first and deploy later.
- Do not mention packages, downloads, or metrics in news entries unless they are real and current (consult `data/catalog.json` and `site/data/packages/*.json`).

## Related

- `.claude/agents/site-manager.md` — handles the actual deploy. Also documents the **"The `data/` Proxy Pattern"** section that explains why `site/data/news.json` is served to browsers via `site/api/v1/news.php` (the `.htaccess` blocks direct `data/` access). Any new public data file under `site/data/` MUST have a corresponding `api/v1/` proxy endpoint.
- `.claude/rules/catalog-sync.md` — activity cache file location and why it must be cleared
- `site/js/news.js` — how body markdown-lite is rendered
- `site/api/v1/news.php` — the proxy endpoint that browsers fetch (do NOT change `news.js` to fetch `data/news.json` directly — it will 403)
- `site/feed.php` — how news items appear in RSS alongside packages (reads `data/news.json` server-side, bypasses the proxy)
