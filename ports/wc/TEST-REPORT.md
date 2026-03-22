# FS-UAE Test Report: wc

## Summary

| Field | Value |
|-------|-------|
| Port | wc |
| Date | 2026-03-22 11:32:32 |
| Duration | 62s |
| Platform | FS-UAE 3.2.35 (A1200, Kickstart 3.1) |
| Binary | `WORK:wc` (39K) |
| Test method | ARexx harness → TAP output |
| Result | **PASS** — 16/16 passed |

## Test Results

```
1..16
ok 1 - default mode counts lines words and bytes
ok 2 - -l flag counts lines only
ok 3 - -w flag counts words only
ok 4 - -c flag counts bytes only
ok 5 - -m flag counts bytes (multibyte equivalent to -c on Amiga)
ok 6 - -h flag shows human-readable output (all columns use fmt_scaled)
ok 7 - multiple files produce per-file counts and total line
ok 8 - -l on multiline file counts ten lines
ok 9 - nonexistent file returns RC 10
ok 10 - invalid flag returns RC 10
ok 11 - empty file returns zero counts
ok 12 - file with no trailing newline counts zero lines
ok 13 - single-character file with newline counts one line one word two bytes
ok 14 - long line file byte count exceeds 1024
ok 15 - WORK volume path resolves correctly for count
ok 16 - multiple flag combination -lw produces two columns
# passed: 16 failed: 0 total: 16
```

### Breakdown

| # | Test | Status | Details |
|---|------|--------|---------|
| 1 | default mode counts lines words and bytes | PASS | |
| 2 | -l flag counts lines only | PASS | |
| 3 | -w flag counts words only | PASS | |
| 4 | -c flag counts bytes only | PASS | |
| 5 | -m flag counts bytes (multibyte equivalent to -c on Amiga) | PASS | |
| 6 | -h flag shows human-readable output (all columns use fmt_scaled) | PASS | |
| 7 | multiple files produce per-file counts and total line | PASS | |
| 8 | -l on multiline file counts ten lines | PASS | |
| 9 | nonexistent file returns RC 10 | PASS | |
| 10 | invalid flag returns RC 10 | PASS | |
| 11 | empty file returns zero counts | PASS | |
| 12 | file with no trailing newline counts zero lines | PASS | |
| 13 | single-character file with newline counts one line one word two bytes | PASS | |
| 14 | long line file byte count exceeds 1024 | PASS | |
| 15 | WORK volume path resolves correctly for count | PASS | |
| 16 | multiple flag combination -lw produces two columns | PASS | |

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
# wc test suite -- OpenBSD wc v1.32
# Tests cover: functional (per-flag), error paths, exit codes, edge cases, Amiga-specific

# --- Functional tests ---

TEST: default mode counts lines words and bytes
CMD: WORK:wc WORK:test-oneline.txt
EXPECT:        1       2      12 WORK:test-oneline.txt
EXPECT_RC: 0

TEST: -l flag counts lines only
CMD: WORK:wc -l WORK:test-oneline.txt
EXPECT:        1 WORK:test-oneline.txt
EXPECT_RC: 0

TEST: -w flag counts words only
CMD: WORK:wc -w WORK:test-oneline.txt
EXPECT:        2 WORK:test-oneline.txt
EXPECT_RC: 0

TEST: -c flag counts bytes only
CMD: WORK:wc -c WORK:test-oneline.txt
EXPECT:       12 WORK:test-oneline.txt
EXPECT_RC: 0

TEST: -m flag counts bytes (multibyte equivalent to -c on Amiga)
CMD: WORK:wc -m WORK:test-oneline.txt
EXPECT:       12 WORK:test-oneline.txt
EXPECT_RC: 0

TEST: -h flag shows human-readable output (all columns use fmt_scaled)
CMD: WORK:wc -h WORK:test-oneline.txt
EXPECT_CONTAINS: 12
EXPECT_RC: 0

TEST: multiple files produce per-file counts and total line
CMD: WORK:wc WORK:test-oneline.txt WORK:test-multiline.txt
EXPECT_CONTAINS: total
EXPECT_RC: 0

TEST: -l on multiline file counts ten lines
CMD: WORK:wc -l WORK:test-multiline.txt
EXPECT:       10 WORK:test-multiline.txt
EXPECT_RC: 0

# --- Error path tests ---

TEST: nonexistent file returns RC 10
CMD: WORK:wc WORK:no-such-file-exists.txt
EXPECT_RC: 10

TEST: invalid flag returns RC 10
CMD: WORK:wc -Z WORK:test-oneline.txt
EXPECT_RC: 10

# --- Edge case tests ---

TEST: empty file returns zero counts
CMD: WORK:wc WORK:test-empty.txt
EXPECT:        0       0       0 WORK:test-empty.txt
EXPECT_RC: 0

TEST: file with no trailing newline counts zero lines
CMD: WORK:wc -l WORK:test-wc-nonewline.txt
EXPECT:        0 WORK:test-wc-nonewline.txt
EXPECT_RC: 0

TEST: single-character file with newline counts one line one word two bytes
CMD: WORK:wc WORK:test-wc-single.txt
EXPECT:        1       1       2 WORK:test-wc-single.txt
EXPECT_RC: 0

TEST: long line file byte count exceeds 1024
CMD: WORK:wc -c WORK:test-longline.txt
EXPECT:     1107 WORK:test-longline.txt
EXPECT_RC: 0

# --- Amiga-specific tests ---

TEST: WORK volume path resolves correctly for count
CMD: WORK:wc -w WORK:test-multiline.txt
EXPECT:       21 WORK:test-multiline.txt
EXPECT_RC: 0

TEST: multiple flag combination -lw produces two columns
CMD: WORK:wc -lw WORK:test-oneline.txt
EXPECT:        1       2 WORK:test-oneline.txt
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
passed=16
failed=0
total=16
```

---
Generated by `make test-fsemu TARGET=ports/wc`
Report template: `toolchain/templates/test-report.md.template`
