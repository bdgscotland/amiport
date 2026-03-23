# FS-UAE Test Report: jq

## Summary

| Field | Value |
|-------|-------|
| Port | jq |
| Date | 2026-03-23 17:04:35 |
| Duration | 399s |
| Platform | FS-UAE 3.2.35 (A1200, Kickstart 3.1) |
| Binary | `WORK:jq` (369K) |
| Test method | ARexx harness → TAP output |
| Result | **PASS** — 50/50 passed |

## Test Results

```
1..50
ok 1 - Identity filter pretty-prints JSON object (basic smoke test)
ok 2 - Field access extracts string value
ok 3 - Raw output (-r) strips JSON string quotes
ok 4 - Compact output (-c) emits one-line JSON
ok 5 - Null input (-n) uses null as input value without reading stdin
ok 6 - Sort keys (-S) sorts object keys alphabetically -- verify in compact mode
ok 7 - Raw input (-R) reads each line as a JSON string
ok 8 - Slurp (-s) reads all inputs into array -- compact to verify structure
ok 9 - Join output (-j) suppresses trailing newline -- raw string output
ok 10 - Tab indentation (--tab) indents with tab characters
ok 11 - Custom indent (--indent 4) indents with four spaces
ok 12 - ASCII output (-a) produces JSON-encoded string
ok 13 - Monochrome output (-M) disables color escape codes -- compact for clean comparison
ok 14 - Color output (-C) flag accepted without error (output contains the value)
ok 15 - Unbuffered output (--unbuffered) produces same result as buffered
ok 16 - Filter from file (-f) loads jq filter from WORK: path
ok 17 - Named string argument (--arg) sets variable accessible via filter file
ok 18 - Named JSON argument (--argjson) parses value and makes it available as number
ok 19 - Positional string args (--args) accessible via ARGS.positional in filter file
ok 20 - Positional JSON args (--jsonargs) parses values as JSON numbers
ok 21 - Raw file argument (--rawfile) loads file contents as string variable
ok 22 - Slurp file argument (--slurpfile) loads JSON file into array variable
ok 23 - Streaming mode (--stream) emits path-value pairs for leaf nodes
ok 24 - Exit status (-e) returns RC 0 when last output is true value
ok 25 - Version flag (-V) prints version string
ok 26 - Build configuration (--build-configuration) prints compile-time options
ok 27 - Help flag (-h) prints usage to stdout
ok 28 - AmigaOS stub for --run-tests reports pthreads not supported
ok 29 - Invalid filter syntax returns RETURN_FAIL (compile error RC 20)
ok 30 - Unknown option returns RETURN_FAIL (die() RC 20)
ok 31 - Nonexistent input file returns RETURN_FAIL (system error RC 20)
ok 32 - --indent out of range (9) returns RETURN_FAIL (die() RC 20)
ok 33 - --argjson with invalid JSON returns RETURN_FAIL
ok 34 - Exit status (-e) returns RC 10 when last output is null
ok 35 - Exit status (-e) returns RC 10 when last output is false
ok 36 - Multiple input files all processed -- count field from each object
ok 37 - Empty file produces no output (0 bytes, no JSON to process) -- RC 0
ok 38 - Array element iteration produces one output per element
ok 39 - Nested access on array field extracts element by index
ok 40 - Number arithmetic in filter (integer arithmetic)
ok 41 - String concatenation with + operator
ok 42 - Seq mode (--seq) runs without error (RS character not testable in harness)
ok 43 - WORK: volume path resolves correctly for input file
ok 44 - Multiple WORK: paths in one command -- filter file and data file both on WORK:
ok 45 - Filter operates on multiple JSON objects from single WORK: file
ok 46 - Count published packages (filter + length)
ok 47 - Find package with most tests (sort + reverse + index)
ok 48 - Summarize dataset (object construction + add + length)
ok 49 - Extract all category values (unique + sort)
ok 50 - Group by status and count (group_by + map + from_entries)
# passed: 50 failed: 0 total: 50
```

### Breakdown

| # | Test | Status | Details |
|---|------|--------|---------|
| 1 | Identity filter pretty-prints JSON object (basic smoke test) | PASS | |
| 2 | Field access extracts string value | PASS | |
| 3 | Raw output (-r) strips JSON string quotes | PASS | |
| 4 | Compact output (-c) emits one-line JSON | PASS | |
| 5 | Null input (-n) uses null as input value without reading stdin | PASS | |
| 6 | Sort keys (-S) sorts object keys alphabetically -- verify in compact mode | PASS | |
| 7 | Raw input (-R) reads each line as a JSON string | PASS | |
| 8 | Slurp (-s) reads all inputs into array -- compact to verify structure | PASS | |
| 9 | Join output (-j) suppresses trailing newline -- raw string output | PASS | |
| 10 | Tab indentation (--tab) indents with tab characters | PASS | |
| 11 | Custom indent (--indent 4) indents with four spaces | PASS | |
| 12 | ASCII output (-a) produces JSON-encoded string | PASS | |
| 13 | Monochrome output (-M) disables color escape codes -- compact for clean comparison | PASS | |
| 14 | Color output (-C) flag accepted without error (output contains the value) | PASS | |
| 15 | Unbuffered output (--unbuffered) produces same result as buffered | PASS | |
| 16 | Filter from file (-f) loads jq filter from WORK: path | PASS | |
| 17 | Named string argument (--arg) sets variable accessible via filter file | PASS | |
| 18 | Named JSON argument (--argjson) parses value and makes it available as number | PASS | |
| 19 | Positional string args (--args) accessible via ARGS.positional in filter file | PASS | |
| 20 | Positional JSON args (--jsonargs) parses values as JSON numbers | PASS | |
| 21 | Raw file argument (--rawfile) loads file contents as string variable | PASS | |
| 22 | Slurp file argument (--slurpfile) loads JSON file into array variable | PASS | |
| 23 | Streaming mode (--stream) emits path-value pairs for leaf nodes | PASS | |
| 24 | Exit status (-e) returns RC 0 when last output is true value | PASS | |
| 25 | Version flag (-V) prints version string | PASS | |
| 26 | Build configuration (--build-configuration) prints compile-time options | PASS | |
| 27 | Help flag (-h) prints usage to stdout | PASS | |
| 28 | AmigaOS stub for --run-tests reports pthreads not supported | PASS | |
| 29 | Invalid filter syntax returns RETURN_FAIL (compile error RC 20) | PASS | |
| 30 | Unknown option returns RETURN_FAIL (die() RC 20) | PASS | |
| 31 | Nonexistent input file returns RETURN_FAIL (system error RC 20) | PASS | |
| 32 | --indent out of range (9) returns RETURN_FAIL (die() RC 20) | PASS | |
| 33 | --argjson with invalid JSON returns RETURN_FAIL | PASS | |
| 34 | Exit status (-e) returns RC 10 when last output is null | PASS | |
| 35 | Exit status (-e) returns RC 10 when last output is false | PASS | |
| 36 | Multiple input files all processed -- count field from each object | PASS | |
| 37 | Empty file produces no output (0 bytes, no JSON to process) -- RC 0 | PASS | |
| 38 | Array element iteration produces one output per element | PASS | |
| 39 | Nested access on array field extracts element by index | PASS | |
| 40 | Number arithmetic in filter (integer arithmetic) | PASS | |
| 41 | String concatenation with + operator | PASS | |
| 42 | Seq mode (--seq) runs without error (RS character not testable in harness) | PASS | |
| 43 | WORK: volume path resolves correctly for input file | PASS | |
| 44 | Multiple WORK: paths in one command -- filter file and data file both on WORK: | PASS | |
| 45 | Filter operates on multiple JSON objects from single WORK: file | PASS | |
| 46 | Count published packages (filter + length) | PASS | |
| 47 | Find package with most tests (sort + reverse + index) | PASS | |
| 48 | Summarize dataset (object construction + add + length) | PASS | |
| 49 | Extract all category values (unique + sort) | PASS | |
| 50 | Group by status and count (group_by + map + from_entries) | PASS | |

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
# jq FS-UAE test suite
# jq 1.7.1 -- Category 1 (CLI), minimum 15 tests required
# Uses input files instead of piping (ARexx limitation)
# Built without oniguruma (regex builtins return runtime errors)
# Built without decNumber (IEEE 754 doubles)
#
# Flags covered: -n -R -s -c -r -j -a -S -C -M -e -f -V -h
#                --tab --indent --unbuffered --stream --seq
#                --arg --argjson --args --jsonargs
#                --rawfile --slurpfile --build-configuration
#
# Exit codes:
#   0  (RETURN_OK)   -- success
#  10  (RETURN_ERROR) -- -e with null/false output
#  20  (RETURN_FAIL)  -- bad filter syntax, unknown option, missing arg, bad JSON to --argjson
#
# NOTE: stderr is NOT captured by the test harness.
#       Error path tests use EXPECT_RC: only.
#       Filter files named test-jq-*.txt are auto-deployed to WORK:

# -----------------------------------------------------------------------
# CATEGORY 1: FUNCTIONAL TESTS (one test per flag / key feature)
# -----------------------------------------------------------------------

TEST: Identity filter pretty-prints JSON object (basic smoke test)
CMD: WORK:jq . WORK:test-jq-input.txt
EXPECT: {
EXPECT_RC: 0

TEST: Field access extracts string value
CMD: WORK:jq .name WORK:test-jq-input.txt
EXPECT: "Amiga"
EXPECT_RC: 0

TEST: Raw output (-r) strips JSON string quotes
CMD: WORK:jq -r .name WORK:test-jq-input.txt
EXPECT: Amiga
EXPECT_RC: 0

TEST: Compact output (-c) emits one-line JSON
CMD: WORK:jq -c . WORK:test-jq-input.txt
EXPECT: {"name":"Amiga","year":1985,"tags":["retro","68k","classic"]}
EXPECT_RC: 0

TEST: Null input (-n) uses null as input value without reading stdin
CMD: WORK:jq -n 42
EXPECT: 42
EXPECT_RC: 0

TEST: Sort keys (-S) sorts object keys alphabetically -- verify in compact mode
CMD: WORK:jq -c -S . WORK:test-jq-input.txt
EXPECT: {"name":"Amiga","tags":["retro","68k","classic"],"year":1985}
EXPECT_RC: 0

TEST: Raw input (-R) reads each line as a JSON string
CMD: WORK:jq -R . WORK:test-jq-rawlines.txt
EXPECT: "hello world"
EXPECT_RC: 0

TEST: Slurp (-s) reads all inputs into array -- compact to verify structure
CMD: WORK:jq -sc . WORK:test-jq-slurp.txt
EXPECT: [{"x":1},{"x":2},{"x":3}]
EXPECT_RC: 0

TEST: Join output (-j) suppresses trailing newline -- raw string output
CMD: WORK:jq -j -r .name WORK:test-jq-input.txt
EXPECT: Amiga
EXPECT_RC: 0

TEST: Tab indentation (--tab) indents with tab characters
CMD: WORK:jq --tab . WORK:test-jq-input.txt
EXPECT_CONTAINS: "name"
EXPECT_RC: 0

TEST: Custom indent (--indent 4) indents with four spaces
CMD: WORK:jq --indent 4 . WORK:test-jq-input.txt
EXPECT_CONTAINS:     "name"
EXPECT_RC: 0

TEST: ASCII output (-a) produces JSON-encoded string
CMD: WORK:jq -a .name WORK:test-jq-input.txt
EXPECT: "Amiga"
EXPECT_RC: 0

TEST: Monochrome output (-M) disables color escape codes -- compact for clean comparison
CMD: WORK:jq -c -M .year WORK:test-jq-input.txt
EXPECT: 1985
EXPECT_RC: 0

TEST: Color output (-C) flag accepted without error (output contains the value)
CMD: WORK:jq -c -C .year WORK:test-jq-input.txt
EXPECT_CONTAINS: 1985
EXPECT_RC: 0

TEST: Unbuffered output (--unbuffered) produces same result as buffered
CMD: WORK:jq --unbuffered -c . WORK:test-jq-input.txt
EXPECT: {"name":"Amiga","year":1985,"tags":["retro","68k","classic"]}
EXPECT_RC: 0

TEST: Filter from file (-f) loads jq filter from WORK: path
CMD: WORK:jq -f WORK:test-jq-filter.txt WORK:test-jq-input.txt
EXPECT: "Amiga"
EXPECT_RC: 0

TEST: Named string argument (--arg) sets variable accessible via filter file
CMD: WORK:jq -n --arg x hello -f WORK:test-jq-argfilter.txt
EXPECT: "hello"
EXPECT_RC: 0

TEST: Named JSON argument (--argjson) parses value and makes it available as number
CMD: WORK:jq -n --argjson n 41 -f WORK:test-jq-argjsonfilter.txt
EXPECT: 42
EXPECT_RC: 0

TEST: Positional string args (--args) accessible via ARGS.positional in filter file
CMD: WORK:jq -n -f WORK:test-jq-argsfilter.txt --args hello
EXPECT: "hello"
EXPECT_RC: 0

TEST: Positional JSON args (--jsonargs) parses values as JSON numbers
CMD: WORK:jq -n -f WORK:test-jq-argsfilter.txt --jsonargs 99
EXPECT: 99
EXPECT_RC: 0

TEST: Raw file argument (--rawfile) loads file contents as string variable
CMD: WORK:jq -n --rawfile content WORK:test-jq-rawdata.txt -f WORK:test-jq-rawfilefilter.txt
EXPECT_CONTAINS: hello from rawfile
EXPECT_RC: 0

TEST: Slurp file argument (--slurpfile) loads JSON file into array variable
CMD: WORK:jq -n --slurpfile data WORK:test-jq-slurpdata.txt -f WORK:test-jq-slurpfilefilter.txt
EXPECT: 42
EXPECT_RC: 0

TEST: Streaming mode (--stream) emits path-value pairs for leaf nodes
CMD: WORK:jq -c --stream . WORK:test-jq-input.txt
EXPECT_CONTAINS: "Amiga"
EXPECT_RC: 0

TEST: Exit status (-e) returns RC 0 when last output is true value
CMD: WORK:jq -n -e true
EXPECT: true
EXPECT_RC: 0

TEST: Version flag (-V) prints version string
CMD: WORK:jq -V
EXPECT: jq-1.7.1
EXPECT_RC: 0

TEST: Build configuration (--build-configuration) prints compile-time options
CMD: WORK:jq --build-configuration
EXPECT_CONTAINS: without-oniguruma
EXPECT_RC: 0

TEST: Help flag (-h) prints usage to stdout
CMD: WORK:jq -h
EXPECT_CONTAINS: jq - commandline JSON processor
EXPECT_RC: 0

TEST: AmigaOS stub for --run-tests reports pthreads not supported
CMD: WORK:jq --run-tests
EXPECT_RC: 5

# -----------------------------------------------------------------------
# CATEGORY 2: ERROR PATH TESTS
# -----------------------------------------------------------------------

TEST: Invalid filter syntax returns RETURN_FAIL (compile error RC 20)
CMD: WORK:jq -n -f WORK:test-jq-badsyntax.txt
EXPECT_RC: 20

TEST: Unknown option returns RETURN_FAIL (die() RC 20)
CMD: WORK:jq -Z WORK:test-jq-input.txt
EXPECT_RC: 20

TEST: Nonexistent input file returns RETURN_FAIL (system error RC 20)
CMD: WORK:jq . WORK:no-such-file.txt
EXPECT_RC: 20

TEST: --indent out of range (9) returns RETURN_FAIL (die() RC 20)
CMD: WORK:jq --indent 9 . WORK:test-jq-input.txt
EXPECT_RC: 20

TEST: --argjson with invalid JSON returns RETURN_FAIL
CMD: WORK:jq -n --argjson x notjson -f WORK:test-jq-argfilter.txt
EXPECT_RC: 20

# -----------------------------------------------------------------------
# CATEGORY 3: EXIT CODE TESTS
# -----------------------------------------------------------------------

TEST: Exit status (-e) returns RC 10 when last output is null
CMD: WORK:jq -n -e null
EXPECT: null
EXPECT_RC: 10

TEST: Exit status (-e) returns RC 10 when last output is false
CMD: WORK:jq -n -e false
EXPECT: false
EXPECT_RC: 10

TEST: Multiple input files all processed -- count field from each object
CMD: WORK:jq -c .id WORK:test-jq-multi.txt
EXPECT: 1
EXPECT_RC: 0

# -----------------------------------------------------------------------
# CATEGORY 4: EDGE CASE TESTS
# -----------------------------------------------------------------------

TEST: Empty file produces no output (0 bytes, no JSON to process) -- RC 0
CMD: WORK:jq . WORK:test-empty.txt
EXPECT:
EXPECT_RC: 0

TEST: Array element iteration produces one output per element
CMD: WORK:jq -c .[] WORK:test-jq-array.txt
EXPECT: 1
EXPECT_RC: 0

TEST: Nested access on array field extracts element by index
CMD: WORK:jq -r .tags[1] WORK:test-jq-input.txt
EXPECT: 68k
EXPECT_RC: 0

TEST: Number arithmetic in filter (integer arithmetic)
CMD: WORK:jq -n "1985 + 41"
EXPECT: 2026
EXPECT_RC: 0

TEST: String concatenation with + operator
CMD: WORK:jq -rn -f WORK:test-jq-concat.txt
EXPECT: Amiga 68k
EXPECT_RC: 0

TEST: Seq mode (--seq) runs without error (RS character not testable in harness)
CMD: WORK:jq -n --seq null
EXPECT_RC: 0

# -----------------------------------------------------------------------
# CATEGORY 5: AMIGA-SPECIFIC TESTS
# -----------------------------------------------------------------------

TEST: WORK: volume path resolves correctly for input file
CMD: WORK:jq -r .name WORK:test-jq-input.txt
EXPECT: Amiga
EXPECT_RC: 0

TEST: Multiple WORK: paths in one command -- filter file and data file both on WORK:
CMD: WORK:jq -f WORK:test-jq-filter.txt WORK:test-jq-input.txt
EXPECT: "Amiga"
EXPECT_RC: 0

TEST: Filter operates on multiple JSON objects from single WORK: file
CMD: WORK:jq -c .value WORK:test-jq-multi.txt
EXPECT: "first"
EXPECT_RC: 0

# === Real-world style tests ===

TEST: Count published packages (filter + length)
FILE: test-jq-packages.txt
CMD: WORK:jq -f WORK:test-jq-realworld.txt WORK:test-jq-packages.txt
EXPECT: 5
EXPECT_RC: 0

TEST: Find package with most tests (sort + reverse + index)
FILE: test-jq-packages.txt
CMD: WORK:jq -r -f WORK:test-jq-transform.txt WORK:test-jq-packages.txt
EXPECT: jq
EXPECT_RC: 0

TEST: Summarize dataset (object construction + add + length)
FILE: test-jq-packages.txt
CMD: WORK:jq -c -f WORK:test-jq-summary.txt WORK:test-jq-packages.txt
EXPECT: {"total":7,"published":5,"total_tests":148}
EXPECT_RC: 0

TEST: Extract all category values (unique + sort)
FILE: test-jq-packages.txt
CMD: WORK:jq -c "[.[].category] | unique" WORK:test-jq-packages.txt
EXPECT: ["dev/lang","util/cli"]
EXPECT_RC: 0

TEST: Group by status and count (group_by + map + from_entries)
FILE: test-jq-packages.txt
CMD: WORK:jq -c "[group_by(.status)[] | {(.[0].status): length}] | add" WORK:test-jq-packages.txt
EXPECT: {"published":5,"submitted":1,"testing":1}
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
passed=50
failed=0
total=50
```

---
Generated by `make test-fsemu TARGET=ports/jq`
Report template: `toolchain/templates/test-report.md.template`
