# FS-UAE Test Report: comm

## Summary

| Field | Value |
|-------|-------|
| Port | comm |
| Date | 2026-03-25 15:51:08 |
| Duration | 32s |
| Platform | FS-UAE 3.2.35 (A1200, Kickstart 3.1) |
| Binary | `WORK:comm` (28K) |
| Test method | ARexx harness → TAP output |
| Result | **PASS** — 30/30 passed |

## Test Results

```
1..30
ok 1 - Default output shows all three columns with tab offsets
ok 2 - -12 flag suppresses file1-only and file2-only columns, shows only common lines
ok 3 - -3 flag suppresses common column, shows only non-common lines
ok 4 - -23 flag shows only lines unique to file1
ok 5 - -13 flag shows only lines unique to file2
ok 6 - -1 flag suppresses file1-only column, file2-only and common remain
ok 7 - -2 flag suppresses file2-only column, file1-only and common remain
ok 8 - -123 flag suppresses all columns, produces no output
ok 9 - -f flag makes comparison case-insensitive, common lines include case variants
ok 10 - Without -f case-sensitive comparison treats apple and APPLE as distinct
ok 11 - With -f all case variants are common, -23 shows no file1-only lines
ok 12 - Error on missing file argument (wrong arg count)
ok 13 - Error on too many arguments triggers usage and RETURN_ERROR
ok 14 - Error on invalid flag triggers usage and RETURN_ERROR
ok 15 - Error on nonexistent file returns RETURN_ERROR
ok 16 - Two empty files produce no output
ok 17 - Single-line file compared with empty file shows that line in col1
ok 18 - Empty file vs single-line file shows that line in col2 with tab prefix
ok 19 - Files with no common lines produce no col3 output under -12
ok 20 - Files with no common lines show both sets under -3
ok 21 - Identical files show all lines in common column under -12
ok 22 - Identical files produce no output under -123
ok 23 - Long-line files handled correctly, common long lines appear under -12
ok 24 - Long-line files with -23 show no file1-only lines when files are identical
ok 25 - Amiga WORK: volume paths resolve correctly for both file arguments
ok 26 - Stress test 200-line files, -23 shows 100 file1-only lines starting with item0001
ok 27 - Stress test 200-line files, -12 shows 100 common lines starting with item0002
ok 28 - Stress test 200-line files, -13 shows file2-only zitem lines starting with zitem0001
ok 29 - Real-world use case: find lines common to both sorted lists via -12
ok 30 - Real-world use case: set difference (file1 minus file2) via -23
# passed: 30 failed: 0 total: 30
```

### Breakdown

| # | Test | Status | Details |
|---|------|--------|---------|
| 1 | Default output shows all three columns with tab offsets | PASS | |
| 2 | -12 flag suppresses file1-only and file2-only columns, shows only common lines | PASS | |
| 3 | -3 flag suppresses common column, shows only non-common lines | PASS | |
| 4 | -23 flag shows only lines unique to file1 | PASS | |
| 5 | -13 flag shows only lines unique to file2 | PASS | |
| 6 | -1 flag suppresses file1-only column, file2-only and common remain | PASS | |
| 7 | -2 flag suppresses file2-only column, file1-only and common remain | PASS | |
| 8 | -123 flag suppresses all columns, produces no output | PASS | |
| 9 | -f flag makes comparison case-insensitive, common lines include case variants | PASS | |
| 10 | Without -f case-sensitive comparison treats apple and APPLE as distinct | PASS | |
| 11 | With -f all case variants are common, -23 shows no file1-only lines | PASS | |
| 12 | Error on missing file argument (wrong arg count) | PASS | |
| 13 | Error on too many arguments triggers usage and RETURN_ERROR | PASS | |
| 14 | Error on invalid flag triggers usage and RETURN_ERROR | PASS | |
| 15 | Error on nonexistent file returns RETURN_ERROR | PASS | |
| 16 | Two empty files produce no output | PASS | |
| 17 | Single-line file compared with empty file shows that line in col1 | PASS | |
| 18 | Empty file vs single-line file shows that line in col2 with tab prefix | PASS | |
| 19 | Files with no common lines produce no col3 output under -12 | PASS | |
| 20 | Files with no common lines show both sets under -3 | PASS | |
| 21 | Identical files show all lines in common column under -12 | PASS | |
| 22 | Identical files produce no output under -123 | PASS | |
| 23 | Long-line files handled correctly, common long lines appear under -12 | PASS | |
| 24 | Long-line files with -23 show no file1-only lines when files are identical | PASS | |
| 25 | Amiga WORK: volume paths resolve correctly for both file arguments | PASS | |
| 26 | Stress test 200-line files, -23 shows 100 file1-only lines starting with item0001 | PASS | |
| 27 | Stress test 200-line files, -12 shows 100 common lines starting with item0002 | PASS | |
| 28 | Stress test 200-line files, -13 shows file2-only zitem lines starting with zitem0001 | PASS | |
| 29 | Real-world use case: find lines common to both sorted lists via -12 | PASS | |
| 30 | Real-world use case: set difference (file1 minus file2) via -23 | PASS | |

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
# test-fsemu-cases.txt -- FS-UAE functional test suite for comm 1.11
# comm compares two sorted files line by line, printing three columns:
#   col1 = lines only in file1 (no prefix)
#   col2 = lines only in file2 (one tab)
#   col3 = lines in both files (two tabs)
# Flags: -1 suppress col1, -2 suppress col2, -3 suppress col3, -f case-insensitive
#
# Test input files:
#   test-comm-file1.txt  -- apple, banana, cherry, elderberry, fig
#   test-comm-file2.txt  -- apple, cherry, date, fig, grape
#   common lines: apple, cherry, fig
#   file1-only:   banana, elderberry
#   file2-only:   date, grape

TEST: Default output shows all three columns with tab offsets
CMD: WORK:comm WORK:test-comm-file1.txt WORK:test-comm-file2.txt
EXPECT_CONTAINS: apple
EXPECT_RC: 0

# Native: comm -12 test-comm-file1.txt test-comm-file2.txt -> apple/cherry/fig
TEST: -12 flag suppresses file1-only and file2-only columns, shows only common lines
CMD: WORK:comm -12 WORK:test-comm-file1.txt WORK:test-comm-file2.txt
EXPECT: apple
EXPECT_LINE: 2,cherry
EXPECT_LINE: 3,fig
EXPECT_RC: 0

# Native: comm -3 test-comm-file1.txt test-comm-file2.txt -> banana/date/elderberry/grape
TEST: -3 flag suppresses common column, shows only non-common lines
CMD: WORK:comm -3 WORK:test-comm-file1.txt WORK:test-comm-file2.txt
EXPECT: banana
EXPECT_LINE: 3,elderberry
EXPECT_RC: 0

TEST: -23 flag shows only lines unique to file1
CMD: WORK:comm -23 WORK:test-comm-file1.txt WORK:test-comm-file2.txt
EXPECT: banana
EXPECT_RC: 0

TEST: -13 flag shows only lines unique to file2
CMD: WORK:comm -13 WORK:test-comm-file1.txt WORK:test-comm-file2.txt
EXPECT: date
EXPECT_RC: 0

TEST: -1 flag suppresses file1-only column, file2-only and common remain
CMD: WORK:comm -1 WORK:test-comm-file1.txt WORK:test-comm-file2.txt
EXPECT_CONTAINS: date
EXPECT_RC: 0

TEST: -2 flag suppresses file2-only column, file1-only and common remain
CMD: WORK:comm -2 WORK:test-comm-file1.txt WORK:test-comm-file2.txt
EXPECT_CONTAINS: banana
EXPECT_RC: 0

TEST: -123 flag suppresses all columns, produces no output
CMD: WORK:comm -123 WORK:test-comm-file1.txt WORK:test-comm-file2.txt
EXPECT:
EXPECT_RC: 0

TEST: -f flag makes comparison case-insensitive, common lines include case variants
CMD: WORK:comm -12 -f WORK:test-comm-casea.txt WORK:test-comm-caseb.txt
EXPECT: apple
EXPECT_RC: 0

TEST: Without -f case-sensitive comparison treats apple and APPLE as distinct
CMD: WORK:comm -23 WORK:test-comm-casea.txt WORK:test-comm-caseb.txt
EXPECT_CONTAINS: apple
EXPECT_RC: 0

TEST: With -f all case variants are common, -23 shows no file1-only lines
CMD: WORK:comm -23 -f WORK:test-comm-casea.txt WORK:test-comm-caseb.txt
EXPECT:
EXPECT_RC: 0

TEST: Error on missing file argument (wrong arg count)
CMD: WORK:comm WORK:test-comm-file1.txt
EXPECT_RC: 10

TEST: Error on too many arguments triggers usage and RETURN_ERROR
CMD: WORK:comm WORK:test-comm-file1.txt WORK:test-comm-file2.txt WORK:test-comm-single.txt
EXPECT_RC: 10

TEST: Error on invalid flag triggers usage and RETURN_ERROR
CMD: WORK:comm -Z WORK:test-comm-file1.txt WORK:test-comm-file2.txt
EXPECT_RC: 10

TEST: Error on nonexistent file returns RETURN_ERROR
CMD: WORK:comm WORK:no-such-file.txt WORK:test-comm-file2.txt
EXPECT_RC: 10

TEST: Two empty files produce no output
CMD: WORK:comm WORK:test-comm-empty.txt WORK:test-comm-empty.txt
EXPECT:
EXPECT_RC: 0

TEST: Single-line file compared with empty file shows that line in col1
CMD: WORK:comm -3 WORK:test-comm-single.txt WORK:test-comm-empty.txt
EXPECT: hello world
EXPECT_RC: 0

TEST: Empty file vs single-line file shows that line in col2 with tab prefix
CMD: WORK:comm -3 WORK:test-comm-empty.txt WORK:test-comm-single.txt
EXPECT_CONTAINS: hello world
EXPECT_RC: 0

TEST: Files with no common lines produce no col3 output under -12
CMD: WORK:comm -12 WORK:test-comm-nocommon1.txt WORK:test-comm-nocommon2.txt
EXPECT:
EXPECT_RC: 0

TEST: Files with no common lines show both sets under -3
CMD: WORK:comm -3 WORK:test-comm-nocommon1.txt WORK:test-comm-nocommon2.txt
EXPECT: aardvark
EXPECT_RC: 0

TEST: Identical files show all lines in common column under -12
CMD: WORK:comm -12 WORK:test-comm-file1.txt WORK:test-comm-file1.txt
EXPECT: apple
EXPECT_RC: 0

TEST: Identical files produce no output under -123
CMD: WORK:comm -123 WORK:test-comm-file1.txt WORK:test-comm-file1.txt
EXPECT:
EXPECT_RC: 0

TEST: Long-line files handled correctly, common long lines appear under -12
CMD: WORK:comm -12 WORK:test-comm-longline1.txt WORK:test-comm-longline2.txt
EXPECT_CONTAINS: MARKER
EXPECT_RC: 0

TEST: Long-line files with -23 show no file1-only lines when files are identical
CMD: WORK:comm -23 WORK:test-comm-longline1.txt WORK:test-comm-longline2.txt
EXPECT:
EXPECT_RC: 0

TEST: Amiga WORK: volume paths resolve correctly for both file arguments
CMD: WORK:comm -12 WORK:test-comm-file1.txt WORK:test-comm-file1.txt
EXPECT: apple
EXPECT_RC: 0

TEST: Stress test 200-line files, -23 shows 100 file1-only lines starting with item0001
CMD: WORK:comm -23 WORK:test-comm-stress1.txt WORK:test-comm-stress2.txt
EXPECT: item0001
EXPECT_RC: 0

TEST: Stress test 200-line files, -12 shows 100 common lines starting with item0002
CMD: WORK:comm -12 WORK:test-comm-stress1.txt WORK:test-comm-stress2.txt
EXPECT: item0002
EXPECT_RC: 0

TEST: Stress test 200-line files, -13 shows file2-only zitem lines starting with zitem0001
CMD: WORK:comm -13 WORK:test-comm-stress1.txt WORK:test-comm-stress2.txt
EXPECT: zitem0001
EXPECT_RC: 0

TEST: Real-world use case: find lines common to both sorted lists via -12
CMD: WORK:comm -12 WORK:test-comm-file1.txt WORK:test-comm-file2.txt
EXPECT_CONTAINS: cherry
EXPECT_RC: 0

TEST: Real-world use case: set difference (file1 minus file2) via -23
CMD: WORK:comm -23 WORK:test-comm-file1.txt WORK:test-comm-file2.txt
EXPECT_CONTAINS: elderberry
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
passed=30
failed=0
total=30
```

---
Generated by `make test-fsemu TARGET=ports/comm`
Report template: `toolchain/templates/test-report.md.template`
