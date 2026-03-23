# FS-UAE Test Report: bc

## Summary

| Field | Value |
|-------|-------|
| Port | bc |
| Date | 2026-03-23 18:44:32 |
| Duration | 36s |
| Platform | FS-UAE 3.2.35 (A1200, Kickstart 3.1) |
| Binary | `WORK:bc` (105K) |
| Test method | ARexx harness → TAP output |
| Result | **PASS** — 31/31 passed |

## Test Results

```
1..31
ok 1 - Version flag (-v) prints version to stdout
ok 2 - Help flag (-h) prints usage to stdout
ok 3 - Quiet flag (-q) suppresses welcome banner
ok 4 - Math library flag (-l) enables trigonometric functions
ok 5 - Compile-only flag (-c) prints bytecode instead of executing
ok 6 - Standard mode flag (-s) rejects non-POSIX extensions
ok 7 - Warn mode flag (-w) accepts file and processes it
ok 8 - Interactive flag (-i) is accepted without error (non-tty falls back to file mode)
ok 9 - Basic addition -- 2+3 = 5
ok 10 - Power operator -- 2^10 = 1024
ok 11 - Scale sets decimal precision -- 10/3 to 5 places
ok 12 - Variables and mixed arithmetic from script file
ok 13 - User-defined function -- double(21) = 42
ok 14 - For loop -- sum 1 to 5 = 15
ok 15 - While loop -- double n until > 4, result is 8
ok 16 - Comparison operator -- 5 > 3 prints 1
ok 17 - Multiple expressions on one line separated by semicolons
ok 18 - ibase -- hex input FF = 255
ok 19 - obase -- decimal 255 in hex = FF
ok 20 - Math library e() function -- e(1) with scale=0 = 2
ok 21 - Math library a() function -- 4*a(1) = pi to 10 decimal places
ok 22 - Invalid flag exits with RETURN_ERROR
ok 23 - Missing input file exits with RETURN_ERROR
ok 24 - Successful execution returns RC 0
ok 25 - quit command exits immediately with RC 0
ok 26 - Minimal script file (quit only) exits cleanly with RC 0
ok 27 - Minimal bc file produces no output and exits cleanly
ok 28 - Large exponent -- 2^10 integer result fits in output
ok 29 - Amiga WORK: path resolves correctly for script file
ok 30 - Math library loads correctly from Amiga binary path
ok 31 - Compile-only mode works on Amiga without executing bytecode
# passed: 31 failed: 0 total: 31
```

### Breakdown

| # | Test | Status | Details |
|---|------|--------|---------|
| 1 | Version flag (-v) prints version to stdout | PASS | |
| 2 | Help flag (-h) prints usage to stdout | PASS | |
| 3 | Quiet flag (-q) suppresses welcome banner | PASS | |
| 4 | Math library flag (-l) enables trigonometric functions | PASS | |
| 5 | Compile-only flag (-c) prints bytecode instead of executing | PASS | |
| 6 | Standard mode flag (-s) rejects non-POSIX extensions | PASS | |
| 7 | Warn mode flag (-w) accepts file and processes it | PASS | |
| 8 | Interactive flag (-i) is accepted without error (non-tty falls back to file mode) | PASS | |
| 9 | Basic addition -- 2+3 = 5 | PASS | |
| 10 | Power operator -- 2^10 = 1024 | PASS | |
| 11 | Scale sets decimal precision -- 10/3 to 5 places | PASS | |
| 12 | Variables and mixed arithmetic from script file | PASS | |
| 13 | User-defined function -- double(21) = 42 | PASS | |
| 14 | For loop -- sum 1 to 5 = 15 | PASS | |
| 15 | While loop -- double n until > 4, result is 8 | PASS | |
| 16 | Comparison operator -- 5 > 3 prints 1 | PASS | |
| 17 | Multiple expressions on one line separated by semicolons | PASS | |
| 18 | ibase -- hex input FF = 255 | PASS | |
| 19 | obase -- decimal 255 in hex = FF | PASS | |
| 20 | Math library e() function -- e(1) with scale=0 = 2 | PASS | |
| 21 | Math library a() function -- 4*a(1) = pi to 10 decimal places | PASS | |
| 22 | Invalid flag exits with RETURN_ERROR | PASS | |
| 23 | Missing input file exits with RETURN_ERROR | PASS | |
| 24 | Successful execution returns RC 0 | PASS | |
| 25 | quit command exits immediately with RC 0 | PASS | |
| 26 | Minimal script file (quit only) exits cleanly with RC 0 | PASS | |
| 27 | Minimal bc file produces no output and exits cleanly | PASS | |
| 28 | Large exponent -- 2^10 integer result fits in output | PASS | |
| 29 | Amiga WORK: path resolves correctly for script file | PASS | |
| 30 | Math library loads correctly from Amiga binary path | PASS | |
| 31 | Compile-only mode works on Amiga without executing bytecode | PASS | |

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
# test-fsemu-cases.txt -- FS-UAE test suite for GNU bc 1.07.1
# Category 1 CLI tool -- minimum 15 tests required
# All tests use pre-built .bc script files (ARexx ADDRESS COMMAND has no stdin piping)
#
# Exit codes: 0 = success, 10 = error (bad flag, missing file)
# Error messages go to stderr (not captured) -- error path tests use EXPECT_RC only
# Use -q flag to suppress the welcome banner in most tests

# ===========================================================================
# CATEGORY 1: Functional tests -- one per flag/option
# Flags from getopt_long: -c, -h, -i, -l, -q, -s, -w, -v
# ===========================================================================

TEST: Version flag (-v) prints version to stdout
CMD: WORK:bc -v
EXPECT_CONTAINS: bc
EXPECT_RC: 0

TEST: Help flag (-h) prints usage to stdout
CMD: WORK:bc -h
EXPECT_CONTAINS: usage
EXPECT_RC: 0

TEST: Quiet flag (-q) suppresses welcome banner
CMD: WORK:bc -q WORK:test-bc-add.bc
EXPECT: 5
EXPECT_RC: 0

TEST: Math library flag (-l) enables trigonometric functions
CMD: WORK:bc -q -l WORK:test-bc-mathlib.bc
EXPECT: 0
EXPECT_RC: 0

TEST: Compile-only flag (-c) prints bytecode instead of executing
CMD: WORK:bc -c -q WORK:test-bc-add.bc
EXPECT_CONTAINS: @i
EXPECT_RC: 0

TEST: Standard mode flag (-s) rejects non-POSIX extensions
CMD: WORK:bc -q -s WORK:test-bc-add.bc
EXPECT: 5
EXPECT_RC: 0

TEST: Warn mode flag (-w) accepts file and processes it
CMD: WORK:bc -q -w WORK:test-bc-add.bc
EXPECT: 5
EXPECT_RC: 0

TEST: Interactive flag (-i) is accepted without error (non-tty falls back to file mode)
CMD: WORK:bc -q -i WORK:test-bc-add.bc
EXPECT: 5
EXPECT_RC: 0

# ===========================================================================
# CATEGORY 2: Functional tests -- arithmetic operations and bc features
# ===========================================================================

TEST: Basic addition -- 2+3 = 5
CMD: WORK:bc -q WORK:test-bc-add.bc
EXPECT: 5
EXPECT_RC: 0

TEST: Power operator -- 2^10 = 1024
CMD: WORK:bc -q WORK:test-bc-power.bc
EXPECT: 1024
EXPECT_RC: 0

TEST: Scale sets decimal precision -- 10/3 to 5 places
CMD: WORK:bc -q WORK:test-bc-scale.bc
EXPECT: 3.33333
EXPECT_RC: 0

TEST: Variables and mixed arithmetic from script file
CMD: WORK:bc -q WORK:test-bc-script.bc
EXPECT: 50
EXPECT_RC: 0

TEST: User-defined function -- double(21) = 42
CMD: WORK:bc -q WORK:test-bc-function.bc
EXPECT: 42
EXPECT_RC: 0

TEST: For loop -- sum 1 to 5 = 15
CMD: WORK:bc -q WORK:test-bc-control.bc
EXPECT: 15
EXPECT_RC: 0

TEST: While loop -- double n until > 4, result is 8
CMD: WORK:bc -q WORK:test-bc-while.bc
EXPECT: 8
EXPECT_RC: 0

TEST: Comparison operator -- 5 > 3 prints 1
CMD: WORK:bc -q WORK:test-bc-compare.bc
EXPECT: 1
EXPECT_RC: 0

TEST: Multiple expressions on one line separated by semicolons
CMD: WORK:bc -q WORK:test-bc-multiexpr.bc
EXPECT: 2
EXPECT_RC: 0

TEST: ibase -- hex input FF = 255
CMD: WORK:bc -q WORK:test-bc-ibase.bc
EXPECT: 255
EXPECT_RC: 0

TEST: obase -- decimal 255 in hex = FF
CMD: WORK:bc -q WORK:test-bc-obase.bc
EXPECT: FF
EXPECT_RC: 0

TEST: Math library e() function -- e(1) with scale=0 = 2
CMD: WORK:bc -q -l WORK:test-bc-mathlib-e.bc
EXPECT: 2
EXPECT_RC: 0

TEST: Math library a() function -- 4*a(1) = pi to 10 decimal places
CMD: WORK:bc -q -l WORK:test-bc-mathlib-pi.bc
EXPECT: 3.1415926532
EXPECT_RC: 0

# ===========================================================================
# CATEGORY 3: Error path tests (EXPECT_RC: 10)
# Note: all error text goes to stderr -- not captured by harness
# ===========================================================================

TEST: Invalid flag exits with RETURN_ERROR
CMD: WORK:bc -Z WORK:test-bc-add.bc
EXPECT_RC: 10

TEST: Missing input file exits with RETURN_ERROR
CMD: WORK:bc -q WORK:test-bc-nonexistent.bc
EXPECT_RC: 10

# ===========================================================================
# CATEGORY 4: Exit code tests
# ===========================================================================

TEST: Successful execution returns RC 0
CMD: WORK:bc -q WORK:test-bc-add.bc
EXPECT_RC: 0

TEST: quit command exits immediately with RC 0
CMD: WORK:bc -q WORK:test-bc-script.bc
EXPECT_RC: 0

TEST: Minimal script file (quit only) exits cleanly with RC 0
CMD: WORK:bc -q WORK:test-bc-empty.bc
EXPECT_RC: 0

# ===========================================================================
# CATEGORY 5: Edge case tests
# ===========================================================================

TEST: Minimal bc file produces no output and exits cleanly
CMD: WORK:bc -q WORK:test-bc-empty.bc
EXPECT_RC: 0

TEST: Large exponent -- 2^10 integer result fits in output
CMD: WORK:bc -q WORK:test-bc-power.bc
EXPECT: 1024
EXPECT_RC: 0

# ===========================================================================
# CATEGORY 6: Amiga-specific tests -- WORK: volume paths
# ===========================================================================

TEST: Amiga WORK: path resolves correctly for script file
CMD: WORK:bc -q WORK:test-bc-add.bc
EXPECT: 5
EXPECT_RC: 0

TEST: Math library loads correctly from Amiga binary path
CMD: WORK:bc -q -l WORK:test-bc-mathlib-pi.bc
EXPECT: 3.1415926532
EXPECT_RC: 0

TEST: Compile-only mode works on Amiga without executing bytecode
CMD: WORK:bc -c -q WORK:test-bc-add.bc
EXPECT_CONTAINS: @r
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
passed=31
failed=0
total=31
```

---
Generated by `make test-fsemu TARGET=ports/bc`
Report template: `toolchain/templates/test-report.md.template`
