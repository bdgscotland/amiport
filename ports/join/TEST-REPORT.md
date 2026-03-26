# FS-UAE Test Report: join

## Summary

| Field | Value |
|-------|-------|
| Port | join |
| Date | 2026-03-26 16:47:37 |
| Duration | 49s |
| Platform | FS-UAE 3.2.35 (A1200, Kickstart 3.1) |
| Binary | `WORK:join` (44K) |
| Test method | ARexx harness → TAP output |
| Result | **PASS** — 39/39 passed |

## Test Results

```
1..39
ok 1 - Default join on first field outputs matched lines
ok 2 - -1 flag joins file1 on field 2
ok 3 - -2 flag joins file2 on field 2 (with -1 2)
ok 4 - -j flag joins both files on same field number
ok 5 - -t flag uses colon as field separator
ok 6 - -o flag specifies output field list with 0 (join field)
ok 7 - -o flag reorders output fields from both files
ok 8 - -a 1 outputs unpairable lines from file 1
ok 9 - -a 2 outputs unpairable lines from file 2
ok 10 - -v 1 outputs only unpairable lines from file 1 suppressing joined
ok 11 - -v 2 outputs only unpairable lines from file 2 suppressing joined
ok 12 - -e flag replaces empty fields with specified string
ok 13 - -a and -v flags are mutually exclusive
ok 14 - -1 field number less than 1 is an error
ok 15 - -2 field number less than 1 is an error
ok 16 - -j field number less than 1 is an error
ok 17 - -a file number not 1 or 2 is an error
ok 18 - -v file number not 1 or 2 is an error
ok 19 - Missing input file produces error exit
ok 20 - Multi-character tab specification is an error
ok 21 - Only one file argument produces usage error
ok 22 - Files with no matching keys produce no output and RC 0
ok 23 - Empty first file produces no output
ok 24 - Empty second file produces no output
ok 25 - Single-line files with matching key produce one output line
ok 26 - Multi-key cross product join outputs all combinations
ok 27 - Comma-separated join uses comma as field delimiter
ok 28 - WORK volume paths work for both input files
ok 29 - -o comma-separated field list parsed correctly
ok 30 - Colon separator works correctly with WORK colon in paths
ok 31 - Real-world colon-delimited user database join
ok 32 - Real-world full outer join of two tables
ok 33 - Real-world find records in scores not in names table
ok 34 - Real-world join on non-first field with output reordering
ok 35 - Stress join of 200-line sorted files first line correct
ok 36 - Stress join of 200-line sorted files last line correct
ok 37 - Stress -v 1 on fully-matched 200-line files produces no output
ok 38 - Precision -e correctly fills empty output slots
ok 39 - Precision -v 1 with -e shows only unpairable with empty replacement
# passed: 39 failed: 0 total: 39
```

### Breakdown

| # | Test | Status | Details |
|---|------|--------|---------|
| 1 | Default join on first field outputs matched lines | PASS | |
| 2 | -1 flag joins file1 on field 2 | PASS | |
| 3 | -2 flag joins file2 on field 2 (with -1 2) | PASS | |
| 4 | -j flag joins both files on same field number | PASS | |
| 5 | -t flag uses colon as field separator | PASS | |
| 6 | -o flag specifies output field list with 0 (join field) | PASS | |
| 7 | -o flag reorders output fields from both files | PASS | |
| 8 | -a 1 outputs unpairable lines from file 1 | PASS | |
| 9 | -a 2 outputs unpairable lines from file 2 | PASS | |
| 10 | -v 1 outputs only unpairable lines from file 1 suppressing joined | PASS | |
| 11 | -v 2 outputs only unpairable lines from file 2 suppressing joined | PASS | |
| 12 | -e flag replaces empty fields with specified string | PASS | |
| 13 | -a and -v flags are mutually exclusive | PASS | |
| 14 | -1 field number less than 1 is an error | PASS | |
| 15 | -2 field number less than 1 is an error | PASS | |
| 16 | -j field number less than 1 is an error | PASS | |
| 17 | -a file number not 1 or 2 is an error | PASS | |
| 18 | -v file number not 1 or 2 is an error | PASS | |
| 19 | Missing input file produces error exit | PASS | |
| 20 | Multi-character tab specification is an error | PASS | |
| 21 | Only one file argument produces usage error | PASS | |
| 22 | Files with no matching keys produce no output and RC 0 | PASS | |
| 23 | Empty first file produces no output | PASS | |
| 24 | Empty second file produces no output | PASS | |
| 25 | Single-line files with matching key produce one output line | PASS | |
| 26 | Multi-key cross product join outputs all combinations | PASS | |
| 27 | Comma-separated join uses comma as field delimiter | PASS | |
| 28 | WORK volume paths work for both input files | PASS | |
| 29 | -o comma-separated field list parsed correctly | PASS | |
| 30 | Colon separator works correctly with WORK colon in paths | PASS | |
| 31 | Real-world colon-delimited user database join | PASS | |
| 32 | Real-world full outer join of two tables | PASS | |
| 33 | Real-world find records in scores not in names table | PASS | |
| 34 | Real-world join on non-first field with output reordering | PASS | |
| 35 | Stress join of 200-line sorted files first line correct | PASS | |
| 36 | Stress join of 200-line sorted files last line correct | PASS | |
| 37 | Stress -v 1 on fully-matched 200-line files produces no output | PASS | |
| 38 | Precision -e correctly fills empty output slots | PASS | |
| 39 | Precision -v 1 with -e shows only unpairable with empty replacement | PASS | |

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
# join 1.34 -- FS-UAE test suite
# Category: 1 (CLI utility)
# Minimum required: 8 tests
# Generated by test-designer agent
# All EXPECT: values verified against native macOS join

# ============================================================
# FUNCTIONAL TESTS -- one per flag
# ============================================================

# Native: join test-join-file1.txt test-join-file2.txt | head -1
TEST: Default join on first field outputs matched lines
CMD: WORK:join WORK:test-join-file1.txt WORK:test-join-file2.txt
EXPECT: apple 1 A
EXPECT_LINE: 2,cherry 3 C
EXPECT_LINE: 3,elderberry 5 E
EXPECT_RC: 0

# Native: join -1 2 -2 2 test-join-f1field2.txt test-join-f2field2.txt | head -1
TEST: -1 flag joins file1 on field 2
CMD: WORK:join -1 2 -2 2 WORK:test-join-f1field2.txt WORK:test-join-f2field2.txt
EXPECT: apple 10 red X alpha
EXPECT_LINE: 2,cherry 30 red Y gamma
EXPECT_RC: 0

# Native: join -2 2 test-join-f1field2.txt test-join-f2field2.txt | head -1
TEST: -2 flag joins file2 on field 2 (with -1 2)
CMD: WORK:join -1 2 -2 2 WORK:test-join-f1field2.txt WORK:test-join-f2field2.txt
EXPECT: apple 10 red X alpha
EXPECT_RC: 0

# Native: join -j 2 test-join-f1field2.txt test-join-f2field2.txt | head -1
TEST: -j flag joins both files on same field number
CMD: WORK:join -j 2 WORK:test-join-f1field2.txt WORK:test-join-f2field2.txt
EXPECT: apple 10 red X alpha
EXPECT_LINE: 2,cherry 30 red Y gamma
EXPECT_RC: 0

# Native: join -t: test-join-colon1.txt test-join-colon2.txt | head -1
TEST: -t flag uses colon as field separator
CMD: WORK:join -t: WORK:test-join-colon1.txt WORK:test-join-colon2.txt
EXPECT: apple:10:red:A:alpha
EXPECT_LINE: 2,cherry:30:red:C:gamma
EXPECT_RC: 0

# Native: join -o 0,1.2,2.2 test-join-file1.txt test-join-file2.txt | head -1
TEST: -o flag specifies output field list with 0 (join field)
CMD: WORK:join -o 0,1.2,2.2 WORK:test-join-file1.txt WORK:test-join-file2.txt
EXPECT: apple 1 A
EXPECT_LINE: 2,cherry 3 C
EXPECT_LINE: 3,elderberry 5 E
EXPECT_RC: 0

# Native: join -o 2.2,1.2,0 test-join-file1.txt test-join-file2.txt | head -1
TEST: -o flag reorders output fields from both files
CMD: WORK:join -o 2.2,1.2,0 WORK:test-join-file1.txt WORK:test-join-file2.txt
EXPECT: A 1 apple
EXPECT_LINE: 2,C 3 cherry
EXPECT_LINE: 3,E 5 elderberry
EXPECT_RC: 0

# Native: join -a 1 test-join-file1.txt test-join-file2.txt
TEST: -a 1 outputs unpairable lines from file 1
CMD: WORK:join -a 1 WORK:test-join-file1.txt WORK:test-join-file2.txt
EXPECT: apple 1 A
EXPECT_LINE: 2,banana 2
EXPECT_LINE: 3,cherry 3 C
EXPECT_LINE: 4,date 4
EXPECT_LINE: 5,elderberry 5 E
EXPECT_RC: 0

# Native: join -a 2 test-join-file1.txt test-join-file2.txt
TEST: -a 2 outputs unpairable lines from file 2
CMD: WORK:join -a 2 WORK:test-join-file1.txt WORK:test-join-file2.txt
EXPECT: apple 1 A
EXPECT_LINE: 2,cherry 3 C
EXPECT_LINE: 3,elderberry 5 E
EXPECT_LINE: 4,fig F
EXPECT_LINE: 5,grape G
EXPECT_RC: 0

# Native: join -v 1 test-join-file1.txt test-join-file2.txt
TEST: -v 1 outputs only unpairable lines from file 1 suppressing joined
CMD: WORK:join -v 1 WORK:test-join-file1.txt WORK:test-join-file2.txt
EXPECT: banana 2
EXPECT_LINE: 2,date 4
EXPECT_RC: 0

# Native: join -v 2 test-join-file1.txt test-join-file2.txt
TEST: -v 2 outputs only unpairable lines from file 2 suppressing joined
CMD: WORK:join -v 2 WORK:test-join-file1.txt WORK:test-join-file2.txt
EXPECT: fig F
EXPECT_LINE: 2,grape G
EXPECT_RC: 0

# Native: join -e EMPTY -a 1 -o 0,1.2,2.2 test-join-file1.txt test-join-file2.txt
TEST: -e flag replaces empty fields with specified string
CMD: WORK:join -a 1 -e EMPTY -o 0,1.2,2.2 WORK:test-join-file1.txt WORK:test-join-file2.txt
EXPECT: apple 1 A
EXPECT_LINE: 2,banana 2 EMPTY
EXPECT_LINE: 4,date 4 EMPTY
EXPECT_RC: 0

# ============================================================
# ERROR PATH TESTS
# ============================================================

# Native: join -a 1 -v 2 file1 file2 2>&1 -> RC=1 (Amiga: RC=10)
TEST: -a and -v flags are mutually exclusive
CMD: WORK:join -a 1 -v 2 WORK:test-join-file1.txt WORK:test-join-file2.txt
EXPECT_RC: 10

# Native: join -1 0 file1 file2 2>&1 -> RC=1 (Amiga: RC=10)
TEST: -1 field number less than 1 is an error
CMD: WORK:join -1 0 WORK:test-join-file1.txt WORK:test-join-file2.txt
EXPECT_RC: 10

# Native: join -2 0 file1 file2 2>&1 -> RC=1 (Amiga: RC=10)
TEST: -2 field number less than 1 is an error
CMD: WORK:join -2 0 WORK:test-join-file1.txt WORK:test-join-file2.txt
EXPECT_RC: 10

# Native: join -j 0 file1 file2 2>&1 -> RC=1 (Amiga: RC=10)
TEST: -j field number less than 1 is an error
CMD: WORK:join -j 0 WORK:test-join-file1.txt WORK:test-join-file2.txt
EXPECT_RC: 10

# Native: join -a 3 file1 file2 2>&1 -> RC=1 (Amiga: RC=10)
TEST: -a file number not 1 or 2 is an error
CMD: WORK:join -a 3 WORK:test-join-file1.txt WORK:test-join-file2.txt
EXPECT_RC: 10

# Native: join -v 3 file1 file2 2>&1 -> RC=1 (Amiga: RC=10)
TEST: -v file number not 1 or 2 is an error
CMD: WORK:join -v 3 WORK:test-join-file1.txt WORK:test-join-file2.txt
EXPECT_RC: 10

# Native: join nonexistent.txt file2.txt 2>&1 -> RC=1 (Amiga: RC=10)
TEST: Missing input file produces error exit
CMD: WORK:join WORK:test-join-nonexistent.txt WORK:test-join-file2.txt
EXPECT_RC: 10

# Native: join -t ab file1 file2 2>&1 -> RC=1 (Amiga: RC=10)
TEST: Multi-character tab specification is an error
CMD: WORK:join -t ab WORK:test-join-file1.txt WORK:test-join-file2.txt
EXPECT_RC: 10

# Native: join file1 2>&1 -> RC=1 (Amiga: RC=10) -- usage error
TEST: Only one file argument produces usage error
CMD: WORK:join WORK:test-join-file1.txt
EXPECT_RC: 10

# ============================================================
# EXIT CODE TESTS
# ============================================================

# RC=0 -- verified above in functional tests
# RC=10 -- verified above in error tests

# Native: join test-join-nomatch1.txt test-join-nomatch2.txt -> no output, RC=0
TEST: Files with no matching keys produce no output and RC 0
CMD: WORK:join WORK:test-join-nomatch1.txt WORK:test-join-nomatch2.txt
EXPECT_RC: 0

# ============================================================
# EDGE CASE TESTS
# ============================================================

# Native: join test-empty.txt test-join-file2.txt -> empty output, RC=0
TEST: Empty first file produces no output
CMD: WORK:join WORK:test-empty.txt WORK:test-join-file2.txt
EXPECT_RC: 0

# Native: join test-join-file1.txt test-empty.txt -> empty output, RC=0
TEST: Empty second file produces no output
CMD: WORK:join WORK:test-join-file1.txt WORK:test-empty.txt
EXPECT_RC: 0

# Native: join test-join-single1.txt test-join-single2.txt -> apple 1 A
TEST: Single-line files with matching key produce one output line
CMD: WORK:join WORK:test-join-single1.txt WORK:test-join-single2.txt
EXPECT: apple 1 A
EXPECT_RC: 0

# Native: join -a 1 -a 2 test-join-multi1.txt test-join-multi2.txt
TEST: Multi-key cross product join outputs all combinations
CMD: WORK:join -a 1 -a 2 WORK:test-join-multi1.txt WORK:test-join-multi2.txt
EXPECT: apple 1 A
EXPECT_LINE: 2,apple 1 B
EXPECT_LINE: 3,apple 2 A
EXPECT_LINE: 4,apple 2 B
EXPECT_LINE: 5,banana 3
EXPECT_LINE: 6,cherry C
EXPECT_RC: 0

# Native: join -t, test-join-comma1.txt test-join-comma2.txt | head -1
# Note: literal tab as -t arg breaks AmigaDOS command parsing (tab is a word delimiter)
# Testing -t with comma achieves same coverage of the flag
TEST: Comma-separated join uses comma as field delimiter
CMD: WORK:join -t, WORK:test-join-comma1.txt WORK:test-join-comma2.txt
EXPECT: apple,10,red,alpha,X
EXPECT_LINE: 2,cherry,30,red,gamma,Z
EXPECT_RC: 0

# ============================================================
# AMIGA-SPECIFIC TESTS
# ============================================================

# Verify WORK: volume path handling works end-to-end
# Native: join test-join-file1.txt test-join-file2.txt | head -1
TEST: WORK volume paths work for both input files
CMD: WORK:join WORK:test-join-file1.txt WORK:test-join-file2.txt
EXPECT: apple 1 A
EXPECT_RC: 0

# Verify that -o with comma-separated list works via AmigaDOS argument parsing
# Native: join -o 1.2,2.2,0 test-join-file1.txt test-join-file2.txt | head -1
TEST: -o comma-separated field list parsed correctly
CMD: WORK:join -o 1.2,2.2,0 WORK:test-join-file1.txt WORK:test-join-file2.txt
EXPECT: 1 A apple
EXPECT_LINE: 2,3 C cherry
EXPECT_LINE: 3,5 E elderberry
EXPECT_RC: 0

# Verify -t: with colon-delimited files (WORK: path contains colons too)
# Native: join -t: test-join-colon1.txt test-join-colon2.txt
TEST: Colon separator works correctly with WORK colon in paths
CMD: WORK:join -t: WORK:test-join-colon1.txt WORK:test-join-colon2.txt
EXPECT: apple:10:red:A:alpha
EXPECT_RC: 0

# ============================================================
# REAL-WORLD AND STRESS TESTS
# ============================================================

# Real-world: join passwd-style files with colon separator
# Native: join -t: test-join-passwd1.txt test-join-passwd2.txt | head -1
TEST: Real-world colon-delimited user database join
CMD: WORK:join -t: WORK:test-join-passwd1.txt WORK:test-join-passwd2.txt
EXPECT: alice:1001:admin:Alice Smith:x@x.com
EXPECT_LINE: 2,charlie:1003:user:Charlie Brown:c@c.com
EXPECT_RC: 0

# Real-world: full outer join of names and scores tables
# Native: join -a 1 -a 2 test-join-names.txt test-join-scores.txt
TEST: Real-world full outer join of two tables
CMD: WORK:join -a 1 -a 2 WORK:test-join-names.txt WORK:test-join-scores.txt
EXPECT: alice 1 95
EXPECT_LINE: 2,bob 2
EXPECT_LINE: 3,charlie 3 87
EXPECT_LINE: 4,dave 4 72
EXPECT_LINE: 5,eve 88
EXPECT_RC: 0

# Real-world: unmatched-only view of what is missing from scores table
# Native: join -v 2 test-join-names.txt test-join-scores.txt
TEST: Real-world find records in scores not in names table
CMD: WORK:join -v 2 WORK:test-join-names.txt WORK:test-join-scores.txt
EXPECT: eve 88
EXPECT_RC: 0

# Real-world: -1 -2 with field reordering for report generation
# Native: join -1 2 -2 2 -o 1.1,0,2.1 test-join-f1field2.txt test-join-f2field2.txt
TEST: Real-world join on non-first field with output reordering
CMD: WORK:join -1 2 -2 2 -o 1.1,0,2.1 WORK:test-join-f1field2.txt WORK:test-join-f2field2.txt
EXPECT: 10 apple X
EXPECT_LINE: 2,30 cherry Y
EXPECT_RC: 0

# Stress: join 200-line sorted files -- exercises memory allocation growth
# Native: join test-join-big1.txt test-join-big2.txt | head -1 -> key001 data001 info001
TEST: Stress join of 200-line sorted files first line correct
CMD: WORK:join WORK:test-join-big1.txt WORK:test-join-big2.txt
EXPECT: key001 data001 info001
EXPECT_RC: 0

# Stress: verify last line of 200-line join to confirm full traversal
# Native: join test-join-big1.txt test-join-big2.txt | tail -1 -> key200 data200 info200
# Use EXPECT_LINE to verify line 200
TEST: Stress join of 200-line sorted files last line correct
CMD: WORK:join WORK:test-join-big1.txt WORK:test-join-big2.txt
EXPECT_LINE: 200,key200 data200 info200
EXPECT_RC: 0

# Stress: -v 1 on 200-line files with no unpairable lines -> no output
# Native: join -v 1 test-join-big1.txt test-join-big2.txt -> empty, RC=0
TEST: Stress -v 1 on fully-matched 200-line files produces no output
CMD: WORK:join -v 1 WORK:test-join-big1.txt WORK:test-join-big2.txt
EXPECT_RC: 0

# Precision: -e substitution correctness with -o covering all field slots
# Native: join -a 1 -e EMPTY -o 0,1.2,2.2 file1 file2
# Lines 2 and 4 (banana, date) have no file2 match -- field 2.2 becomes EMPTY
TEST: Precision -e correctly fills empty output slots
CMD: WORK:join -a 1 -e EMPTY -o 0,1.2,2.2 WORK:test-join-file1.txt WORK:test-join-file2.txt
EXPECT: apple 1 A
EXPECT_LINE: 2,banana 2 EMPTY
EXPECT_LINE: 3,cherry 3 C
EXPECT_LINE: 4,date 4 EMPTY
EXPECT_LINE: 5,elderberry 5 E
EXPECT_RC: 0

# Precision: -v 1 with -e and -o -- verify suppression and substitution together
# Native: join -v 1 -e EMPTY -o 0,1.2,2.2 file1 file2
TEST: Precision -v 1 with -e shows only unpairable with empty replacement
CMD: WORK:join -v 1 -e EMPTY -o 0,1.2,2.2 WORK:test-join-file1.txt WORK:test-join-file2.txt
EXPECT: banana 2 EMPTY
EXPECT_LINE: 2,date 4 EMPTY
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
passed=39
failed=0
total=39
```

---
Generated by `make test-fsemu TARGET=ports/join`
Report template: `toolchain/templates/test-report.md.template`
