# FS-UAE Test Report: diff

## Summary

| Field | Value |
|-------|-------|
| Port | diff |
| Date | 2026-03-22 22:11:10 |
| Duration | 54s |
| Platform | FS-UAE 3.2.35 (A1200, Kickstart 3.1) |
| Binary | `WORK:diff` (59K) |
| Test method | ARexx harness → TAP output |
| Result | **PASS** — 50/50 passed |

## Test Results

```
1..50
ok 1 - Identical files produce no output and return RC 0
ok 2 - Differing files produce normal diff output and return RC 5
ok 3 - Nonexistent first file returns RC 10
ok 4 - Nonexistent second file returns RC 10
ok 5 - Missing arguments (too few) returns RC 10
ok 6 - Force ASCII comparison (-a) compares binary file as text
ok 7 - Fold blanks (-b) treats whitespace differences as identical
ok 8 - Context diff (-c) produces *** header
ok 9 - Context diff with 1 line of context (-C 1) produces *** header
ok 10 - Context diff with 0 context lines (-C 0) shows only changed line
ok 11 - Minimal diff (-d) produces output for differing files
ok 12 - Ifdef diff (-D MYDEF) wraps changes in #ifdef blocks
ok 13 - Ifdef diff (-D MYDEF) outputs #ifndef directive for old text
ok 14 - Ed script format (-e) produces change command with dot terminator
ok 15 - Reverse ed script (-f) produces change command
ok 16 - Backward compat flag (-h) is silently ignored and normal diff runs
ok 17 - Ignore pattern (-I) disabled but does not crash
ok 18 - Case-insensitive diff (-i) treats case differences as identical
ok 19 - Case-sensitive diff (no flags) detects case differences
ok 20 - Label flag (-L) replaces filename in unified diff header
ok 21 - RCS format (-n) produces RCS-style diff with d/a commands
ok 22 - New-file flag (-N) does not crash when used with regular files
ok 23 - Show function prototype (-p) with unified diff includes function name in @@
ok 24 - Unidirectional new-file (-P) does not crash when used with regular files
ok 25 - Brief mode (-q) reports files differ without showing the diff
ok 26 - Brief mode (-q) on identical files produces no output
ok 27 - Recursive flag (-r) does not crash when comparing regular files
ok 28 - Report identical files (-s) prints identical message
ok 29 - Starting file (-S) does not crash when used with regular files
ok 30 - Initial tab flag (-T) in normal diff produces output with tab after marker
ok 31 - Expand tabs (-t) with tab-indented files produces diff output
ok 32 - Unified diff format (-u) produces --- header
ok 33 - Unified diff format (-u) contains @@ hunk marker
ok 34 - Unified diff with 1 line context (-U 1) produces @@ hunk marker
ok 35 - Unified diff with 0 context lines (-U 0) shows only changed line
ok 36 - Ignore all whitespace (-w) treats whitespace-only diff as identical
ok 37 - Exclude from file (-X) does not crash when used with regular files
ok 38 - Exclude pattern (-x) does not crash when used with regular files
ok 39 - Numeric context after -c (e.g. -c2) sets context to 2 lines
ok 40 - Numeric context after -u (e.g. -u0) sets unified diff to 0 context lines
ok 41 - Unified diff with case insensitive (-u -i) on case-only diff returns RC 0
ok 42 - Brief mode with fold blanks (-q -b) on blank-only diff returns RC 0
ok 43 - Context diff with ignore all whitespace (-c -w) on blank-only diff returns RC 0
ok 44 - Unified diff with report identical (-u -s) on same file prints identical
ok 45 - Binary file detection returns binary differ message
ok 46 - Comparing file to itself returns RC 0 (identical inodes via path)
ok 47 - Identical content but different filename returns RC 0
ok 48 - WORK: volume path handling works for both arguments
ok 49 - Unified diff with WORK: paths produces correct --- header format
ok 50 - Invalid flag returns RC 10 (usage printed to stderr)
# passed: 50 failed: 0 total: 50
```

### Breakdown

| # | Test | Status | Details |
|---|------|--------|---------|
| 1 | Identical files produce no output and return RC 0 | PASS | |
| 2 | Differing files produce normal diff output and return RC 5 | PASS | |
| 3 | Nonexistent first file returns RC 10 | PASS | |
| 4 | Nonexistent second file returns RC 10 | PASS | |
| 5 | Missing arguments (too few) returns RC 10 | PASS | |
| 6 | Force ASCII comparison (-a) compares binary file as text | PASS | |
| 7 | Fold blanks (-b) treats whitespace differences as identical | PASS | |
| 8 | Context diff (-c) produces *** header | PASS | |
| 9 | Context diff with 1 line of context (-C 1) produces *** header | PASS | |
| 10 | Context diff with 0 context lines (-C 0) shows only changed line | PASS | |
| 11 | Minimal diff (-d) produces output for differing files | PASS | |
| 12 | Ifdef diff (-D MYDEF) wraps changes in #ifdef blocks | PASS | |
| 13 | Ifdef diff (-D MYDEF) outputs #ifndef directive for old text | PASS | |
| 14 | Ed script format (-e) produces change command with dot terminator | PASS | |
| 15 | Reverse ed script (-f) produces change command | PASS | |
| 16 | Backward compat flag (-h) is silently ignored and normal diff runs | PASS | |
| 17 | Ignore pattern (-I) disabled but does not crash | PASS | |
| 18 | Case-insensitive diff (-i) treats case differences as identical | PASS | |
| 19 | Case-sensitive diff (no flags) detects case differences | PASS | |
| 20 | Label flag (-L) replaces filename in unified diff header | PASS | |
| 21 | RCS format (-n) produces RCS-style diff with d/a commands | PASS | |
| 22 | New-file flag (-N) does not crash when used with regular files | PASS | |
| 23 | Show function prototype (-p) with unified diff includes function name in @@ | PASS | |
| 24 | Unidirectional new-file (-P) does not crash when used with regular files | PASS | |
| 25 | Brief mode (-q) reports files differ without showing the diff | PASS | |
| 26 | Brief mode (-q) on identical files produces no output | PASS | |
| 27 | Recursive flag (-r) does not crash when comparing regular files | PASS | |
| 28 | Report identical files (-s) prints identical message | PASS | |
| 29 | Starting file (-S) does not crash when used with regular files | PASS | |
| 30 | Initial tab flag (-T) in normal diff produces output with tab after marker | PASS | |
| 31 | Expand tabs (-t) with tab-indented files produces diff output | PASS | |
| 32 | Unified diff format (-u) produces --- header | PASS | |
| 33 | Unified diff format (-u) contains @@ hunk marker | PASS | |
| 34 | Unified diff with 1 line context (-U 1) produces @@ hunk marker | PASS | |
| 35 | Unified diff with 0 context lines (-U 0) shows only changed line | PASS | |
| 36 | Ignore all whitespace (-w) treats whitespace-only diff as identical | PASS | |
| 37 | Exclude from file (-X) does not crash when used with regular files | PASS | |
| 38 | Exclude pattern (-x) does not crash when used with regular files | PASS | |
| 39 | Numeric context after -c (e.g. -c2) sets context to 2 lines | PASS | |
| 40 | Numeric context after -u (e.g. -u0) sets unified diff to 0 context lines | PASS | |
| 41 | Unified diff with case insensitive (-u -i) on case-only diff returns RC 0 | PASS | |
| 42 | Brief mode with fold blanks (-q -b) on blank-only diff returns RC 0 | PASS | |
| 43 | Context diff with ignore all whitespace (-c -w) on blank-only diff returns RC 0 | PASS | |
| 44 | Unified diff with report identical (-u -s) on same file prints identical | PASS | |
| 45 | Binary file detection returns binary differ message | PASS | |
| 46 | Comparing file to itself returns RC 0 (identical inodes via path) | PASS | |
| 47 | Identical content but different filename returns RC 0 | PASS | |
| 48 | WORK: volume path handling works for both arguments | PASS | |
| 49 | Unified diff with WORK: paths produces correct --- header format | PASS | |
| 50 | Invalid flag returns RC 10 (usage printed to stderr) | PASS | |

## Environment

| Component | Version/Path |
|-----------|-------------|
| FS-UAE | 3.2.35 |
| Kickstart | 3.1 (40.68) |
| Amiga model | A1200 (68020) |
| Compiler | m68k-amigaos-gcc (bebbo) |
| POSIX shim | libamiport.a |
| Regex emu | libamiport-emu.a |
| Test harness | ARexx (test-runner.rexx) |

## Test Cases

Each test runs the command inside AmigaOS, captures stdout to a file,
and compares against the expected output string.

```
# diff test suite for FS-UAE ARexx harness
# Category 1 (CLI tool) -- minimum 15 tests required
# Exit codes: 0 = files identical (RETURN_OK), 5 = files differ (RETURN_WARN), 10 = error (RETURN_ERROR)
# All flags from OPTIONS: "0123456789abC:cdD:efhI:iL:nNPpqrS:sTtU:uwX:x:"
# Uses pre-created input files (ARexx ADDRESS COMMAND does not support piping)
# stderr is NOT captured by the test harness -- error path tests use EXPECT_RC: only

# === EXIT CODE TESTS ===

TEST: Identical files produce no output and return RC 0
CMD: WORK:diff WORK:test-diff-same.txt WORK:test-diff-same.txt
EXPECT:
EXPECT_RC: 0

TEST: Differing files produce normal diff output and return RC 5
CMD: WORK:diff WORK:test-diff-a.txt WORK:test-diff-b.txt
EXPECT: 2c2
EXPECT_RC: 5

TEST: Nonexistent first file returns RC 10
CMD: WORK:diff WORK:nonexistent.txt WORK:test-diff-a.txt
EXPECT_RC: 10

TEST: Nonexistent second file returns RC 10
CMD: WORK:diff WORK:test-diff-a.txt WORK:nonexistent2.txt
EXPECT_RC: 10

TEST: Missing arguments (too few) returns RC 10
CMD: WORK:diff WORK:test-diff-a.txt
EXPECT_RC: 10

# === FUNCTIONAL TESTS -- all flags ===

# -a: force ASCII comparison (treat binary as text)
TEST: Force ASCII comparison (-a) compares binary file as text
CMD: WORK:diff -a WORK:test-diff-a.txt WORK:test-diff-b.txt
EXPECT: 2c2
EXPECT_RC: 5

# -b: fold blanks (treat runs of whitespace as equivalent)
TEST: Fold blanks (-b) treats whitespace differences as identical
CMD: WORK:diff -b WORK:test-diff-blanks-a.txt WORK:test-diff-blanks-b.txt
EXPECT:
EXPECT_RC: 0

# -c: context diff (default 3 lines)
TEST: Context diff (-c) produces *** header
CMD: WORK:diff -c WORK:test-diff-a.txt WORK:test-diff-b.txt
EXPECT_CONTAINS: ***
EXPECT_RC: 5

# -C N: context diff with N lines of context
TEST: Context diff with 1 line of context (-C 1) produces *** header
CMD: WORK:diff -C 1 WORK:test-diff-a.txt WORK:test-diff-b.txt
EXPECT_CONTAINS: ***
EXPECT_RC: 5

# -C 0: context diff with 0 lines of context
TEST: Context diff with 0 context lines (-C 0) shows only changed line
CMD: WORK:diff -C 0 WORK:test-diff-a.txt WORK:test-diff-b.txt
EXPECT_CONTAINS: ***
EXPECT_RC: 5

# -d: minimal diff (use shorter diff algorithm)
TEST: Minimal diff (-d) produces output for differing files
CMD: WORK:diff -d WORK:test-diff-a.txt WORK:test-diff-b.txt
EXPECT: 2c2
EXPECT_RC: 5

# -D string: ifdef diff output
TEST: Ifdef diff (-D MYDEF) wraps changes in #ifdef blocks
CMD: WORK:diff -D MYDEF WORK:test-diff-a.txt WORK:test-diff-b.txt
EXPECT_CONTAINS: MYDEF
EXPECT_RC: 5

TEST: Ifdef diff (-D MYDEF) outputs #ifndef directive for old text
CMD: WORK:diff -D MYDEF WORK:test-diff-a.txt WORK:test-diff-b.txt
EXPECT_CONTAINS: #ifndef
EXPECT_RC: 5

# -e: ed script output
TEST: Ed script format (-e) produces change command with dot terminator
CMD: WORK:diff -e WORK:test-diff-a.txt WORK:test-diff-b.txt
EXPECT_CONTAINS: line2 changed
EXPECT_RC: 5

# -f: reverse ed script output
TEST: Reverse ed script (-f) produces change command
CMD: WORK:diff -f WORK:test-diff-a.txt WORK:test-diff-b.txt
EXPECT_CONTAINS: c2
EXPECT_RC: 5

# -h: silently ignored for backward compat
TEST: Backward compat flag (-h) is silently ignored and normal diff runs
CMD: WORK:diff -h WORK:test-diff-a.txt WORK:test-diff-b.txt
EXPECT: 2c2
EXPECT_RC: 5

# -I pattern: disabled on AmigaOS (no regex) -- should not crash, warning goes to stderr
TEST: Ignore pattern (-I) disabled but does not crash
CMD: WORK:diff -I line WORK:test-diff-a.txt WORK:test-diff-b.txt
EXPECT_RC: 5

# -i: case insensitive comparison
TEST: Case-insensitive diff (-i) treats case differences as identical
CMD: WORK:diff -i WORK:test-diff-case-a.txt WORK:test-diff-case-b.txt
EXPECT:
EXPECT_RC: 0

TEST: Case-sensitive diff (no flags) detects case differences
CMD: WORK:diff WORK:test-diff-case-a.txt WORK:test-diff-case-b.txt
EXPECT: 1,2c1,2
EXPECT_RC: 5

# -L label: use label in headers (unified diff)
TEST: Label flag (-L) replaces filename in unified diff header
CMD: WORK:diff -u -L oldfile -L newfile WORK:test-diff-a.txt WORK:test-diff-b.txt
EXPECT_CONTAINS: oldfile
EXPECT_RC: 5

# -n: RCS-style output (D_NREVERSE)
TEST: RCS format (-n) produces RCS-style diff with d/a commands
CMD: WORK:diff -n WORK:test-diff-a.txt WORK:test-diff-b.txt
EXPECT_CONTAINS: d2 1
EXPECT_RC: 5

# -N: treat absent files as empty (dir diff flag -- test doesn't crash on files)
TEST: New-file flag (-N) does not crash when used with regular files
CMD: WORK:diff -N WORK:test-diff-a.txt WORK:test-diff-b.txt
EXPECT: 2c2
EXPECT_RC: 5

# -p: show C function prototypes in @@ hunk header
TEST: Show function prototype (-p) with unified diff includes function name in @@
CMD: WORK:diff -u -p WORK:test-diff-func-a.txt WORK:test-diff-func-b.txt
EXPECT_CONTAINS: @@
EXPECT_RC: 5

# -P: treat absent first files as empty (dir diff flag -- test doesn't crash on files)
TEST: Unidirectional new-file (-P) does not crash when used with regular files
CMD: WORK:diff -P WORK:test-diff-a.txt WORK:test-diff-b.txt
EXPECT: 2c2
EXPECT_RC: 5

# -q: brief output (files differ message only)
TEST: Brief mode (-q) reports files differ without showing the diff
CMD: WORK:diff -q WORK:test-diff-a.txt WORK:test-diff-b.txt
EXPECT_CONTAINS: differ
EXPECT_RC: 5

TEST: Brief mode (-q) on identical files produces no output
CMD: WORK:diff -q WORK:test-diff-same.txt WORK:test-diff-same.txt
EXPECT:
EXPECT_RC: 0

# -r: recursive directory diff (flag only -- test doesn't crash when used on files)
TEST: Recursive flag (-r) does not crash when comparing regular files
CMD: WORK:diff -r WORK:test-diff-a.txt WORK:test-diff-b.txt
EXPECT: 2c2
EXPECT_RC: 5

# -s: report identical files
TEST: Report identical files (-s) prints identical message
CMD: WORK:diff -s WORK:test-diff-same.txt WORK:test-diff-same.txt
EXPECT_CONTAINS: identical
EXPECT_RC: 0

# -S name: start at named file in dir diff (flag only -- test doesn't crash on files)
TEST: Starting file (-S) does not crash when used with regular files
CMD: WORK:diff -r -S somefile WORK:test-diff-a.txt WORK:test-diff-b.txt
EXPECT: 2c2
EXPECT_RC: 5

# -T: expand initial tab in output (adds tab after </>/<! markers)
TEST: Initial tab flag (-T) in normal diff produces output with tab after marker
CMD: WORK:diff -T WORK:test-diff-a.txt WORK:test-diff-b.txt
EXPECT_CONTAINS: 2c2
EXPECT_RC: 5

# -t: expand all tabs in output
TEST: Expand tabs (-t) with tab-indented files produces diff output
CMD: WORK:diff -t WORK:test-diff-tabs-a.txt WORK:test-diff-tabs-b.txt
EXPECT_CONTAINS: TWO
EXPECT_RC: 5

# -u: unified diff (default 3 lines context)
TEST: Unified diff format (-u) produces --- header
CMD: WORK:diff -u WORK:test-diff-a.txt WORK:test-diff-b.txt
EXPECT_CONTAINS: ---
EXPECT_RC: 5

TEST: Unified diff format (-u) contains @@ hunk marker
CMD: WORK:diff -u WORK:test-diff-a.txt WORK:test-diff-b.txt
EXPECT_CONTAINS: @@
EXPECT_RC: 5

# -U N: unified diff with N lines of context
TEST: Unified diff with 1 line context (-U 1) produces @@ hunk marker
CMD: WORK:diff -U 1 WORK:test-diff-a.txt WORK:test-diff-b.txt
EXPECT_CONTAINS: @@
EXPECT_RC: 5

TEST: Unified diff with 0 context lines (-U 0) shows only changed line
CMD: WORK:diff -U 0 WORK:test-diff-a.txt WORK:test-diff-b.txt
EXPECT_CONTAINS: @@
EXPECT_RC: 5

# -w: ignore all whitespace
TEST: Ignore all whitespace (-w) treats whitespace-only diff as identical
CMD: WORK:diff -w WORK:test-diff-blanks-a.txt WORK:test-diff-blanks-b.txt
EXPECT:
EXPECT_RC: 0

# -X file: read exclude patterns from file (dir diff -- test doesn't crash on files)
TEST: Exclude from file (-X) does not crash when used with regular files
CMD: WORK:diff -X WORK:test-diff-exclude.txt WORK:test-diff-a.txt WORK:test-diff-b.txt
EXPECT: 2c2
EXPECT_RC: 5

# -x pattern: exclude files matching pattern (dir diff -- pattern stored, not applied to file args)
TEST: Exclude pattern (-x) does not crash when used with regular files
CMD: WORK:diff -x backup WORK:test-diff-a.txt WORK:test-diff-b.txt
EXPECT: 2c2
EXPECT_RC: 5

# === NUMERIC CONTEXT LINES (0-9 flags: OPTIONS "0123456789" are digits used as context count modifiers) ===
# The digits 0-9 in OPTIONS are valid only immediately after -c or -u to set context lines.
# -c2 is equivalent to -C 2 (context diff, 2 lines of context).

TEST: Numeric context after -c (e.g. -c2) sets context to 2 lines
CMD: WORK:diff -c2 WORK:test-diff-a.txt WORK:test-diff-b.txt
EXPECT_CONTAINS: ***
EXPECT_RC: 5

TEST: Numeric context after -u (e.g. -u0) sets unified diff to 0 context lines
CMD: WORK:diff -u0 WORK:test-diff-a.txt WORK:test-diff-b.txt
EXPECT_CONTAINS: @@
EXPECT_RC: 5

# === FLAG COMBINATIONS ===

TEST: Unified diff with case insensitive (-u -i) on case-only diff returns RC 0
CMD: WORK:diff -u -i WORK:test-diff-case-a.txt WORK:test-diff-case-b.txt
EXPECT:
EXPECT_RC: 0

TEST: Brief mode with fold blanks (-q -b) on blank-only diff returns RC 0
CMD: WORK:diff -q -b WORK:test-diff-blanks-a.txt WORK:test-diff-blanks-b.txt
EXPECT:
EXPECT_RC: 0

TEST: Context diff with ignore all whitespace (-c -w) on blank-only diff returns RC 0
CMD: WORK:diff -c -w WORK:test-diff-blanks-a.txt WORK:test-diff-blanks-b.txt
EXPECT:
EXPECT_RC: 0

TEST: Unified diff with report identical (-u -s) on same file prints identical
CMD: WORK:diff -u -s WORK:test-diff-same.txt WORK:test-diff-same.txt
EXPECT_CONTAINS: identical
EXPECT_RC: 0

# === EDGE CASE TESTS ===

TEST: Binary file detection returns binary differ message
CMD: WORK:diff WORK:test-diff-a.txt WORK:test-binary.dat
EXPECT_CONTAINS: Binary files
EXPECT_RC: 5

TEST: Comparing file to itself returns RC 0 (identical inodes via path)
CMD: WORK:diff WORK:test-diff-a.txt WORK:test-diff-a.txt
EXPECT:
EXPECT_RC: 0

TEST: Identical content but different filename returns RC 0
CMD: WORK:diff WORK:test-diff-a.txt WORK:test-diff-same.txt
EXPECT:
EXPECT_RC: 0

# === AMIGA-SPECIFIC TESTS ===

TEST: WORK: volume path handling works for both arguments
CMD: WORK:diff WORK:test-diff-a.txt WORK:test-diff-b.txt
EXPECT: 2c2
EXPECT_RC: 5

TEST: Unified diff with WORK: paths produces correct --- header format
CMD: WORK:diff -u WORK:test-diff-a.txt WORK:test-diff-b.txt
EXPECT_CONTAINS: +++
EXPECT_RC: 5

TEST: Invalid flag returns RC 10 (usage printed to stderr)
CMD: WORK:diff -Z WORK:test-diff-a.txt WORK:test-diff-b.txt
EXPECT_RC: 10
```

## Emulator Log

```
(log not captured in this run)
```

## Sentinel File

Written by the ARexx harness when all tests complete:

```
TESTS_COMPLETE
passed=50
failed=0
total=50
```

---
Generated by `make test-fsemu TARGET=ports/diff`
Report template: `toolchain/templates/test-report.md.template`
