# FS-UAE Test Report: cal

## Summary

| Field | Value |
|-------|-------|
| Port | cal |
| Date | 2026-03-21 22:22:29 |
| Duration | 62s |
| Platform | FS-UAE 3.2.35 (A1200, Kickstart 3.1) |
| Binary | `WORK:cal` (31K) |
| Test method | ARexx harness → TAP output |
| Result | **PASS** — 22/22 passed |

## Test Results

```
1..22
ok 1 - Display specific month and year (March 2026)
ok 2 - Display full year with -y flag
ok 3 - Julian day numbers for a month (-j)
ok 4 - Monday-first week layout (-m flag)
ok 5 - Week numbers displayed (-w flag)
ok 6 - Week numbers output contains bracket notation
ok 7 - Abbreviated month name accepted (mar = March)
ok 8 - Julian full-year view (-j -y)
ok 9 - September 1752 Gregorian reformation special case
ok 10 - Year-only argument displays full year for that year
ok 11 - Boundary -- January of year 1 (earliest valid year)
ok 12 - Boundary -- December 9999 (latest valid year)
ok 13 - No arguments returns current month without error
ok 14 - Error -- invalid month number 0
ok 15 - Error -- invalid month number 13
ok 16 - Error -- year 0 is out of valid range 1-9999
ok 17 - Error -- year 10000 exceeds valid range 1-9999
ok 18 - Error -- unrecognised month name string
ok 19 - Error -- invalid flag triggers usage message
ok 20 - Error -- too many arguments (month year extra)
ok 21 - Error -- -j and -w flags are mutually exclusive
ok 22 - Amiga WORK: volume path -- cal binary resolves from WORK: correctly
# passed: 22 failed: 0 total: 22
```

### Breakdown

| # | Test | Status | Details |
|---|------|--------|---------|
| 1 | Display specific month and year (March 2026) | PASS | |
| 2 | Display full year with -y flag | PASS | |
| 3 | Julian day numbers for a month (-j) | PASS | |
| 4 | Monday-first week layout (-m flag) | PASS | |
| 5 | Week numbers displayed (-w flag) | PASS | |
| 6 | Week numbers output contains bracket notation | PASS | |
| 7 | Abbreviated month name accepted (mar = March) | PASS | |
| 8 | Julian full-year view (-j -y) | PASS | |
| 9 | September 1752 Gregorian reformation special case | PASS | |
| 10 | Year-only argument displays full year for that year | PASS | |
| 11 | Boundary -- January of year 1 (earliest valid year) | PASS | |
| 12 | Boundary -- December 9999 (latest valid year) | PASS | |
| 13 | No arguments returns current month without error | PASS | |
| 14 | Error -- invalid month number 0 | PASS | |
| 15 | Error -- invalid month number 13 | PASS | |
| 16 | Error -- year 0 is out of valid range 1-9999 | PASS | |
| 17 | Error -- year 10000 exceeds valid range 1-9999 | PASS | |
| 18 | Error -- unrecognised month name string | PASS | |
| 19 | Error -- invalid flag triggers usage message | PASS | |
| 20 | Error -- too many arguments (month year extra) | PASS | |
| 21 | Error -- -j and -w flags are mutually exclusive | PASS | |
| 22 | Amiga WORK: volume path -- cal binary resolves from WORK: correctly | PASS | |

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
# test-fsemu-cases.txt -- FS-UAE ARexx test suite for cal (OpenBSD 1.32)
# Category 1 (CLI), minimum 8 tests required.
# Cal reads no stdin -- all tests use argument-only invocation.
# No input files needed: cal generates its own output from date arithmetic.

TEST: Display specific month and year (March 2026)
CMD: WORK:cal 3 2026
EXPECT: March 2026
EXPECT_RC: 0

TEST: Display full year with -y flag
CMD: WORK:cal -y 2026
EXPECT_CONTAINS: January
EXPECT_RC: 0

TEST: Julian day numbers for a month (-j)
CMD: WORK:cal -j 3 2026
EXPECT: March 2026
EXPECT_RC: 0

TEST: Monday-first week layout (-m flag)
CMD: WORK:cal -m 3 2026
EXPECT_CONTAINS: Mo Tu We Th Fr Sa Su
EXPECT_RC: 0

TEST: Week numbers displayed (-w flag)
CMD: WORK:cal -w 3 2026
EXPECT_CONTAINS: March 2026
EXPECT_RC: 0

TEST: Week numbers output contains bracket notation
CMD: WORK:cal -w 3 2026
EXPECT_CONTAINS: [
EXPECT_RC: 0

TEST: Abbreviated month name accepted (mar = March)
CMD: WORK:cal mar 2026
EXPECT: March 2026
EXPECT_RC: 0

TEST: Julian full-year view (-j -y)
CMD: WORK:cal -j -y 2026
EXPECT_CONTAINS: 2026
EXPECT_RC: 0

TEST: September 1752 Gregorian reformation special case
CMD: WORK:cal 9 1752
EXPECT: September 1752
EXPECT_RC: 0

TEST: Year-only argument displays full year for that year
CMD: WORK:cal 2000
EXPECT_CONTAINS: January
EXPECT_RC: 0

TEST: Boundary -- January of year 1 (earliest valid year)
CMD: WORK:cal 1 1
EXPECT: January 1
EXPECT_RC: 0

TEST: Boundary -- December 9999 (latest valid year)
CMD: WORK:cal 12 9999
EXPECT: December 9999
EXPECT_RC: 0

TEST: No arguments returns current month without error
CMD: WORK:cal
EXPECT_CONTAINS: Su Mo Tu We Th Fr Sa
EXPECT_RC: 0

TEST: Error -- invalid month number 0
CMD: WORK:cal 0 2026
EXPECT_RC: 10

TEST: Error -- invalid month number 13
CMD: WORK:cal 13 2026
EXPECT_RC: 10

TEST: Error -- year 0 is out of valid range 1-9999
CMD: WORK:cal 1 0
EXPECT_RC: 10

TEST: Error -- year 10000 exceeds valid range 1-9999
CMD: WORK:cal 1 10000
EXPECT_RC: 10

TEST: Error -- unrecognised month name string
CMD: WORK:cal foo 2026
EXPECT_RC: 10

TEST: Error -- invalid flag triggers usage message
CMD: WORK:cal -z
EXPECT_RC: 10

TEST: Error -- too many arguments (month year extra)
CMD: WORK:cal 3 2026 extra
EXPECT_RC: 10

TEST: Error -- -j and -w flags are mutually exclusive
CMD: WORK:cal -j -w 3 2026
EXPECT_RC: 10

TEST: Amiga WORK: volume path -- cal binary resolves from WORK: correctly
CMD: WORK:cal 6 2026
EXPECT: June 2026
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
passed=22
failed=0
total=22
```

---
Generated by `make test-fsemu TARGET=ports/cal`
Report template: `toolchain/templates/test-report.md.template`
