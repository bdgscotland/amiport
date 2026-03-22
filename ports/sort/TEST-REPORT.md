# FS-UAE Test Report: sort

## Summary

| Field | Value |
|-------|-------|
| Port | sort |
| Date | 2026-03-22 13:32:34 |
| Duration | 121s |
| Platform | FS-UAE 3.2.35 (A1200, Kickstart 3.1) |
| Binary | `WORK:sort` (43K) |
| Test method | ARexx harness → TAP output |
| Result | **PASS** — 18/18 passed |

## Test Results

```
1..18
ok 1 - basic alphabetical sort (no flags)
ok 2 - reverse sort (-r flag)
ok 3 - numeric sort (-n flag) sorts by value not lexicographic order
ok 4 - floating-point numeric sort (-g flag)
ok 5 - case-insensitive sort (-f flag)
ok 6 - unique sort removes duplicates (-u flag)
ok 7 - check order on already-sorted file (-c flag) returns 0
ok 8 - check order on unsorted file (-c flag) returns 10
ok 9 - month sort (-M flag) orders by calendar month
ok 10 - key field sort (-k and -t flags) sort by second colon-separated field
ok 11 - skip leading blanks (-b flag) ignores indentation before comparison
ok 12 - output to file (-o flag) writes result to T: path
ok 13 - unknown option returns error exit code
ok 14 - nonexistent input file returns error exit code
ok 15 - empty file combined with normal file produces correct output
ok 16 - -c with multiple input files returns error (only one file allowed with -c)
ok 17 - reverse numeric sort (-r -n combined) sorts numbers descending
ok 18 - Amiga WORK: volume path accepted as input file (Amiga-specific path)
# passed: 18 failed: 0 total: 18
```

### Breakdown

| # | Test | Status | Details |
|---|------|--------|---------|
| 1 | basic alphabetical sort (no flags) | PASS | |
| 2 | reverse sort (-r flag) | PASS | |
| 3 | numeric sort (-n flag) sorts by value not lexicographic order | PASS | |
| 4 | floating-point numeric sort (-g flag) | PASS | |
| 5 | case-insensitive sort (-f flag) | PASS | |
| 6 | unique sort removes duplicates (-u flag) | PASS | |
| 7 | check order on already-sorted file (-c flag) returns 0 | PASS | |
| 8 | check order on unsorted file (-c flag) returns 10 | PASS | |
| 9 | month sort (-M flag) orders by calendar month | PASS | |
| 10 | key field sort (-k and -t flags) sort by second colon-separated field | PASS | |
| 11 | skip leading blanks (-b flag) ignores indentation before comparison | PASS | |
| 12 | output to file (-o flag) writes result to T: path | PASS | |
| 13 | unknown option returns error exit code | PASS | |
| 14 | nonexistent input file returns error exit code | PASS | |
| 15 | empty file combined with normal file produces correct output | PASS | |
| 16 | -c with multiple input files returns error (only one file allowed with -c) | PASS | |
| 17 | reverse numeric sort (-r -n combined) sorts numbers descending | PASS | |
| 18 | Amiga WORK: volume path accepted as input file (Amiga-specific path) | PASS | |

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
# sort test suite for FS-UAE ARexx harness
# Category 1 CLI tool - minimum 8 tests required
# All input via pre-built files (ARexx ADDRESS COMMAND has no stdin piping)

TEST: basic alphabetical sort (no flags)
CMD: WORK:sort WORK:test-sort-alpha.txt
EXPECT: apple
EXPECT_RC: 0

TEST: reverse sort (-r flag)
CMD: WORK:sort -r WORK:test-sort-alpha.txt
EXPECT: elderberry
EXPECT_RC: 0

TEST: numeric sort (-n flag) sorts by value not lexicographic order
CMD: WORK:sort -n WORK:test-sort-numeric.txt
EXPECT: 1
EXPECT_RC: 0

TEST: floating-point numeric sort (-g flag)
CMD: WORK:sort -g WORK:test-sort-float.txt
EXPECT: 0.5
EXPECT_RC: 0

TEST: case-insensitive sort (-f flag)
CMD: WORK:sort -f WORK:test-sort-case.txt
EXPECT: apple
EXPECT_RC: 0

TEST: unique sort removes duplicates (-u flag)
CMD: WORK:sort -u WORK:test-sort-dupes.txt
EXPECT: apple
EXPECT_RC: 0

TEST: check order on already-sorted file (-c flag) returns 0
CMD: WORK:sort -c WORK:test-sort-sorted.txt
EXPECT_RC: 0

TEST: check order on unsorted file (-c flag) returns 10
CMD: WORK:sort -c WORK:test-sort-unsorted.txt
EXPECT_RC: 10

TEST: month sort (-M flag) orders by calendar month
CMD: WORK:sort -M WORK:test-sort-months.txt
EXPECT: JAN
EXPECT_RC: 0

TEST: key field sort (-k and -t flags) sort by second colon-separated field
CMD: WORK:sort -t : -k 2,2 WORK:test-sort-fields.txt
EXPECT: alpha:1
EXPECT_RC: 0

TEST: skip leading blanks (-b flag) ignores indentation before comparison
CMD: WORK:sort -b WORK:test-sort-blanks.txt
EXPECT:    apple
EXPECT_RC: 0

TEST: output to file (-o flag) writes result to T: path
CMD: WORK:sort -o T:sort-out-test.txt WORK:test-sort-alpha.txt
EXPECT_RC: 0

TEST: unknown option returns error exit code
CMD: WORK:sort -Z WORK:test-sort-alpha.txt
EXPECT_RC: 10

TEST: nonexistent input file returns error exit code
CMD: WORK:sort WORK:nonexistent-file.txt
EXPECT_RC: 10

TEST: empty file combined with normal file produces correct output
CMD: WORK:sort WORK:test-sort-alpha.txt WORK:test-empty.txt
EXPECT: apple
EXPECT_RC: 0

TEST: -c with multiple input files returns error (only one file allowed with -c)
CMD: WORK:sort -c WORK:test-sort-sorted.txt WORK:test-sort-alpha.txt
EXPECT_RC: 10

TEST: reverse numeric sort (-r -n combined) sorts numbers descending
CMD: WORK:sort -r -n WORK:test-sort-numeric.txt
EXPECT: 200
EXPECT_RC: 0

TEST: Amiga WORK: volume path accepted as input file (Amiga-specific path)
CMD: WORK:sort WORK:test-sort-sorted.txt
EXPECT: apple
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
passed=18
failed=0
total=18
```

---
Generated by `make test-fsemu TARGET=ports/sort`
Report template: `toolchain/templates/test-report.md.template`
