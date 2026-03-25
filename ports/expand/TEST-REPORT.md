# FS-UAE Test Report: expand

## Summary

| Field | Value |
|-------|-------|
| Port | expand |
| Date | 2026-03-25 14:31:57 |
| Duration | 38s |
| Platform | FS-UAE 3.2.35 (A1200, Kickstart 3.1) |
| Binary | `WORK:expand` (35K) |
| Test method | ARexx harness → TAP output |
| Result | **PASS** — 29/29 passed |

## Test Results

```
1..29
ok 1 - Default 8-column tab stop expands single tab
ok 2 - Default 8-column tab stop with leading tab runs without error
ok 3 - Flag -t 4 expands to 4-column tab stops
ok 4 - Flag -t 4 leading tab runs without error
ok 5 - Flag -t with comma-separated list of stops
ok 6 - Flag -t 1 gives one space per tab
ok 7 - Obsolete -N syntax accepted as tab stop (same as -t N)
ok 8 - Multiple file arguments processed sequentially
ok 9 - File with no tabs passes through unchanged
ok 10 - Empty file produces no output and exits cleanly
ok 11 - Unknown flag returns RETURN_ERROR
ok 12 - Non-numeric -t argument returns RETURN_ERROR
ok 13 - Zero tab stop value returns RETURN_ERROR
ok 14 - Non-ascending tab stop list returns RETURN_ERROR
ok 15 - Nonexistent file returns RETURN_ERROR
ok 16 - File with mixed tabs and spaces preserves spaces
ok 17 - Backspace character passes through and adjusts column count
ok 18 - Long line with many tabs (>1024 chars input) processes correctly
ok 19 - Shared special-chars file (has tabs) first line expands correctly
ok 20 - WORK: volume path used for input file resolves correctly
ok 21 - Multiple WORK: volume paths in single command work
ok 22 - Real-world: expand multiline file preserves line count
ok 23 - Real-world: expand multiline file with embedded tab expands correctly
ok 24 - Real-world: two files expand sequentially with correct tab output
ok 25 - Real-world: -t4 on a file with 4-char columns aligns correctly
ok 26 - Stress: 200-line tab-indented file expands without crash
ok 27 - Stress: deeply-indented line (5 tabs) in stress file expands correctly
ok 28 - Precision: default tab on foo-bar-baz line produces correct spacing
ok 29 - Precision: -t4 on 4-char prefix aligns next column at 8
# passed: 29 failed: 0 total: 29
```

### Breakdown

| # | Test | Status | Details |
|---|------|--------|---------|
| 1 | Default 8-column tab stop expands single tab | PASS | |
| 2 | Default 8-column tab stop with leading tab runs without error | PASS | |
| 3 | Flag -t 4 expands to 4-column tab stops | PASS | |
| 4 | Flag -t 4 leading tab runs without error | PASS | |
| 5 | Flag -t with comma-separated list of stops | PASS | |
| 6 | Flag -t 1 gives one space per tab | PASS | |
| 7 | Obsolete -N syntax accepted as tab stop (same as -t N) | PASS | |
| 8 | Multiple file arguments processed sequentially | PASS | |
| 9 | File with no tabs passes through unchanged | PASS | |
| 10 | Empty file produces no output and exits cleanly | PASS | |
| 11 | Unknown flag returns RETURN_ERROR | PASS | |
| 12 | Non-numeric -t argument returns RETURN_ERROR | PASS | |
| 13 | Zero tab stop value returns RETURN_ERROR | PASS | |
| 14 | Non-ascending tab stop list returns RETURN_ERROR | PASS | |
| 15 | Nonexistent file returns RETURN_ERROR | PASS | |
| 16 | File with mixed tabs and spaces preserves spaces | PASS | |
| 17 | Backspace character passes through and adjusts column count | PASS | |
| 18 | Long line with many tabs (>1024 chars input) processes correctly | PASS | |
| 19 | Shared special-chars file (has tabs) first line expands correctly | PASS | |
| 20 | WORK: volume path used for input file resolves correctly | PASS | |
| 21 | Multiple WORK: volume paths in single command work | PASS | |
| 22 | Real-world: expand multiline file preserves line count | PASS | |
| 23 | Real-world: expand multiline file with embedded tab expands correctly | PASS | |
| 24 | Real-world: two files expand sequentially with correct tab output | PASS | |
| 25 | Real-world: -t4 on a file with 4-char columns aligns correctly | PASS | |
| 26 | Stress: 200-line tab-indented file expands without crash | PASS | |
| 27 | Stress: deeply-indented line (5 tabs) in stress file expands correctly | PASS | |
| 28 | Precision: default tab on foo-bar-baz line produces correct spacing | PASS | |
| 29 | Precision: -t4 on 4-char prefix aligns next column at 8 | PASS | |

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
# test-fsemu-cases.txt -- FS-UAE functional test suite for expand 1.15
# expand: convert tabs to spaces
# Source: OpenBSD expand.c rev 1.15
# Category: 1 (CLI tool)
# Minimum tests required: 15
#
# Flags tested: -t tablist (comma-separated or single number)
# Obsolete syntax: -N (numeric flag, same as -t N)
# Error paths tested:
#   err(10, "%s", argv[0])    -- nonexistent file
#   errx(10, "Bad tab stop spec")  -- invalid/zero/negative/non-ascending -t
#   errx(10, "Too many tab stops") -- >100 tab stops
#   usage()/exit(10)          -- unknown flag
#
# NOTE: error messages go to stderr -- not captured by harness.
# Error path tests use EXPECT_RC: only.
#
# NOTE: expand reads stdin when no file argument is given.
# All CMD lines pass an explicit file argument to prevent stdin hang.

# --- FUNCTIONAL TESTS ---

TEST: Default 8-column tab stop expands single tab
CMD: WORK:expand WORK:test-expand-tabs.txt
EXPECT: hello   world
EXPECT_RC: 0

TEST: Default 8-column tab stop with leading tab runs without error
CMD: WORK:expand WORK:test-expand-onlytabs.txt
EXPECT_RC: 0

TEST: Flag -t 4 expands to 4-column tab stops
CMD: WORK:expand -t 4 WORK:test-expand-t4.txt
EXPECT: a   b
EXPECT_RC: 0

TEST: Flag -t 4 leading tab runs without error
CMD: WORK:expand -t 4 WORK:test-expand-onlytabs.txt
EXPECT_RC: 0

TEST: Flag -t with comma-separated list of stops
CMD: WORK:expand -t 4,8,12 WORK:test-expand-mixed.txt
EXPECT_CONTAINS: col1
EXPECT_RC: 0

TEST: Flag -t 1 gives one space per tab
CMD: WORK:expand -t 1 WORK:test-expand-tabs.txt
EXPECT_CONTAINS: hello world
EXPECT_RC: 0

TEST: Obsolete -N syntax accepted as tab stop (same as -t N)
CMD: WORK:expand -4 WORK:test-expand-t4.txt
EXPECT: a   b
EXPECT_RC: 0

TEST: Multiple file arguments processed sequentially
CMD: WORK:expand WORK:test-expand-multifile1.txt WORK:test-expand-multifile2.txt
EXPECT: file one content
EXPECT_RC: 0

TEST: File with no tabs passes through unchanged
CMD: WORK:expand WORK:test-expand-notabs.txt
EXPECT: hello world
EXPECT_RC: 0

TEST: Empty file produces no output and exits cleanly
CMD: WORK:expand WORK:test-empty.txt
EXPECT_RC: 0

# --- ERROR PATH TESTS ---

TEST: Unknown flag returns RETURN_ERROR
CMD: WORK:expand -Z WORK:test-empty.txt
EXPECT_RC: 10

TEST: Non-numeric -t argument returns RETURN_ERROR
CMD: WORK:expand -t abc WORK:test-empty.txt
EXPECT_RC: 10

TEST: Zero tab stop value returns RETURN_ERROR
CMD: WORK:expand -t 0 WORK:test-empty.txt
EXPECT_RC: 10

TEST: Non-ascending tab stop list returns RETURN_ERROR
CMD: WORK:expand -t 8,4 WORK:test-empty.txt
EXPECT_RC: 10

TEST: Nonexistent file returns RETURN_ERROR
CMD: WORK:expand WORK:nonexistent-file.txt
EXPECT_RC: 10

# --- EDGE CASE TESTS ---

TEST: File with mixed tabs and spaces preserves spaces
CMD: WORK:expand WORK:test-expand-mixed.txt
EXPECT_CONTAINS: col1    col2    col3
EXPECT_RC: 0

TEST: Backspace character passes through and adjusts column count
CMD: WORK:expand WORK:test-expand-backspace.txt
EXPECT_CONTAINS: abc
EXPECT_RC: 0

TEST: Long line with many tabs (>1024 chars input) processes correctly
CMD: WORK:expand WORK:test-expand-longtabs.txt
EXPECT_CONTAINS: col000
EXPECT_RC: 0

TEST: Shared special-chars file (has tabs) first line expands correctly
CMD: WORK:expand WORK:test-special-chars.txt
EXPECT: line    with    tabs
EXPECT_RC: 0

# --- AMIGA-SPECIFIC TESTS ---

TEST: WORK: volume path used for input file resolves correctly
CMD: WORK:expand WORK:test-expand-tabs.txt
EXPECT_CONTAINS: hello
EXPECT_RC: 0

TEST: Multiple WORK: volume paths in single command work
CMD: WORK:expand WORK:test-expand-multifile1.txt WORK:test-expand-multifile2.txt
EXPECT_CONTAINS: file two content
EXPECT_RC: 0

# --- REAL-WORLD AND STRESS TESTS ---

TEST: Real-world: expand multiline file preserves line count
CMD: WORK:expand WORK:test-multiline.txt
EXPECT: hello world
EXPECT_RC: 0

TEST: Real-world: expand multiline file with embedded tab expands correctly
CMD: WORK:expand WORK:test-multiline.txt
EXPECT_CONTAINS: line with       tabs
EXPECT_RC: 0

TEST: Real-world: two files expand sequentially with correct tab output
CMD: WORK:expand WORK:test-expand-multifile1.txt WORK:test-expand-multifile2.txt
EXPECT_CONTAINS: line    two
EXPECT_RC: 0

TEST: Real-world: -t4 on a file with 4-char columns aligns correctly
CMD: WORK:expand -t 4 WORK:test-expand-t4.txt
EXPECT_CONTAINS: 1234    5678
EXPECT_RC: 0

TEST: Stress: 200-line tab-indented file expands without crash
CMD: WORK:expand WORK:test-expand-stress.txt
EXPECT_CONTAINS: line 0000 with 1 tab indents
EXPECT_RC: 0

TEST: Stress: deeply-indented line (5 tabs) in stress file expands correctly
CMD: WORK:expand WORK:test-expand-stress.txt
EXPECT_CONTAINS: line 0004 with 5 tab indents
EXPECT_RC: 0

TEST: Precision: default tab on foo-bar-baz line produces correct spacing
CMD: WORK:expand WORK:test-expand-tabs.txt
EXPECT_CONTAINS: foo     bar     baz
EXPECT_RC: 0

TEST: Precision: -t4 on 4-char prefix aligns next column at 8
CMD: WORK:expand -t 4 WORK:test-expand-t4.txt
EXPECT_CONTAINS: 1234    5678
EXPECT_RC: 0
```

## Emulator Log

```
(log not captured in this run)
```

## Sentinel File

Written by the ARexx harness when all tests complete:

```
TESTS_COMPLETE
passed=29
failed=0
total=29
```

---
Generated by `make test-fsemu TARGET=ports/expand`
Report template: `toolchain/templates/test-report.md.template`
