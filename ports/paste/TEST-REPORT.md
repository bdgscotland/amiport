# FS-UAE Test Report: paste

## Summary

| Field | Value |
|-------|-------|
| Port | paste |
| Date | 2026-03-26 13:37:56 |
| Duration | 27s |
| Platform | FS-UAE 3.2.35 (A1200, Kickstart 3.1) |
| Binary | `WORK:paste` (37K) |
| Test method | ARexx harness → TAP output |
| Result | **PASS** — 24/24 passed |

## Test Results

```
1..24
ok 1 - Default parallel paste two files uses tab delimiter
ok 2 - Parallel paste three files produces three tab-separated columns
ok 3 - Short file padded with empty field when other files have more lines
ok 4 - -d comma sets comma as field delimiter
ok 5 - -d colon sets colon as field delimiter
ok 6 - -d with two-char string uses delimiters cyclically
ok 7 - -d backslash-t produces tab delimiter explicitly
ok 8 - -d backslash-zero uses null byte meaning no delimiter between fields
ok 9 - -s sequential mode merges all lines from one file onto one line
ok 10 - -s sequential mode produces one output line per input file
ok 11 - -s combined with -d uses custom delimiter between sequential values
ok 12 - -s combined with cyclic -d applies delimiters cyclically in sequential mode
ok 13 - Invalid flag prints usage to stderr and exits RC=10
ok 14 - Nonexistent input file exits RC=10
ok 15 - Empty file in parallel mode produces leading tab with other file content
ok 16 - Single file in parallel mode outputs each line unchanged
ok 17 - Amiga WORK volume paths handled correctly for two-file paste
ok 18 - Amiga WORK volume paths work for -s mode
ok 19 - Real-world CSV construction with comma delimiter and header row
ok 20 - Real-world cyclic delimiters with padding when files have unequal lengths
ok 21 - Stress parallel paste of 20-line files processes all lines correctly
ok 22 - Stress sequential mode on 20-line file produces single output line
ok 23 - Stress three-file parallel paste of 20-line files
ok 24 - -d backslash-n in sequential mode puts each field on its own line
# passed: 24 failed: 0 total: 24
```

### Breakdown

| # | Test | Status | Details |
|---|------|--------|---------|
| 1 | Default parallel paste two files uses tab delimiter | PASS | |
| 2 | Parallel paste three files produces three tab-separated columns | PASS | |
| 3 | Short file padded with empty field when other files have more lines | PASS | |
| 4 | -d comma sets comma as field delimiter | PASS | |
| 5 | -d colon sets colon as field delimiter | PASS | |
| 6 | -d with two-char string uses delimiters cyclically | PASS | |
| 7 | -d backslash-t produces tab delimiter explicitly | PASS | |
| 8 | -d backslash-zero uses null byte meaning no delimiter between fields | PASS | |
| 9 | -s sequential mode merges all lines from one file onto one line | PASS | |
| 10 | -s sequential mode produces one output line per input file | PASS | |
| 11 | -s combined with -d uses custom delimiter between sequential values | PASS | |
| 12 | -s combined with cyclic -d applies delimiters cyclically in sequential mode | PASS | |
| 13 | Invalid flag prints usage to stderr and exits RC=10 | PASS | |
| 14 | Nonexistent input file exits RC=10 | PASS | |
| 15 | Empty file in parallel mode produces leading tab with other file content | PASS | |
| 16 | Single file in parallel mode outputs each line unchanged | PASS | |
| 17 | Amiga WORK volume paths handled correctly for two-file paste | PASS | |
| 18 | Amiga WORK volume paths work for -s mode | PASS | |
| 19 | Real-world CSV construction with comma delimiter and header row | PASS | |
| 20 | Real-world cyclic delimiters with padding when files have unequal lengths | PASS | |
| 21 | Stress parallel paste of 20-line files processes all lines correctly | PASS | |
| 22 | Stress sequential mode on 20-line file produces single output line | PASS | |
| 23 | Stress three-file parallel paste of 20-line files | PASS | |
| 24 | -d backslash-n in sequential mode puts each field on its own line | PASS | |

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
# test-fsemu-cases.txt -- FS-UAE test suite for paste 1.27
# paste: merge lines from files side by side
# Flags covered: (default), -d <list>, -s
# Category: 1 (CLI utility)
# Minimum: 8 tests. This suite has 20 tests.

# ===== FUNCTIONAL TESTS: DEFAULT PARALLEL MODE =====

# Native: paste test-paste-a.txt test-paste-b.txt | head -1
TEST: Default parallel paste two files uses tab delimiter
CMD: WORK:paste WORK:test-paste-a.txt WORK:test-paste-b.txt
EXPECT: alpha	one
EXPECT_LINE: 2,beta	two
EXPECT_LINE: 3,gamma	three
EXPECT_RC: 0

# Native: paste test-paste-a.txt test-paste-b.txt test-paste-c.txt | head -1
TEST: Parallel paste three files produces three tab-separated columns
CMD: WORK:paste WORK:test-paste-a.txt WORK:test-paste-b.txt WORK:test-paste-c.txt
EXPECT: alpha	one	apple
EXPECT_LINE: 2,beta	two	orange
EXPECT_LINE: 3,gamma	three	banana
EXPECT_RC: 0

# Native: paste test-paste-short.txt test-paste-a.txt | sed -n '3p'
TEST: Short file padded with empty field when other files have more lines
CMD: WORK:paste WORK:test-paste-short.txt WORK:test-paste-a.txt
EXPECT: x	alpha
EXPECT_LINE: 2,y	beta
EXPECT_LINE: 3,	gamma
EXPECT_RC: 0

# ===== FUNCTIONAL TESTS: -d FLAG =====

# Native: paste -d, test-paste-a.txt test-paste-b.txt | head -1
TEST: -d comma sets comma as field delimiter
CMD: WORK:paste -d, WORK:test-paste-a.txt WORK:test-paste-b.txt
EXPECT: alpha,one
EXPECT_LINE: 2,beta,two
EXPECT_LINE: 3,gamma,three
EXPECT_RC: 0

# Native: paste -d: test-paste-a.txt test-paste-b.txt | head -1
TEST: -d colon sets colon as field delimiter
CMD: WORK:paste -d: WORK:test-paste-a.txt WORK:test-paste-b.txt
EXPECT: alpha:one
EXPECT_RC: 0

# Native: paste -d '|:' test-paste-a.txt test-paste-b.txt test-paste-c.txt | head -1
TEST: -d with two-char string uses delimiters cyclically
CMD: WORK:paste -d|: WORK:test-paste-a.txt WORK:test-paste-b.txt WORK:test-paste-c.txt
EXPECT: alpha|one:apple
EXPECT_LINE: 2,beta|two:orange
EXPECT_RC: 0

# Native: paste -d '\t' test-paste-a.txt test-paste-b.txt | head -1
TEST: -d backslash-t produces tab delimiter explicitly
CMD: WORK:paste -d\t WORK:test-paste-a.txt WORK:test-paste-b.txt
EXPECT: alpha	one
EXPECT_RC: 0

# Native: paste -d '\0' test-paste-a.txt test-paste-b.txt | head -1
TEST: -d backslash-zero uses null byte meaning no delimiter between fields
CMD: WORK:paste -d\0 WORK:test-paste-a.txt WORK:test-paste-b.txt
EXPECT: alphaone
EXPECT_LINE: 2,betatwo
EXPECT_RC: 0

# ===== FUNCTIONAL TESTS: -s FLAG =====

# Native: paste -s test-paste-a.txt | head -1
TEST: -s sequential mode merges all lines from one file onto one line
CMD: WORK:paste -s WORK:test-paste-a.txt
EXPECT: alpha	beta	gamma
EXPECT_RC: 0

# Native: paste -s test-paste-a.txt test-paste-b.txt
TEST: -s sequential mode produces one output line per input file
CMD: WORK:paste -s WORK:test-paste-a.txt WORK:test-paste-b.txt
EXPECT: alpha	beta	gamma
EXPECT_LINE: 2,one	two	three
EXPECT_RC: 0

# Native: paste -s -d, test-paste-a.txt
TEST: -s combined with -d uses custom delimiter between sequential values
CMD: WORK:paste -s -d, WORK:test-paste-a.txt
EXPECT: alpha,beta,gamma
EXPECT_RC: 0

# Native: paste -s -d '|:' test-paste-a.txt
TEST: -s combined with cyclic -d applies delimiters cyclically in sequential mode
CMD: WORK:paste -s -d|: WORK:test-paste-a.txt
EXPECT: alpha|beta:gamma
EXPECT_RC: 0

# ===== ERROR PATH TESTS =====

# Note: running paste with NO arguments at all would hang (reads stdin).
# The -Z invalid flag test triggers usage() and exits without reading stdin.
TEST: Invalid flag prints usage to stderr and exits RC=10
CMD: WORK:paste -Z WORK:test-paste-a.txt
EXPECT_RC: 10

# Missing/nonexistent file causes err(10, filename) with RC=10
# Error message goes to stderr (not captured) -- verify only via RC
TEST: Nonexistent input file exits RC=10
CMD: WORK:paste WORK:test-paste-nonexistent.txt
EXPECT_RC: 10

# ===== EDGE CASE TESTS =====

# Native: paste /dev/null test-paste-a.txt | head -1
TEST: Empty file in parallel mode produces leading tab with other file content
CMD: WORK:paste WORK:test-empty.txt WORK:test-paste-a.txt
EXPECT: 	alpha
EXPECT_RC: 0

# Native: paste test-paste-a.txt (single file parallel = one column)
TEST: Single file in parallel mode outputs each line unchanged
CMD: WORK:paste WORK:test-paste-a.txt
EXPECT: alpha
EXPECT_LINE: 2,beta
EXPECT_LINE: 3,gamma
EXPECT_RC: 0

# ===== AMIGA-SPECIFIC TESTS =====

# Verify WORK: volume paths work correctly with multiple file arguments
# Native: paste test-paste-names.txt test-paste-ages.txt | head -1
TEST: Amiga WORK volume paths handled correctly for two-file paste
CMD: WORK:paste WORK:test-paste-names.txt WORK:test-paste-ages.txt
EXPECT: name	age
EXPECT_RC: 0

TEST: Amiga WORK volume paths work for -s mode
CMD: WORK:paste -s WORK:test-paste-b.txt
EXPECT: one	two	three
EXPECT_RC: 0

# ===== REAL-WORLD AND STRESS TESTS =====

# Real-world: build a CSV from two files (names and ages column with header)
# Native: paste -d, test-paste-names.txt test-paste-ages.txt
TEST: Real-world CSV construction with comma delimiter and header row
CMD: WORK:paste -d, WORK:test-paste-names.txt WORK:test-paste-ages.txt
EXPECT: name,age
EXPECT_LINE: 2,john,25
EXPECT_LINE: 4,bob,45
EXPECT_RC: 0

# Real-world: three-file cyclic delimiter - later lines show padding behavior
# Native: paste -d '|:' test-paste-a.txt test-paste-b.txt test-paste-c.txt | sed -n '4p'
TEST: Real-world cyclic delimiters with padding when files have unequal lengths
CMD: WORK:paste -d|: WORK:test-paste-a.txt WORK:test-paste-b.txt WORK:test-paste-c.txt
EXPECT: alpha|one:apple
EXPECT_LINE: 4,|:cherry
EXPECT_LINE: 5,|:mango
EXPECT_RC: 0

# Stress: 20-line parallel paste verifies getline free-list does not exhaust
# Native: paste test-paste-nums.txt test-paste-alpha.txt | head -1
TEST: Stress parallel paste of 20-line files processes all lines correctly
CMD: WORK:paste WORK:test-paste-nums.txt WORK:test-paste-alpha.txt
EXPECT: 1	a
EXPECT_LINE: 20,20	t
EXPECT_RC: 0

# Stress: sequential mode on 20-line file - verifies cyclic delimiter counter resets
# Native: paste -s test-paste-nums.txt | cut -f1
TEST: Stress sequential mode on 20-line file produces single output line
CMD: WORK:paste -s WORK:test-paste-nums.txt
EXPECT: 1	2	3	4	5	6	7	8	9	10	11	12	13	14	15	16	17	18	19	20
EXPECT_RC: 0

# Stress: three 20-line files in parallel
# Native: paste test-paste-nums.txt test-paste-alpha.txt test-paste-nums.txt | head -1
TEST: Stress three-file parallel paste of 20-line files
CMD: WORK:paste WORK:test-paste-nums.txt WORK:test-paste-alpha.txt WORK:test-paste-nums.txt
EXPECT: 1	a	1
EXPECT_LINE: 20,20	t	20
EXPECT_RC: 0

# Precision: -d backslash-n in sequential mode uses newline as delimiter
# Each field goes on its own line (newline replaces between-field separator)
# Native: paste -d '\n' -s test-paste-a.txt
TEST: -d backslash-n in sequential mode puts each field on its own line
CMD: WORK:paste -d\n -s WORK:test-paste-a.txt
EXPECT: alpha
EXPECT_LINE: 2,beta
EXPECT_LINE: 3,gamma
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
passed=24
failed=0
total=24
```

---
Generated by `make test-fsemu TARGET=ports/paste`
Report template: `toolchain/templates/test-report.md.template`
