#!/usr/bin/env python3
"""
Convert Amiga Intern (1992) raw OCR text to structured markdown chapters.

The raw text comes from archive.org's DJVU OCR of the book.
This script:
1. Splits the text into logical chapters
2. Removes page headers/footers and page numbers
3. Cleans up OCR double-spacing artifacts
4. Outputs one markdown file per major section
"""

import re
import os
import sys

RAW_FILE = os.path.join(os.path.dirname(__file__), '..', 'docs', 'references', 'amiga-intern-raw.txt')
OUT_DIR = os.path.join(os.path.dirname(__file__), '..', 'docs', 'references', 'amiga-intern')

# Chapter definitions: (slug, title, start_line, end_line)
# These are 1-indexed line numbers from the raw file
# We'll fill end_line programmatically based on next chapter's start
CHAPTERS = [
    # Part 1: System Programming
    ('01-kickstart', 'Kickstart 2.04', 524),
    ('02-using-amiga-3000', 'Using the Amiga 3000', 1337),
    ('03-01-asl-library', 'The ASL Library', 1992),
    ('03-02-commodities-library', 'The Commodities Library', 3383),
    ('03-03-diskfont-library', 'The Diskfont Library', 4542),
    ('03-04-dos-library', 'The DOS Library', 4988),
    ('03-05-exec-library', 'The Exec Library', 14519),
    ('03-06-expansion-library', 'The Expansion Library', 20437),
    ('03-07-gadtools-library', 'The GadTools Library', 21094),
    ('03-08-graphics-library', 'The Graphics Library', 22573),
    ('03-09-icon-library', 'The Icon Library', 30293),
    ('03-10-iffparse-library', 'The IFFParse Library', 30555),
    ('03-11-intuition-library', 'The Intuition Library', 31796),
    ('03-12-layers-library', 'The Layers Library', 40872),
    ('03-13-math-libraries', 'The Math Libraries', 41487),
    ('03-14-math-trans-libraries', 'The MathTrans Libraries', 41809),
    ('03-15-utility-library', 'The Utility Library', 42208),
    ('03-16-workbench-library', 'The Workbench Library', 42790),
    # Part 2: ARexx
    ('04-arexx-intro', 'ARexx Introduction', 43599),
    ('05-arexx-syntax', 'ARexx Syntax', 44112),
    ('06-arexx-instructions', 'ARexx Instructions', 44743),
    ('07-arexx-functions', 'ARexx Functions', 45739),
    ('09-arexx-on-amiga', 'ARexx on the Amiga', 48968),
    ('10-arexx-interface', 'The ARexx Interface', 50087),
    # Part 3: A3000 Hardware (HIGH PRIORITY)
    ('11-01-processor-generations', 'Processor Generations', 53014),
    ('11-02-the-68030', 'The 68030', 53178),
    ('11-03-cia-8520', 'The CIA 8520', 59250),
    ('11-04-custom-chips', 'Custom Chips and the Amiga', 60941),
    ('11-05-amiga-interfaces', 'The Amiga Interfaces', 61896),
    ('11-06-keyboard', 'The Keyboard', 64541),
    ('11-07-01-memory-layout', 'The Memory Layout', 64909),
    ('11-07-02-fundamentals', 'Hardware Fundamentals', 68263),
    ('11-07-03-interrupts', 'Interrupts', 69293),
    ('11-07-04-copper', 'The Copper Coprocessor', 69576),
    ('11-07-05-playfields', 'Playfields', 70508),
    ('11-07-06-sprites', 'Sprites', 73171),
    ('11-07-07-ecs', 'ECS Capabilities', 75165),
    ('11-07-08-blitter', 'The Blitter', 76525),
    ('11-07-09-sound', 'Sound Output', 80577),
    ('11-07-10-input-devices', 'Mouse, Joystick and Paddles', 82785),
    ('11-07-11-serial', 'The Serial Interfaces', 83340),
    ('11-07-12-disk-controller', 'The Disk Controller', 83989),
]

# Page header patterns to strip (these repeat on every page)
PAGE_HEADERS = [
    re.compile(r'^1\.\s+Kickstart\s+2\.04\s*$'),
    re.compile(r'^2\.\s+Using\s+the\s+Amiga\s+3000\s*$'),
    re.compile(r'^3\.\s+Programming\s+with\s+AmigaOS\s+2[\.\s]*(x|s|jc)?\s*$'),
    re.compile(r'^5\.\s+Programming\s+with\s+AmigaOS\s+2[\.\s]*(x|s|jc)?\s*$'),
    re.compile(r'^4\.\s+ARexx\s*$'),
    re.compile(r'^5\.\s+ARexx\s+Syntax\s*$'),
    re.compile(r'^7\.\s+ARexx\s+Functions\s*$'),
    re.compile(r'^9\.\s+ARexx\s+on\s+the\s+Amiga\s*$'),
    re.compile(r'^10\.\s+The\s+ARexx\s+Interface\s*$'),
    re.compile(r'^11\.\d+\s+(The\s+68030|Processor\s+Generations|The\s+CIA|Custom\s+Chips|The\s+Amiga\s+Interfaces|The\s+Keyboard|Programming\s+the\s+Hardware)\s*$'),
    re.compile(r'^11\.\s+The\s+A3\s*000\s+Hardware\s*$'),
    re.compile(r'^6\.\d+\s+(I/O|110|Structured|ARexx\s+Control|Commands)\s'),
    re.compile(r'^7\.\d+\s+(ARexx|Built-in|I/O|110)\s'),
    re.compile(r'^Part\s+\d+\s*-\s*Introduction\s*$'),
]

# Standalone page numbers
PAGE_NUMBER = re.compile(r'^\d{1,4}\s*$')

# Roman numeral page markers
ROMAN_PAGE = re.compile(r'^[ivxIVX]+\s*$')


def clean_ocr_line(line):
    """Clean up OCR artifacts from a single line."""
    # Remove double spacing between words (OCR artifact)
    # But preserve intentional indentation
    stripped = line.lstrip()
    indent = len(line) - len(stripped)

    # Collapse multiple spaces between words to single space
    cleaned = re.sub(r'(\S)  +(\S)', r'\1 \2', stripped)

    # Fix common OCR artifacts
    cleaned = cleaned.replace('ZZZZZZ', '')
    cleaned = cleaned.replace("ZZ'", '')
    cleaned = cleaned.replace('ZI', '')
    cleaned = cleaned.replace('"Z ', '')
    cleaned = cleaned.replace('""Z', '')
    cleaned = cleaned.replace('"..', '')
    cleaned = cleaned.replace('..Z', '')
    cleaned = cleaned.replace(';', '')  # Only standalone semicolons

    # Clean up trailing OCR noise
    cleaned = re.sub(r'\s*[Z"]+\s*$', '', cleaned)

    return cleaned


def is_page_header(line):
    """Check if a line is a repeating page header."""
    stripped = line.strip()
    for pat in PAGE_HEADERS:
        if pat.match(stripped):
            return True
    return False


def is_page_number(line):
    """Check if a line is just a page number."""
    stripped = line.strip()
    return bool(PAGE_NUMBER.match(stripped)) or bool(ROMAN_PAGE.match(stripped))


def process_chapter(lines, title, slug):
    """Process a chapter's lines into clean markdown."""
    output = []

    # Track if we're in a code/register block
    prev_blank = False

    for line in lines:
        stripped = line.strip()

        # Skip page headers
        if is_page_header(line):
            continue

        # Skip standalone page numbers
        if is_page_number(line):
            continue

        # Clean the line
        cleaned = clean_ocr_line(line)

        # Skip empty lines that would create triple+ blanks
        if not cleaned.strip():
            if prev_blank:
                continue
            prev_blank = True
            output.append('')
            continue
        else:
            prev_blank = False

        # Detect section headers (11.7.1  Title format)
        header_match = re.match(r'^(\d+(?:\.\d+)+)\s+(.+)$', cleaned.strip())
        if header_match:
            num = header_match.group(1)
            header_title = header_match.group(2)
            depth = num.count('.')
            level = min(depth + 1, 4)  # h2, h3, h4
            output.append(f'\n{"#" * level} {num} {header_title}\n')
            continue

        output.append(cleaned)

    # Post-process: join paragraph lines separated by single blank lines
    # The OCR inserts a blank line after every text line. We need to join
    # consecutive text lines into paragraphs, keeping real paragraph breaks.

    def is_table_or_data_line(text):
        """Detect lines that are register tables, address maps, or data."""
        t = text.strip()
        # Hex addresses like $BFE001 or $00F80000
        if re.match(r'^\$[0-9A-Fa-f]{3,}', t):
            return True
        # Register names (ALL CAPS with digits, short)
        if re.match(r'^[A-Z][A-Z0-9_]{2,}\s', t) and len(t) < 100:
            return True
        # Lines starting with register-like patterns
        if re.match(r'^(Name|Reg\.addr|Chip|R/W|p/d|Function)\s', t):
            return True
        # Short data lines (< 60 chars, likely table entries)
        if len(t) < 60 and re.search(r'\$[0-9A-Fa-f]{3,}', t):
            return True
        # Pin descriptions (e.g. "D0-D15", "A1-A20")
        if re.match(r'^[A-Z]+\d+[-/][A-Z]*\d+', t):
            return True
        return False

    def is_prose_line(text):
        """Detect lines that are part of flowing prose."""
        t = text.strip()
        if not t:
            return False
        if t.startswith('#'):
            return False
        if is_table_or_data_line(t):
            return False
        # Prose: starts with lowercase or common sentence starters, or is long
        if len(t) > 80:
            return True
        if t[0].islower():
            return True
        # Sentences starting with articles, prepositions, etc.
        if re.match(r'^(The|A |An |In |On |If |It |To |This|That|These|Those|For|From|With|When|While|Since|After|Before|However|Although|But|Also|Each|Every|All|As |At |By |Or |And |Now|Here)', t):
            return True
        return False

    joined = []
    i = 0
    while i < len(output):
        line = output[i]

        # If this is prose text, join consecutive prose lines
        if is_prose_line(line):
            para_lines = [line.strip()]
            i += 1
            while i < len(output):
                if i < len(output) and output[i] == '':
                    # Check if next line after blank is also prose
                    if (i + 1 < len(output) and is_prose_line(output[i + 1])):
                        i += 1
                        para_lines.append(output[i].strip())
                        i += 1
                    else:
                        break
                elif is_prose_line(output[i]):
                    para_lines.append(output[i].strip())
                    i += 1
                else:
                    break
            joined.append(' '.join(para_lines))
        else:
            joined.append(line)
            i += 1

    # Deduplicate headers (keep only first occurrence of each header)
    seen_headers = set()
    deduped = []
    for line in joined:
        stripped = line.strip()
        if stripped.startswith('#'):
            if stripped in seen_headers:
                continue
            seen_headers.add(stripped)
        deduped.append(line)

    # Build final markdown
    md = f'# {title}\n\n'
    md += f'*From "Amiga Intern" (1992) by Kuhnert, Maelger, Schemmel. Abacus.*\n\n'
    md += '\n'.join(deduped)

    # Final cleanup: collapse 3+ newlines to 2
    md = re.sub(r'\n{3,}', '\n\n', md)

    return md


def main():
    with open(RAW_FILE, 'r', encoding='utf-8', errors='replace') as f:
        all_lines = f.readlines()

    total_lines = len(all_lines)
    print(f"Read {total_lines} lines from raw OCR text")

    os.makedirs(OUT_DIR, exist_ok=True)

    # Process each chapter
    for i, (slug, title, start) in enumerate(CHAPTERS):
        # End line is start of next chapter, or end of file
        if i + 1 < len(CHAPTERS):
            end = CHAPTERS[i + 1][2]
        else:
            end = total_lines

        # Extract lines (convert to 0-indexed)
        chapter_lines = all_lines[start - 1:end - 1]

        print(f"Processing {slug}: lines {start}-{end} ({len(chapter_lines)} lines)")

        md = process_chapter(chapter_lines, title, slug)

        out_path = os.path.join(OUT_DIR, f'{slug}.md')
        with open(out_path, 'w', encoding='utf-8') as f:
            f.write(md)

        print(f"  -> {out_path} ({len(md)} bytes)")

    print(f"\nDone! {len(CHAPTERS)} chapters written to {OUT_DIR}")


if __name__ == '__main__':
    main()
