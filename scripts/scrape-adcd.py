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
# Pass 1: Crawl
# --------------------------------------------------------------------------- #

def do_crawl(args):
    """Pass 1: Fetch HTML pages and convert to raw markdown."""
    # TODO: Implement HTML fetching, content extraction, and markdown conversion
    print(f"crawl: output={args.output}, manual={getattr(args, 'manual', None)}, "
          f"limit={getattr(args, 'limit', None)}, dry_run={getattr(args, 'dry_run', False)}")
    print("TODO: Pass 1 not yet implemented")


# --------------------------------------------------------------------------- #
# Pass 2: Enrich
# --------------------------------------------------------------------------- #

def do_enrich(args):
    """Pass 2: Add frontmatter, detect functions, resolve links, build indexes."""
    # TODO: Implement YAML frontmatter injection, function signature detection,
    #       cross-reference resolution, and index generation
    print(f"enrich: output={args.output}")
    print("TODO: Pass 2 not yet implemented")


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
