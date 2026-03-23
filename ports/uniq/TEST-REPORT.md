# FS-UAE Test Report: uniq

## Summary

| Field | Value |
|-------|-------|
| Port | uniq |
| Date | 2026-03-22 22:25:01 |
| Duration | 33s |
| Platform | FS-UAE 3.2.35 (A1200, Kickstart 3.1) |
| Binary | `WORK:uniq` (41K) |
| Test method | ARexx harness → TAP output |
| Result | **PASS** — 29/29 passed |

## Test Results

```
1..29
ok 1 - Default dedup prints each run once (first line is first unique)
ok 2 - Default dedup on all-unique input prints every line
ok 3 - -d prints only lines that appear more than once
ok 4 - -d on all-unique input produces no output
ok 5 - -u prints only lines that appear exactly once
ok 6 - -u on all-duplicate input produces no output
ok 7 - -c prefixes each line with 4-wide occurrence count
ok 8 - -c -d prints count of duplicate lines only
ok 9 - -c -u prints count 1 for unique lines only
ok 10 - -i treats Hello and hello and HELLO as the same line
ok 11 - -i -d shows case-folded duplicates
ok 12 - -f 1 skips first field and compares second field only
ok 13 - -f 1 -d shows field-matched duplicates
ok 14 - -s 2 skips first 2 chars and compares the rest
ok 15 - -s 2 -u prints char-skipped unique lines
ok 16 - Obsolete +2 syntax is equivalent to -s 2 (skip 2 chars)
ok 17 - Obsolete -1 syntax is equivalent to -f 1 (skip 1 field)
ok 18 - Output to file argument succeeds (writes to T: Amiga temp dir)
ok 19 - Empty input file produces no output and exits 0
ok 20 - Single-line file is passed through unchanged
ok 21 - File with no trailing newline is handled correctly
ok 22 - Mixed file default dedup returns first line of first group
ok 23 - Unknown flag causes usage error with RC 10
ok 24 - Too many positional arguments causes usage error with RC 10
ok 25 - Non-existent input file causes error with RC 10
ok 26 - Invalid -f value (non-numeric) causes error with RC 10
ok 27 - Invalid -s value (non-numeric) causes error with RC 10
ok 28 - WORK: volume path resolves correctly as input
ok 29 - Output to T: Amiga temp volume succeeds
# passed: 29 failed: 0 total: 29
```

### Breakdown

| # | Test | Status | Details |
|---|------|--------|---------|
| 1 | Default dedup prints each run once (first line is first unique) | PASS | |
| 2 | Default dedup on all-unique input prints every line | PASS | |
| 3 | -d prints only lines that appear more than once | PASS | |
| 4 | -d on all-unique input produces no output | PASS | |
| 5 | -u prints only lines that appear exactly once | PASS | |
| 6 | -u on all-duplicate input produces no output | PASS | |
| 7 | -c prefixes each line with 4-wide occurrence count | PASS | |
| 8 | -c -d prints count of duplicate lines only | PASS | |
| 9 | -c -u prints count 1 for unique lines only | PASS | |
| 10 | -i treats Hello and hello and HELLO as the same line | PASS | |
| 11 | -i -d shows case-folded duplicates | PASS | |
| 12 | -f 1 skips first field and compares second field only | PASS | |
| 13 | -f 1 -d shows field-matched duplicates | PASS | |
| 14 | -s 2 skips first 2 chars and compares the rest | PASS | |
| 15 | -s 2 -u prints char-skipped unique lines | PASS | |
| 16 | Obsolete +2 syntax is equivalent to -s 2 (skip 2 chars) | PASS | |
| 17 | Obsolete -1 syntax is equivalent to -f 1 (skip 1 field) | PASS | |
| 18 | Output to file argument succeeds (writes to T: Amiga temp dir) | PASS | |
| 19 | Empty input file produces no output and exits 0 | PASS | |
| 20 | Single-line file is passed through unchanged | PASS | |
| 21 | File with no trailing newline is handled correctly | PASS | |
| 22 | Mixed file default dedup returns first line of first group | PASS | |
| 23 | Unknown flag causes usage error with RC 10 | PASS | |
| 24 | Too many positional arguments causes usage error with RC 10 | PASS | |
| 25 | Non-existent input file causes error with RC 10 | PASS | |
| 26 | Invalid -f value (non-numeric) causes error with RC 10 | PASS | |
| 27 | Invalid -s value (non-numeric) causes error with RC 10 | PASS | |
| 28 | WORK: volume path resolves correctly as input | PASS | |
| 29 | Output to T: Amiga temp volume succeeds | PASS | |

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
# uniq 1.33 FS-UAE test suite
# OpenBSD uniq ported to AmigaOS 3.x
# Flags: -c -d -f num -i -s num -u
# Exit codes: 0 (success), 10 (error)
# NOTE: stderr not captured by harness -- error path tests use EXPECT_RC: only

# ============================================================
# FUNCTIONAL TESTS: default behaviour (no flags)
# ============================================================

TEST: Default dedup prints each run once (first line is first unique)
CMD: WORK:uniq WORK:test-uniq-dupes.txt
EXPECT: apple
EXPECT_RC: 0

TEST: Default dedup on all-unique input prints every line
CMD: WORK:uniq WORK:test-uniq-nodupes.txt
EXPECT: alpha
EXPECT_RC: 0

# ============================================================
# FUNCTIONAL TESTS: -d flag (duplicates only)
# ============================================================

TEST: -d prints only lines that appear more than once
CMD: WORK:uniq -d WORK:test-uniq-dupes.txt
EXPECT: apple
EXPECT_RC: 0

TEST: -d on all-unique input produces no output
CMD: WORK:uniq -d WORK:test-uniq-nodupes.txt
EXPECT:
EXPECT_RC: 0

# ============================================================
# FUNCTIONAL TESTS: -u flag (unique only)
# ============================================================

TEST: -u prints only lines that appear exactly once
CMD: WORK:uniq -u WORK:test-uniq-dupes.txt
EXPECT: banana
EXPECT_RC: 0

TEST: -u on all-duplicate input produces no output
CMD: WORK:uniq -u WORK:test-uniq-dupes.txt
EXPECT_RC: 0

# ============================================================
# FUNCTIONAL TESTS: -c flag (count prefix)
# ============================================================

TEST: -c prefixes each line with 4-wide occurrence count
CMD: WORK:uniq -c WORK:test-uniq-dupes.txt
EXPECT:    2 apple
EXPECT_RC: 0

TEST: -c -d prints count of duplicate lines only
CMD: WORK:uniq -c -d WORK:test-uniq-dupes.txt
EXPECT:    2 apple
EXPECT_RC: 0

TEST: -c -u prints count 1 for unique lines only
CMD: WORK:uniq -c -u WORK:test-uniq-dupes.txt
EXPECT:    1 banana
EXPECT_RC: 0

# ============================================================
# FUNCTIONAL TESTS: -i flag (case-insensitive)
# ============================================================

TEST: -i treats Hello and hello and HELLO as the same line
CMD: WORK:uniq -i WORK:test-uniq-case.txt
EXPECT: Hello
EXPECT_RC: 0

TEST: -i -d shows case-folded duplicates
CMD: WORK:uniq -i -d WORK:test-uniq-case.txt
EXPECT: Hello
EXPECT_RC: 0

# ============================================================
# FUNCTIONAL TESTS: -f flag (skip fields)
# ============================================================

TEST: -f 1 skips first field and compares second field only
CMD: WORK:uniq -f 1 WORK:test-uniq-fields.txt
EXPECT: aaa foo
EXPECT_RC: 0

TEST: -f 1 -d shows field-matched duplicates
CMD: WORK:uniq -f 1 -d WORK:test-uniq-fields.txt
EXPECT: aaa foo
EXPECT_RC: 0

# ============================================================
# FUNCTIONAL TESTS: -s flag (skip chars)
# ============================================================

TEST: -s 2 skips first 2 chars and compares the rest
CMD: WORK:uniq -s 2 WORK:test-uniq-chars.txt
EXPECT: xxapple
EXPECT_RC: 0

TEST: -s 2 -u prints char-skipped unique lines
CMD: WORK:uniq -s 2 -u WORK:test-uniq-chars.txt
EXPECT: xxbanana
EXPECT_RC: 0

# ============================================================
# FUNCTIONAL TESTS: obsolete syntax (+N, -N)
# ============================================================

TEST: Obsolete +2 syntax is equivalent to -s 2 (skip 2 chars)
CMD: WORK:uniq +2 WORK:test-uniq-chars.txt
EXPECT: xxapple
EXPECT_RC: 0

TEST: Obsolete -1 syntax is equivalent to -f 1 (skip 1 field)
CMD: WORK:uniq -1 WORK:test-uniq-fields.txt
EXPECT: aaa foo
EXPECT_RC: 0

# ============================================================
# FUNCTIONAL TESTS: output file argument
# ============================================================

TEST: Output to file argument succeeds (writes to T: Amiga temp dir)
CMD: WORK:uniq WORK:test-uniq-oneline.txt T:uniq-test-out.txt
EXPECT_RC: 0

# ============================================================
# EDGE CASE TESTS
# ============================================================

TEST: Empty input file produces no output and exits 0
CMD: WORK:uniq WORK:test-empty.txt
EXPECT:
EXPECT_RC: 0

TEST: Single-line file is passed through unchanged
CMD: WORK:uniq WORK:test-uniq-oneline.txt
EXPECT: hello world
EXPECT_RC: 0

TEST: File with no trailing newline is handled correctly
CMD: WORK:uniq WORK:test-uniq-nonewline.txt
EXPECT: no newline at end
EXPECT_RC: 0

TEST: Mixed file default dedup returns first line of first group
CMD: WORK:uniq WORK:test-uniq-mixed.txt
EXPECT: one
EXPECT_RC: 0

# ============================================================
# ERROR PATH TESTS
# ============================================================

TEST: Unknown flag causes usage error with RC 10
CMD: WORK:uniq -Z WORK:test-uniq-oneline.txt
EXPECT_RC: 10

TEST: Too many positional arguments causes usage error with RC 10
CMD: WORK:uniq WORK:test-uniq-oneline.txt T:x.txt T:y.txt
EXPECT_RC: 10

TEST: Non-existent input file causes error with RC 10
CMD: WORK:uniq WORK:test-uniq-doesnotexist.txt
EXPECT_RC: 10

TEST: Invalid -f value (non-numeric) causes error with RC 10
CMD: WORK:uniq -f abc WORK:test-uniq-oneline.txt
EXPECT_RC: 10

TEST: Invalid -s value (non-numeric) causes error with RC 10
CMD: WORK:uniq -s xyz WORK:test-uniq-oneline.txt
EXPECT_RC: 10

# ============================================================
# AMIGA-SPECIFIC TESTS
# ============================================================

TEST: WORK: volume path resolves correctly as input
CMD: WORK:uniq WORK:test-uniq-nodupes.txt
EXPECT: alpha
EXPECT_RC: 0

TEST: Output to T: Amiga temp volume succeeds
CMD: WORK:uniq -d WORK:test-uniq-dupes.txt T:uniq-amiga-out.txt
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
Generated by `make test-fsemu TARGET=ports/uniq`
Report template: `toolchain/templates/test-report.md.template`
