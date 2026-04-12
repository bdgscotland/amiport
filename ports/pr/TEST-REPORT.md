# FS-UAE Test Report: pr

## Summary

| Field | Value |
|-------|-------|
| Port | pr |
| Date | 2026-04-11 22:07:21 |
| Duration | 44s |
| Platform | FS-UAE 3.2.35 (A1200, Kickstart 3.1) |
| Binary | `WORK:pr` (58K) |
| Test method | ARexx harness → TAP output |
| Result | **PASS** — 32/32 passed |

## Test Results

```
1..32
ok 1 - -t flag suppresses header and trailer
ok 2 - -n flag adds line numbers with default width 5
ok 3 - -l N sets custom page length triggering pagination
ok 4 - -d flag double-spaces output
ok 5 - -h flag sets custom header text
ok 6 - -2 flag produces two-column vertical output
ok 7 - -3 flag produces three-column vertical output
ok 8 - -a flag produces across (row-major) column output
ok 9 - -o N adds left margin offset spaces
ok 10 - -w N sets page width for multi-column output
ok 11 - -s with colon separator character between columns
ok 12 - -f flag uses formfeed as page trailer
ok 13 - -m flag merges two files side by side
ok 14 - +N skips to page N in output
ok 15 - -r flag suppresses file open error messages (but still exits 10)
ok 16 - invalid flag causes usage error
ok 17 - missing input file causes error exit
ok 18 - -a without multiple columns is an error
ok 19 - -m with -a flag is an error
ok 20 - -m with column flag is an error
ok 21 - empty file produces no output with rc 0
ok 22 - -l below 11 forces header suppression
ok 23 - file with blank lines and tabs is processed correctly
ok 24 - stdin input via redirect produces output with -t
ok 25 - WORK: path works with combined flags -t -n -o
ok 26 - two WORK: file arguments processed sequentially
ok 27 - real-world default pagination of 80-line file produces page 2
ok 28 - real-world numbered two-column layout on 80-line file
ok 29 - real-world short page produces multiple pages on 80-line file
ok 30 - stress - line numbering on 80-line file has correct first entry
ok 31 - stress - three-column vertical layout on 80-line file
ok 32 - precision - page boundary at line 57 with default 66-line pages
# passed: 32 failed: 0 total: 32
```

### Breakdown

| # | Test | Status | Details |
|---|------|--------|---------|
| 1 | -t flag suppresses header and trailer | PASS | |
| 2 | -n flag adds line numbers with default width 5 | PASS | |
| 3 | -l N sets custom page length triggering pagination | PASS | |
| 4 | -d flag double-spaces output | PASS | |
| 5 | -h flag sets custom header text | PASS | |
| 6 | -2 flag produces two-column vertical output | PASS | |
| 7 | -3 flag produces three-column vertical output | PASS | |
| 8 | -a flag produces across (row-major) column output | PASS | |
| 9 | -o N adds left margin offset spaces | PASS | |
| 10 | -w N sets page width for multi-column output | PASS | |
| 11 | -s with colon separator character between columns | PASS | |
| 12 | -f flag uses formfeed as page trailer | PASS | |
| 13 | -m flag merges two files side by side | PASS | |
| 14 | +N skips to page N in output | PASS | |
| 15 | -r flag suppresses file open error messages (but still exits 10) | PASS | |
| 16 | invalid flag causes usage error | PASS | |
| 17 | missing input file causes error exit | PASS | |
| 18 | -a without multiple columns is an error | PASS | |
| 19 | -m with -a flag is an error | PASS | |
| 20 | -m with column flag is an error | PASS | |
| 21 | empty file produces no output with rc 0 | PASS | |
| 22 | -l below 11 forces header suppression | PASS | |
| 23 | file with blank lines and tabs is processed correctly | PASS | |
| 24 | stdin input via redirect produces output with -t | PASS | |
| 25 | WORK: path works with combined flags -t -n -o | PASS | |
| 26 | two WORK: file arguments processed sequentially | PASS | |
| 27 | real-world default pagination of 80-line file produces page 2 | PASS | |
| 28 | real-world numbered two-column layout on 80-line file | PASS | |
| 29 | real-world short page produces multiple pages on 80-line file | PASS | |
| 30 | stress - line numbering on 80-line file has correct first entry | PASS | |
| 31 | stress - three-column vertical layout on 80-line file | PASS | |
| 32 | precision - page boundary at line 57 with default 66-line pages | PASS | |

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
# pr - print file pagination utility
# Test suite for ports/pr (OpenBSD pr 1.46)
# Category: 1 (CLI utility)
#
# Input files:
#   test-pr-ten.txt        - 10 numbered lines
#   test-pr-long.txt       - 80 lines for pagination stress testing
#   test-pr-merge1.txt     - 2-line file for merge (-m) tests
#   test-pr-merge2.txt     - 2-line file for merge (-m) tests
# Shared files used:
#   test-multiline.txt     - 10 lines of varied content
#   test-empty.txt         - empty file

# -----------------------------------------------------------------------
# FUNCTIONAL TESTS: flags from setup() getopt string
# -----------------------------------------------------------------------

# Native: pr -t test-pr-ten.txt | head -1
TEST: -t flag suppresses header and trailer
CMD: WORK:pr -t WORK:test-pr-ten.txt
EXPECT: line one
EXPECT_RC: 0

# Native: pr -t -n test-pr-ten.txt | head -1
TEST: -n flag adds line numbers with default width 5
CMD: WORK:pr -t -n WORK:test-pr-ten.txt
EXPECT:     1	line one
EXPECT_LINE: 2,    2	line two
EXPECT_LINE: 10,   10	line ten
EXPECT_RC: 0

# Native: pr -l 15 test-pr-ten.txt | head -1 (blank); line 3 = header
TEST: -l N sets custom page length triggering pagination
CMD: WORK:pr -l 15 WORK:test-pr-ten.txt
EXPECT_CONTAINS: Page 1
EXPECT_CONTAINS: Page 2
EXPECT_RC: 0

# Native: pr -t -d test-pr-ten.txt | head -3
TEST: -d flag double-spaces output
CMD: WORK:pr -t -d WORK:test-pr-ten.txt
EXPECT: line one
EXPECT_LINE: 2,
EXPECT_LINE: 3,line two
EXPECT_RC: 0

# Native: pr -l 15 -h "MY HEADER" test-pr-ten.txt | grep "MY HEADER"
TEST: -h flag sets custom header text
CMD: WORK:pr -l 15 -h MY_HEADER WORK:test-pr-ten.txt
EXPECT_CONTAINS: MY_HEADER
EXPECT_CONTAINS: Page 1
EXPECT_RC: 0

# Native: pr -t -2 test-pr-ten.txt | head -1
TEST: -2 flag produces two-column vertical output
CMD: WORK:pr -t -2 WORK:test-pr-ten.txt
EXPECT_CONTAINS: line  one
EXPECT_CONTAINS: line  six
EXPECT_LINE: 2,line  two			    line  seven
EXPECT_RC: 0

# Native: pr -t -3 test-pr-ten.txt | head -1
TEST: -3 flag produces three-column vertical output
CMD: WORK:pr -t -3 WORK:test-pr-ten.txt
EXPECT_CONTAINS: line  one
EXPECT_CONTAINS: line  five
EXPECT_CONTAINS: line  nine
EXPECT_LINE: 2,line  two		line  six		line  ten
EXPECT_RC: 0

# Native: pr -t -2 -a test-pr-ten.txt | head -1
TEST: -a flag produces across (row-major) column output
CMD: WORK:pr -t -2 -a WORK:test-pr-ten.txt
EXPECT_CONTAINS: line  one
EXPECT_CONTAINS: line  two
EXPECT_RC: 0

# Native: pr -t -o 4 test-pr-ten.txt | head -1
TEST: -o N adds left margin offset spaces
CMD: WORK:pr -t -o 4 WORK:test-pr-ten.txt
EXPECT:     line one
EXPECT_LINE: 2,     line two
EXPECT_RC: 0

# Native: pr -t -2 -w 30 test-pr-ten.txt | head -1
TEST: -w N sets page width for multi-column output
CMD: WORK:pr -t -2 -w 30 WORK:test-pr-ten.txt
EXPECT_CONTAINS: line  one
EXPECT_CONTAINS: line  six
EXPECT_RC: 0

# Native: pr -t -2 -s: test-pr-ten.txt | head -1
TEST: -s with colon separator character between columns
CMD: WORK:pr -t -2 -s: WORK:test-pr-ten.txt
EXPECT_CONTAINS: line  one:line  six
EXPECT_RC: 0

# Native: pr -l 12 -f test-pr-ten.txt | grep "Page 1"
TEST: -f flag uses formfeed as page trailer
CMD: WORK:pr -l 12 -f WORK:test-pr-ten.txt
EXPECT_CONTAINS: Page 1
EXPECT_CONTAINS: Page 2
EXPECT_RC: 0

# Native: pr -t -m test-pr-merge1.txt test-pr-merge2.txt | head -1
TEST: -m flag merges two files side by side
CMD: WORK:pr -t -m WORK:test-pr-merge1.txt WORK:test-pr-merge2.txt
EXPECT_CONTAINS: file1line1
EXPECT_CONTAINS: file2line1
EXPECT_LINE: 2,file1line2			    file2line2
EXPECT_RC: 0

# Native: pr -l 12 +2 test-pr-ten.txt | grep "Page 2"
TEST: +N skips to page N in output
CMD: WORK:pr -l 12 +2 WORK:test-pr-ten.txt
EXPECT_CONTAINS: Page 2
EXPECT_RC: 0

# Native: pr -t -r /nonexistent; echo RC=1
TEST: -r flag suppresses file open error messages (but still exits 10)
CMD: WORK:pr -r WORK:test-pr-notexist.txt
EXPECT_RC: 10

# -----------------------------------------------------------------------
# ERROR PATH TESTS
# -----------------------------------------------------------------------

# Native: pr -Z; echo RC=1
TEST: invalid flag causes usage error
CMD: WORK:pr -Z WORK:test-pr-ten.txt
EXPECT_RC: 10

# Native: pr /tmp/no-such-file.txt; echo RC=1
TEST: missing input file causes error exit
CMD: WORK:pr WORK:test-pr-notexist.txt
EXPECT_RC: 10

# Native: pr -a /file (no columns); echo RC=1
TEST: -a without multiple columns is an error
CMD: WORK:pr -t -a WORK:test-pr-ten.txt
EXPECT_RC: 10

# Native: pr -m -a file1 file2; echo RC=1
TEST: -m with -a flag is an error
CMD: WORK:pr -t -m -a WORK:test-pr-merge1.txt WORK:test-pr-merge2.txt
EXPECT_RC: 10

# Native: pr -m -2 file1 file2; echo RC=1
TEST: -m with column flag is an error
CMD: WORK:pr -t -m -2 WORK:test-pr-merge1.txt WORK:test-pr-merge2.txt
EXPECT_RC: 10

# -----------------------------------------------------------------------
# EDGE CASE TESTS
# -----------------------------------------------------------------------

# Native: pr -t test-empty.txt; echo RC=0
TEST: empty file produces no output with rc 0
CMD: WORK:pr -t WORK:test-empty.txt
EXPECT_RC: 0

# Native: pr -l 8 test-pr-ten.txt | head -1
# When -l <= HEADLEN+TAILLEN (10), header is suppressed automatically
TEST: -l below 11 forces header suppression
CMD: WORK:pr -l 8 WORK:test-pr-ten.txt
EXPECT: line one
EXPECT_RC: 0

# Native: pr -t test-multiline.txt | head -1
TEST: file with blank lines and tabs is processed correctly
CMD: WORK:pr -t WORK:test-multiline.txt
EXPECT: hello world
EXPECT_LINE: 10,last line
EXPECT_RC: 0

# Native: pr -t < test-pr-ten.txt | head -1
# pr reads stdin when no file given; AmigaDOS supports < redirection in Execute
TEST: stdin input via redirect produces output with -t
CMD: WORK:pr -t <WORK:test-pr-ten.txt
EXPECT: line one
EXPECT_RC: 0

# -----------------------------------------------------------------------
# AMIGA-SPECIFIC TESTS
# -----------------------------------------------------------------------

# Test with WORK: volume path (AmigaDOS path handling)
# Native: pr -t -n -o 2 test-pr-ten.txt | head -1
TEST: WORK: path works with combined flags -t -n -o
CMD: WORK:pr -t -n -o 2 WORK:test-pr-ten.txt
EXPECT:       1	line one
EXPECT_RC: 0

# Test processing two files sequentially via AmigaDOS paths
TEST: two WORK: file arguments processed sequentially
CMD: WORK:pr -t WORK:test-pr-merge1.txt WORK:test-pr-merge2.txt
EXPECT: file1line1
EXPECT_LINE: 3,file2line1
EXPECT_RC: 0

# -----------------------------------------------------------------------
# REAL-WORLD AND STRESS TESTS
# -----------------------------------------------------------------------

# Real-world: paginating an 80-line document with default 66-line pages
# Native: pr test-pr-long.txt | grep "Page 2"
TEST: real-world default pagination of 80-line file produces page 2
CMD: WORK:pr WORK:test-pr-long.txt
EXPECT_CONTAINS: Page 1
EXPECT_CONTAINS: Page 2
EXPECT_RC: 0

# Real-world: numbered multi-column layout (classic pr usage)
# Native: pr -t -n -2 test-pr-long.txt | head -1
TEST: real-world numbered two-column layout on 80-line file
CMD: WORK:pr -t -n -2 WORK:test-pr-long.txt
EXPECT_CONTAINS: Line  1
EXPECT_RC: 0

# Real-world: narrow page for printing (24-line terminal simulation)
# Native: pr -l 24 -t test-pr-long.txt | wc-derived line count
TEST: real-world short page produces multiple pages on 80-line file
CMD: WORK:pr -l 24 WORK:test-pr-long.txt
EXPECT_CONTAINS: Page 1
EXPECT_CONTAINS: Page 2
EXPECT_CONTAINS: Page 3
EXPECT_RC: 0

# Stress: line numbering on full 80-line file, verify first and last lines
# Native: pr -t -n test-pr-long.txt | head -1 and tail -1
TEST: stress - line numbering on 80-line file has correct first entry
CMD: WORK:pr -t -n WORK:test-pr-long.txt
EXPECT:     1	Line 1: The quick brown fox jumps over the lazy dog
EXPECT_LINE: 80,   80	Line 80: The quick brown fox jumps over the lazy dog
EXPECT_RC: 0

# Stress: 3-column layout on 80-line file (exercises vertical column balancing)
# Native: pr -t -3 test-pr-long.txt | head -1
TEST: stress - three-column vertical layout on 80-line file
CMD: WORK:pr -t -3 WORK:test-pr-long.txt
EXPECT_CONTAINS: Line  1
EXPECT_CONTAINS: Line  28
EXPECT_RC: 0

# Precision: verify exact page boundary with default 66-line pages
# Native: pr -t test-pr-long.txt | sed -n '57p'
# Default: 66 line page, header=5+trailer=5, content=56 lines per page
# Page 1: lines 1-56, Page 2: lines 57-80
TEST: precision - page boundary at line 57 with default 66-line pages
CMD: WORK:pr -t WORK:test-pr-long.txt
EXPECT_LINE: 57,Line 57: The quick brown fox jumps over the lazy dog
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
passed=32
failed=0
total=32
```

---
Generated by `make test-fsemu TARGET=ports/pr`
Report template: `toolchain/templates/test-report.md.template`
