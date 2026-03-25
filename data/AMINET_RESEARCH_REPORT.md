# Aminet Research Report: 25 Unix Tools

**Date:** 2026-03-25
**Researcher:** aminet-researcher agent
**Scope:** 25 core Unix utilities for potential Amiga porting

## Executive Summary

Of the 25 tools researched:
- **3 tools exist** on Aminet with usable implementations (md5, date, cat as TYPE equivalent, printf as library)
- **22 tools are missing** standalone implementations on Aminet
- **High-value candidates**: whoami, logname, tty, nl, expr, test, seq, which (available via Geek Gadgets, but not standalone)

## Detailed Findings

### Confirmed Existing Ports

#### md5 (Multiple Implementations)
- **Status:** SKIP (already ported)
- **Aminet Packages:**
  - `util/crypt/MD5SUM.lha` — MD5 checksum utility
  - `util/misc/md5sum2.lha` — Alternative MD5sum implementation
  - `util/rexx/ARexxMD5.lha` — ARexx-based MD5
  - `util/misc/md5.lha` — Another variant
- **Quality:** Multiple implementations available; users have choices
- **Architecture:** Support for m68k Amiga 3.x confirmed
- **Verdict:** SKIP — No value in porting another implementation

#### date (AmigaDOS DATE + Helpers)
- **Status:** UPGRADE (outdated variant exists)
- **Existing:**
  - AmigaDOS native `DATE` command (sets/displays system date)
  - `util/time/adate.lha` — Amiga-specific date utility (1990s era)
- **Limitation:** AmigaDOS DATE format differs from POSIX date(1)
  - AmigaDOS: `DD-MMM-YY HH:MM:SS` (e.g., `25-MAR-26 15:30:00`)
  - POSIX: Unlimited format string support (`+%Y-%m-%d`, etc.)
- **Value:** Modern POSIX date(1) would be useful for scripts expecting standard format strings
- **Verdict:** UPGRADE — Fresh port of OpenBSD/GNU date would add value

#### cat (AmigaDOS TYPE Equivalent)
- **Status:** UPGRADE (different tool, similar function)
- **Existing:**
  - AmigaDOS `TYPE` command — reads and displays files
- **Difference:**
  - TYPE doesn't support file concatenation (its primary purpose)
  - TYPE doesn't support `-` for stdin, piping, or multiple files
  - No options like `-n` (line numbering), `-v` (visible non-printing), etc.
- **Value:** Proper POSIX cat would unlock pipe-based text processing
- **Verdict:** UPGRADE — Unix pipes and concatenation require true cat(1)

#### printf (Library Function, No Shell Command)
- **Status:** UPGRADE (function exists, shell command missing)
- **Existing:**
  - `amiga.lib/printf()` — C library function in AmigaOS SDK
- **Missing:**
  - `printf` shell command (POSIX utility for formatted output)
- **Use Case:** Shell scripts expecting `printf "format" args` syntax
- **Verdict:** UPGRADE — Shell printf utility missing despite library function

### Confirmed Missing (High-Value Candidates)

#### whoami (User Identity)
- **Status:** PORT (missing)
- **Research:** No standalone Aminet package found; part of Geek Gadgets
- **AmigaOS Equivalent:** None (no native user identity concept on classic Amiga)
- **Use Case:** Scripts/programs checking current user; permissions checking
- **Complexity:** Trivial (single system call + output)
- **Verdict:** PORT — Low complexity, useful for user/permission scripts

#### logname (Login Name)
- **Status:** PORT (missing)
- **Research:** No standalone Aminet package
- **AmigaOS Equivalent:** `LOGNAME` environment variable (if set)
- **Use Case:** Scripts checking login username
- **Complexity:** Trivial (env var lookup + output)
- **Verdict:** PORT — Complements whoami; useful for user identity

#### tty (Terminal Name)
- **Status:** PORT (missing)
- **Research:** No standalone Aminet package found
- **AmigaOS Equivalent:** None (device model is different: `CON:` vs `/dev/tty`)
- **Use Case:** Detecting if running interactively; terminal info
- **Complexity:** Low (check if Input() is interactive + return device name)
- **Interop Note:** Uses `IsInteractive()` from dos.library
- **Verdict:** PORT — Useful for interactive program detection

#### nl (Line Numbering)
- **Status:** PORT (missing)
- **Research:** No Aminet package; part of Geek Gadgets
- **Functionality:** Adds line numbers to file output
- **Complexity:** Low (read lines, format with numbers)
- **Verdict:** PORT — Trivial utility; useful for log formatting

#### expr (Expression Evaluation)
- **Status:** PORT (missing)
- **Research:** No standalone package; Geek Gadgets has it
- **Use Case:** Shell arithmetic in scripts (before bash)
- **Complexity:** Medium (arithmetic parser + evaluation)
- **Verdict:** PORT — Enables classic shell scripts with math

#### test (Conditional Testing / `[` builtin)
- **Status:** PORT (missing)
- **Research:** No standalone Aminet package
- **AmigaOS Equivalent:** `IF` conditional syntax (different model)
- **Use Case:** Shell script conditionals (`[ -f file ]`)
- **Complexity:** Medium (argument parsing, file/string tests)
- **Verdict:** PORT — Critical for POSIX shell script compatibility

#### seq (Number Sequences)
- **Status:** PORT (missing)
- **Research:** No Aminet package found
- **Functionality:** Generate number sequences (1 2 3...)
- **Complexity:** Low (numeric iteration + formatting)
- **Verdict:** PORT — Useful for bash/loop replacement in Amiga shells

#### which (Command Locator)
- **Status:** PORT (missing)
- **Research:** No standalone package; Geek Gadgets has it
- **Use Case:** Locating executables in PATH (script debugging)
- **Complexity:** Low (PATH parsing + file existence check)
- **Verdict:** PORT — Useful for script portability debugging

#### jot (Numeric Generation with Formatting)
- **Status:** PORT (optional)
- **Research:** BSD utility, not on Aminet
- **Functionality:** More feature-rich than seq; generates random numbers, ranges, etc.
- **Complexity:** Medium (formatting, randomness)
- **Verdict:** PORT (if prioritizing seeded randomness) or SKIP (seq sufficient)

### Confirmed Missing (Lower-Priority)

#### colrm (Column Removal)
- **Status:** PORT (lower priority)
- **Research:** No Aminet package
- **Niche:** Text filtering tool
- **Complexity:** Low
- **Verdict:** PORT (if extending text filter suite)

#### cmp (File Comparison)
- **Status:** PORT (useful but has alternatives)
- **Research:** No Aminet package
- **Alternatives:** AmigaDOS TYPE + DIFF concepts
- **Complexity:** Low-Medium
- **Verdict:** PORT (complements diff but not critical)

#### cksum (Checksum Verification)
- **Status:** PORT (has alternatives)
- **Research:** No standalone package; md5sum available
- **Overlap:** md5, md5sum already exist on Aminet
- **Verdict:** SKIP (md5sum supersedes; no added value)

#### dc (Desk Calculator)
- **Status:** PORT (optional)
- **Research:** No Aminet package
- **Niche:** Arbitrary-precision arithmetic
- **Complexity:** High (parsing, bignum arithmetic)
- **Verdict:** SKIP (expr sufficient for scripts; calc tools exist)

#### factor (Prime Factorization)
- **Status:** PORT (niche)
- **Research:** No Aminet package
- **Niche:** Educational/mathematical
- **Complexity:** Low-Medium
- **Verdict:** SKIP (too specialized)

#### look (Word Dictionary Lookup)
- **Status:** PORT (niche)
- **Research:** No Aminet package
- **Niche:** Dictionary/word tools
- **Complexity:** Low
- **Verdict:** SKIP (too specialized for most users)

#### strings (Binary Analysis)
- **Status:** PORT (developer tool)
- **Research:** No standalone package
- **Use Case:** Extracting printable strings from binaries
- **Complexity:** Low (scan for printable sequences)
- **Verdict:** PORT (if extending developer tools suite)

#### pr (Print Formatter)
- **Status:** PORT (formatting tool)
- **Research:** No Aminet package
- **Functionality:** Multi-column output, headers, pagination
- **Complexity:** Medium
- **Verdict:** PORT (if extending text formatting suite)

#### col (Reverse Line Feed Filter)
- **Status:** PORT (specialty tool)
- **Research:** No Aminet package
- **Niche:** Post-processing overstriking (typewriter legacy)
- **Complexity:** Low
- **Verdict:** SKIP (obsolete use case)

#### vis (Visible Non-Printing Characters)
- **Status:** PORT (specialized)
- **Research:** No Aminet package
- **Functionality:** Display non-printing chars as escape sequences
- **Complexity:** Low
- **Verdict:** PORT (useful for debugging; similar to cat -v)

#### rs (Reshape/Tabulate)
- **Status:** PORT (specialized)
- **Research:** No Aminet package
- **Functionality:** Reshape data into columns/rows
- **Complexity:** Medium
- **Verdict:** SKIP (niche; awk/sed better fit)

#### nohup (Background Job Immunity)
- **Status:** PORT (job control)
- **Research:** No Aminet package
- **AmigaOS Equivalent:** `Run >NIL:` command backgrounding
- **Complexity:** Low (redirect stderr, ignore SIGHUP)
- **Verdict:** PORT (if prioritizing job control; AmigaDOS Run works but different syntax)

## Recommendations by Priority

### Tier 1 (High Value, Low Complexity)
**Recommended for immediate porting:**
1. **whoami** — User identity (trivial complexity, enables user-aware scripts)
2. **logname** — Login name (trivial, complements whoami)
3. **tty** — Terminal detection (low complexity, useful for interactive checks)
4. **nl** — Line numbering (trivial, useful for log formatting)
5. **test** — Shell conditionals (medium complexity, **critical** for script compatibility)

### Tier 2 (Good Value, Medium Complexity)
**Recommended for follow-up:**
6. **expr** — Shell arithmetic (medium complexity, enables math in scripts)
7. **seq** — Number generation (low complexity, bash replacement)
8. **which** — Command locator (low complexity, helps script debugging)
9. **printf** — Format command (shell variant; C library exists but CLI missing)
10. **date** — POSIX date formatting (medium complexity; upgrade over adate.lha)

### Tier 3 (Specialized/Lower Priority)
**Consider if extending specialist tools:**
- **cmp**, **pr**, **col**, **colrm**, **vis**, **strings** — Text processing extensions
- **jot** — Alternative to seq (more features, higher complexity)
- **nohup** — Job control (Amiga Run serves similar purpose)

### Tier 4 (Niche/Skip)
**Not recommended:**
- **cksum** — Superseded by md5sum already on Aminet
- **dc** — Specialized arithmetic; expr sufficient
- **factor** — Educational only
- **look** — Dictionary/word tool too specialized
- **rs** — Better handled by awk/sed
- **col** — Obsolete typewriter legacy

## Research Sources

- [Aminet - The World's Largest Amiga Software Archive](https://www.aminet.net/)
- [Geek Gadgets - GNU Tools for Amiga](http://www.chingu.asia/wiki/index.php?title=Installing+the+Geek+Gadgets)
- [AmigaOS Manual - AmigaDOS Command Reference](https://wiki.amigaos.net/wiki/AmigaOS_Manual:_AmigaDOS_Command_Reference)
- [AMIX (Amiga Unix)](https://www.amigaunix.com/)
- Individual Aminet package pages (MD5SUM.lha, md5sum2.lha, adate.lha, etc.)

## Implementation Notes

### Porting Strategy
- **Geek Gadgets dependency:** Many of these tools already exist in Geek Gadgets but are not available as standalone binaries. A "Geek Gadgets port" simply bundles the already-ported GNU version with Amiga-specific documentation and startup code.
- **Source selection:** Use OpenBSD coreutils as source (like existing ports: grep, uniq, tr) or GNU coreutils when needed.
- **Test coverage:** Emphasize shell script integration testing — these tools gain value through scripting support.

### Known Integration Points
- **whoami + logname** — Often used together for user identity verification
- **test + expr** — Core shell script utilities; should be ported together
- **tty + isatty()** — Relate to console/interactive detection; share code patterns
- **seq + jot** — Number generation tools; if porting one, consider both

## Conclusion

**Of the 25 tools:**
- **3 effectively exist** on Aminet (md5, date, cat/TYPE)
- **5 are critically valuable** for script compatibility (whoami, logname, tty, test, nl)
- **5 more add significant value** with medium effort (expr, seq, which, printf, date upgrade)
- **Remaining 12 are specialists** or lower priority

**Recommended next step:** Port Tier 1 tools first (whoami, logname, tty, nl, test). These unlock POSIX shell script compatibility with minimal effort. Tier 2 (expr, seq, which, printf, date) should follow once Tier 1 is integrated and tested.
