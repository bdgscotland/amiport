#!/usr/bin/env python3
"""
analyze-posix-usage.py — Corpus analysis of POSIX function usage across OpenBSD tools

Scans C source files from OpenBSD coreutils to build a frequency table of POSIX
function calls. Cross-references against posix-tiers.md and newlib-availability.md
to produce a prioritized gap list: functions that are (a) used frequently and
(b) not yet shimmed or available in libnix.

Usage:
    python3 scripts/analyze-posix-usage.py [--no-clone] [--fixture PATH]

    --no-clone  Skip git clone, use existing checkout in /tmp/openbsd-src
    --fixture   Run against fixture directory instead of OpenBSD source
"""

#   OpenBSD GitHub ──sparse──▶ usr.bin/ .c files ──regex──▶ frequency table
#        │                          │                             │
#        ▼                          ▼                             ▼
#   git clone             scan for \bfunc\s*\(            cross-ref posix-tiers
#                                                         + newlib-avail
#                                                               │
#                                                               ▼
#                                                    PRIORITIZED GAP LIST
#                                                    (Missing + freq≥3)

import os
import re
import sys
import json
import subprocess
from collections import defaultdict
from pathlib import Path

# Target tools to analyze
TARGET_TOOLS = [
    # Already ported
    "cal", "cut", "diff", "grep", "sed",
    # Next candidates (TODOS.md)
    "tr", "uniq", "tee", "basename", "tail", "sort", "xargs",
    # Extended corpus
    "paste", "comm", "fold", "expand", "rev", "nl", "tsort", "join",
    "od", "strings", "fmt", "colrm", "column", "env", "printenv",
    "yes", "true", "false", "sleep", "echo", "test", "wc", "head",
]

# Known POSIX/BSD functions to scan for (curated list to reduce false positives)
POSIX_FUNCTIONS = [
    # File I/O
    "open", "close", "read", "write", "lseek", "dup", "dup2", "pipe",
    "fcntl", "ioctl", "ftruncate", "pread", "pwrite", "fsync",
    # File operations
    "stat", "fstat", "lstat", "chmod", "fchmod", "chown", "fchown",
    "unlink", "rename", "link", "symlink", "readlink", "mkdir", "rmdir",
    "access", "truncate", "mkstemp", "mkstemps", "mkdtemp", "realpath",
    # Directory
    "opendir", "readdir", "closedir", "scandir", "alphasort", "getcwd", "chdir",
    # stdio extensions
    "fopen", "fclose", "fread", "fwrite", "fgets", "fputs",
    "fprintf", "printf", "sprintf", "snprintf", "vsnprintf",
    "asprintf", "vasprintf", "getline", "getdelim", "fgetln",
    "tmpfile", "tmpnam", "popen", "pclose", "fileno", "fdopen",
    "setbuf", "setvbuf",
    # String
    "strlcpy", "strlcat", "strdup", "strndup", "strsep", "strtok_r",
    "strcasecmp", "strncasecmp", "strcasestr", "memccpy",
    # Memory
    "mmap", "munmap", "reallocarray", "recallocarray", "freezero",
    # Process
    "fork", "execve", "execvp", "execlp", "waitpid", "wait",
    "system", "getpid", "getppid", "getuid", "getgid", "geteuid", "getegid",
    "setuid", "setgid", "setsid",
    # Signals
    "signal", "sigaction", "sigprocmask", "kill", "raise", "alarm",
    # Environment
    "getenv", "setenv", "unsetenv", "putenv",
    # Time
    "time", "gettimeofday", "clock_gettime", "localtime", "gmtime",
    "strftime", "strptime", "mktime", "difftime", "nanosleep",
    # Sleep
    "sleep", "usleep",
    # Terminal
    "isatty", "ttyname", "tcgetattr", "tcsetattr",
    # Regex
    "regcomp", "regexec", "regfree", "regerror",
    # Pattern matching
    "fnmatch", "glob", "globfree",
    # Network (for completeness)
    "socket", "connect", "bind", "listen", "accept", "send", "recv",
    "select", "poll", "gethostbyname", "getaddrinfo",
    # Misc
    "getopt", "getopt_long", "err", "errx", "warn", "warnx",
    "pledge", "unveil", "arc4random", "arc4random_uniform",
    "strtol", "strtoul", "strtoll", "strtoull", "strtonum",
    "qsort", "bsearch", "setlocale",
    # BSD specifics
    "explicit_bzero", "timingsafe_memcmp", "vis", "unvis",
    "fts_open", "fts_read", "fts_close",
    "setprogname", "getprogname",
]

# Build regex pattern - match function calls (name followed by open paren)
FUNC_PATTERN = re.compile(
    r'\b(' + '|'.join(re.escape(f) for f in POSIX_FUNCTIONS) + r')\s*\('
)

OPENBSD_REPO = "https://github.com/openbsd/src.git"
CLONE_DIR = "/tmp/openbsd-src"


def clone_openbsd(no_clone=False):
    """Sparse-clone OpenBSD src repo for usr.bin/ only."""
    if no_clone and os.path.isdir(CLONE_DIR):
        print(f"Using existing checkout at {CLONE_DIR}")
        return CLONE_DIR

    if os.path.isdir(CLONE_DIR):
        print(f"Removing old checkout at {CLONE_DIR}")
        subprocess.run(["rm", "-rf", CLONE_DIR], check=True)

    print("Cloning OpenBSD src (sparse, usr.bin/ only)...")
    subprocess.run([
        "git", "clone", "--depth=1", "--filter=blob:none",
        "--sparse", OPENBSD_REPO, CLONE_DIR
    ], check=True)
    subprocess.run(
        ["git", "sparse-checkout", "set", "usr.bin"],
        cwd=CLONE_DIR, check=True
    )
    print("Clone complete.")
    return CLONE_DIR


def scan_tool(src_dir, tool_name):
    """Scan all .c files for a tool and return function usage counts."""
    tool_path = Path(src_dir) / "usr.bin" / tool_name
    if not tool_path.is_dir():
        return None

    usage = defaultdict(int)
    c_files = list(tool_path.glob("*.c"))
    if not c_files:
        return None

    for c_file in c_files:
        try:
            content = c_file.read_text(errors='replace')
            # Strip C comments to reduce false positives
            content = re.sub(r'/\*.*?\*/', '', content, flags=re.DOTALL)
            content = re.sub(r'//.*$', '', content, flags=re.MULTILINE)
            # Strip string literals
            content = re.sub(r'"[^"\\]*(?:\\.[^"\\]*)*"', '""', content)

            for match in FUNC_PATTERN.finditer(content):
                usage[match.group(1)] += 1
        except Exception as e:
            print(f"  Warning: could not read {c_file}: {e}", file=sys.stderr)

    return dict(usage) if usage else None


def parse_posix_tiers(root):
    """Parse posix-tiers.md to get implementation status of each function."""
    tiers_path = root / "docs" / "posix-tiers.md"
    status = {}

    if not tiers_path.exists():
        print(f"Warning: {tiers_path} not found", file=sys.stderr)
        return status

    content = tiers_path.read_text()
    # Match table rows with function names and shim wrappers
    for match in re.finditer(r'\|`([a-z_]+)\(\)`\s*\|`amiport', content):
        status[match.group(1)] = "shimmed"

    # Match Tier 2 emulation entries (amiport_emu_ prefix)
    for match in re.finditer(r'\|`([a-z_]+)\(\)`\s*\|`amiport_emu', content):
        status[match.group(1)] = "shimmed"

    # Also match Planned entries
    for match in re.finditer(r'\|`([a-z_]+)\(\)`\s*\|.*\|(Low|Medium|High)', content):
        func = match.group(1)
        if func not in status:
            status[func] = "planned"

    return status


def parse_bsd_isms(root):
    """Parse bsd-isms.md for shimmed/stub/inline BSD functions."""
    bsd_path = root / "docs" / "references" / "bsd-isms.md"
    status = {}

    if not bsd_path.exists():
        return status

    content = bsd_path.read_text()
    for line in content.splitlines():
        m = re.match(r'^\|\s*`([a-z_]+)\(\)`\s*\|.*?\|\s*(Shimmed|Stub|Inline|Missing)', line)
        if m:
            func = m.group(1)
            s = m.group(2).lower()
            if s in ("shimmed", "stub", "inline"):
                status[func] = "shimmed"
            else:
                status[func] = "missing"

    return status


def parse_newlib_availability(root):
    """Parse newlib-availability.md for function availability status."""
    ref_path = root / "docs" / "references" / "newlib-availability.md"
    status = {}

    if not ref_path.exists():
        print(f"Warning: {ref_path} not found", file=sys.stderr)
        return status

    content = ref_path.read_text()
    for line in content.splitlines():
        # Extract status (second-to-last column after function names)
        m = re.search(r'\|\s*(Available|Missing|Partial|Use Shim)\s*\|', line)
        if not m:
            continue
        s = m.group(1).lower().replace("use shim", "shimmed")
        # Extract ALL function names from the row (handles multi-func rows like
        # "| `printf`, `fprintf`, `sprintf`, `snprintf` | Available |")
        for func_match in re.finditer(r'`([a-z_]+)`', line.split('|')[1] if '|' in line else ''):
            status[func_match.group(1)] = s

    return status


def main():
    import argparse
    parser = argparse.ArgumentParser(description="Analyze POSIX function usage in OpenBSD tools")
    parser.add_argument("--no-clone", action="store_true", help="Use existing /tmp/openbsd-src")
    parser.add_argument("--fixture", type=str, help="Use fixture directory instead of OpenBSD")
    parser.add_argument("--json", action="store_true", help="Output JSON instead of text")
    args = parser.parse_args()

    root = Path(__file__).resolve().parent.parent

    if args.fixture:
        src_dir = args.fixture
        tools = [d.name for d in Path(src_dir).glob("usr.bin/*") if d.is_dir()]
    else:
        src_dir = clone_openbsd(no_clone=args.no_clone)
        tools = TARGET_TOOLS

    # Scan all tools
    print(f"\nScanning {len(tools)} tools...")
    tool_usage = {}
    for tool in sorted(tools):
        result = scan_tool(src_dir, tool)
        if result is not None:
            tool_usage[tool] = result
            print(f"  {tool}: {len(result)} unique functions, {sum(result.values())} total calls")
        else:
            print(f"  {tool}: not found or no .c files (skipped)")

    # Aggregate: function → set of tools that use it
    func_tools = defaultdict(set)
    func_total_calls = defaultdict(int)
    for tool, usage in tool_usage.items():
        for func, count in usage.items():
            func_tools[func].add(tool)
            func_total_calls[func] += count

    # Cross-reference with posix-tiers.md, newlib-availability.md, and bsd-isms.md
    tier_status = parse_posix_tiers(root)
    newlib_status = parse_newlib_availability(root)
    bsd_status = parse_bsd_isms(root)

    # Build combined status for each function
    # Priority: shimmed > available > planned > partial > missing > unknown
    def get_status(func):
        if func in tier_status:
            return tier_status[func]
        if func in bsd_status:
            return bsd_status[func]
        if func in newlib_status:
            return newlib_status[func]
        return "unknown"

    # Sort by tool count (descending), then by total calls
    ranked = sorted(
        func_tools.keys(),
        key=lambda f: (len(func_tools[f]), func_total_calls[f]),
        reverse=True
    )

    if args.json:
        output = {
            "tools_scanned": len(tool_usage),
            "tools_skipped": len(tools) - len(tool_usage),
            "functions": [
                {
                    "name": func,
                    "tool_count": len(func_tools[func]),
                    "total_calls": func_total_calls[func],
                    "tools": sorted(func_tools[func]),
                    "status": get_status(func),
                }
                for func in ranked
            ]
        }
        json.dump(output, sys.stdout, indent=2)
        return

    # Text output
    total_tools = len(tool_usage)
    print(f"\n{'='*72}")
    print(f"  POSIX Function Usage Across {total_tools} OpenBSD CLI Tools")
    print(f"{'='*72}\n")

    # Category: Missing + high frequency (implement via /extend-shim)
    print(f"{'MISSING + HIGH FREQUENCY (implement via /extend-shim):'}")
    print(f"{'─'*72}")
    gap_count = 0
    for func in ranked:
        status = get_status(func)
        tool_count = len(func_tools[func])
        if tool_count >= 3 and status in ("missing", "unknown"):
            print(f"  {func:24s} used in {tool_count:2d}/{total_tools} tools  Status: {status.upper()}")
            gap_count += 1
    if gap_count == 0:
        print("  (none — all frequent functions are covered!)")

    # Category: Shimmed (already covered)
    print(f"\n{'ALREADY SHIMMED:'}")
    print(f"{'─'*72}")
    for func in ranked[:20]:  # Top 20 only
        status = get_status(func)
        tool_count = len(func_tools[func])
        if status == "shimmed":
            print(f"  {func:24s} used in {tool_count:2d}/{total_tools} tools  ✓")

    # Category: Available in libnix
    print(f"\n{'AVAILABLE IN LIBNIX (no shim needed):'}")
    print(f"{'─'*72}")
    for func in ranked[:20]:  # Top 20 only
        status = get_status(func)
        tool_count = len(func_tools[func])
        if status == "available":
            print(f"  {func:24s} used in {tool_count:2d}/{total_tools} tools  ✓")

    # Category: Tier 3 (redesign needed)
    print(f"\n{'TIER 3 — REDESIGN REQUIRED:'}")
    print(f"{'─'*72}")
    tier3_funcs = ["fork", "execve", "execvp", "execlp", "waitpid", "wait",
                   "pthread_create", "pthread_join", "kill", "sigaction"]
    for func in tier3_funcs:
        if func in func_tools:
            tool_count = len(func_tools[func])
            print(f"  {func:24s} used in {tool_count:2d}/{total_tools} tools  — needs redesign")

    # Summary
    print(f"\n{'='*72}")
    print(f"  SUMMARY")
    print(f"{'='*72}")
    print(f"  Tools scanned:     {total_tools}")
    print(f"  Tools skipped:     {len(tools) - total_tools}")
    print(f"  Unique functions:  {len(ranked)}")
    print(f"  Gaps (freq≥3):     {gap_count}")
    print(f"{'='*72}")

    # POSIX category coverage
    categories = {
        "File I/O": ["open", "close", "read", "write", "lseek", "dup", "dup2",
                      "pread", "pwrite", "ftruncate", "pipe", "fcntl", "ioctl", "fsync"],
        "File ops": ["stat", "fstat", "lstat", "chmod", "chown", "unlink", "rename",
                     "link", "symlink", "readlink", "mkdir", "rmdir", "access",
                     "mkstemp", "realpath"],
        "Directory": ["opendir", "readdir", "closedir", "scandir", "getcwd", "chdir"],
        "String": ["strlcpy", "strlcat", "strdup", "strndup", "strsep", "strtok_r",
                   "strcasecmp", "strncasecmp"],
        "Process": ["fork", "execve", "execvp", "waitpid", "system", "getpid"],
        "Signal": ["signal", "sigaction", "kill", "raise", "alarm"],
        "Time": ["time", "gettimeofday", "localtime", "gmtime", "strftime", "mktime"],
        "Environment": ["getenv", "setenv", "unsetenv"],
        "Regex": ["regcomp", "regexec", "regfree"],
    }

    print(f"\n{'POSIX CATEGORY COVERAGE:'}")
    print(f"{'─'*72}")
    for cat_name, cat_funcs in categories.items():
        total = len(cat_funcs)
        covered = sum(1 for f in cat_funcs if get_status(f) in ("shimmed", "available", "partial"))
        pct = (covered * 100 // total) if total > 0 else 0
        bar = "█" * (pct // 5) + "░" * (20 - pct // 5)
        print(f"  {cat_name:14s} {bar} {pct:3d}% ({covered}/{total})")


if __name__ == "__main__":
    main()
