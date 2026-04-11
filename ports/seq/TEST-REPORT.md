# FS-UAE Test Report: seq

## Summary

| Field | Value |
|-------|-------|
| Port | seq |
| Date | 2026-04-11 17:10:06 |
| Duration | 64s |
| Platform | FS-UAE 3.2.35 (A1200, Kickstart 3.1) |
| Binary | `WORK:seq` (54K) |
| Test method | ARexx harness → TAP output |
| Result | **PASS** — 47/47 passed |

## Test Results

```
1..47
ok 1 - Single argument: count from 1 to N
ok 2 - Two arguments: first and last
ok 3 - Three arguments: first, increment, last
ok 4 - Single value: seq 1 produces exactly one line
ok 5 - First equals last: single output line
ok 6 - Descending sequence with negative increment
ok 7 - Sequence starting at negative number
ok 8 - Real-world countdown from 10 to 1
ok 9 - Floating point: tenths from 0.1 to 0.5
ok 10 - Floating point: half-integer steps
ok 11 - Exponent notation input values
ok 12 - -w pads single-digit numbers to two digits (1-10 range)
ok 13 - -w pads numbers to three digits (1-100 range)
ok 14 - -w pads near-boundary: 8, 9, 10 get zero-padded to 08, 09, 10
ok 15 - -w twice switches to space padding (not zero)
ok 16 - -w on descending sequence pads correctly
ok 17 - -s colon separator replaces newlines between values (no trailing sep)
ok 18 - -s space separator produces space-delimited single line
ok 19 - -f zero-pads using printf format %03g
ok 20 - -f with prefix text in format string
ok 21 - -f with %e scientific notation format
ok 22 - -f with %f fixed-point format and precision
ok 23 - -f with uppercase %G format (valid conversion specifier)
ok 24 - -v is unknown flag returns RC 10
ok 25 - -h is unknown flag returns RC 10
ok 26 - -w and -s combined: zero-padded values with custom separator
ok 27 - -f and -s combined: formatted values with dash separator
ok 28 - No arguments exits with RC=10
ok 29 - Too many arguments (4) exits with RC=10
ok 30 - Zero increment exits with RC=10
ok 31 - Negative increment for ascending range exits with RC=10
ok 32 - Positive increment for descending range exits with RC=10
ok 33 - -f with no conversion specifier exits with RC=10
ok 34 - -f with integer format %d exits with RC=10
ok 35 - Non-numeric argument exits with RC=2
ok 36 - Invalid option flag exits with RC=10
ok 37 - Edge: first equals last with explicit increment
ok 38 - Edge: last value not reachable by step skips it gracefully
ok 39 - Float accumulation rounding: 0.1 steps to 1.0 gives 10 lines
ok 40 - Large step size: 0 100 500 produces 6 values
ok 41 - Amiga WORK volume path: basic sequence via WORK: prefix
ok 42 - Amiga-safe separator (colon used as AmigaDOS path separator)
ok 43 - Real-world: generate padded filename list with prefix and extension
ok 44 - Real-world: scientific notation sequence for engineering use
ok 45 - Stress: 500-element sequence (line 1, 250, 500)
ok 46 - Stress: 500-element zero-padded sequence (generate_format with 3 digits)
ok 47 - Precision: float sequence endpoint rounding correction produces 0.5 as last line
# passed: 47 failed: 0 total: 47
```

### Breakdown

| # | Test | Status | Details |
|---|------|--------|---------|
| 1 | Single argument: count from 1 to N | PASS | |
| 2 | Two arguments: first and last | PASS | |
| 3 | Three arguments: first, increment, last | PASS | |
| 4 | Single value: seq 1 produces exactly one line | PASS | |
| 5 | First equals last: single output line | PASS | |
| 6 | Descending sequence with negative increment | PASS | |
| 7 | Sequence starting at negative number | PASS | |
| 8 | Real-world countdown from 10 to 1 | PASS | |
| 9 | Floating point: tenths from 0.1 to 0.5 | PASS | |
| 10 | Floating point: half-integer steps | PASS | |
| 11 | Exponent notation input values | PASS | |
| 12 | -w pads single-digit numbers to two digits (1-10 range) | PASS | |
| 13 | -w pads numbers to three digits (1-100 range) | PASS | |
| 14 | -w pads near-boundary: 8, 9, 10 get zero-padded to 08, 09, 10 | PASS | |
| 15 | -w twice switches to space padding (not zero) | PASS | |
| 16 | -w on descending sequence pads correctly | PASS | |
| 17 | -s colon separator replaces newlines between values (no trailing sep) | PASS | |
| 18 | -s space separator produces space-delimited single line | PASS | |
| 19 | -f zero-pads using printf format %03g | PASS | |
| 20 | -f with prefix text in format string | PASS | |
| 21 | -f with %e scientific notation format | PASS | |
| 22 | -f with %f fixed-point format and precision | PASS | |
| 23 | -f with uppercase %G format (valid conversion specifier) | PASS | |
| 24 | -v is unknown flag returns RC 10 | PASS | |
| 25 | -h is unknown flag returns RC 10 | PASS | |
| 26 | -w and -s combined: zero-padded values with custom separator | PASS | |
| 27 | -f and -s combined: formatted values with dash separator | PASS | |
| 28 | No arguments exits with RC=10 | PASS | |
| 29 | Too many arguments (4) exits with RC=10 | PASS | |
| 30 | Zero increment exits with RC=10 | PASS | |
| 31 | Negative increment for ascending range exits with RC=10 | PASS | |
| 32 | Positive increment for descending range exits with RC=10 | PASS | |
| 33 | -f with no conversion specifier exits with RC=10 | PASS | |
| 34 | -f with integer format %d exits with RC=10 | PASS | |
| 35 | Non-numeric argument exits with RC=2 | PASS | |
| 36 | Invalid option flag exits with RC=10 | PASS | |
| 37 | Edge: first equals last with explicit increment | PASS | |
| 38 | Edge: last value not reachable by step skips it gracefully | PASS | |
| 39 | Float accumulation rounding: 0.1 steps to 1.0 gives 10 lines | PASS | |
| 40 | Large step size: 0 100 500 produces 6 values | PASS | |
| 41 | Amiga WORK volume path: basic sequence via WORK: prefix | PASS | |
| 42 | Amiga-safe separator (colon used as AmigaDOS path separator) | PASS | |
| 43 | Real-world: generate padded filename list with prefix and extension | PASS | |
| 44 | Real-world: scientific notation sequence for engineering use | PASS | |
| 45 | Stress: 500-element sequence (line 1, 250, 500) | PASS | |
| 46 | Stress: 500-element zero-padded sequence (generate_format with 3 digits) | PASS | |
| 47 | Precision: float sequence endpoint rounding correction produces 0.5 as last line | PASS | |

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
# test-fsemu-cases.txt -- FS-UAE test suite for seq 1.8 (OpenBSD)
# seq: print sequences of numbers
# Category 1 (CLI tool) -- minimum 15 tests
# Flags: -f <format>, -s <string>, -w (equal-width), -v (version), -h (help)
# Arguments: [first [incr]] last
#
# Separator behavior: OpenBSD seq places sep BEFORE each value except the
# first (not after each), so output has no trailing separator.
# Example: seq -s ':' 1 3 -> '1:2:3' (no trailing colon)
#
# Error exit codes from ported source:
#   RC=0:  success, -v (version), -h (help/usage)
#   RC=10: zero increment, wrong direction, invalid format, too few/many args
#   RC=2:  invalid floating point argument (e_atof fails with err(2,...))

# === FUNCTIONAL TESTS: Basic positional arguments ===

# Native: /tmp/seq_openbsd 5 | head -1
TEST: Single argument: count from 1 to N
CMD: WORK:seq 5
EXPECT: 1
EXPECT_LINE: 5,5
EXPECT_RC: 0

# Native: /tmp/seq_openbsd 1 5 | head -1
TEST: Two arguments: first and last
CMD: WORK:seq 1 5
EXPECT: 1
EXPECT_LINE: 5,5
EXPECT_RC: 0

# Native: /tmp/seq_openbsd 1 2 10 | head -1
TEST: Three arguments: first, increment, last
CMD: WORK:seq 1 2 10
EXPECT: 1
EXPECT_LINE: 3,5
EXPECT_LINE: 5,9
EXPECT_RC: 0

# Native: /tmp/seq_openbsd 1 | head -1
TEST: Single value: seq 1 produces exactly one line
CMD: WORK:seq 1
EXPECT: 1
EXPECT_LINE: 1,1
EXPECT_RC: 0

# Native: /tmp/seq_openbsd 5 5 | head -1
TEST: First equals last: single output line
CMD: WORK:seq 5 5
EXPECT: 5
EXPECT_RC: 0

# === FUNCTIONAL TESTS: Negative and descending sequences ===

# Native: /tmp/seq_openbsd 5 -1 1
TEST: Descending sequence with negative increment
CMD: WORK:seq 5 -1 1
EXPECT: 5
EXPECT_LINE: 3,3
EXPECT_LINE: 5,1
EXPECT_RC: 0

# Native: /tmp/seq_openbsd -3 3
TEST: Sequence starting at negative number
CMD: WORK:seq -3 3
EXPECT: -3
EXPECT_LINE: 4,0
EXPECT_LINE: 7,3
EXPECT_RC: 0

# Native: /tmp/seq_openbsd 10 -1 1
TEST: Real-world countdown from 10 to 1
CMD: WORK:seq 10 -1 1
EXPECT: 10
EXPECT_LINE: 5,6
EXPECT_LINE: 10,1
EXPECT_RC: 0

# === FUNCTIONAL TESTS: Floating point ===

# Native: /tmp/seq_openbsd 0.1 0.1 0.5
TEST: Floating point: tenths from 0.1 to 0.5
CMD: WORK:seq 0.1 0.1 0.5
EXPECT: 0.1
EXPECT_LINE: 3,0.3
EXPECT_LINE: 5,0.5
EXPECT_RC: 0

# Native: /tmp/seq_openbsd 1.0 0.5 3.0
TEST: Floating point: half-integer steps
CMD: WORK:seq 1.0 0.5 3.0
EXPECT: 1
EXPECT_LINE: 2,1.5
EXPECT_LINE: 5,3
EXPECT_RC: 0

# Native: /tmp/seq_openbsd 1e0 1e0 5e0
TEST: Exponent notation input values
CMD: WORK:seq 1e0 1e0 5e0
EXPECT: 1
EXPECT_LINE: 3,3
EXPECT_LINE: 5,5
EXPECT_RC: 0

# === FUNCTIONAL TESTS: -w flag (equal-width / zero-pad) ===

# Native: /tmp/seq_openbsd -w 1 10 | head -1
TEST: -w pads single-digit numbers to two digits (1-10 range)
CMD: WORK:seq -w 1 10
EXPECT: 01
EXPECT_LINE: 9,09
EXPECT_LINE: 10,10
EXPECT_RC: 0

# Native: /tmp/seq_openbsd -w 1 100 | head -1
TEST: -w pads numbers to three digits (1-100 range)
CMD: WORK:seq -w 1 100
EXPECT: 001
EXPECT_LINE: 50,050
EXPECT_LINE: 100,100
EXPECT_RC: 0

# Native: /tmp/seq_openbsd -w 8 10
TEST: -w pads near-boundary: 8, 9, 10 get zero-padded to 08, 09, 10
CMD: WORK:seq -w 8 10
EXPECT: 08
EXPECT_LINE: 2,09
EXPECT_LINE: 3,10
EXPECT_RC: 0

# Native: /tmp/seq_openbsd -w -w 1 10 | head -1
TEST: -w twice switches to space padding (not zero)
CMD: WORK:seq -w -w 1 10
EXPECT:  1
EXPECT_LINE: 10, 10
EXPECT_RC: 0

# Native: /tmp/seq_openbsd -w 10 -1 1 | head -1
TEST: -w on descending sequence pads correctly
CMD: WORK:seq -w 10 -1 1
EXPECT: 10
EXPECT_LINE: 2,09
EXPECT_LINE: 10,01
EXPECT_RC: 0

# === FUNCTIONAL TESTS: -s flag (separator) ===

# Native: /tmp/seq_openbsd -s ':' 1 3
TEST: -s colon separator replaces newlines between values (no trailing sep)
CMD: WORK:seq -s : 1 3
EXPECT: 1:2:3
EXPECT_RC: 0

# Native: /tmp/seq_openbsd -s ' ' 1 3
TEST: -s space separator produces space-delimited single line
CMD: WORK:seq -s " " 1 3
EXPECT: 1 2 3
EXPECT_RC: 0

# === FUNCTIONAL TESTS: -f flag (printf format string) ===

# Native: /tmp/seq_openbsd -f '%03g' 1 5 | head -1
TEST: -f zero-pads using printf format %03g
CMD: WORK:seq -f %03g 1 5
EXPECT: 001
EXPECT_LINE: 3,003
EXPECT_LINE: 5,005
EXPECT_RC: 0

# Native: /tmp/seq_openbsd -f 'item-%g' 1 3 | head -1
TEST: -f with prefix text in format string
CMD: WORK:seq -f item-%g 1 3
EXPECT: item-1
EXPECT_LINE: 2,item-2
EXPECT_LINE: 3,item-3
EXPECT_RC: 0

# Native: /tmp/seq_openbsd -f '%e' 1 3 | head -1
TEST: -f with %e scientific notation format
CMD: WORK:seq -f %e 1 3
EXPECT: 1.000000e+00
EXPECT_LINE: 2,2.000000e+00
EXPECT_LINE: 3,3.000000e+00
EXPECT_RC: 0

# Native: /tmp/seq_openbsd -f '%05.2f' 1 3 | head -1
TEST: -f with %f fixed-point format and precision
CMD: WORK:seq -f %05.2f 1 3
EXPECT: 01.00
EXPECT_LINE: 2,02.00
EXPECT_LINE: 3,03.00
EXPECT_RC: 0

# Native: /tmp/seq_openbsd -f '%G' 1 3 | head -1
TEST: -f with uppercase %G format (valid conversion specifier)
CMD: WORK:seq -f %G 1 3
EXPECT: 1
EXPECT_LINE: 2,2
EXPECT_LINE: 3,3
EXPECT_RC: 0

# === FUNCTIONAL TESTS: -v (version) and -h (help) flags ===
# Note: -v and -h are Amiga-ported additions not present in original OpenBSD source

# amiport: -v and -h are NOT implemented in OpenBSD seq 1.8
# These are unknown flags, so seq exits with usage error
TEST: -v is unknown flag returns RC 10
CMD: WORK:seq -v
EXPECT_RC: 10

TEST: -h is unknown flag returns RC 10
CMD: WORK:seq -h
EXPECT_RC: 10

# === FUNCTIONAL TESTS: Combined flags ===

# Native: /tmp/seq_openbsd -w -s ' ' 1 5
TEST: -w and -s combined: zero-padded values with custom separator
CMD: WORK:seq -w -s " " 1 5
EXPECT: 1 2 3 4 5
EXPECT_RC: 0

# Native: /tmp/seq_openbsd -f '%03g' -s '-' 1 5
TEST: -f and -s combined: formatted values with dash separator
CMD: WORK:seq -f %03g -s - 1 5
EXPECT: 001-002-003-004-005
EXPECT_RC: 0

# === ERROR PATH TESTS ===

# Source: usage(10) when argc < 1
# Safe to run with no args: seq checks argc count before any I/O, never reads stdin
# stderr not captured; verify RC=10 only
TEST: No arguments exits with RC=10
CMD: WORK:seq
EXPECT_RC: 10

TEST: Too many arguments (4) exits with RC=10
CMD: WORK:seq 1 2 3 4
EXPECT_RC: 10

# Source: errx(10, "zero %screment", ...) when incr==0.0
# stderr not captured; verify RC=10 only
TEST: Zero increment exits with RC=10
CMD: WORK:seq 1 0 5
EXPECT_RC: 10

# Source: errx(10, "needs positive increment") when incr<0 && first<last
TEST: Negative increment for ascending range exits with RC=10
CMD: WORK:seq 1 -1 5
EXPECT_RC: 10

# Source: errx(10, "needs negative decrement") when incr>0 && first>last
TEST: Positive increment for descending range exits with RC=10
CMD: WORK:seq 5 1 1
EXPECT_RC: 10

# Source: errx(10, "invalid format string: ...") when valid_format() returns 0
# Invalid: no conversion specifier
TEST: -f with no conversion specifier exits with RC=10
CMD: WORK:seq -f hello 1 3
EXPECT_RC: 10

# Invalid: integer conversion (only float conversions A/a/E/e/F/f/G/g allowed)
TEST: -f with integer format %d exits with RC=10
CMD: WORK:seq -f %d 1 3
EXPECT_RC: 10

# Source: errx(2, "invalid floating point argument: %s") when strtod fails
# Note: e_atof uses err(2,...) which exits with code 2 (not 10)
TEST: Non-numeric argument exits with RC=2
CMD: WORK:seq abc
EXPECT_RC: 2

# Invalid option flag -> usage(10)
TEST: Invalid option flag exits with RC=10
CMD: WORK:seq -Z 1 5
EXPECT_RC: 10

# === EDGE CASE TESTS ===

# Single value (first == last with any increment)
# Native: /tmp/seq_openbsd 1 1
TEST: Edge: first equals last with explicit increment
CMD: WORK:seq 1 1 1
EXPECT: 1
EXPECT_RC: 0

# Range that misses last due to step (9 not in 1,3,5,7 sequence)
# Native: /tmp/seq_openbsd 1 2 8
TEST: Edge: last value not reachable by step skips it gracefully
CMD: WORK:seq 1 2 8
EXPECT: 1
EXPECT_LINE: 4,7
EXPECT_RC: 0

# Floating point accumulation: 0.1+0.1+...+0.1 may drift
# Native: /tmp/seq_openbsd 0.1 0.1 1.0 | wc -l -> 10 lines
# Line 10 should be '1' (rounding correction logic in source)
TEST: Float accumulation rounding: 0.1 steps to 1.0 gives 10 lines
CMD: WORK:seq 0.1 0.1 1.0
EXPECT: 0.1
EXPECT_LINE: 10,1
EXPECT_RC: 0

# Large step: sequence with step larger than range
# Native: /tmp/seq_openbsd 0 100 500
TEST: Large step size: 0 100 500 produces 6 values
CMD: WORK:seq 0 100 500
EXPECT: 0
EXPECT_LINE: 3,200
EXPECT_LINE: 6,500
EXPECT_RC: 0

# === AMIGA-SPECIFIC TESTS ===

# Verify program runs from WORK: volume path (basic Amiga path handling)
# Native: /tmp/seq_openbsd 3
TEST: Amiga WORK volume path: basic sequence via WORK: prefix
CMD: WORK:seq 3
EXPECT: 1
EXPECT_LINE: 3,3
EXPECT_RC: 0

# Verify -s with tab-like separator doesn't cause AmigaDOS issues
# Native: /tmp/seq_openbsd -s : 1 4
TEST: Amiga-safe separator (colon used as AmigaDOS path separator)
CMD: WORK:seq -s : 1 4
EXPECT: 1:2:3:4
EXPECT_RC: 0

# === REAL-WORLD AND STRESS TESTS ===

# Real-world: generate padded batch filenames
# Native: /tmp/seq_openbsd -f 'file%03g.txt' 1 5 | head -1
TEST: Real-world: generate padded filename list with prefix and extension
CMD: WORK:seq -f file%03g.txt 1 5
EXPECT: file001.txt
EXPECT_LINE: 3,file003.txt
EXPECT_LINE: 5,file005.txt
EXPECT_RC: 0

# Real-world: generate engineering-notation list for scientific work
# Native: /tmp/seq_openbsd -f '%e' 1.0e3 1.0e3 5.0e3 | head -1
TEST: Real-world: scientific notation sequence for engineering use
CMD: WORK:seq -f %e 1.0e3 1.0e3 5.0e3
EXPECT: 1.000000e+03
EXPECT_LINE: 3,3.000000e+03
EXPECT_LINE: 5,5.000000e+03
EXPECT_RC: 0

# Stress: 500-element sequence (maximum per stress-test size limit)
# Native: /tmp/seq_openbsd 1 500 | head -1
TEST: Stress: 500-element sequence (line 1, 250, 500)
CMD: WORK:seq 1 500
EXPECT: 1
EXPECT_LINE: 5,5
EXPECT_LINE: 500,500
EXPECT_RC: 0

# Stress: 500-element zero-padded sequence tests memory/format path
# Native: /tmp/seq_openbsd -w 1 500 | head -1
TEST: Stress: 500-element zero-padded sequence (generate_format with 3 digits)
CMD: WORK:seq -w 1 500
EXPECT: 001
EXPECT_LINE: 100,100
EXPECT_LINE: 500,500
EXPECT_RC: 0

# Precision: float sequence where rounding correction triggers
# The source has a special rounding-correction path (lines 276-286)
# This tests the asprintf double-check logic that catches rounding drift
# Native: /tmp/seq_openbsd 0.1 0.1 0.5 | tail -1
TEST: Precision: float sequence endpoint rounding correction produces 0.5 as last line
CMD: WORK:seq 0.1 0.1 0.5
EXPECT: 0.1
EXPECT_LINE: 5,0.5
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
passed=47
failed=0
total=47
```

---
Generated by `make test-fsemu TARGET=ports/seq`
Report template: `toolchain/templates/test-report.md.template`
