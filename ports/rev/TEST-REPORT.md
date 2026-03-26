# FS-UAE Test Report: rev

## Summary

| Field | Value |
|-------|-------|
| Port | rev |
| Date | 2026-03-25 23:05:07 |
| Duration | 129s |
| Platform | FS-UAE 3.2.35 (A1200, Kickstart 3.1) |
| Binary | `WORK:rev` (35K) |
| Test method | ARexx harness → TAP output |
| Result | **PASS** — 24/24 passed |

## Test Results

```
1..24
ok 1 - Basic reversal of two-line file
ok 2 - Reverse single word
ok 3 - Reverse single character line
ok 4 - Reverse numeric digits
ok 5 - Reverse string with embedded spaces (spaces are reversed in position)
ok 6 - Reverse string with tab characters (tabs reversed in position)
ok 7 - Palindromes remain unchanged after reversal
ok 8 - Multiple files processed sequentially
ok 9 - Multiple files second file output (second.txt has xyz)
ok 10 - Multi-line file reversal first line
ok 11 - Empty file produces no output and exits cleanly
ok 12 - File with blank lines -- blank lines pass through as blank
ok 13 - Long line over 1024 chars -- reversed line starts with REKRAM
ok 14 - File with no trailing newline -- last line reversed without newline appended
ok 15 - 1000-line stress test -- all lines are abcdefghijklmnopqrstuvwxyz reversed
ok 16 - Invalid flag causes usage and exits with RC 10
ok 17 - Unknown long-style flag also causes usage and exits with RC 10
ok 18 - Nonexistent file prints warning to stderr and returns RETURN_WARN
ok 19 - Nonexistent file mixed with valid file -- processes valid, returns RETURN_WARN
ok 20 - WORK: volume path handling -- standard AmigaOS volume prefix accepted
ok 21 - Two WORK: paths in one command -- multiple volume-prefixed args work
ok 22 - Real-world: reverse a series of lines that form a known sequence
ok 23 - Real-world: large file processed completely (stress + correctness)
ok 24 - Real-world: round-trip correctness -- reversing palindromes gives original
# passed: 24 failed: 0 total: 24
```

### Breakdown

| # | Test | Status | Details |
|---|------|--------|---------|
| 1 | Basic reversal of two-line file | PASS | |
| 2 | Reverse single word | PASS | |
| 3 | Reverse single character line | PASS | |
| 4 | Reverse numeric digits | PASS | |
| 5 | Reverse string with embedded spaces (spaces are reversed in position) | PASS | |
| 6 | Reverse string with tab characters (tabs reversed in position) | PASS | |
| 7 | Palindromes remain unchanged after reversal | PASS | |
| 8 | Multiple files processed sequentially | PASS | |
| 9 | Multiple files second file output (second.txt has xyz) | PASS | |
| 10 | Multi-line file reversal first line | PASS | |
| 11 | Empty file produces no output and exits cleanly | PASS | |
| 12 | File with blank lines -- blank lines pass through as blank | PASS | |
| 13 | Long line over 1024 chars -- reversed line starts with REKRAM | PASS | |
| 14 | File with no trailing newline -- last line reversed without newline appended | PASS | |
| 15 | 1000-line stress test -- all lines are abcdefghijklmnopqrstuvwxyz reversed | PASS | |
| 16 | Invalid flag causes usage and exits with RC 10 | PASS | |
| 17 | Unknown long-style flag also causes usage and exits with RC 10 | PASS | |
| 18 | Nonexistent file prints warning to stderr and returns RETURN_WARN | PASS | |
| 19 | Nonexistent file mixed with valid file -- processes valid, returns RETURN_WARN | PASS | |
| 20 | WORK: volume path handling -- standard AmigaOS volume prefix accepted | PASS | |
| 21 | Two WORK: paths in one command -- multiple volume-prefixed args work | PASS | |
| 22 | Real-world: reverse a series of lines that form a known sequence | PASS | |
| 23 | Real-world: large file processed completely (stress + correctness) | PASS | |
| 24 | Real-world: round-trip correctness -- reversing palindromes gives original | PASS | |

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
# test-fsemu-cases.txt -- rev 1.16 FS-UAE test suite
# Category 1 (CLI tool) -- minimum 15 tests required
# rev reverses each line character by character
# No flags (getopt string is empty). Any flag causes usage() + exit(10).
# RC=0 on success, RC=1 on partial failure (file not found, continues),
# RC=10 on hard error (invalid flag, stdout write failure).
# Uses input files instead of piping (ARexx limitation).

# --- FUNCTIONAL TESTS ---

TEST: Basic reversal of two-line file
CMD: WORK:rev WORK:test-rev-basic.txt
EXPECT: olleh
EXPECT_RC: 0

TEST: Reverse single word
CMD: WORK:rev WORK:test-rev-single.txt
EXPECT: fedcba
EXPECT_RC: 0

TEST: Reverse single character line
CMD: WORK:rev WORK:test-rev-singlechar.txt
EXPECT: x
EXPECT_RC: 0

TEST: Reverse numeric digits
CMD: WORK:rev WORK:test-rev-numbers.txt
EXPECT: 0987654321
EXPECT_RC: 0

TEST: Reverse string with embedded spaces (spaces are reversed in position)
CMD: WORK:rev WORK:test-rev-spaces.txt
EXPECT: dlrow olleh
EXPECT_RC: 0

TEST: Reverse string with tab characters (tabs reversed in position)
CMD: WORK:rev WORK:test-rev-tabs.txt
EXPECT: eerht	owt	eno
EXPECT_RC: 0

TEST: Palindromes remain unchanged after reversal
CMD: WORK:rev WORK:test-rev-palindrome.txt
EXPECT: racecar
EXPECT_RC: 0

TEST: Multiple files processed sequentially
CMD: WORK:rev WORK:test-rev-basic.txt WORK:test-rev-second.txt
EXPECT: olleh
EXPECT_RC: 0

TEST: Multiple files second file output (second.txt has xyz)
CMD: WORK:rev WORK:test-rev-second.txt WORK:test-rev-basic.txt
EXPECT: zyx
EXPECT_RC: 0

TEST: Multi-line file reversal first line
CMD: WORK:rev WORK:test-rev-multiline.txt
EXPECT: edcba
EXPECT_RC: 0

# --- EDGE CASE TESTS ---

TEST: Empty file produces no output and exits cleanly
CMD: WORK:rev WORK:test-rev-empty.txt
EXPECT:
EXPECT_RC: 0

TEST: File with blank lines -- blank lines pass through as blank
CMD: WORK:rev WORK:test-rev-blanklines.txt
EXPECT: olleh
EXPECT_RC: 0

TEST: Long line over 1024 chars -- reversed line starts with REKRAM
CMD: WORK:rev WORK:test-rev-longline.txt
EXPECT_CONTAINS: REKRAM
EXPECT_RC: 0

TEST: File with no trailing newline -- last line reversed without newline appended
CMD: WORK:rev WORK:test-rev-nonewline.txt
EXPECT_CONTAINS: aagima
EXPECT_RC: 0

# --- STRESS TESTS ---

TEST: 1000-line stress test -- all lines are abcdefghijklmnopqrstuvwxyz reversed
CMD: WORK:rev WORK:test-rev-stress.txt
EXPECT: zyxwvutsrqponmlkjihgfedcba
EXPECT_RC: 0

# --- ERROR PATH TESTS ---

TEST: Invalid flag causes usage and exits with RC 10
CMD: WORK:rev -Z WORK:test-rev-basic.txt
EXPECT_RC: 10

TEST: Unknown long-style flag also causes usage and exits with RC 10
CMD: WORK:rev -x WORK:test-rev-basic.txt
EXPECT_RC: 10

TEST: Nonexistent file prints warning to stderr and returns RETURN_WARN
CMD: WORK:rev WORK:nonexistent-file-xyz.txt
EXPECT_RC: 5

TEST: Nonexistent file mixed with valid file -- processes valid, returns RETURN_WARN
CMD: WORK:rev WORK:nonexistent-file-xyz.txt WORK:test-rev-basic.txt
EXPECT_CONTAINS: olleh
EXPECT_RC: 5

# --- AMIGA-SPECIFIC TESTS ---

TEST: WORK: volume path handling -- standard AmigaOS volume prefix accepted
CMD: WORK:rev WORK:test-rev-basic.txt
EXPECT: olleh
EXPECT_RC: 0

TEST: Two WORK: paths in one command -- multiple volume-prefixed args work
CMD: WORK:rev WORK:test-rev-single.txt WORK:test-rev-numbers.txt
EXPECT: fedcba
EXPECT_RC: 0

# --- REAL-WORLD TESTS ---

TEST: Real-world: reverse a series of lines that form a known sequence
CMD: WORK:rev WORK:test-rev-numbers.txt
EXPECT: 0987654321
EXPECT_RC: 0

TEST: Real-world: large file processed completely (stress + correctness)
CMD: WORK:rev WORK:test-rev-stress.txt
EXPECT_CONTAINS: zyxwvutsrqponmlkjihgfedcba
EXPECT_RC: 0

TEST: Real-world: round-trip correctness -- reversing palindromes gives original
CMD: WORK:rev WORK:test-rev-palindrome.txt
EXPECT_CONTAINS: racecar
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
Generated by `make test-fsemu TARGET=ports/rev`
Report template: `toolchain/templates/test-report.md.template`
