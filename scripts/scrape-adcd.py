#!/usr/bin/env python3
"""
scrape-adcd.py — Scrape ADCD (Amiga Developer CD) HTML manuals into markdown.

Two-pass pipeline:
  Pass 1 (crawl): Fetch HTML pages from the ADCD mirror, extract content,
                   convert to markdown, and write per-chapter .md files.
  Pass 2 (enrich): Add YAML frontmatter, detect function signatures,
                    resolve cross-references, and generate index files.

Usage:
  # Crawl all manuals:
  python3 scripts/scrape-adcd.py crawl --output docs/references/adcd/

  # Crawl a single manual:
  python3 scripts/scrape-adcd.py crawl --output docs/references/adcd/ --manual libraries

  # Enrich previously crawled files:
  python3 scripts/scrape-adcd.py enrich --output docs/references/adcd/

  # Run both passes:
  python3 scripts/scrape-adcd.py all --output docs/references/adcd/
"""

import argparse
import collections
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

# --------------------------------------------------------------------------- #
# Constants
# --------------------------------------------------------------------------- #

BASE_URL = "http://amigadev.elowar.com"

# Each entry: (slug, guide_dir, display_title)
MANUALS = [
    ("libraries", "Libraries_Manual_guide", "Amiga RKM Libraries"),
    ("devices", "Devices_Manual_guide", "Amiga RKM Devices"),
    ("hardware", "Hardware_Manual_guide", "Amiga Hardware Reference Manual"),
    ("amiga-mail", "AmigaMail_Vol2_guide", "Amiga Mail Volume II"),
    ("autodocs-2.0", "Includes_and_Autodocs_2._guide", "Includes and Autodocs 2.0"),
    ("autodocs-3.5", "Includes_and_Autodocs_3._guide", "Includes and Autodocs 3.5"),
]

USER_AGENT = "amiport-scraper/1.0 (Amiga porting toolkit; +https://github.com/amiport)"

REQUEST_DELAY = 1.0  # seconds between HTTP requests (be polite)

# Prefixes to strip from <title> tags
TITLE_PREFIXES = [
    "Amiga(R) RKM Libraries: ",
    "Amiga\u00ae RKM Libraries: ",
    "Amiga(R) RKM Devices: ",
    "Amiga\u00ae RKM Devices: ",
    "Amiga Hardware Reference Manual: ",
    "Amiga Mail Vol.2 Guide: ",
    "Amiga ROM Kernel Reference Manual: ",
]

# Characters that indicate a line is code rather than ASCII art
CODE_CHARS = set("=(){};")

# Common Amiga type names for type detection (~50 types)
KNOWN_AMIGA_TYPES = {
    # Exec
    "Task", "Process", "MsgPort", "Message", "Node", "List", "MinList",
    "MinNode", "Library", "Device", "IORequest", "IOStdReq", "Interrupt",
    "MemHeader", "SignalSemaphore", "Resident",
    # DOS
    "FileInfoBlock", "FileHandle", "FileLock", "InfoData", "DateStamp",
    "AnchorPath", "RDArgs", "DosPacket", "DosList",
    # Intuition
    "Window", "Screen", "IntuiMessage", "Gadget", "PropInfo", "StringInfo",
    "Image", "Border", "IntuiText", "Menu", "MenuItem", "Requester",
    "NewScreen", "NewWindow", "EasyStruct",
    # Graphics
    "RastPort", "BitMap", "ViewPort", "View", "ColorMap", "Layer",
    "TextFont", "TextAttr", "TmpRas", "AreaInfo", "SimpleSprite",
    # Other
    "TagItem", "Hook", "ClockData", "NewGadget", "NewMenu",
    "FileRequester", "FontRequester", "DiskObject", "AppWindow", "IFFHandle",
}

# Mapping of library slugs to Amiga include paths
LIB_TO_INCLUDE = {
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
    "timer": "devices/timer.h",
    "workbench": "proto/wb.h",
}


# --------------------------------------------------------------------------- #
# Utility functions
# --------------------------------------------------------------------------- #

def slugify_title(title: str, used_slugs: Optional[set] = None) -> str:
    """Convert a title string to a URL/filename-safe slug.

    - Lowercase, replace non-alphanumeric with hyphens
    - Collapse multiple hyphens, strip leading/trailing hyphens
    - Truncate to 60 characters (on a hyphen boundary if possible)
    - If used_slugs is provided, append -2, -3, ... to avoid collisions
    """
    # Convert HTML entities first
    text = convert_entities(title)
    # Lowercase
    text = text.lower()
    # Replace non-alphanumeric with hyphens
    text = re.sub(r"[^a-z0-9]+", "-", text)
    # Collapse multiple hyphens and strip edges
    text = re.sub(r"-+", "-", text).strip("-")
    # Truncate to 60 chars on a hyphen boundary
    if len(text) > 60:
        text = text[:60]
        # Try to break on a hyphen boundary
        last_hyphen = text.rfind("-")
        if last_hyphen > 30:
            text = text[:last_hyphen]
        text = text.rstrip("-")

    base = text
    if used_slugs is not None:
        if base in used_slugs:
            n = 2
            while f"{base}-{n}" in used_slugs:
                n += 1
            text = f"{base}-{n}"
        used_slugs.add(text)

    return text


def convert_entities(text: str) -> str:
    """Convert HTML numeric and named entities to plain text.

    Handles &#NNN; numeric entities, standard named entities (&amp;, &lt;,
    &gt;, &quot;), and converts &reg; to (R).
    """
    # Replace &reg; with (R) before generic unescape
    text = re.sub(r"&reg;", "(R)", text, flags=re.IGNORECASE)
    # Use stdlib html.unescape for everything else
    text = html.unescape(text)
    return text


# --------------------------------------------------------------------------- #
# Pass 1: Content extraction and conversion
# --------------------------------------------------------------------------- #

def fetch_page(url: str) -> Optional[str]:
    """Fetch an HTML page via HTTP GET. Returns HTML string or None on failure.

    Uses latin-1 decoding (common for old HTML). Retries once on timeout.
    """
    req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    for attempt in range(2):
        try:
            with urllib.request.urlopen(req, timeout=30) as resp:
                return resp.read().decode("latin-1")
        except (urllib.error.URLError, OSError) as e:
            if attempt == 0:
                print(f"  warning: fetch failed for {url}, retrying... ({e})",
                      file=sys.stderr)
                time.sleep(1)
            else:
                print(f"  error: fetch failed for {url}: {e}", file=sys.stderr)
                return None
    return None


def extract_body(html_text: str) -> str:
    """Extract the main content body from an AG2HTML page.

    Looks for content between BODY=START and BODY=END markers.
    Falls back to <pre> content if markers are absent.
    Strips <a name="..."> anchors and <img> navigation tags.
    """
    # Try BODY markers first
    m = re.search(
        r"<!--\s*AG2HTML:\s*BODY=START\s*-->(.*?)<!--\s*AG2HTML:\s*BODY=END\s*-->",
        html_text, re.DOTALL
    )
    if m:
        body = m.group(1)
    else:
        # Fallback: extract <pre> content
        m = re.search(r"<pre[^>]*>(.*?)</pre>", html_text, re.DOTALL | re.IGNORECASE)
        if m:
            body = m.group(1)
        else:
            body = html_text

    # Strip <a name="..."> anchors (self-closing or with </a>)
    body = re.sub(r'<a\s+name="[^"]*"\s*>\s*</a>', '', body, flags=re.IGNORECASE)
    body = re.sub(r'<a\s+name="[^"]*"\s*/?>', '', body, flags=re.IGNORECASE)

    # Strip <img> navigation tags
    body = re.sub(r'<img\s+[^>]*>', '', body, flags=re.IGNORECASE)

    return body


def extract_links(html_text: str) -> list:
    """Extract href links from <a> tags, filtering to node/guide links.

    Returns a list of relative URL strings. Skips anchor-only (#) links
    and <a name="..."> tags.
    """
    links = []
    for m in re.finditer(r'<a\s+href="([^"]*)"', html_text, re.IGNORECASE):
        href = m.group(1)
        # Skip anchor-only links
        if href.startswith("#"):
            continue
        # Only include links that look like node or guide pages
        if "node" in href or "_guide" in href:
            links.append(href)
    return links


def extract_title(html_text: str) -> str:
    """Extract and clean the page title from a <title> tag.

    Strips common ADCD prefixes and converts HTML entities.
    """
    m = re.search(r"<title>(.*?)</title>", html_text, re.IGNORECASE | re.DOTALL)
    if not m:
        return "Untitled"
    title = m.group(1).strip()
    title = convert_entities(title)
    # Strip known prefixes
    for prefix in TITLE_PREFIXES:
        if title.startswith(prefix):
            title = title[len(prefix):]
            break
    return title.strip()


def convert_html_to_md(body: str) -> str:
    """Convert extracted HTML body to markdown.

    - Converts <a href="url">text</a> to [text](url)
    - Strips remaining HTML tags
    - Converts HTML entities
    - Detects indented code blocks (4+ leading spaces with code characters)
      and wraps them in fenced ```c blocks
    - Preserves indented ASCII art (no code characters) as-is
    """
    # Convert <a href="...">text</a> to markdown links
    body = re.sub(
        r'<a\s+href="([^"]*)"[^>]*>(.*?)</a>',
        r'[\2](\1)',
        body,
        flags=re.IGNORECASE | re.DOTALL
    )

    # Strip remaining HTML tags
    body = re.sub(r'<[^>]+>', '', body)

    # Convert HTML entities
    body = convert_entities(body)

    # Detect and wrap indented code blocks
    lines = body.split("\n")
    result = []
    i = 0
    while i < len(lines):
        line = lines[i]
        # Check if this line is indented (4+ spaces)
        if re.match(r"    ", line) and line.strip():
            # Collect the entire indented block
            block_lines = []
            while i < len(lines) and (re.match(r"    ", lines[i]) or not lines[i].strip()):
                block_lines.append(lines[i])
                i += 1
            # Remove trailing blank lines from block
            while block_lines and not block_lines[-1].strip():
                result.append(block_lines.pop())

            if not block_lines:
                continue

            # Check if block contains code characters
            block_text = "".join(block_lines)
            has_code = any(c in block_text for c in CODE_CHARS)

            if has_code:
                result.append("```c")
                for bl in block_lines:
                    result.append(bl)
                result.append("```")
            else:
                result.extend(block_lines)
        else:
            result.append(line)
            i += 1

    return "\n".join(result)


def resolve_links(md_text: str, node_map: dict, current_manual: str) -> str:
    """Replace node URLs in markdown links with resolved markdown paths.

    Looks up each ../Guide/nodeXXXX.html reference in node_map.
    Known nodes get their path substituted; unknown nodes are left as-is.
    """
    def _replace(m):
        text = m.group(1)
        url = m.group(2)
        # Extract the guide_dir/nodeXXXX part from ../guide_dir/nodeXXXX.html
        match = re.match(r"\.\./([^/]+/node[0-9A-Fa-f]+)\.html", url)
        if not match:
            return m.group(0)
        key = match.group(1)
        if key in node_map:
            return f"[{text}]({node_map[key]['path']})"
        return m.group(0)

    return re.sub(r'\[([^\]]*)\]\(([^)]*)\)', _replace, md_text)


def crawl_manual(toc_url, guide_dir, manual_slug, manual_dir, node_map,
                 used_slugs, limit=None, dry_run=False):
    """BFS crawl a single manual starting from its TOC page.

    For each page: fetch, extract title/body, convert to markdown,
    slugify filename, write to manual_dir, add to node_map.
    Only follows links within the same manual's guide_dir.
    Rate-limits with REQUEST_DELAY between fetches.

    Returns (pages_count, warnings_count).
    """
    pages = 0
    warnings = 0
    visited = set()
    queue = collections.deque()

    # Seed with TOC URL
    queue.append(toc_url)

    while queue:
        if limit is not None and pages >= limit:
            break

        url = queue.popleft()

        # Normalize URL to get the node key
        # URL looks like: http://base/read/ADCD_2.1/Guide_dir/nodeXXXX.html
        # We want: Guide_dir/nodeXXXX (without .html)
        parts = url.rsplit("/", 2)
        if len(parts) >= 2:
            node_file = parts[-1]  # nodeXXXX.html
            node_key = f"{guide_dir}/{node_file.replace('.html', '')}"
        else:
            node_key = url

        if node_key in visited:
            continue
        visited.add(node_key)

        if dry_run:
            print(f"  [dry-run] {url}")
            pages += 1
            continue

        # Fetch
        html_text = fetch_page(url)
        if html_text is None:
            warnings += 1
            continue

        # Rate limit
        time.sleep(REQUEST_DELAY)

        # Extract title
        title = extract_title(html_text)

        # Extract body
        body = extract_body(html_text)
        if "AG2HTML: BODY=START" not in html_text:
            print(f"  warning: no BODY markers in {url}", file=sys.stderr)
            warnings += 1

        # Convert to markdown
        md_content = convert_html_to_md(body)

        # Generate slug for filename
        slug = slugify_title(title, used_slugs)
        filename = f"{slug}.md"
        filepath = manual_dir / filename

        # Write markdown file
        filepath.write_text(f"# {title}\n\n{md_content}\n", encoding="utf-8")

        # Record in node map
        node_map[node_key] = {
            "manual": manual_slug,
            "path": f"{manual_slug}/{filename}",
            "title": title,
            "slug": slug,
        }

        pages += 1
        print(f"  [{pages}] {slug} <- {node_file if 'node_file' in dir() else url}")

        # Extract and queue links within the same manual
        for link in extract_links(html_text):
            # Resolve relative link against current URL
            # Links look like: ../Guide_dir/nodeXXXX.html
            link_match = re.match(r"\.\./([^/]+)/([^/]+\.html)", link)
            if link_match:
                link_guide = link_match.group(1)
                link_file = link_match.group(2)
                link_key = f"{link_guide}/{link_file.replace('.html', '')}"

                # Record cross-manual links but don't follow them
                if link_guide != guide_dir:
                    continue

                if link_key not in visited:
                    full_url = f"{BASE_URL}/read/ADCD_2.1/{link_guide}/{link_file}"
                    queue.append(full_url)

    return pages, warnings


def do_crawl(args):
    """Pass 1: Fetch HTML pages and convert to raw markdown."""
    output_dir = Path(args.output)
    raw_dir = output_dir / "raw"
    manual_filter = getattr(args, "manual", None)
    limit = getattr(args, "limit", None)
    dry_run = getattr(args, "dry_run", False)

    # Global node map: "Guide_dir/nodeXXXX" -> {manual, path, title, slug}
    node_map = {}
    used_slugs = set()
    total_pages = 0
    total_warnings = 0

    # Filter manuals if --manual is set
    manuals_to_crawl = MANUALS
    if manual_filter:
        manuals_to_crawl = [m for m in MANUALS if m[0] == manual_filter]

    for manual_slug, guide_dir, display_title in manuals_to_crawl:
        print(f"\n{'='*60}")
        print(f"Crawling: {display_title} ({manual_slug})")
        print(f"{'='*60}")

        manual_dir = raw_dir / manual_slug
        manual_dir.mkdir(parents=True, exist_ok=True)

        # TOC is typically node0003.html but we start from the guide index
        toc_url = f"{BASE_URL}/read/ADCD_2.1/{guide_dir}/node0003.html"

        pages, warns = crawl_manual(
            toc_url, guide_dir, manual_slug, manual_dir,
            node_map, used_slugs, limit=limit, dry_run=dry_run
        )
        total_pages += pages
        total_warnings += warns
        print(f"  => {pages} pages, {warns} warnings")

    # Write global node map
    node_map_path = raw_dir / "node-map.json"
    raw_dir.mkdir(parents=True, exist_ok=True)
    with open(node_map_path, "w", encoding="utf-8") as f:
        json.dump(node_map, f, indent=2, sort_keys=True)

    print(f"\n{'='*60}")
    print(f"CRAWL COMPLETE: {total_pages} pages, {total_warnings} warnings")
    print(f"Node map: {node_map_path} ({len(node_map)} entries)")
    print(f"{'='*60}")


# --------------------------------------------------------------------------- #
# Pass 2: Enrich
# --------------------------------------------------------------------------- #

def load_known_functions(autodocs_dir: str) -> set:
    """Scan autodoc .md files for function names.

    Parses lines matching ``- **FunctionName** (V...)`` or ``- **FunctionName** —``.
    Returns a set of function name strings.  Skips README.md.
    """
    functions = set()
    autodocs_path = Path(autodocs_dir)
    if not autodocs_path.is_dir():
        return functions
    for md_file in sorted(autodocs_path.glob("*.md")):
        if md_file.name == "README.md":
            continue
        with open(md_file, "r", encoding="utf-8") as f:
            for line in f:
                m = re.match(r"^- \*\*(\w+)\*\*", line)
                if m:
                    functions.add(m.group(1))
    return functions


def build_func_to_library_map(autodocs_dir: str) -> dict:
    """Build a mapping of function_name -> library_name from autodoc files.

    The library name is derived from the filename (e.g., exec.library.md -> exec.library).
    """
    func_map = {}
    autodocs_path = Path(autodocs_dir)
    if not autodocs_path.is_dir():
        return func_map
    for md_file in sorted(autodocs_path.glob("*.md")):
        if md_file.name == "README.md":
            continue
        lib_name = md_file.stem  # e.g., "exec.library"
        with open(md_file, "r", encoding="utf-8") as f:
            for line in f:
                m = re.match(r"^- \*\*(\w+)\*\*", line)
                if m:
                    func_map[m.group(1)] = lib_name
    return func_map


def detect_functions(text: str, known_functions: set) -> set:
    """Detect references to known AmigaOS functions in text.

    A function is considered referenced if it appears as ``FuncName()`` in the text.
    Returns a set of matched function names.
    """
    found = set()
    for func in known_functions:
        # Match FuncName() — word boundary before, literal parens after
        if re.search(r'\b' + re.escape(func) + r'\(\)', text):
            found.add(func)
    return found


def detect_libraries(functions: set, func_to_library: dict) -> set:
    """Given a set of function names, return the set of library names they belong to."""
    libs = set()
    for func in functions:
        if func in func_to_library:
            libs.add(func_to_library[func])
    return libs


def detect_types(text: str) -> set:
    """Detect references to known Amiga types in text.

    Matches:
    - ``struct TypeName`` patterns (PascalCase struct names)
    - Standalone matches against KNOWN_AMIGA_TYPES (must be PascalCase / exact match)

    Returns a set of type name strings.
    """
    found = set()
    # Match "struct TypeName" where TypeName starts with uppercase
    for m in re.finditer(r'\bstruct\s+([A-Z][A-Za-z0-9]+)\b', text):
        found.add(m.group(1))
    # Match known types as standalone words (must appear with exact case)
    for type_name in KNOWN_AMIGA_TYPES:
        if re.search(r'\b' + re.escape(type_name) + r'\b', text):
            found.add(type_name)
    return found


def extract_code_examples(text: str, library: str, section_slug: str,
                          source_path: str) -> list:
    """Find indented code blocks (4+ spaces) that look like real C code.

    Requirements for extraction:
    - Block must be 5+ non-blank lines
    - Must contain at least one function call pattern: ``FuncName(``
    - Must contain at least one code marker: #include, {, return, ;

    Returns list of dicts: {code, library, section, source}.
    """
    lines = text.split("\n")
    examples = []
    i = 0
    code_markers = {"#include", "{", "return", ";"}

    while i < len(lines):
        line = lines[i]
        # Check if line is indented (4+ spaces) and non-blank
        if re.match(r"    ", line) and line.strip():
            block_lines = []
            while i < len(lines) and (re.match(r"    ", lines[i]) or not lines[i].strip()):
                block_lines.append(lines[i])
                i += 1
            # Remove trailing blank lines
            while block_lines and not block_lines[-1].strip():
                block_lines.pop()

            # Count non-blank lines
            non_blank = [bl for bl in block_lines if bl.strip()]
            if len(non_blank) < 5:
                continue

            block_text = "\n".join(block_lines)

            # Check for function call pattern: SomeName(
            has_func_call = bool(re.search(r'[A-Za-z_]\w*\s*\(', block_text))
            if not has_func_call:
                continue

            # Check for code markers
            has_marker = any(marker in block_text for marker in code_markers)
            if not has_marker:
                continue

            examples.append({
                "code": block_text,
                "library": library,
                "section": section_slug,
                "source": source_path,
            })
        else:
            i += 1

    return examples


def generate_frontmatter(title: str, manual: str, chapter: str, section: str,
                         functions: list, libraries: list) -> str:
    """Generate YAML frontmatter block with --- delimiters.

    Functions and libraries are formatted as bracket-lists.
    """
    func_list = ", ".join(sorted(functions)) if functions else ""
    lib_list = ", ".join(sorted(libraries)) if libraries else ""
    lines = [
        "---",
        f"title: {title}",
        f"manual: {manual}",
        f"chapter: {chapter}",
        f"section: {section}",
        f"functions: [{func_list}]",
        f"libraries: [{lib_list}]",
        "---",
    ]
    return "\n".join(lines) + "\n"


def generate_see_also(functions: set, func_to_library: dict,
                      enriched_file_depth: int = 2) -> str:
    """Generate a 'See Also' section linking detected functions to autodoc entries.

    enriched_file_depth is the number of directory levels from the enriched file
    to the adcd root (e.g., docs/references/adcd/libraries/foo.md -> depth 1 from adcd root).
    """
    if not functions:
        return ""

    prefix = "../" * enriched_file_depth
    lines = ["\n---\n", "## See Also\n"]
    for func in sorted(functions):
        lib = func_to_library.get(func, "unknown.library")
        anchor = func.lower()
        lines.append(f"- [{func} \u2014 {lib}]({prefix}autodocs/{lib}.md#{anchor})")
    lines.append("")
    return "\n".join(lines)


def generate_index(output_dir: Path, all_files: list):
    """Generate INDEX.md — alphabetical index of all titles and functions."""
    index_path = output_dir / "INDEX.md"
    lines = ["# ADCD Reference Index\n",
             "Alphabetical index of all pages and detected functions.\n"]

    # Sort by title
    sorted_files = sorted(all_files, key=lambda x: x.get("title", "").lower())

    lines.append("## Pages\n")
    lines.append("| Title | Manual | Functions |")
    lines.append("|-------|--------|-----------|")
    for entry in sorted_files:
        title = entry.get("title", "Untitled")
        manual = entry.get("manual", "")
        funcs = ", ".join(sorted(entry.get("functions", set())))
        path = entry.get("path", "")
        lines.append(f"| [{title}]({path}) | {manual} | {funcs} |")

    lines.append("")
    index_path.write_text("\n".join(lines), encoding="utf-8")
    print(f"  Generated {index_path}")


def generate_functions_xref(output_dir: Path, function_index: dict):
    """Generate FUNCTIONS.md — for each known function, list all pages that mention it."""
    xref_path = output_dir / "FUNCTIONS.md"
    lines = ["# ADCD Function Cross-Reference\n",
             "For each AmigaOS function, lists all ADCD pages that reference it.\n"]

    for func in sorted(function_index.keys()):
        pages = function_index[func]
        lines.append(f"### {func}\n")
        for page in sorted(pages, key=lambda p: p.get("title", "").lower()):
            title = page.get("title", "Untitled")
            path = page.get("path", "")
            lines.append(f"- [{title}]({path})")
        lines.append("")

    xref_path.write_text("\n".join(lines), encoding="utf-8")
    print(f"  Generated {xref_path}")


def generate_attribution(output_dir: Path):
    """Generate ATTRIBUTION.md with copyright notice."""
    attr_path = output_dir / "ATTRIBUTION.md"
    content = """# Attribution

Source: Amiga Developer CD v2.1, hosted at amigadev.elowar.com.
Original content (C) Commodore-Amiga / Amiga Inc.

This material is reproduced for reference purposes in the amiport project.
All rights remain with the original copyright holders.
"""
    attr_path.write_text(content, encoding="utf-8")
    print(f"  Generated {attr_path}")


def generate_readme(output_dir: Path, all_files: list):
    """Generate README.md with contents table and quick links."""
    readme_path = output_dir / "README.md"

    # Count pages per manual
    manual_counts = collections.Counter()
    for entry in all_files:
        manual_counts[entry.get("manual", "unknown")] += 1

    lines = [
        "# ADCD Reference Documentation\n",
        "Scraped and enriched from the Amiga Developer CD v2.1.\n",
        "## Contents\n",
        "| Manual | Pages | Directory |",
        "|--------|-------|-----------|",
    ]

    for slug, _, display_title in MANUALS:
        count = manual_counts.get(slug, 0)
        if count > 0:
            lines.append(f"| {display_title} | {count} | [{slug}/]({slug}/) |")

    lines.extend([
        "",
        "## Quick Links\n",
        "- [INDEX.md](INDEX.md) — Alphabetical page index",
        "- [FUNCTIONS.md](FUNCTIONS.md) — Function cross-reference",
        "- [ATTRIBUTION.md](ATTRIBUTION.md) — Copyright and source info",
        "",
    ])

    readme_path.write_text("\n".join(lines), encoding="utf-8")
    print(f"  Generated {readme_path}")


def generate_types_index(output_dir: Path, type_index: dict):
    """Generate TYPES.md — for each type, list all pages where it appears."""
    types_path = output_dir / "TYPES.md"
    lines = ["# ADCD Type Cross-Reference\n",
             "For each Amiga type, lists all ADCD pages that reference it.\n"]

    for type_name in sorted(type_index.keys()):
        pages = type_index[type_name]
        lines.append(f"### {type_name}\n")
        for path, title in sorted(pages, key=lambda p: p[1].lower()):
            lines.append(f"- [{title}]({path})")
        lines.append("")

    types_path.write_text("\n".join(lines), encoding="utf-8")
    print(f"  Generated {types_path}")


def generate_includes_json(output_dir: Path, node_map: dict):
    """Generate INCLUDES.json mapping Amiga include paths to ADCD chapters.

    Uses LIB_TO_INCLUDE to map library slugs to include file paths.
    Searches node_map titles for library name matches.
    """
    includes = {}

    for lib_slug, include_path in sorted(LIB_TO_INCLUDE.items()):
        # Search node_map for chapters mentioning this library
        chapters = []
        lib_name = f"{lib_slug}.library"

        for node_key, node_info in node_map.items():
            title = node_info.get("title", "").lower()
            # Match the library slug in the title
            if lib_slug.lower() in title:
                chapters.append({
                    "title": node_info.get("title", ""),
                    "path": node_info.get("path", ""),
                })

        # Determine autodoc path
        autodoc_path = f"autodocs/{lib_name}.md"

        includes[include_path] = {
            "library": lib_name,
            "chapters": chapters,
            "autodoc": autodoc_path,
        }

    includes_path = output_dir / "INCLUDES.json"
    with open(includes_path, "w", encoding="utf-8") as f:
        json.dump(includes, f, indent=2, sort_keys=True)
    print(f"  Generated {includes_path}")


def do_enrich(args):
    """Pass 2: Add frontmatter, detect functions, resolve links, build indexes."""
    output_dir = Path(args.output)
    raw_dir = output_dir / "raw"

    # Initialize tracking variables
    all_files = []
    function_index = {}
    type_index = {}
    all_examples = []

    # Load node map from Pass 1
    node_map_path = raw_dir / "node-map.json"
    if not node_map_path.exists():
        print(f"error: node-map.json not found at {node_map_path}", file=sys.stderr)
        print("Run 'crawl' first to generate raw markdown.", file=sys.stderr)
        sys.exit(1)

    with open(node_map_path, "r", encoding="utf-8") as f:
        node_map = json.load(f)

    # Load known functions from autodocs
    project_root = Path(__file__).resolve().parent.parent
    autodocs_dir = project_root / "docs" / "references" / "autodocs"
    known_functions = load_known_functions(str(autodocs_dir))
    func_to_library = build_func_to_library_map(str(autodocs_dir))

    print(f"Loaded {len(known_functions)} known functions from {len(set(func_to_library.values()))} libraries")

    # Process each manual
    total_enriched = 0
    total_functions_found = 0

    for manual_slug, guide_dir, display_title in MANUALS:
        raw_manual_dir = raw_dir / manual_slug
        if not raw_manual_dir.is_dir():
            continue

        enriched_manual_dir = output_dir / manual_slug
        enriched_manual_dir.mkdir(parents=True, exist_ok=True)

        print(f"\n{'='*60}")
        print(f"Enriching: {display_title} ({manual_slug})")
        print(f"{'='*60}")

        md_files = sorted(raw_manual_dir.glob("*.md"))
        for md_file in md_files:
            # Read raw markdown
            raw_text = md_file.read_text(encoding="utf-8")

            # Extract title from first line (# Title)
            title_match = re.match(r"^# (.+)$", raw_text, re.MULTILINE)
            title = title_match.group(1) if title_match else md_file.stem

            # Strip existing title line for re-assembly
            if title_match:
                content = raw_text[title_match.end():].lstrip("\n")
            else:
                content = raw_text

            # Detect functions
            detected_funcs = detect_functions(raw_text, known_functions)
            detected_libs = detect_libraries(detected_funcs, func_to_library)

            # Resolve internal links
            content = resolve_links(content, node_map, manual_slug)

            # Determine chapter/section from title or slug
            chapter = manual_slug
            section = md_file.stem

            # Generate frontmatter
            fm = generate_frontmatter(
                title=title,
                manual=manual_slug,
                chapter=chapter,
                section=section,
                functions=sorted(detected_funcs),
                libraries=sorted(detected_libs),
            )

            # Generate see also section
            see_also = generate_see_also(detected_funcs, func_to_library,
                                         enriched_file_depth=1)

            # Attribution header
            attribution = (
                "> *Source: Amiga Developer CD v2.1. "
                "(C) Commodore-Amiga / Amiga Inc.*\n\n"
            )

            # Assemble enriched file
            enriched = fm + "\n" + f"# {title}\n\n" + attribution + content
            if see_also:
                enriched = enriched.rstrip("\n") + "\n" + see_also

            # Write enriched file
            enriched_path = enriched_manual_dir / md_file.name
            enriched_path.write_text(enriched, encoding="utf-8")

            # Track for indexes
            rel_path = f"{manual_slug}/{md_file.name}"
            file_entry = {
                "title": title,
                "manual": manual_slug,
                "path": rel_path,
                "functions": detected_funcs,
                "libraries": detected_libs,
            }
            all_files.append(file_entry)

            # Update function cross-reference index
            for func in detected_funcs:
                if func not in function_index:
                    function_index[func] = []
                function_index[func].append({
                    "title": title,
                    "path": rel_path,
                })

            # Detect types
            types_found = detect_types(raw_text)
            for type_name in types_found:
                if type_name not in type_index:
                    type_index[type_name] = []
                type_index[type_name].append((rel_path, title))

            # Extract code examples
            slug = md_file.stem
            examples = extract_code_examples(raw_text, manual_slug, slug, rel_path)
            all_examples.extend(examples)

            total_enriched += 1
            total_functions_found += len(detected_funcs)

    # Generate index files
    print(f"\n{'='*60}")
    print("Generating index files...")
    print(f"{'='*60}")

    generate_index(output_dir, all_files)
    generate_functions_xref(output_dir, function_index)
    generate_attribution(output_dir)
    generate_readme(output_dir, all_files)

    # Generate TYPES.md
    generate_types_index(output_dir, type_index)

    # Generate INCLUDES.json
    generate_includes_json(output_dir, node_map)

    # Write extracted examples
    examples_dir = output_dir / "examples"
    example_count = 0
    used_example_names = {}
    for ex in all_examples:
        lib_dir = examples_dir / ex["library"]
        lib_dir.mkdir(parents=True, exist_ok=True)
        base_name = slugify_title(ex["section"])
        filename = base_name + ".c"
        # Handle collisions per library
        key = f"{ex['library']}/{filename}"
        if key in used_example_names:
            used_example_names[key] += 1
            filename = f"{base_name}-{used_example_names[key]}.c"
        else:
            used_example_names[key] = 1
        filepath = lib_dir / filename
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
    if example_count:
        print(f"  Examples: {example_count} code examples extracted")

    print(f"\n{'='*60}")
    print(f"ENRICH COMPLETE: {total_enriched} files enriched")
    print(f"Functions detected: {total_functions_found} references to "
          f"{len(function_index)} unique functions")
    if type_index:
        print(f"Types detected: {len(type_index)} unique types")
    if example_count:
        print(f"Code examples: {example_count} extracted")
    print(f"{'='*60}")


# --------------------------------------------------------------------------- #
# CLI
# --------------------------------------------------------------------------- #

def main():
    parser = argparse.ArgumentParser(
        prog="scrape-adcd",
        description="Scrape ADCD HTML manuals into markdown reference docs.",
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    # -- crawl subcommand --
    p_crawl = subparsers.add_parser("crawl", help="Pass 1: fetch and convert HTML to markdown")
    p_crawl.add_argument("--output", required=True, help="Output directory for markdown files")
    p_crawl.add_argument("--manual", choices=[m[0] for m in MANUALS],
                         help="Crawl only this manual (default: all)")
    p_crawl.add_argument("--limit", type=int, default=None,
                         help="Max pages to fetch per manual (for testing)")
    p_crawl.add_argument("--dry-run", action="store_true",
                         help="Print URLs without fetching")
    p_crawl.set_defaults(func=do_crawl)

    # -- enrich subcommand --
    p_enrich = subparsers.add_parser("enrich", help="Pass 2: add frontmatter and cross-references")
    p_enrich.add_argument("--output", required=True, help="Directory containing crawled markdown")
    p_enrich.set_defaults(func=do_enrich)

    # -- all subcommand (crawl + enrich) --
    p_all = subparsers.add_parser("all", help="Run both passes (crawl then enrich)")
    p_all.add_argument("--output", required=True, help="Output directory for markdown files")
    p_all.add_argument("--manual", choices=[m[0] for m in MANUALS],
                       help="Crawl only this manual (default: all)")
    p_all.add_argument("--limit", type=int, default=None,
                       help="Max pages to fetch per manual (for testing)")
    p_all.set_defaults(func=lambda args: (do_crawl(args), do_enrich(args)))

    args = parser.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
