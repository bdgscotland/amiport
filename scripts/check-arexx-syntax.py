#!/usr/bin/env python3
"""
check-arexx-syntax.py -- Smoke-test ARexx files for common fatal issues.

Checks for:
1. Non-ASCII characters (cause "Error 8: Unrecognized token" on ARexx)
2. Three-level compound variables (ARexx only supports stem.tail)
3. \\= instead of ~= (unreliable on some ARexx interpreters)

Usage:
    check-arexx-syntax.py <file.rexx> [<file2.rexx> ...]
    check-arexx-syntax.py --all   # Check all .rexx files in the project

Exit codes:
    0 = all checks passed
    1 = issues found
"""

import sys
import os
import re
import glob


def check_file(filepath):
    """Check a single ARexx file. Returns list of error strings."""
    errors = []
    try:
        lines = open(filepath, encoding="iso-8859-1").readlines()
    except (IOError, OSError) as e:
        return [f"  Cannot read: {e}"]

    # Check 1: First line must be a comment
    if lines and not lines[0].strip().startswith("/*"):
        errors.append("  Line 1: first line is not a comment (ARexx requires this)")

    for i, line in enumerate(lines, 1):
        raw = line.rstrip("\n")

        # Check 2: Non-ASCII characters
        for j, ch in enumerate(raw):
            if ord(ch) > 127:
                errors.append(
                    f"  Line {i} col {j+1}: non-ASCII byte 0x{ord(ch):02x} '{ch}'"
                )

        stripped = raw.strip()

        # Skip comment lines
        if stripped.startswith("/*") or stripped.startswith("*"):
            continue

        # Check 3: Three-level compound variables (stem.x.y = ...)
        m = re.search(r"\b(\w+\.\w+\.\w+)\s*=", stripped)
        if m:
            var = m.group(1)
            if not any(c in var for c in ["[", "("]):
                errors.append(
                    f"  Line {i}: three-level compound '{var}'"
                    " -- ARexx only supports stem.tail"
                )

        # Check 4: \\= usage (not inside comments)
        comment_start = raw.find("/*")
        code_part = raw[:comment_start] if comment_start >= 0 else raw
        if "\\=" in code_part:
            errors.append(f"  Line {i}: uses \\= instead of ~=")

    return errors


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <file.rexx> [...]")
        print(f"       {sys.argv[0]} --all")
        sys.exit(2)

    if sys.argv[1] == "--all":
        files = sorted(glob.glob("**/*.rexx", recursive=True))
        if not files:
            print("No .rexx files found")
            sys.exit(0)
    else:
        files = sys.argv[1:]

    total_errors = 0
    for filepath in files:
        if not os.path.isfile(filepath):
            print(f"SKIP: {filepath} (not found)")
            continue
        errors = check_file(filepath)
        if errors:
            print(f"FAIL: {filepath}")
            for e in errors:
                print(e)
            total_errors += len(errors)
        else:
            print(f"PASS: {filepath}")

    if total_errors > 0:
        print(f"\n{total_errors} issue(s) found")
        sys.exit(1)
    else:
        print(f"\nAll {len(files)} file(s) clean")
        sys.exit(0)


if __name__ == "__main__":
    main()
