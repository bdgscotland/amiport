#!/usr/bin/env python3
"""
parse-autodocs.py — Convert Amiga NDK autodoc files to per-library markdown.

Handles two input formats:
  1. Raw .doc.txt files (from wiki.amigaos.net or local NDK)
  2. Directory of .doc files (extracted NDK)

The autodoc format uses paired header lines to delimit entries:
  library/FunctionName                              library/FunctionName
followed by sections: NAME, SYNOPSIS, FUNCTION, INPUTS, RESULT, etc.

Usage:
  # Fetch from wiki.amigaos.net and parse:
  python3 scripts/parse-autodocs.py --fetch --output docs/references/autodocs/

  # Parse local NDK autodoc files:
  python3 scripts/parse-autodocs.py --input docs/references/ndk/autodocs/ --output docs/references/autodocs/

  # Parse a single file:
  python3 scripts/parse-autodocs.py --input exec.doc.txt --output docs/references/autodocs/

  # Filter to classic AmigaOS (V39 and below):
  python3 scripts/parse-autodocs.py --fetch --max-version 39 --output docs/references/autodocs/
"""

import argparse
import os
import re
import sys
import textwrap
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional

# Libraries to fetch from wiki.amigaos.net
# These cover the core AmigaOS 3.x APIs relevant to amiport
FETCH_LIBRARIES = [
    "exec",
    "dos",
    "intuition",
    "graphics",
    "utility",
    "gadtools",
    "asl",
    "commodities",
    "console",
    "diskfont",
    "icon",
    "iffparse",
    "input",
    "keymap",
    "layers",
    "locale",
    "mathffp",
    "mathieeedoubbas",
    "rexxsyslib",
    "timer",
    "workbench",
]

AUTODOC_URL = "https://wiki.amigaos.net/amiga/autodocs/{}.doc.txt"


@dataclass
class AutodocEntry:
    """A single function/command autodoc entry."""
    library: str = ""
    name: str = ""
    sections: dict = field(default_factory=dict)
    raw: str = ""

    @property
    def version(self) -> Optional[int]:
        """Extract version from NAME section, e.g. '(V36)'."""
        name_text = self.sections.get("NAME", "")
        m = re.search(r'\(V(\d+)\)', name_text)
        return int(m.group(1)) if m else None

    @property
    def is_obsolete(self) -> bool:
        name_text = self.sections.get("NAME", "")
        return "DEPRECATED" in name_text or "OBSOLETE" in name_text


def fetch_autodoc(library: str) -> Optional[str]:
    """Fetch an autodoc .txt file from wiki.amigaos.net."""
    import urllib.request
    import urllib.error

    url = AUTODOC_URL.format(library)
    try:
        req = urllib.request.Request(url, headers={"User-Agent": "amiport/1.0"})
        with urllib.request.urlopen(req, timeout=30) as resp:
            return resp.read().decode("latin-1", errors="replace")
    except urllib.error.URLError as e:
        print(f"  Warning: could not fetch {url}: {e}", file=sys.stderr)
        return None


def parse_autodoc_text(text: str) -> list:
    """Parse autodoc text into a list of AutodocEntry objects.

    The format uses paired header lines to delimit entries:
      library/FuncName                                library/FuncName
    Followed by sections like NAME, SYNOPSIS, FUNCTION, etc.
    """
    entries = []

    # Split on the paired header pattern:
    # "library/Name<spaces>library/Name" at the start of lines
    # Also handles the form-feed separator used in some autodoc files
    header_re = re.compile(
        r'^(\S+/\S+)\s{2,}\1\s*$',
        re.MULTILINE
    )

    # Find all entry headers and their positions
    headers = list(header_re.finditer(text))

    if not headers:
        # Try alternate format: entries separated by form-feed
        parts = text.split('\f')
        for part in parts:
            part = part.strip()
            if not part:
                continue
            entry = _parse_single_entry(part)
            if entry and entry.name:
                entries.append(entry)
        return entries

    for i, match in enumerate(headers):
        # Extract the text between this header and the next
        start = match.end()
        end = headers[i + 1].start() if i + 1 < len(headers) else len(text)
        entry_text = text[start:end].strip()

        # Parse library/name from the header
        full_name = match.group(1)
        if '/' in full_name:
            library, name = full_name.split('/', 1)
        else:
            library = ""
            name = full_name

        entry = AutodocEntry(library=library, name=name)
        entry.raw = entry_text
        _parse_sections(entry, entry_text)

        if entry.name:
            entries.append(entry)

    return entries


def _parse_single_entry(text: str) -> Optional[AutodocEntry]:
    """Parse a single form-feed-delimited autodoc entry."""
    lines = text.split('\n')
    if not lines:
        return None

    # First non-empty line should be the header
    header = ""
    for line in lines:
        line = line.strip()
        if line and not line.startswith('*'):
            header = line
            break

    if '/' not in header:
        return None

    # May have "library/Name  library/Name" or just "library/Name"
    parts = header.split()
    full_name = parts[0]
    library, name = full_name.split('/', 1)

    entry = AutodocEntry(library=library, name=name)
    entry.raw = text
    _parse_sections(entry, text)
    return entry


def _parse_sections(entry: AutodocEntry, text: str):
    """Extract named sections from autodoc entry text."""
    # Known section headers
    section_names = [
        "NAME", "SYNOPSIS", "FUNCTION", "INPUTS", "INPUT", "TAGS",
        "RESULT", "RESULTS", "RETURN", "RETURNS",
        "EXAMPLE", "EXAMPLES", "NOTE", "NOTES", "WARNING",
        "BUGS", "SEE ALSO", "INTERNALS",
    ]

    # Build regex: section header is a line with just the section name
    # (possibly with leading whitespace)
    section_re = re.compile(
        r'^\s+(' + '|'.join(re.escape(s) for s in section_names) + r')\s*$',
        re.MULTILINE
    )

    matches = list(section_re.finditer(text))

    for i, match in enumerate(matches):
        section_name = match.group(1).strip()
        start = match.end()
        end = matches[i + 1].start() if i + 1 < len(matches) else len(text)

        # Clean up the section body
        body = text[start:end]
        # Remove trailing asterisk lines (entry terminators)
        body = re.sub(r'\n\s*\*{3,}/?\s*$', '', body)
        # Dedent: remove common leading whitespace (usually tab or spaces)
        body_lines = body.split('\n')
        cleaned = []
        for line in body_lines:
            # Remove leading tab+spaces or just spaces (autodoc indent)
            cleaned_line = re.sub(r'^\t', '', line)
            cleaned_line = re.sub(r'^       ', '', cleaned_line)  # 7 spaces
            cleaned.append(cleaned_line)
        body = '\n'.join(cleaned).strip()

        # Normalize section names
        normalized = section_name.upper()
        if normalized in ("INPUT", "INPUTS", "TAGS"):
            normalized = "INPUTS"
        elif normalized in ("RESULT", "RESULTS", "RETURN", "RETURNS"):
            normalized = "RESULT"
        elif normalized in ("EXAMPLE", "EXAMPLES"):
            normalized = "EXAMPLE"
        elif normalized in ("NOTE", "NOTES", "WARNING"):
            normalized = "NOTES"

        entry.sections[normalized] = body


# Known OS4-only function name prefixes/patterns (no version annotations in autodocs)
_OS4_PATTERNS = re.compile(
    r'^('
    r'ASOT_'            # AllocSysObjectTags types
    r'|AVL_'            # AVL tree functions
    r'|debug/'          # debug sub-library
    r'|NewStackSwap'    # V50+
    r'|AllocNamedMemory'  # V50+
    r'|AttemptNamedMemory'  # V50+
    r'|FreeNamedMemory'  # V50+
    r'|DeleteInterface'  # V50+ interface system
    r'|DeleteLibrary'   # V50+
    r'|GetInterface'    # V50+
    r'|DropInterface'   # V50+
    r'|MakeInterface'   # V50+
    r')',
    re.IGNORECASE
)


def _is_os4_only(entry: AutodocEntry) -> bool:
    """Detect OS4-only entries that lack version annotations."""
    if _OS4_PATTERNS.match(entry.name):
        return True
    # Entries with sub-paths (e.g., debug/Foo) are OS4 sub-libraries
    if '/' in entry.name:
        return True
    # Check for OS4 keywords in SYNOPSIS (IExec->, APTR interface, etc.)
    synopsis = entry.sections.get("SYNOPSIS", "")
    if "IExec->" in synopsis or "IIntuition->" in synopsis or "IIDOS->" in synopsis:
        return True
    return False


def entry_to_markdown(entry: AutodocEntry) -> str:
    """Convert a single AutodocEntry to markdown."""
    lines = []

    # Header
    version_str = f" (V{entry.version})" if entry.version else ""
    obsolete_str = " [OBSOLETE]" if entry.is_obsolete else ""
    lines.append(f"### {entry.library}/{entry.name}{version_str}{obsolete_str}")
    lines.append("")

    # NAME section as description
    if "NAME" in entry.sections:
        name_text = entry.sections["NAME"]
        # Extract the description part after the " -- "
        desc_match = re.search(r'--\s*(.+)', name_text, re.DOTALL)
        if desc_match:
            desc = desc_match.group(1).strip()
            # Clean up version annotation for the description
            desc = re.sub(r'\s*\(V\d+\)\s*', '', desc).strip()
            if desc:
                lines.append(f"**{desc}**")
                lines.append("")

    # SYNOPSIS as code block
    if "SYNOPSIS" in entry.sections:
        lines.append("**Synopsis:**")
        lines.append("```c")
        lines.append(entry.sections["SYNOPSIS"])
        lines.append("```")
        lines.append("")

    # FUNCTION as prose
    if "FUNCTION" in entry.sections:
        lines.append("**Description:**")
        lines.append(entry.sections["FUNCTION"])
        lines.append("")

    # INPUTS
    if "INPUTS" in entry.sections:
        lines.append("**Inputs:**")
        lines.append(entry.sections["INPUTS"])
        lines.append("")

    # RESULT
    if "RESULT" in entry.sections:
        lines.append("**Result:**")
        lines.append(entry.sections["RESULT"])
        lines.append("")

    # EXAMPLE
    if "EXAMPLE" in entry.sections:
        lines.append("**Example:**")
        lines.append("```c")
        lines.append(entry.sections["EXAMPLE"])
        lines.append("```")
        lines.append("")

    # NOTES
    if "NOTES" in entry.sections:
        lines.append("**Notes:**")
        lines.append(entry.sections["NOTES"])
        lines.append("")

    # BUGS
    if "BUGS" in entry.sections:
        lines.append("**Bugs:**")
        lines.append(entry.sections["BUGS"])
        lines.append("")

    # SEE ALSO
    if "SEE ALSO" in entry.sections:
        see_also = entry.sections["SEE ALSO"].strip()
        if see_also:
            lines.append(f"**See also:** {see_also}")
            lines.append("")

    lines.append("---")
    lines.append("")
    return '\n'.join(lines)


def library_to_markdown(library: str, entries: list, max_version: Optional[int] = None) -> str:
    """Convert all entries for a library into a single markdown document."""
    lines = []

    # Filter by version if requested
    if max_version is not None:
        filtered = []
        for e in entries:
            v = e.version
            if v is not None and v > max_version:
                continue
            # Skip known OS4-only patterns when filtering for classic AmigaOS
            if _is_os4_only(e):
                continue
            filtered.append(e)
        entries = filtered

    # Separate obsolete entries
    current = [e for e in entries if not e.is_obsolete]
    obsolete = [e for e in entries if e.is_obsolete]

    # Avoid "exec.library.library" — only append .library if not already present
    lib_display = library if library.endswith(".library") or library.endswith(".device") else f"{library}.library"
    lines.append(f"# {lib_display} — Autodoc Reference")
    lines.append("")
    if max_version:
        lines.append(f"*Filtered to AmigaOS V{max_version} and below.*")
        lines.append("")
    lines.append(f"**Functions:** {len(current)}")
    if obsolete:
        lines.append(f"**Obsolete:** {len(obsolete)}")
    lines.append("")

    # Table of contents
    lines.append("## Function Index")
    lines.append("")
    for entry in sorted(current, key=lambda e: e.name.lower()):
        version_str = f" (V{entry.version})" if entry.version else ""
        desc = ""
        if "NAME" in entry.sections:
            m = re.search(r'--\s*(.+?)(?:\s*\(V\d+\))?\s*$',
                          entry.sections["NAME"], re.DOTALL)
            if m:
                desc = m.group(1).strip().split('\n')[0][:60]
        lines.append(f"- **{entry.name}**{version_str} — {desc}")
    lines.append("")

    # Full entries
    lines.append("## Functions")
    lines.append("")
    for entry in sorted(current, key=lambda e: e.name.lower()):
        lines.append(entry_to_markdown(entry))

    # Obsolete section (collapsed)
    if obsolete:
        lines.append("## Obsolete Functions")
        lines.append("")
        lines.append("*These functions are deprecated and should not be used in new code.*")
        lines.append("")
        for entry in sorted(obsolete, key=lambda e: e.name.lower()):
            lines.append(entry_to_markdown(entry))

    return '\n'.join(lines)


def process_file(filepath: str) -> dict:
    """Parse an autodoc file and return entries grouped by library."""
    with open(filepath, 'r', encoding='latin-1', errors='replace') as f:
        text = f.read()
    return process_text(text, os.path.basename(filepath))


def process_text(text: str, source_name: str = "") -> dict:
    """Parse autodoc text and return entries grouped by library."""
    entries = parse_autodoc_text(text)

    by_library = {}
    for entry in entries:
        lib = entry.library or source_name.replace('.doc.txt', '').replace('.doc', '')
        if lib not in by_library:
            by_library[lib] = []
        by_library[lib].append(entry)

    return by_library


def main():
    parser = argparse.ArgumentParser(
        description="Convert Amiga NDK autodocs to per-library markdown"
    )
    parser.add_argument(
        "--input", "-i",
        help="Input file or directory of .doc/.doc.txt files"
    )
    parser.add_argument(
        "--fetch", "-f",
        action="store_true",
        help="Fetch autodocs from wiki.amigaos.net"
    )
    parser.add_argument(
        "--libraries",
        help="Comma-separated list of libraries to fetch (default: core set)"
    )
    parser.add_argument(
        "--output", "-o",
        default="docs/references/autodocs",
        help="Output directory for markdown files"
    )
    parser.add_argument(
        "--max-version",
        type=int,
        default=None,
        help="Filter to functions at or below this AmigaOS version (e.g., 39 for OS 3.x)"
    )
    parser.add_argument(
        "--summary",
        action="store_true",
        help="Print summary of parsed entries"
    )

    args = parser.parse_args()

    if not args.fetch and not args.input:
        parser.error("Specify --fetch to download from wiki.amigaos.net, or --input for local files")

    os.makedirs(args.output, exist_ok=True)

    all_libraries = {}
    total_entries = 0

    if args.fetch:
        libs = args.libraries.split(',') if args.libraries else FETCH_LIBRARIES
        print(f"Fetching autodocs for {len(libs)} libraries from wiki.amigaos.net...")

        for lib in libs:
            lib = lib.strip()
            print(f"  Fetching {lib}.doc.txt...", end=" ", flush=True)
            text = fetch_autodoc(lib)
            if text:
                by_library = process_text(text, f"{lib}.doc.txt")
                for lib_name, entries in by_library.items():
                    if lib_name not in all_libraries:
                        all_libraries[lib_name] = []
                    all_libraries[lib_name].extend(entries)
                count = sum(len(e) for e in by_library.values())
                print(f"{count} entries")
                total_entries += count
            else:
                print("FAILED")

    if args.input:
        input_path = Path(args.input)
        if input_path.is_file():
            files = [input_path]
        elif input_path.is_dir():
            files = sorted(input_path.glob("*.doc*"))
        else:
            print(f"Error: {args.input} not found", file=sys.stderr)
            sys.exit(1)

        for filepath in files:
            print(f"  Parsing {filepath.name}...", end=" ", flush=True)
            by_library = process_file(str(filepath))
            for lib_name, entries in by_library.items():
                if lib_name not in all_libraries:
                    all_libraries[lib_name] = []
                all_libraries[lib_name].extend(entries)
            count = sum(len(e) for e in by_library.values())
            print(f"{count} entries")
            total_entries += count

    # Generate markdown files
    print(f"\nGenerating markdown for {len(all_libraries)} libraries ({total_entries} total entries)...")

    generated = []
    for lib_name, entries in sorted(all_libraries.items()):
        md = library_to_markdown(lib_name, entries, args.max_version)
        out_file = os.path.join(args.output, f"{lib_name}.md")
        with open(out_file, 'w') as f:
            f.write(md)

        filtered_count = len([e for e in entries
                              if not e.is_obsolete and
                              (args.max_version is None or
                               e.version is None or
                               e.version <= args.max_version)])
        generated.append((lib_name, filtered_count))
        print(f"  {out_file} ({filtered_count} functions)")

    # Generate index
    index_lines = [
        "# AmigaOS Autodoc Reference",
        "",
        "Auto-generated from NDK autodoc files by `scripts/parse-autodocs.py`.",
        "",
    ]
    if args.max_version:
        index_lines.append(f"*Filtered to AmigaOS V{args.max_version} and below.*")
        index_lines.append("")

    index_lines.append("## Libraries")
    index_lines.append("")
    index_lines.append("| Library | Functions | File |")
    index_lines.append("|---------|-----------|------|")
    for lib_name, count in sorted(generated):
        index_lines.append(f"| {lib_name} | {count} | [{lib_name}.md]({lib_name}.md) |")
    index_lines.append("")
    index_lines.append(f"**Total: {sum(c for _, c in generated)} functions across {len(generated)} libraries**")

    index_file = os.path.join(args.output, "README.md")
    with open(index_file, 'w') as f:
        f.write('\n'.join(index_lines))
    print(f"\n  {index_file} (index)")

    print(f"\nDone. {len(generated)} library reference files generated in {args.output}/")

    if args.summary:
        print("\n=== Summary ===")
        for lib_name, count in sorted(generated, key=lambda x: -x[1]):
            print(f"  {lib_name:20s} {count:4d} functions")


if __name__ == "__main__":
    main()
