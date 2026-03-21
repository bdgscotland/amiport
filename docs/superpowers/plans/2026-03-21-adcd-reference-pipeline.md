# ADCD-to-Markdown Reference Pipeline Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Scrape the complete Amiga Developer CD v2.1 (~6,700 HTML pages across 6 manuals) from amigadev.elowar.com, convert to agent-optimized markdown with cross-references, and integrate into the amiport porting agents.

**Architecture:** Two-pass Python pipeline. Pass 1 crawls each manual's HTML navigation tree, extracts content from AG2HTML `<pre>` blocks, converts to raw markdown, and builds a global node-ID-to-filepath map. Pass 2 enriches with YAML frontmatter, function/type detection, cross-references, porting summaries, code example extraction, and index generation. Single script with `crawl`/`enrich`/`all` subcommands.

**Tech Stack:** Python 3 (stdlib only: urllib, html.parser, argparse, json, re, pathlib). No external dependencies.

**Design docs:**
- `docs/designs/adcd-reference-pipeline.md` — CEO plan with expansion specs
- `~/.gstack/projects/bdgscotland-amiport/duncan-main-design-20260321-114829.md` — design doc

---

## Reviewer Fixes (applied after plan review)

The following issues were identified by the plan reviewer and MUST be addressed during implementation:

1. **Bug: `all_examples = []`** — Add `all_examples = []` initialization in `do_enrich()` alongside `type_index = {}`. Without this, Task 6 Step 5 will cause a NameError.

2. **Missing: Chapter subdirectory hierarchy** — The design doc specifies `libraries/20-exec-memory/memory-functions.md` not `libraries/memory-functions.md`. The crawler should detect chapter boundaries from the TOC structure and create numbered subdirectories. Each chapter gets an `_index.md` overview file.

3. **Missing: Porting summaries (CEO Expansion 1)** — Add `porting_summary` YAML frontmatter to chapter `_index.md` files. Schema: `{relevance: HIGH|MEDIUM|LOW, summary: "...", posix_equivalents: [...]}`. Determine relevance by matching chapter content against `posix-to-amiga-map.md`.

4. **Missing: Fenced code block detection** — `convert_html_to_md()` must detect indented code blocks (4+ spaces with code characters like `=`, `(`, `)`, `{`, `}`, `;`) and wrap them in triple-backtick fences with `c` language hint.

5. **Missing: "See also" cross-references** — Enriched files must include "See also" sections linking detected functions to their autodoc entries (e.g., `See also: [AllocMem](../../autodocs/exec.library.md#allocmem)`).

6. **Missing: Enrich existing autodocs** — Append "See also: [ADCD reference](link)" sections to existing `docs/references/autodocs/*.md` files. Never overwrite existing content.

7. **Missing: Context7 metadata** — Generate `context7.json` manifest listing all files with their frontmatter metadata. Research Context7's expected format; fall back to a simple JSON manifest.

8. **Missing: Conditional VERSION-DIFF.md** — If Task 4 Step 3 confirms Autodoc structures are matchable, implement `generate_version_diff()` to produce VERSION-DIFF.md.

9. **Missing: Test for `resolve_links()`** — Add unit test with known node in map, unknown node, cross-manual link, non-node URL.

10. **Task 8 Steps 5-6** — Provide exact text for README.md and architecture.md updates (implementer should read existing files and follow patterns).

The implementer MUST read the design doc (`docs/designs/adcd-reference-pipeline.md`) and the original design doc for the full specifications of each expansion. The plan code below is a starting point, not the complete implementation.

## File Structure

| File | Responsibility |
|------|---------------|
| `scripts/scrape-adcd.py` | Main script: crawl, convert, enrich. Subcommands: `crawl`, `enrich`, `all` |
| `tests/test_scrape_adcd.py` | Unit tests for all conversion/detection/enrichment functions |
| `docs/references/adcd/raw/` | Pass 1 output (gitignored). Raw markdown + `node-map.json` |
| `docs/references/adcd/{manual}/` | Pass 2 output. Enriched markdown with frontmatter |
| `docs/references/adcd/INDEX.md` | Master alphabetical index |
| `docs/references/adcd/FUNCTIONS.md` | Function cross-reference (function → all locations) |
| `docs/references/adcd/TYPES.md` | Struct/typedef/enum index |
| `docs/references/adcd/INCLUDES.json` | Include file → ADCD chapter mapping |
| `docs/references/adcd/examples/{library}/*.c` | Extracted code examples |
| `docs/references/adcd/ATTRIBUTION.md` | Copyright attribution |
| `docs/references/adcd/README.md` | Overview and usage guide |
| `.gitignore` | Add `docs/references/adcd/raw/` entry |
| `.gitattributes` | Create with `linguist-documentation` for adcd/ |
| `Makefile` | Add `scrape-adcd` target |
| `.claude/agents/code-transformer.md` | Add ADCD reference paths |
| `.claude/agents/source-analyzer.md` | Add function cross-reference |
| `.claude/agents/build-manager.md` | Add device documentation |

---

## Task 1: Project Setup — gitignore, gitattributes, Makefile

**Files:**
- Modify: `.gitignore`
- Create: `.gitattributes`
- Modify: `Makefile`

- [ ] **Step 1: Add raw directory to .gitignore**

Append to `.gitignore`:
```
# ADCD scraper intermediate files (regenerable)
docs/references/adcd/raw/
```

- [ ] **Step 2: Create .gitattributes**

```
docs/references/adcd/** linguist-documentation
```

- [ ] **Step 3: Add Makefile target**

Add to `Makefile` `.PHONY` line: `scrape-adcd`

Add target:
```makefile
scrape-adcd:
	python3 scripts/scrape-adcd.py all --output docs/references/adcd/
```

Add help entry:
```
@echo "  scrape-adcd      Scrape ADCD and generate reference docs"
```

- [ ] **Step 4: Commit**

```bash
git add .gitignore .gitattributes Makefile
git commit -m "Add ADCD scraper infrastructure: gitignore, gitattributes, Makefile target"
```

---

## Task 2: Script Skeleton — argparse + subcommands

**Files:**
- Create: `scripts/scrape-adcd.py`

- [ ] **Step 1: Write the test for CLI argument parsing**

Create `tests/test_scrape_adcd.py`:
```python
"""Tests for scripts/scrape-adcd.py — ADCD scraper."""
import sys
import os
import pytest

# Add scripts/ to path so we can import
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'scripts'))

from scrape_adcd import slugify_title, convert_entities


class TestSlugifyTitle:
    def test_basic(self):
        assert slugify_title("Exec Memory Allocation") == "exec-memory-allocation"

    def test_special_chars(self):
        assert slugify_title("I/O Connectors & Interfaces") == "i-o-connectors-interfaces"

    def test_truncation(self):
        long_title = "A" * 80
        result = slugify_title(long_title)
        assert len(result) <= 60

    def test_html_entities(self):
        assert slugify_title("Amiga&#174; RKM") == "amiga-r-rkm"

    def test_collision(self):
        used = {"exec-memory"}
        assert slugify_title("Exec Memory", used_slugs=used) == "exec-memory-2"

    def test_collision_multiple(self):
        used = {"exec-memory", "exec-memory-2"}
        assert slugify_title("Exec Memory", used_slugs=used) == "exec-memory-3"


class TestConvertEntities:
    def test_numeric_entity(self):
        assert convert_entities("Amiga&#174; Hardware") == "Amiga(R) Hardware"

    def test_named_lt_gt(self):
        assert convert_entities("a &#060; b &#062; c") == "a < b > c"

    def test_ampersand(self):
        assert convert_entities("A &amp; B") == "A & B"

    def test_no_entities(self):
        assert convert_entities("plain text") == "plain text"
```

- [ ] **Step 2: Run tests to verify they fail**

Run: `python3 -m pytest tests/test_scrape_adcd.py -v`
Expected: ImportError — scrape_adcd module not found

- [ ] **Step 3: Write the script skeleton**

Create `scripts/scrape-adcd.py`:
```python
#!/usr/bin/env python3
"""
scrape-adcd.py — Scrape and convert the Amiga Developer CD v2.1 to markdown.

Two-pass pipeline:
  Pass 1 (crawl): Fetch HTML pages, convert to raw markdown, build node map
  Pass 2 (enrich): Add YAML frontmatter, cross-references, indexes, examples

Usage:
  python3 scripts/scrape-adcd.py crawl [--output DIR] [--manual NAME] [--limit N]
  python3 scripts/scrape-adcd.py enrich [--output DIR]
  python3 scripts/scrape-adcd.py all [--output DIR]

Source: amigadev.elowar.com/read/ADCD_2.1/
"""

import argparse
import html
import json
import os
import re
import sys
import time
import urllib.request
import urllib.error
from pathlib import Path
from typing import Optional

# Base URL for the ADCD site (HTTP, not HTTPS — self-signed cert)
BASE_URL = "http://amigadev.elowar.com/read/ADCD_2.1"

# Manuals to crawl, in priority order
MANUALS = [
    ("libraries", "Libraries_Manual_guide", "Amiga RKM Libraries"),
    ("devices", "Devices_Manual_guide", "Amiga RKM Devices"),
    ("hardware", "Hardware_Manual_guide", "Amiga Hardware Reference Manual"),
    ("amiga-mail", "AmigaMail_Vol2_guide", "Amiga Mail Volume II"),
    ("autodocs-2.0", "Includes_and_Autodocs_2._guide", "Includes and Autodocs 2.0"),
    ("autodocs-3.5", "Includes_and_Autodocs_3._guide", "Includes and Autodocs 3.5"),
]

USER_AGENT = "amiport-adcd-scraper/1.0 (open-source Amiga porting toolkit)"
REQUEST_DELAY = 0.1  # 100ms between requests


def slugify_title(title, used_slugs=None):
    """Convert a title to a filesystem-safe slug.

    - Lowercase, hyphens for separators
    - Truncate at 60 chars
    - Append -2, -3 etc on collision
    """
    # Convert HTML entities first
    title = convert_entities(title)
    # Lowercase, replace non-alnum with hyphens
    slug = re.sub(r'[^a-z0-9]+', '-', title.lower()).strip('-')
    # Truncate
    if len(slug) > 60:
        slug = slug[:60].rstrip('-')
    # Handle collisions
    if used_slugs is not None:
        base = slug
        counter = 2
        while slug in used_slugs:
            slug = f"{base}-{counter}"
            counter += 1
        used_slugs.add(slug)
    return slug


def convert_entities(text):
    """Convert HTML entities to plain text."""
    # Handle numeric entities
    text = re.sub(r'&#(\d+);', lambda m: chr(int(m.group(1))), text)
    # Handle named entities
    text = text.replace('&amp;', '&')
    text = text.replace('&lt;', '<')
    text = text.replace('&gt;', '>')
    text = text.replace('&quot;', '"')
    # Replace (R) symbol with text
    text = text.replace('\u00ae', '(R)')
    return text


def main():
    parser = argparse.ArgumentParser(
        description="Scrape and convert Amiga Developer CD v2.1 to markdown"
    )
    subparsers = parser.add_subparsers(dest="command", help="Pipeline stage")

    # crawl subcommand
    crawl_parser = subparsers.add_parser("crawl", help="Pass 1: fetch and convert HTML to raw markdown")
    crawl_parser.add_argument("--output", default="docs/references/adcd", help="Output directory")
    crawl_parser.add_argument("--manual", help="Crawl only this manual (e.g., 'libraries')")
    crawl_parser.add_argument("--limit", type=int, help="Limit pages per manual (for testing)")
    crawl_parser.add_argument("--dry-run", action="store_true", help="Crawl but don't write files")

    # enrich subcommand
    enrich_parser = subparsers.add_parser("enrich", help="Pass 2: add frontmatter, indexes, cross-refs")
    enrich_parser.add_argument("--output", default="docs/references/adcd", help="Output directory")

    # all subcommand
    all_parser = subparsers.add_parser("all", help="Run both passes")
    all_parser.add_argument("--output", default="docs/references/adcd", help="Output directory")
    all_parser.add_argument("--manual", help="Crawl only this manual")
    all_parser.add_argument("--limit", type=int, help="Limit pages per manual")

    args = parser.parse_args()

    if args.command is None:
        parser.print_help()
        sys.exit(1)

    if args.command == "crawl":
        do_crawl(args)
    elif args.command == "enrich":
        do_enrich(args)
    elif args.command == "all":
        do_crawl(args)
        do_enrich(args)


def do_crawl(args):
    """Pass 1: Crawl and convert HTML to raw markdown."""
    print("Pass 1: Crawl & Convert")
    print("=" * 60)
    # TODO: implement in Task 3
    pass


def do_enrich(args):
    """Pass 2: Enrich with frontmatter, indexes, cross-references."""
    print("Pass 2: Enrich")
    print("=" * 60)
    # TODO: implement in Task 5
    pass


if __name__ == "__main__":
    main()
```

- [ ] **Step 4: Run tests to verify they pass**

Run: `python3 -m pytest tests/test_scrape_adcd.py -v`
Expected: All 8 tests PASS

- [ ] **Step 5: Commit**

```bash
git add scripts/scrape-adcd.py tests/test_scrape_adcd.py
git commit -m "Add ADCD scraper skeleton with CLI subcommands and utility functions"
```

---

## Task 3: Pass 1 — HTML Fetching and Conversion

**Files:**
- Modify: `scripts/scrape-adcd.py`
- Modify: `tests/test_scrape_adcd.py`

- [ ] **Step 1: Write tests for HTML extraction and conversion**

Add to `tests/test_scrape_adcd.py`:
```python
from scrape_adcd import extract_body, convert_html_to_md, extract_links


# Real HTML sample from the site (Exec Memory Allocation chapter)
SAMPLE_HTML = '''<pre>
<!-- AG2HTML: BODY=START -->
Normally, an application uses the <a href="../Includes_and_Autodocs_2._guide/node0332.html">AllocMem()</a> function to ask for memory:

    APTR AllocMem(ULONG byteSize, ULONG attributes);
<a name="line5"></a>
The byteSize argument is the amount of memory the application needs.

 <a href="../Libraries_Manual_guide/node02A8.html">Memory Attributes</a>
 <a href="../Libraries_Manual_guide/node02A9.html">Allocating System Memory</a>
<!-- AG2HTML: BODY=END -->
</pre>'''

SAMPLE_HTML_NO_MARKERS = '''<pre>
Some content without body markers.

With <a href="../foo/node0001.html">a link</a>.
</pre>'''


class TestExtractBody:
    def test_with_markers(self):
        body = extract_body(SAMPLE_HTML)
        assert "AllocMem()" in body
        assert "BODY=START" not in body
        assert "BODY=END" not in body

    def test_without_markers(self):
        body = extract_body(SAMPLE_HTML_NO_MARKERS)
        assert "Some content" in body
        assert "a link" in body

    def test_strips_pre_tags(self):
        body = extract_body(SAMPLE_HTML)
        assert "<pre>" not in body

    def test_strips_anchor_names(self):
        body = extract_body(SAMPLE_HTML)
        assert '<a name="line5">' not in body


class TestExtractLinks:
    def test_extracts_relative_links(self):
        links = extract_links(SAMPLE_HTML)
        assert "../Includes_and_Autodocs_2._guide/node0332.html" in links
        assert "../Libraries_Manual_guide/node02A8.html" in links

    def test_ignores_anchor_names(self):
        links = extract_links(SAMPLE_HTML)
        # anchor-only links (name= targets) should not be in link list
        for link in links:
            assert "node" in link  # all real links have node IDs


class TestConvertHtmlToMd:
    def test_converts_links(self):
        body = extract_body(SAMPLE_HTML)
        md = convert_html_to_md(body)
        # Links should be converted to markdown format
        assert "[AllocMem()]" in md
        assert "[Memory Attributes]" in md

    def test_preserves_indented_code(self):
        body = extract_body(SAMPLE_HTML)
        md = convert_html_to_md(body)
        assert "APTR AllocMem" in md

    def test_strips_nav_images(self):
        html_with_nav = '<img src="../images/toc.gif" alt="[Contents]" border=0>'
        md = convert_html_to_md(html_with_nav)
        assert "toc.gif" not in md
        assert "[Contents]" not in md
```

- [ ] **Step 2: Run tests to verify they fail**

Run: `python3 -m pytest tests/test_scrape_adcd.py -v -k "Extract or Convert"`
Expected: ImportError for extract_body, extract_links, convert_html_to_md

- [ ] **Step 3: Implement extraction and conversion functions**

Add these functions to `scripts/scrape-adcd.py`:

```python
def fetch_page(url):
    """Fetch a page from the ADCD site. Returns HTML string or None on failure."""
    try:
        req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
        with urllib.request.urlopen(req, timeout=30) as resp:
            return resp.read().decode("latin-1", errors="replace")
    except urllib.error.URLError as e:
        print(f"  Warning: fetch failed for {url}: {e}", file=sys.stderr)
        return None
    except Exception as e:
        print(f"  Warning: unexpected error fetching {url}: {e}", file=sys.stderr)
        return None


def extract_body(html_text):
    """Extract the body content from an AG2HTML page.

    Looks for <!-- AG2HTML: BODY=START --> and <!-- AG2HTML: BODY=END --> markers.
    Falls back to extracting content between <pre> tags if markers are missing.
    """
    # Try markers first
    start_marker = "<!-- AG2HTML: BODY=START -->"
    end_marker = "<!-- AG2HTML: BODY=END -->"

    start_idx = html_text.find(start_marker)
    end_idx = html_text.find(end_marker)

    if start_idx != -1 and end_idx != -1:
        body = html_text[start_idx + len(start_marker):end_idx]
    else:
        # Fallback: extract between <pre> tags
        pre_match = re.search(r'<pre>(.*?)</pre>', html_text, re.DOTALL)
        if pre_match:
            body = pre_match.group(1)
        else:
            body = html_text

    # Strip <a name="..."> anchor tags (not real links)
    body = re.sub(r'<a\s+name="[^"]*">\s*</a>', '', body)
    body = re.sub(r'<a\s+name="[^"]*">', '', body)

    # Strip navigation images
    body = re.sub(r'<img\s+[^>]*>', '', body)

    return body.strip()


def extract_links(html_text):
    """Extract all href links from an HTML page. Returns list of URLs."""
    links = []
    for match in re.finditer(r'<a\s+href="([^"]+)"', html_text):
        href = match.group(1)
        # Skip anchor-only links and external links
        if href.startswith('#'):
            continue
        if 'node' in href or '_guide' in href:
            links.append(href)
    return links


def extract_title(html_text):
    """Extract the page title from <title> tag."""
    match = re.search(r'<title>(.*?)</title>', html_text, re.DOTALL)
    if match:
        title = match.group(1).strip()
        title = convert_entities(title)
        # Strip common prefixes
        for prefix in ["Amiga(R) RKM Libraries: ", "Amiga(R) RKM Devices: ",
                       "Amiga(R) Hardware Reference Manual: "]:
            if title.startswith(prefix):
                title = title[len(prefix):]
        return title
    return "untitled"


def convert_html_to_md(body):
    """Convert AG2HTML body content to markdown.

    The body is pre-formatted text with <a href> links interspersed.
    """
    # Convert <a href="...">text</a> to [text](url)
    body = re.sub(
        r'<a\s+href="([^"]+)"[^>]*>(.*?)</a>',
        lambda m: f'[{m.group(2)}]({m.group(1)})',
        body,
        flags=re.DOTALL
    )

    # Strip any remaining HTML tags
    body = re.sub(r'<[^>]+>', '', body)

    # Convert HTML entities
    body = convert_entities(body)

    return body
```

- [ ] **Step 4: Run tests to verify they pass**

Run: `python3 -m pytest tests/test_scrape_adcd.py -v`
Expected: All tests PASS

- [ ] **Step 5: Implement the crawl logic in `do_crawl()`**

Replace the `do_crawl()` stub with the full recursive crawler:

```python
def do_crawl(args):
    """Pass 1: Crawl and convert HTML to raw markdown."""
    print("Pass 1: Crawl & Convert")
    print("=" * 60)

    output_dir = Path(args.output) / "raw"
    output_dir.mkdir(parents=True, exist_ok=True)

    # Global node map: node_id -> {"manual": ..., "path": ..., "title": ...}
    node_map = {}
    # Track used slugs globally to avoid collisions
    used_slugs = set()

    manuals_to_crawl = MANUALS
    if hasattr(args, 'manual') and args.manual:
        manuals_to_crawl = [(m, g, t) for m, g, t in MANUALS if m == args.manual]
        if not manuals_to_crawl:
            print(f"Error: unknown manual '{args.manual}'", file=sys.stderr)
            print(f"Available: {', '.join(m for m, _, _ in MANUALS)}", file=sys.stderr)
            sys.exit(1)

    limit = getattr(args, 'limit', None)
    dry_run = getattr(args, 'dry_run', False)

    total_pages = 0
    total_warnings = 0

    for manual_slug, guide_dir, manual_title in manuals_to_crawl:
        print(f"\n--- {manual_title} ({guide_dir}) ---")
        manual_dir = output_dir / manual_slug
        if not dry_run:
            manual_dir.mkdir(parents=True, exist_ok=True)

        toc_url = f"{BASE_URL}/{guide_dir}/node0000.html"
        pages, warnings = crawl_manual(
            toc_url, guide_dir, manual_slug, manual_dir,
            node_map, used_slugs, limit=limit, dry_run=dry_run
        )
        total_pages += pages
        total_warnings += warnings

    # Write node map
    if not dry_run:
        map_path = output_dir / "node-map.json"
        with open(map_path, 'w') as f:
            json.dump(node_map, f, indent=2, sort_keys=True)
        print(f"\nNode map written: {map_path} ({len(node_map)} entries)")

    print(f"\n{'=' * 60}")
    print(f"Pass 1 complete: {total_pages} pages, {total_warnings} warnings")


def crawl_manual(toc_url, guide_dir, manual_slug, manual_dir,
                 node_map, used_slugs, limit=None, dry_run=False):
    """Crawl a single manual recursively from its TOC page."""
    visited = set()
    queue = [toc_url]
    pages = 0
    warnings = 0
    chapter_counter = 0

    while queue:
        if limit and pages >= limit:
            break

        url = queue.pop(0)
        if url in visited:
            continue
        visited.add(url)

        # Extract node ID from URL
        node_match = re.search(r'(node[0-9A-Fa-f]+)\.html', url)
        if not node_match:
            continue
        node_id = node_match.group(1)

        # Rate limit
        time.sleep(REQUEST_DELAY)

        # Fetch
        page_html = fetch_page(url)
        if page_html is None:
            # Retry once on failure
            time.sleep(1)
            page_html = fetch_page(url)
            if page_html is None:
                warnings += 1
                continue

        # Extract title and body
        title = extract_title(page_html)
        body = extract_body(page_html)

        if not body or len(body.strip()) < 10:
            warnings += 1
            continue

        # Check for BODY markers
        if "BODY=START" not in page_html and "BODY=END" not in page_html:
            print(f"  Warning: no BODY markers in {node_id} ({title})", file=sys.stderr)
            warnings += 1

        # Convert to markdown
        md_content = convert_html_to_md(body)

        # Generate slug and path
        slug = slugify_title(title, used_slugs)
        md_filename = f"{slug}.md"

        # Track in node map
        node_key = f"{guide_dir}/{node_id}"
        node_map[node_key] = {
            "manual": manual_slug,
            "path": f"{manual_slug}/{md_filename}",
            "title": title,
            "slug": slug,
        }

        # Write file
        if not dry_run:
            filepath = manual_dir / md_filename
            with open(filepath, 'w') as f:
                f.write(md_content)

        pages += 1
        if pages % 50 == 0:
            print(f"  {pages} pages crawled...")

        # Extract links for recursive crawl (only within same manual)
        for link in extract_links(page_html):
            # Resolve relative link
            if link.startswith('../'):
                # Link like ../Libraries_Manual_guide/nodeXXXX.html
                parts = link.split('/')
                if len(parts) >= 2:
                    link_guide = parts[-2]
                    link_file = parts[-1]
                    full_url = f"{BASE_URL}/{link_guide}/{link_file}"
                    # Only follow links within the same manual
                    if link_guide == guide_dir:
                        if full_url not in visited:
                            queue.append(full_url)
                    # Still record cross-manual links in node_map for later resolution
            elif not link.startswith('http'):
                full_url = f"{BASE_URL}/{guide_dir}/{link}"
                if full_url not in visited:
                    queue.append(full_url)

    print(f"  Completed: {pages} pages, {warnings} warnings")
    return pages, warnings
```

- [ ] **Step 6: Test the crawl with a small limit**

Run: `python3 scripts/scrape-adcd.py crawl --manual libraries --limit 5 --output /tmp/adcd-test`
Expected: 5 pages crawled, files created in `/tmp/adcd-test/raw/libraries/`

Verify output:
```bash
ls /tmp/adcd-test/raw/libraries/
cat /tmp/adcd-test/raw/node-map.json | python3 -m json.tool | head -20
cat /tmp/adcd-test/raw/libraries/*.md | head -30
```

- [ ] **Step 7: Commit**

```bash
git add scripts/scrape-adcd.py tests/test_scrape_adcd.py
git commit -m "Implement Pass 1: recursive HTML crawler with markdown conversion"
```

---

## Task 4: Pass 1 — Run Full Crawl

**Files:**
- No code changes — this is an execution task

- [ ] **Step 1: Run the full crawl on all 6 manuals**

Run: `python3 scripts/scrape-adcd.py crawl --output docs/references/adcd`
Expected: ~6,700 pages crawled in ~20 minutes. Node map at `docs/references/adcd/raw/node-map.json`.

- [ ] **Step 2: Verify output quality**

Spot-check several files across different manuals:
```bash
wc -l docs/references/adcd/raw/libraries/*.md | tail -5
wc -l docs/references/adcd/raw/devices/*.md | tail -5
cat docs/references/adcd/raw/node-map.json | python3 -c "import json,sys; d=json.load(sys.stdin); print(f'{len(d)} entries')"
```

- [ ] **Step 3: Inspect Autodocs 2.0 vs 3.5 structure**

Check whether Expansion 4 (version diff) is feasible:
```bash
python3 -c "
import json
with open('docs/references/adcd/raw/node-map.json') as f:
    nm = json.load(f)
ad20 = [v for k,v in nm.items() if 'Autodocs_2' in k]
ad35 = [v for k,v in nm.items() if 'Autodocs_3' in k]
print(f'Autodocs 2.0: {len(ad20)} pages')
print(f'Autodocs 3.5: {len(ad35)} pages')
# Check if titles contain library/function patterns
for entry in ad20[:10]:
    print(f'  2.0: {entry[\"title\"]}')
for entry in ad35[:10]:
    print(f'  3.5: {entry[\"title\"]}')
"
```

If function names are matchable by title, Expansion 4 proceeds. If not, log it as deferred.

- [ ] **Step 4: Do NOT commit raw files** (they're gitignored)

Just verify they exist:
```bash
du -sh docs/references/adcd/raw/
```

---

## Task 5: Pass 2 — Core Enrichment (frontmatter + link resolution)

**Files:**
- Modify: `scripts/scrape-adcd.py`
- Modify: `tests/test_scrape_adcd.py`

- [ ] **Step 1: Write tests for function detection**

Add to `tests/test_scrape_adcd.py`:
```python
from scrape_adcd import detect_functions, load_known_functions


class TestDetectFunctions:
    def test_matches_known_function_with_parens(self):
        known = {"AllocMem", "FreeMem", "Open", "Close"}
        text = "Call AllocMem() to get memory, then FreeMem() to release it."
        found = detect_functions(text, known)
        assert "AllocMem" in found
        assert "FreeMem" in found

    def test_ignores_without_parens(self):
        known = {"Open", "Close"}
        text = "Open the window and Close it when done."
        found = detect_functions(text, known)
        assert len(found) == 0

    def test_matches_with_parens(self):
        known = {"Open", "Close"}
        text = "Call Open() to get a handle, then Close() when done."
        found = detect_functions(text, known)
        assert "Open" in found
        assert "Close" in found

    def test_returns_unique_set(self):
        known = {"AllocMem"}
        text = "AllocMem() here and AllocMem() there."
        found = detect_functions(text, known)
        assert found == {"AllocMem"}
```

- [ ] **Step 2: Write tests for frontmatter generation**

```python
from scrape_adcd import generate_frontmatter


class TestGenerateFrontmatter:
    def test_basic(self):
        fm = generate_frontmatter(
            title="Memory Functions",
            manual="libraries",
            chapter=20,
            section="memory-functions",
            functions={"AllocMem", "FreeMem"},
            libraries={"exec.library"}
        )
        assert "title: " in fm
        assert "Memory Functions" in fm
        assert "manual: libraries" in fm
        assert "AllocMem" in fm
```

- [ ] **Step 3: Run tests to verify they fail**

Run: `python3 -m pytest tests/test_scrape_adcd.py -v -k "DetectFunctions or Frontmatter"`
Expected: FAIL — functions not defined

- [ ] **Step 4: Implement detection and enrichment functions**

Add to `scripts/scrape-adcd.py`:
```python
def load_known_functions(autodocs_dir):
    """Load the set of known function names from existing autodoc files."""
    functions = set()
    autodocs_path = Path(autodocs_dir)
    if not autodocs_path.exists():
        print(f"  Warning: autodocs dir not found: {autodocs_dir}", file=sys.stderr)
        return functions

    for md_file in autodocs_path.glob("*.md"):
        if md_file.name == "README.md":
            continue
        # Parse function names from the markdown index
        with open(md_file) as f:
            for line in f:
                # Match "- **FunctionName** (V36)" pattern
                m = re.match(r'- \*\*(\w+)\*\*', line)
                if m:
                    functions.add(m.group(1))
    return functions


def detect_functions(text, known_functions):
    """Detect known AmigaOS function names in text.

    Only matches FunctionName() with trailing parentheses to avoid
    false positives on common words like Open, Close, Read, Write.
    """
    found = set()
    for func in known_functions:
        # Match func followed by () — word boundary before, ( after
        pattern = r'\b' + re.escape(func) + r'\(\)'
        if re.search(pattern, text):
            found.add(func)
    return found


def detect_libraries(functions, func_to_library):
    """Given a set of function names, return the set of libraries they belong to."""
    libs = set()
    for func in functions:
        if func in func_to_library:
            libs.add(func_to_library[func])
    return libs


def generate_frontmatter(title, manual, chapter, section, functions, libraries):
    """Generate YAML frontmatter for an enriched markdown file."""
    lines = ["---"]
    lines.append(f'title: "{title}"')
    lines.append(f"manual: {manual}")
    if chapter is not None:
        lines.append(f"chapter: {chapter}")
    lines.append(f'section: "{section}"')
    if functions:
        func_list = ", ".join(sorted(functions))
        lines.append(f"functions_mentioned: [{func_list}]")
    if libraries:
        lib_list = ", ".join(sorted(libraries))
        lines.append(f"libraries_referenced: [{lib_list}]")
    lines.append("---")
    return "\n".join(lines)


def resolve_links(md_text, node_map, current_manual):
    """Replace node ID links with resolved markdown paths."""
    def replace_link(match):
        link_text = match.group(1)
        url = match.group(2)

        # Extract guide_dir/nodeXXXX from the URL
        m = re.search(r'([^/]+_guide|[^/]+_\d+\._guide)/(node[0-9A-Fa-f]+)\.html', url)
        if not m:
            return f'[{link_text}]({url})'

        guide_dir = m.group(1)
        node_id = m.group(2)
        key = f"{guide_dir}/{node_id}"

        if key in node_map:
            resolved_path = node_map[key]["path"]
            return f'[{link_text}]({resolved_path})'
        else:
            return f'[{link_text}]({url})'

    return re.sub(r'\[([^\]]+)\]\(([^)]+)\)', replace_link, md_text)
```

- [ ] **Step 5: Run tests to verify they pass**

Run: `python3 -m pytest tests/test_scrape_adcd.py -v`
Expected: All tests PASS

- [ ] **Step 6: Implement `do_enrich()` with core enrichment**

Replace the `do_enrich()` stub:
```python
def do_enrich(args):
    """Pass 2: Enrich with frontmatter, indexes, cross-references."""
    print("Pass 2: Enrich")
    print("=" * 60)

    output_dir = Path(args.output)
    raw_dir = output_dir / "raw"

    # Load node map
    node_map_path = raw_dir / "node-map.json"
    if not node_map_path.exists():
        print("Error: node-map.json not found. Run 'crawl' first.", file=sys.stderr)
        sys.exit(1)

    with open(node_map_path) as f:
        node_map = json.load(f)

    print(f"Loaded node map: {len(node_map)} entries")

    # Load known functions from existing autodocs
    known_functions = load_known_functions("docs/references/autodocs")
    print(f"Loaded {len(known_functions)} known functions from autodocs")

    # Build function-to-library map
    func_to_library = build_func_to_library_map("docs/references/autodocs")

    # Process each raw file
    all_files = []  # List of (path, frontmatter_data, md_content)
    function_index = {}  # function -> [file_paths]
    type_index = {}  # type -> [file_paths]

    for manual_slug, _, manual_title in MANUALS:
        manual_raw_dir = raw_dir / manual_slug
        if not manual_raw_dir.exists():
            print(f"  Skipping {manual_slug} (not crawled)")
            continue

        manual_out_dir = output_dir / manual_slug
        manual_out_dir.mkdir(parents=True, exist_ok=True)

        print(f"\n--- Enriching: {manual_title} ---")

        for md_file in sorted(manual_raw_dir.glob("*.md")):
            with open(md_file) as f:
                raw_content = f.read()

            # Find this file's node map entry
            slug = md_file.stem
            entry = None
            for k, v in node_map.items():
                if v.get("slug") == slug and v.get("manual") == manual_slug:
                    entry = v
                    break

            title = entry["title"] if entry else slug
            section = slug

            # Detect functions
            functions = detect_functions(raw_content, known_functions)
            libraries = detect_libraries(functions, func_to_library)

            # Resolve links
            enriched = resolve_links(raw_content, node_map, manual_slug)

            # Generate frontmatter
            fm = generate_frontmatter(
                title=title,
                manual=manual_slug,
                chapter=None,
                section=section,
                functions=functions,
                libraries=libraries,
            )

            # Add attribution header
            attribution = (
                "> Source: Amiga Developer CD v2.1, hosted at amigadev.elowar.com. "
                "Original content (C) Commodore-Amiga / Amiga Inc.\n\n"
            )

            full_content = fm + "\n\n" + attribution + enriched

            # Write enriched file
            out_path = manual_out_dir / md_file.name
            with open(out_path, 'w') as f:
                f.write(full_content)

            # Track for index building
            rel_path = f"{manual_slug}/{md_file.name}"
            all_files.append((rel_path, title, functions, libraries))

            # Build function index
            for func in functions:
                if func not in function_index:
                    function_index[func] = []
                function_index[func].append((rel_path, title))

        print(f"  Enriched {len(list(manual_raw_dir.glob('*.md')))} files")

    # Generate INDEX.md
    generate_index(output_dir, all_files)

    # Generate FUNCTIONS.md
    generate_functions_xref(output_dir, function_index)

    # Generate ATTRIBUTION.md
    generate_attribution(output_dir)

    # Generate README.md
    generate_readme(output_dir, all_files)

    print(f"\n{'=' * 60}")
    print(f"Pass 2 complete: {len(all_files)} files enriched")


def build_func_to_library_map(autodocs_dir):
    """Build a map of function_name -> library_name from autodoc files."""
    func_map = {}
    autodocs_path = Path(autodocs_dir)
    if not autodocs_path.exists():
        return func_map
    for md_file in autodocs_path.glob("*.md"):
        if md_file.name == "README.md":
            continue
        lib_name = md_file.stem  # e.g., "dos.library"
        with open(md_file) as f:
            for line in f:
                m = re.match(r'- \*\*(\w+)\*\*', line)
                if m:
                    func_map[m.group(1)] = lib_name
    return func_map


def generate_index(output_dir, all_files):
    """Generate INDEX.md — master alphabetical index."""
    entries = {}  # term -> [(path, title)]
    for path, title, functions, libraries in all_files:
        # Index by title
        if title not in entries:
            entries[title] = []
        entries[title].append((path, title))
        # Index by function
        for func in functions:
            if func not in entries:
                entries[func] = []
            entries[func].append((path, title))

    lines = ["# ADCD Master Index\n"]
    lines.append("> Alphabetical index of all topics, functions, and concepts.\n")
    for term in sorted(entries.keys(), key=str.lower):
        locations = entries[term]
        links = ", ".join(f"[{t}]({p})" for p, t in locations[:5])
        lines.append(f"- **{term}** — {links}")

    index_path = output_dir / "INDEX.md"
    with open(index_path, 'w') as f:
        f.write("\n".join(lines))
    print(f"  INDEX.md: {len(entries)} entries")


def generate_functions_xref(output_dir, function_index):
    """Generate FUNCTIONS.md — function cross-reference."""
    lines = ["# Function Cross-Reference\n"]
    lines.append("> For every known AmigaOS function, all ADCD pages that discuss it.\n")

    for func in sorted(function_index.keys()):
        locations = function_index[func]
        links = ", ".join(f"[{t}]({p})" for p, t in locations)
        lines.append(f"### {func}\n")
        for path, title in locations:
            lines.append(f"- [{title}]({path})")
        lines.append("")

    xref_path = output_dir / "FUNCTIONS.md"
    with open(xref_path, 'w') as f:
        f.write("\n".join(lines))
    print(f"  FUNCTIONS.md: {len(function_index)} functions cross-referenced")


def generate_attribution(output_dir):
    """Generate ATTRIBUTION.md."""
    content = """# Attribution

This documentation was converted from the **Amiga Developer CD v2.1** (ADCD),
originally published by Commodore-Amiga, Inc.

**Source:** [amigadev.elowar.com](http://amigadev.elowar.com/)

**Original Copyright:** (C) Commodore-Amiga, Inc. / Amiga, Inc.

The ADCD content has been freely hosted at amigadev.elowar.com for over 20 years.
This conversion to markdown format is provided as part of the
[amiport](https://github.com/bdgscotland/amiport) project — an AI-powered toolkit
for porting POSIX/Linux C software to AmigaOS.

If a rights holder objects to this conversion, the generated files can be removed.
The conversion script (`scripts/scrape-adcd.py`) is the primary artifact; the
generated documentation is reproducible from it.
"""
    with open(output_dir / "ATTRIBUTION.md", 'w') as f:
        f.write(content)


def generate_readme(output_dir, all_files):
    """Generate README.md for the ADCD reference docs."""
    manual_counts = {}
    for path, _, _, _ in all_files:
        manual = path.split('/')[0]
        manual_counts[manual] = manual_counts.get(manual, 0) + 1

    lines = ["# AmigaOS Developer Reference (ADCD 2.1)\n"]
    lines.append("Converted from the Amiga Developer CD v2.1 by `scripts/scrape-adcd.py`.\n")
    lines.append("## Contents\n")
    lines.append("| Manual | Pages | Directory |")
    lines.append("|--------|-------|-----------|")
    for manual_slug, _, manual_title in MANUALS:
        count = manual_counts.get(manual_slug, 0)
        if count > 0:
            lines.append(f"| {manual_title} | {count} | [{manual_slug}/]({manual_slug}/) |")

    lines.append(f"\n**Total: {len(all_files)} pages**\n")
    lines.append("## Quick Links\n")
    lines.append("- [Master Index](INDEX.md)")
    lines.append("- [Function Cross-Reference](FUNCTIONS.md)")
    lines.append("- [Type Index](TYPES.md)")
    lines.append("- [Attribution](ATTRIBUTION.md)")

    with open(output_dir / "README.md", 'w') as f:
        f.write("\n".join(lines))
```

- [ ] **Step 7: Run tests**

Run: `python3 -m pytest tests/test_scrape_adcd.py -v`
Expected: All tests PASS

- [ ] **Step 8: Commit**

```bash
git add scripts/scrape-adcd.py tests/test_scrape_adcd.py
git commit -m "Implement Pass 2: enrichment with frontmatter, function detection, indexes"
```

---

## Task 6: Pass 2 — Expansions (porting summaries, includes, types, examples)

**Files:**
- Modify: `scripts/scrape-adcd.py`
- Modify: `tests/test_scrape_adcd.py`

- [ ] **Step 1: Write tests for type detection**

Add to `tests/test_scrape_adcd.py`:
```python
from scrape_adcd import detect_types, KNOWN_AMIGA_TYPES


class TestDetectTypes:
    def test_struct_keyword(self):
        text = "Fill in the struct FileInfoBlock fields."
        found = detect_types(text)
        assert "FileInfoBlock" in found

    def test_known_type_standalone(self):
        text = "Pass the Window pointer to CloseWindow()."
        found = detect_types(text)
        assert "Window" in found

    def test_ignores_common_words(self):
        text = "The window was open and the screen was bright."
        found = detect_types(text)
        # "window" lowercase should not match
        assert "Window" not in found or "window" not in text.split()


class TestExtractExamples:
    def test_detects_code_block(self):
        from scrape_adcd import extract_code_examples
        text = """Some prose text.

    #include <proto/exec.h>

    int main(void)
    {
        APTR mem = AllocMem(1024, MEMF_PUBLIC);
        if (mem) {
            FreeMem(mem, 1024);
        }
        return 0;
    }

More prose after."""
        examples = extract_code_examples(text, "exec", "test-section", "libraries/test.md")
        assert len(examples) >= 1
        assert "AllocMem" in examples[0]["code"]
```

- [ ] **Step 2: Run tests to verify they fail**

Run: `python3 -m pytest tests/test_scrape_adcd.py -v -k "DetectTypes or ExtractExamples"`
Expected: FAIL

- [ ] **Step 3: Implement expansion functions**

Add to `scripts/scrape-adcd.py`:

```python
# Seed list of known Amiga types (structs, typedefs)
# Derived from common AmigaOS programming patterns
KNOWN_AMIGA_TYPES = {
    # Exec
    "Task", "Process", "MsgPort", "Message", "Node", "List", "MinList",
    "MinNode", "Library", "Device", "IORequest", "IOStdReq", "Interrupt",
    "MemHeader", "MemChunk", "MemEntry", "MemList", "Resident",
    "SignalSemaphore", "SemaphoreRequest",
    # DOS
    "FileInfoBlock", "FileHandle", "FileLock", "InfoData", "DateStamp",
    "AnchorPath", "RDArgs", "CSource", "CommandLineInterface",
    "DosPacket", "StandardPacket", "DevProc", "DosList", "AssignList",
    # Intuition
    "Window", "Screen", "IntuiMessage", "Gadget", "PropInfo", "StringInfo",
    "Image", "Border", "IntuiText", "Menu", "MenuItem", "Requester",
    "NewScreen", "NewWindow", "EasyStruct", "Remember",
    # Graphics
    "RastPort", "BitMap", "ViewPort", "View", "ColorMap", "Layer",
    "TextFont", "TextAttr", "TmpRas", "AreaInfo", "GelsInfo",
    "SimpleSprite", "UCopList", "CopList", "CopIns",
    # GadTools
    "NewGadget", "NewMenu",
    # ASL
    "FileRequester", "FontRequester", "ScreenModeRequester",
    # Workbench
    "DiskObject", "AppWindow", "AppIcon", "AppMenuItem",
    # IFF
    "IFFHandle",
    # Other
    "TagItem", "Hook", "ClockData",
}


def detect_types(text):
    """Detect known Amiga struct/type names in text."""
    found = set()
    # Match "struct TypeName" pattern
    for m in re.finditer(r'\bstruct\s+(\w+)', text):
        type_name = m.group(1)
        if type_name[0].isupper():
            found.add(type_name)
    # Match known types as standalone words (PascalCase only)
    for type_name in KNOWN_AMIGA_TYPES:
        if re.search(r'\b' + re.escape(type_name) + r'\b', text):
            found.add(type_name)
    return found


def extract_code_examples(text, library, section_slug, source_path):
    """Extract code examples from markdown text.

    Returns list of dicts: {"code": str, "library": str, "section": str, "source": str}
    """
    examples = []
    lines = text.split('\n')
    i = 0
    while i < len(lines):
        # Detect indented blocks (4+ spaces)
        if lines[i].startswith('    ') and lines[i].strip():
            block_lines = []
            while i < len(lines) and (lines[i].startswith('    ') or lines[i].strip() == ''):
                block_lines.append(lines[i])
                i += 1
                # Stop on 2+ consecutive blank lines
                blank_count = 0
                for bl in block_lines[-2:]:
                    if bl.strip() == '':
                        blank_count += 1
                if blank_count >= 2:
                    break

            code = '\n'.join(line[4:] if line.startswith('    ') else line
                           for line in block_lines).strip()

            # Filter: must be 5+ lines, contain a function call, and have code markers
            code_lines = [l for l in code.split('\n') if l.strip()]
            has_func_call = bool(re.search(r'\w+\(', code))
            has_code_marker = any(c in code for c in ['#include', '{', 'return', ';'])

            if len(code_lines) >= 5 and has_func_call and has_code_marker:
                examples.append({
                    "code": code,
                    "library": library,
                    "section": section_slug,
                    "source": source_path,
                })
        else:
            i += 1

    return examples


def generate_includes_json(output_dir, node_map):
    """Generate INCLUDES.json mapping include paths to ADCD chapters."""
    # Map library names to proto header paths
    lib_to_include = {
        "exec": "proto/exec.h",
        "dos": "proto/dos.h",
        "intuition": "proto/intuition.h",
        "graphics": "proto/graphics.h",
        "utility": "proto/utility.h",
        "gadtools": "proto/gadtools.h",
        "asl": "proto/asl.h",
        "commodities": "proto/commodities.h",
        "console": "devices/console.h",
        "diskfont": "proto/diskfont.h",
        "icon": "proto/icon.h",
        "iffparse": "proto/iffparse.h",
        "input": "devices/input.h",
        "keymap": "proto/keymap.h",
        "layers": "proto/layers.h",
        "locale": "proto/locale.h",
        "mathffp": "proto/mathffp.h",
        "mathieeedoubbas": "proto/mathieeedoubbas.h",
        "rexxsyslib": "proto/rexxsyslib.h",
        "timer": "devices/timer.h",
        "workbench": "proto/wb.h",
    }

    includes = {}
    for lib_short, include_path in lib_to_include.items():
        lib_name = f"{lib_short}.library"
        # Find chapters that mention this library
        chapters = []
        for key, entry in node_map.items():
            title_lower = entry["title"].lower()
            if lib_short in title_lower:
                chapters.append(entry["path"])

        if chapters:
            includes[include_path] = {
                "library": lib_name,
                "chapters": sorted(set(chapters))[:10],  # Cap at 10
                "autodoc": f"autodocs/{lib_name}.md",
            }

    path = output_dir / "INCLUDES.json"
    with open(path, 'w') as f:
        json.dump(includes, f, indent=2, sort_keys=True)
    print(f"  INCLUDES.json: {len(includes)} include mappings")


def generate_types_index(output_dir, type_index):
    """Generate TYPES.md — struct/typedef/enum index."""
    lines = ["# AmigaOS Type Index\n"]
    lines.append("> Structs, typedefs, and enums referenced across all ADCD manuals.\n")

    for type_name in sorted(type_index.keys()):
        locations = type_index[type_name]
        lines.append(f"## {type_name}\n")
        for path, title in locations[:10]:
            lines.append(f"- [{title}]({path})")
        lines.append("")

    with open(output_dir / "TYPES.md", 'w') as f:
        f.write("\n".join(lines))
    print(f"  TYPES.md: {len(type_index)} types indexed")
```

- [ ] **Step 4: Run tests to verify they pass**

Run: `python3 -m pytest tests/test_scrape_adcd.py -v`
Expected: All tests PASS

- [ ] **Step 5: Integrate expansions into `do_enrich()`**

Add type detection, example extraction, and includes generation to the `do_enrich()` function's main loop and post-processing. Add these tracking variables and calls:

In the per-file loop, add:
```python
            # Detect types
            types = detect_types(raw_content)
            for type_name in types:
                if type_name not in type_index:
                    type_index[type_name] = []
                type_index[type_name].append((rel_path, title))

            # Extract code examples
            examples = extract_code_examples(raw_content, manual_slug, slug, rel_path)
            all_examples.extend(examples)
```

After the main loop, add:
```python
    # Generate TYPES.md
    generate_types_index(output_dir, type_index)

    # Generate INCLUDES.json
    generate_includes_json(output_dir, node_map)

    # Write extracted examples
    examples_dir = output_dir / "examples"
    example_count = 0
    for ex in all_examples:
        lib_dir = examples_dir / ex["library"]
        lib_dir.mkdir(parents=True, exist_ok=True)
        filename = slugify_title(ex["section"]) + ".c"
        filepath = lib_dir / filename
        if not filepath.exists():  # Don't overwrite
            header = (
                f'/* Source: ADCD 2.1\n'
                f' * Section: {ex["section"]}\n'
                f' * Library: {ex["library"]}\n'
                f' * ADCD reference: {ex["source"]}\n'
                f' */\n\n'
            )
            with open(filepath, 'w') as f:
                f.write(header + ex["code"] + '\n')
            example_count += 1
    print(f"  Examples: {example_count} code examples extracted")
```

- [ ] **Step 6: Run tests**

Run: `python3 -m pytest tests/test_scrape_adcd.py -v`
Expected: All tests PASS

- [ ] **Step 7: Commit**

```bash
git add scripts/scrape-adcd.py tests/test_scrape_adcd.py
git commit -m "Add Pass 2 expansions: type index, includes mapping, code examples"
```

---

## Task 7: Run Full Pipeline and Verify

**Files:**
- No code changes — execution and verification

- [ ] **Step 1: Run the full pipeline**

Run: `python3 scripts/scrape-adcd.py all --output docs/references/adcd`
Expected: Pass 1 crawls ~6,700 pages (~20 min), Pass 2 enriches all files.

- [ ] **Step 2: Verify output structure**

```bash
find docs/references/adcd -name "*.md" -not -path "*/raw/*" | wc -l
ls docs/references/adcd/INDEX.md docs/references/adcd/FUNCTIONS.md docs/references/adcd/TYPES.md
ls docs/references/adcd/INCLUDES.json
ls docs/references/adcd/ATTRIBUTION.md docs/references/adcd/README.md
ls docs/references/adcd/examples/ | head -10
du -sh docs/references/adcd/
```

- [ ] **Step 3: Spot-check content quality**

```bash
# Check a Libraries Manual file has frontmatter
head -20 docs/references/adcd/libraries/exec-memory-allocation.md 2>/dev/null || head -20 docs/references/adcd/libraries/*.md | head -30
# Check FUNCTIONS.md has entries
wc -l docs/references/adcd/FUNCTIONS.md
head -30 docs/references/adcd/FUNCTIONS.md
# Check TYPES.md
head -30 docs/references/adcd/TYPES.md
# Check an extracted example
cat docs/references/adcd/examples/libraries/*.c 2>/dev/null | head -30 || echo "Check examples dir structure"
```

- [ ] **Step 4: Run all tests one final time**

Run: `python3 -m pytest tests/test_scrape_adcd.py -v`
Expected: All tests PASS

---

## Task 8: Agent Integration and Documentation

**Files:**
- Modify: `.claude/agents/code-transformer.md`
- Modify: `.claude/agents/source-analyzer.md`
- Modify: `.claude/agents/build-manager.md`
- Modify: `CLAUDE.md`
- Modify: `README.md`
- Modify: `docs/architecture.md`

- [ ] **Step 1: Update code-transformer agent**

Add to `.claude/agents/code-transformer.md` after the "## Transformation Order" section:

```markdown
## Reference Documentation

When making transformation decisions, consult these ADCD reference docs for HOW to use AmigaOS functions:
- `docs/references/adcd/libraries/` — Full prose + examples for Exec, DOS, Intuition, Graphics
- `docs/references/adcd/INCLUDES.json` — Maps `#include <proto/*.h>` to relevant chapters
- `docs/references/adcd/FUNCTIONS.md` — Cross-reference: find all documentation for any function
- `docs/references/adcd/examples/` — Real AmigaOS code examples by library
- `docs/references/autodocs/` — API function signatures (existing)
```

- [ ] **Step 2: Update source-analyzer agent**

Add to `.claude/agents/source-analyzer.md` after the "## Approach" section:

```markdown
## ADCD Reference

For understanding AmigaOS API patterns and usage:
- `docs/references/adcd/FUNCTIONS.md` — Find all ADCD pages that discuss any AmigaOS function
- `docs/references/adcd/TYPES.md` — Index of all AmigaOS structs, typedefs, enums
- `docs/references/adcd/INCLUDES.json` — Map include file paths to documentation chapters
```

- [ ] **Step 3: Update build-manager agent**

Add to `.claude/agents/build-manager.md` after "## Compiler Knowledge":

```markdown
## Device Documentation

For linking decisions and device I/O patterns:
- `docs/references/adcd/devices/` — Full RKM Devices Manual (console, timer, serial, etc.)
- `docs/references/adcd/INCLUDES.json` — Maps device headers to documentation
```

- [ ] **Step 4: Update CLAUDE.md**

Add to the "Key References" section:
```markdown
- `docs/references/adcd/` — **Complete ADCD 2.1 in markdown** (Libraries, Devices, Hardware, Amiga Mail, Autodocs)
- `docs/references/adcd/FUNCTIONS.md` — Function cross-reference across all ADCD manuals
- `docs/references/adcd/TYPES.md` — Struct/typedef/enum index
- `docs/references/adcd/INCLUDES.json` — Include file → documentation chapter mapping
```

Add to "Build Instructions":
```
make scrape-adcd   # Scrape ADCD and regenerate reference docs
```

- [ ] **Step 5: Update README.md**

Add a "Reference Documentation" section describing the ADCD markdown conversion.

- [ ] **Step 6: Update docs/architecture.md**

Add the ADCD reference docs to the architecture diagram and component table.

- [ ] **Step 7: Commit**

```bash
git add .claude/agents/code-transformer.md .claude/agents/source-analyzer.md .claude/agents/build-manager.md CLAUDE.md README.md docs/architecture.md
git commit -m "Integrate ADCD reference docs into agents and project documentation"
```

---

## Task 9: Final Commit — Generated ADCD Docs

**Files:**
- Add all generated docs under `docs/references/adcd/` (excluding `raw/`)

- [ ] **Step 1: Verify no raw files will be committed**

```bash
git status docs/references/adcd/raw/
# Should show nothing (gitignored)
```

- [ ] **Step 2: Add and commit all generated docs**

```bash
git add docs/references/adcd/ .gitattributes
git add -f docs/references/adcd/  # Force-add in case gitignore patterns interfere
git status  # Verify only enriched files, not raw/
git commit -m "Add complete ADCD 2.1 reference documentation in markdown

Scraped and converted ~6,700 pages from amigadev.elowar.com:
- RKM Libraries Manual (Exec, DOS, Intuition, Graphics, etc.)
- RKM Devices Manual (console, timer, serial, etc.)
- Hardware Reference Manual
- Amiga Mail Volume II
- Includes and Autodocs 2.0/3.5

Each file has YAML frontmatter with function/type/library metadata.
Cross-reference indexes: FUNCTIONS.md, TYPES.md, INCLUDES.json.
Extracted code examples in examples/ directory."
```

- [ ] **Step 3: Verify repo size is reasonable**

```bash
du -sh docs/references/adcd/
git diff --stat HEAD~1
```
