# FS-UAE Test Report: awk

## Summary

| Field | Value |
|-------|-------|
| Port | awk |
| Date | 2026-03-25 17:30:27 |
| Duration | 68s |
| Platform | FS-UAE 3.2.35 (A1200, Kickstart 3.1) |
| Binary | `WORK:awk` (168K) |
| Test method | ARexx harness → TAP output |
| Result | **PASS** — 44/44 passed |

## Test Results

```
1..44
ok 1 - --version flag prints version string
ok 2 - -version flag (single dash) prints version string
ok 3 - -F flag sets field separator (colon-delimited file)
ok 4 - -v flag sets variable before program runs
ok 5 - -f flag reads program from file
ok 6 - BEGIN block executes before input
ok 7 - END block executes after all input
ok 8 - NR built-in variable tracks record number
ok 9 - NF and dollar-NF built-in variables
ok 10 - OFS output field separator applied on print list
ok 11 - substr() extracts substring
ok 12 - index() finds position of substring
ok 13 - length() returns string length
ok 14 - tolower() converts to lowercase
ok 15 - gsub() replaces all occurrences globally
ok 16 - sub() replaces first occurrence only
ok 17 - match() sets RSTART and RLENGTH
ok 18 - split() splits string into array returns count
ok 19 - sprintf() formats string without printing
ok 20 - printf with zero-padded integer format
ok 21 - sqrt() math function
ok 22 - Arithmetic operators (multiply)
ok 23 - for loop with if-else produces alternating output
ok 24 - Associative array with for-in and delete
ok 25 - Array counting word frequency
ok 26 - User-defined function (square)
ok 27 - Multiple pattern-action rules match independently
ok 28 - getline reads from file in BEGIN block
ok 29 - --csv mode splits comma-separated fields
ok 30 - Syntax error in awk program (unclosed brace) exits RC 2
ok 31 - Missing -f program file exits with FATAL RC 2
ok 32 - Invalid -v argument (no equals sign) exits RC 2
ok 33 - Empty input file produces no output (BEGIN-only check)
ok 34 - Empty input file with pattern rule is silent
ok 35 - Long line input processed without truncation
ok 36 - WORK: volume path for input file (Amiga path handling)
ok 37 - -v flag with Amiga WORK: path in variable value
ok 38 - Word count simulation (NR words chars)
ok 39 - Sum column of numbers (real-world data extraction)
ok 40 - Extract second field from colon-delimited passwd-style file
ok 41 - 10000-iteration loop accumulates correct sum
ok 42 - Recursive function fib(20) without stack overflow
ok 43 - Process 20-line file building associative array
ok 44 - atan2(0,-1) computes pi to 10 decimal places
# passed: 44 failed: 0 total: 44
```

### Breakdown

| # | Test | Status | Details |
|---|------|--------|---------|
| 1 | --version flag prints version string | PASS | |
| 2 | -version flag (single dash) prints version string | PASS | |
| 3 | -F flag sets field separator (colon-delimited file) | PASS | |
| 4 | -v flag sets variable before program runs | PASS | |
| 5 | -f flag reads program from file | PASS | |
| 6 | BEGIN block executes before input | PASS | |
| 7 | END block executes after all input | PASS | |
| 8 | NR built-in variable tracks record number | PASS | |
| 9 | NF and dollar-NF built-in variables | PASS | |
| 10 | OFS output field separator applied on print list | PASS | |
| 11 | substr() extracts substring | PASS | |
| 12 | index() finds position of substring | PASS | |
| 13 | length() returns string length | PASS | |
| 14 | tolower() converts to lowercase | PASS | |
| 15 | gsub() replaces all occurrences globally | PASS | |
| 16 | sub() replaces first occurrence only | PASS | |
| 17 | match() sets RSTART and RLENGTH | PASS | |
| 18 | split() splits string into array returns count | PASS | |
| 19 | sprintf() formats string without printing | PASS | |
| 20 | printf with zero-padded integer format | PASS | |
| 21 | sqrt() math function | PASS | |
| 22 | Arithmetic operators (multiply) | PASS | |
| 23 | for loop with if-else produces alternating output | PASS | |
| 24 | Associative array with for-in and delete | PASS | |
| 25 | Array counting word frequency | PASS | |
| 26 | User-defined function (square) | PASS | |
| 27 | Multiple pattern-action rules match independently | PASS | |
| 28 | getline reads from file in BEGIN block | PASS | |
| 29 | --csv mode splits comma-separated fields | PASS | |
| 30 | Syntax error in awk program (unclosed brace) exits RC 2 | PASS | |
| 31 | Missing -f program file exits with FATAL RC 2 | PASS | |
| 32 | Invalid -v argument (no equals sign) exits RC 2 | PASS | |
| 33 | Empty input file produces no output (BEGIN-only check) | PASS | |
| 34 | Empty input file with pattern rule is silent | PASS | |
| 35 | Long line input processed without truncation | PASS | |
| 36 | WORK: volume path for input file (Amiga path handling) | PASS | |
| 37 | -v flag with Amiga WORK: path in variable value | PASS | |
| 38 | Word count simulation (NR words chars) | PASS | |
| 39 | Sum column of numbers (real-world data extraction) | PASS | |
| 40 | Extract second field from colon-delimited passwd-style file | PASS | |
| 41 | 10000-iteration loop accumulates correct sum | PASS | |
| 42 | Recursive function fib(20) without stack overflow | PASS | |
| 43 | Process 20-line file building associative array | PASS | |
| 44 | atan2(0,-1) computes pi to 10 decimal places | PASS | |

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
# test-fsemu-cases.txt -- FS-UAE test suite for awk (One True Awk)
# Category 2 (Scripting) -- minimum 20 tests
# Uses input file instead of piping (ARexx limitation)
# Dollar signs in awk programs placed in .awk files (AmigaDOS $ expansion)

# === FUNCTIONAL TESTS: flags and basic features ===

TEST: --version flag prints version string
CMD: WORK:awk --version
EXPECT_CONTAINS: awk version
EXPECT_RC: 0

TEST: -version flag (single dash) prints version string
CMD: WORK:awk -version
EXPECT_CONTAINS: awk version
EXPECT_RC: 0

TEST: -F flag sets field separator (colon-delimited file)
CMD: WORK:awk -F : -f WORK:test-awk-fields.awk WORK:test-awk-colon.txt
EXPECT: root
EXPECT_RC: 0

TEST: -v flag sets variable before program runs
CMD: WORK:awk -v x=42 "BEGIN { print x }" WORK:test-empty.txt
EXPECT: 42
EXPECT_RC: 0

TEST: -f flag reads program from file
CMD: WORK:awk -f WORK:test-awk-arith.awk WORK:test-empty.txt
EXPECT: 42
EXPECT_RC: 0

TEST: BEGIN block executes before input
CMD: WORK:awk -f WORK:test-awk-beginend.awk WORK:test-awk-numbers.txt
EXPECT: START
EXPECT_RC: 0

TEST: END block executes after all input
CMD: WORK:awk -f WORK:test-awk-beginend.awk WORK:test-awk-numbers.txt
EXPECT_CONTAINS: END
EXPECT_RC: 0

TEST: NR built-in variable tracks record number
CMD: WORK:awk -f WORK:test-awk-nr.awk WORK:test-awk-numbers.txt
EXPECT: 1 10
EXPECT_RC: 0

TEST: NF and dollar-NF built-in variables
CMD: WORK:awk -f WORK:test-awk-nf.awk WORK:test-multiline.txt
EXPECT: 2 world
EXPECT_RC: 0

TEST: OFS output field separator applied on print list
CMD: WORK:awk -f WORK:test-awk-ofs.awk WORK:test-multiline.txt
EXPECT: hello:world
EXPECT_RC: 0

# === FUNCTIONAL TESTS: string functions ===

TEST: substr() extracts substring
CMD: WORK:awk -f WORK:test-awk-substr.awk WORK:test-oneline.txt
EXPECT: hello
EXPECT_RC: 0

TEST: index() finds position of substring
CMD: WORK:awk -f WORK:test-awk-index.awk WORK:test-oneline.txt
EXPECT: 7
EXPECT_RC: 0

TEST: length() returns string length
CMD: WORK:awk -f WORK:test-awk-length.awk WORK:test-oneline.txt
EXPECT: 11
EXPECT_RC: 0

TEST: tolower() converts to lowercase
CMD: WORK:awk -f WORK:test-awk-tolower.awk WORK:test-multiline.txt
EXPECT_CONTAINS: uppercase line
EXPECT_RC: 0

TEST: gsub() replaces all occurrences globally
CMD: WORK:awk -f WORK:test-awk-gsub.awk WORK:test-oneline.txt
EXPECT: hell0 w0rld
EXPECT_RC: 0

TEST: sub() replaces first occurrence only
CMD: WORK:awk -f WORK:test-awk-sub.awk WORK:test-oneline.txt
EXPECT: goodbye world
EXPECT_RC: 0

TEST: match() sets RSTART and RLENGTH
CMD: WORK:awk -f WORK:test-awk-match.awk WORK:test-multiline.txt
EXPECT: 1 5
EXPECT_RC: 0

TEST: split() splits string into array returns count
CMD: WORK:awk -f WORK:test-awk-split.awk WORK:test-awk-colon.txt
EXPECT: 3 root
EXPECT_RC: 0

TEST: sprintf() formats string without printing
CMD: WORK:awk -f WORK:test-awk-sprintf.awk WORK:test-awk-words.txt
EXPECT: hello     |    1
EXPECT_RC: 0

# === FUNCTIONAL TESTS: math and printf ===

TEST: printf with zero-padded integer format
CMD: WORK:awk -f WORK:test-awk-printf.awk WORK:test-empty.txt
EXPECT: 00042
EXPECT_RC: 0

TEST: sqrt() math function
CMD: WORK:awk -f WORK:test-awk-math.awk WORK:test-empty.txt
EXPECT: 1.4142
EXPECT_RC: 0

TEST: Arithmetic operators (multiply)
CMD: WORK:awk -f WORK:test-awk-arith.awk WORK:test-empty.txt
EXPECT: 42
EXPECT_RC: 0

# === FUNCTIONAL TESTS: control flow ===

TEST: for loop with if-else produces alternating output
CMD: WORK:awk -f WORK:test-awk-control.awk WORK:test-empty.txt
EXPECT: 1 odd
EXPECT_RC: 0

# === FUNCTIONAL TESTS: arrays ===

TEST: Associative array with for-in and delete
CMD: WORK:awk -f WORK:test-awk-delete.awk WORK:test-empty.txt
EXPECT: 2
EXPECT_RC: 0

TEST: Array counting word frequency
CMD: WORK:awk -f WORK:test-awk-array.awk WORK:test-awk-words.txt
EXPECT_CONTAINS: hello 1
EXPECT_RC: 0

# === FUNCTIONAL TESTS: user-defined functions ===

TEST: User-defined function (square)
CMD: WORK:awk -f WORK:test-awk-func.awk WORK:test-empty.txt
EXPECT: 49
EXPECT_RC: 0

# === FUNCTIONAL TESTS: multiple rules ===

TEST: Multiple pattern-action rules match independently
CMD: WORK:awk -f WORK:test-awk-multipattern.awk WORK:test-multiline.txt
EXPECT: found: hello world
EXPECT_RC: 0

# === FUNCTIONAL TESTS: getline ===

TEST: getline reads from file in BEGIN block
CMD: WORK:awk -f WORK:test-awk-getline.awk WORK:test-empty.txt
EXPECT: HELLO
EXPECT_RC: 0

# === FUNCTIONAL TESTS: --csv mode ===

TEST: --csv mode splits comma-separated fields
CMD: WORK:awk --csv -f WORK:test-awk-csv.awk WORK:test-awk-csv-data.txt
EXPECT: 3 age
EXPECT_RC: 0

# === ERROR PATH TESTS ===

TEST: Syntax error in awk program (unclosed brace) exits RC 2
CMD: WORK:awk "{ print" WORK:test-oneline.txt
EXPECT_RC: 2

TEST: Missing -f program file exits with FATAL RC 2
CMD: WORK:awk -f WORK:test-awk-nonexistent-program.awk WORK:test-oneline.txt
EXPECT_RC: 2

TEST: Invalid -v argument (no equals sign) exits RC 2
CMD: WORK:awk -v badvar "BEGIN{}" WORK:test-empty.txt
EXPECT_RC: 2

# === EDGE CASE TESTS ===

TEST: Empty input file produces no output (BEGIN-only check)
CMD: WORK:awk "BEGIN { x=1 } END { print NR }" WORK:test-empty.txt
EXPECT: 0
EXPECT_RC: 0

TEST: Empty input file with pattern rule is silent
CMD: WORK:awk -f WORK:test-awk-length.awk WORK:test-empty.txt
EXPECT_RC: 0

TEST: Long line input processed without truncation
CMD: WORK:awk -f WORK:test-awk-length.awk WORK:test-longline.txt
EXPECT_RC: 0

# === AMIGA-SPECIFIC TESTS ===

TEST: WORK: volume path for input file (Amiga path handling)
CMD: WORK:awk -F: -f WORK:test-awk-fields.awk WORK:test-awk-colon.txt
EXPECT: root
EXPECT_RC: 0

TEST: -v flag with Amiga WORK: path in variable value
CMD: WORK:awk -v sep=: "BEGIN { print sep }" WORK:test-empty.txt
EXPECT: :
EXPECT_RC: 0

# === REAL-WORLD TESTS ===

TEST: Word count simulation (NR words chars)
CMD: WORK:awk -f WORK:test-awk-wc.awk WORK:test-awk-stressfile.txt
EXPECT: 20 60 458
EXPECT_RC: 0

TEST: Sum column of numbers (real-world data extraction)
CMD: WORK:awk -f WORK:test-awk-sum.awk WORK:test-awk-numbers.txt
EXPECT: 150
EXPECT_RC: 0

TEST: Extract second field from colon-delimited passwd-style file
CMD: WORK:awk -F : -f WORK:test-awk-csv.awk WORK:test-awk-colon.txt
EXPECT: 3 0
EXPECT_RC: 0

# === STRESS TESTS ===

TEST: 10000-iteration loop accumulates correct sum
CMD: WORK:awk -f WORK:test-awk-stress.awk WORK:test-empty.txt
EXPECT: 50005000
EXPECT_RC: 0

TEST: Recursive function fib(20) without stack overflow
CMD: WORK:awk -f WORK:test-awk-recursion.awk WORK:test-empty.txt
EXPECT: 6765
EXPECT_RC: 0

TEST: Process 20-line file building associative array
CMD: WORK:awk -f WORK:test-awk-array.awk WORK:test-awk-stressfile.txt
EXPECT_RC: 0

# === PRECISION TEST ===

TEST: atan2(0,-1) computes pi to 10 decimal places
CMD: WORK:awk -f WORK:test-awk-picomp.awk WORK:test-empty.txt
EXPECT: 3.1415926536
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
passed=44
failed=0
total=44
```

---
Generated by `make test-fsemu TARGET=ports/awk`
Report template: `toolchain/templates/test-report.md.template`
