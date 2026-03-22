# FS-UAE Test Report: grep

## Summary

| Field | Value |
|-------|-------|
| Port | grep |
| Date | 2026-03-22 16:19:07 |
| Duration | 61s |
| Platform | FS-UAE 3.2.35 (A1200, Kickstart 3.1) |
| Binary | `WORK:grep` (63K) |
| Test method | ARexx harness → TAP output |
| Result | **PASS** — 53/53 passed |

## Test Results

```
1..53
ok 1 - Basic pattern match returns first matching line
ok 2 - Case-insensitive match (-i) matches both hello and Hello
ok 3 - Invert match (-v) returns non-matching lines
ok 4 - Count matching lines (-c)
ok 5 - Line numbers (-n) prefix output with line number
ok 6 - Fixed string match (-F) treats dot as literal not regex wildcard
ok 7 - Extended regex (-E) matches character class
ok 8 - Word boundary match (-w) does not match substrings
ok 9 - Print only matching part (-o) prints just the match
ok 10 - Max count (-m 1) stops after first match
ok 11 - List files with matches (-l) prints filename only
ok 12 - List files without matches (-L) prints filename when no match
ok 13 - After-context (-A 1) prints one line after match
ok 14 - Before-context (-B 1) prints one line before match
ok 15 - Combined context (-A 1 -B 1) prints one line before and after match
ok 16 - Pattern from file (-f) reads patterns from file
ok 17 - Explicit pattern with -e flag matches correctly
ok 18 - Whole-line match (-x) only matches complete line
ok 19 - Force filename header (-H) on single-file search
ok 20 - Suppress filename header (-h) in multi-file search
ok 21 - Byte offset (-b) prints byte offset before each match
ok 22 - Quiet mode (-q) produces no output on match
ok 23 - Suppress error messages (-s) on missing file silences stderr
ok 24 - Treat binary as text (-a) processes binary file as text
ok 25 - Binary-files option text treats binary as text (no match in dat = RC 5)
ok 26 - Binary-files option without-match skips binary file (no binary file notice)
ok 27 - Line-buffered output flag does not break normal matching
ok 28 - Null-separated output (--null) with -l produces NUL after filename
ok 29 - Custom label (--label) replaces filename in multi-file output
ok 30 - Anchored pattern match on line start using BRE caret
ok 31 - Flag combination -n -i case-insensitive with line numbers
ok 32 - Flag combination -c -v count of non-matching lines
ok 33 - Multiple -e patterns matches lines with either pattern
ok 34 - Nonexistent file returns RETURN_ERROR (RC=10)
ok 35 - No pattern argument returns RETURN_ERROR
ok 36 - Invalid regex pattern returns RETURN_ERROR
ok 37 - Bad --binary-files value returns RETURN_ERROR
ok 38 - Match found returns RETURN_OK (RC=0)
ok 39 - No match returns RETURN_WARN (RC=5)
ok 40 - Empty file produces no output and returns RETURN_WARN (RC=5)
ok 41 - Nonexistent file returns RETURN_ERROR (RC=10)
ok 42 - Empty file produces no output on any pattern
ok 43 - Binary file prints binary file notice when match found
ok 44 - Very long line (>1024 chars) is processed correctly
ok 45 - Pattern matching special regex chars in -F mode (literal bracket)
ok 46 - Count on empty file returns zero
ok 47 - Whole-line (-x) does not match partial line
ok 48 - Case-insensitive (-i) with -c counts both cases
ok 49 - -o with multiple matches on one line prints each on own line
ok 50 - Amiga WORK: volume path resolves correctly
ok 51 - Amiga path with colon in volume name does not confuse pattern parser
ok 52 - Multiple Amiga WORK: path arguments in one invocation
ok 53 - -H flag prints full WORK: path in multi-file output
# passed: 53 failed: 0 total: 53
```

### Breakdown

| # | Test | Status | Details |
|---|------|--------|---------|
| 1 | Basic pattern match returns first matching line | PASS | |
| 2 | Case-insensitive match (-i) matches both hello and Hello | PASS | |
| 3 | Invert match (-v) returns non-matching lines | PASS | |
| 4 | Count matching lines (-c) | PASS | |
| 5 | Line numbers (-n) prefix output with line number | PASS | |
| 6 | Fixed string match (-F) treats dot as literal not regex wildcard | PASS | |
| 7 | Extended regex (-E) matches character class | PASS | |
| 8 | Word boundary match (-w) does not match substrings | PASS | |
| 9 | Print only matching part (-o) prints just the match | PASS | |
| 10 | Max count (-m 1) stops after first match | PASS | |
| 11 | List files with matches (-l) prints filename only | PASS | |
| 12 | List files without matches (-L) prints filename when no match | PASS | |
| 13 | After-context (-A 1) prints one line after match | PASS | |
| 14 | Before-context (-B 1) prints one line before match | PASS | |
| 15 | Combined context (-A 1 -B 1) prints one line before and after match | PASS | |
| 16 | Pattern from file (-f) reads patterns from file | PASS | |
| 17 | Explicit pattern with -e flag matches correctly | PASS | |
| 18 | Whole-line match (-x) only matches complete line | PASS | |
| 19 | Force filename header (-H) on single-file search | PASS | |
| 20 | Suppress filename header (-h) in multi-file search | PASS | |
| 21 | Byte offset (-b) prints byte offset before each match | PASS | |
| 22 | Quiet mode (-q) produces no output on match | PASS | |
| 23 | Suppress error messages (-s) on missing file silences stderr | PASS | |
| 24 | Treat binary as text (-a) processes binary file as text | PASS | |
| 25 | Binary-files option text treats binary as text (no match in dat = RC 5) | PASS | |
| 26 | Binary-files option without-match skips binary file (no binary file notice) | PASS | |
| 27 | Line-buffered output flag does not break normal matching | PASS | |
| 28 | Null-separated output (--null) with -l produces NUL after filename | PASS | |
| 29 | Custom label (--label) replaces filename in multi-file output | PASS | |
| 30 | Anchored pattern match on line start using BRE caret | PASS | |
| 31 | Flag combination -n -i case-insensitive with line numbers | PASS | |
| 32 | Flag combination -c -v count of non-matching lines | PASS | |
| 33 | Multiple -e patterns matches lines with either pattern | PASS | |
| 34 | Nonexistent file returns RETURN_ERROR (RC=10) | PASS | |
| 35 | No pattern argument returns RETURN_ERROR | PASS | |
| 36 | Invalid regex pattern returns RETURN_ERROR | PASS | |
| 37 | Bad --binary-files value returns RETURN_ERROR | PASS | |
| 38 | Match found returns RETURN_OK (RC=0) | PASS | |
| 39 | No match returns RETURN_WARN (RC=5) | PASS | |
| 40 | Empty file produces no output and returns RETURN_WARN (RC=5) | PASS | |
| 41 | Nonexistent file returns RETURN_ERROR (RC=10) | PASS | |
| 42 | Empty file produces no output on any pattern | PASS | |
| 43 | Binary file prints binary file notice when match found | PASS | |
| 44 | Very long line (>1024 chars) is processed correctly | PASS | |
| 45 | Pattern matching special regex chars in -F mode (literal bracket) | PASS | |
| 46 | Count on empty file returns zero | PASS | |
| 47 | Whole-line (-x) does not match partial line | PASS | |
| 48 | Case-insensitive (-i) with -c counts both cases | PASS | |
| 49 | -o with multiple matches on one line prints each on own line | PASS | |
| 50 | Amiga WORK: volume path resolves correctly | PASS | |
| 51 | Amiga path with colon in volume name does not confuse pattern parser | PASS | |
| 52 | Multiple Amiga WORK: path arguments in one invocation | PASS | |
| 53 | -H flag prints full WORK: path in multi-file output | PASS | |

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
# grep FS-UAE test suite
# OpenBSD grep 1.68 -- Category 1 (CLI), minimum 15 tests required
# Uses input files instead of piping (ARexx limitation)
# optstr: +0123456789A:B:CEFGHILRUVabce:f:hilm:noqrsuvwxy

# -----------------------------------------------------------------------
# CATEGORY 1: FUNCTIONAL TESTS (one test per flag)
# -----------------------------------------------------------------------

TEST: Basic pattern match returns first matching line
CMD: WORK:grep hello WORK:test-grep-input.txt
EXPECT: hello world
EXPECT_RC: 0

TEST: Case-insensitive match (-i) matches both hello and Hello
CMD: WORK:grep -i hello WORK:test-grep-input.txt
EXPECT: hello world
EXPECT_RC: 0

TEST: Invert match (-v) returns non-matching lines
CMD: WORK:grep -v hello WORK:test-grep-input.txt
EXPECT: foo bar
EXPECT_RC: 0

TEST: Count matching lines (-c)
CMD: WORK:grep -c hello WORK:test-grep-input.txt
EXPECT: 1
EXPECT_RC: 0

TEST: Line numbers (-n) prefix output with line number
CMD: WORK:grep -n hello WORK:test-grep-input.txt
EXPECT: 1:hello world
EXPECT_RC: 0

TEST: Fixed string match (-F) treats dot as literal not regex wildcard
CMD: WORK:grep -F a.b WORK:test-grep-input.txt
EXPECT: a.b
EXPECT_RC: 0

TEST: Extended regex (-E) matches character class
CMD: WORK:grep -E ca[rt] WORK:test-grep-input.txt
EXPECT: cat
EXPECT_RC: 0

TEST: Word boundary match (-w) does not match substrings
CMD: WORK:grep -w cat WORK:test-grep-input.txt
EXPECT: cat
EXPECT_RC: 0

TEST: Print only matching part (-o) prints just the match
CMD: WORK:grep -o hello WORK:test-grep-input.txt
EXPECT: hello
EXPECT_RC: 0

TEST: Max count (-m 1) stops after first match
CMD: WORK:grep -m 1 hello WORK:test-grep-input.txt
EXPECT: hello world
EXPECT_RC: 0

TEST: List files with matches (-l) prints filename only
CMD: WORK:grep -l hello WORK:test-grep-input.txt
EXPECT: WORK:test-grep-input.txt
EXPECT_RC: 0

TEST: List files without matches (-L) prints filename when no match
CMD: WORK:grep -L NOMATCH WORK:test-grep-input.txt
EXPECT: WORK:test-grep-input.txt
EXPECT_RC: 5

TEST: After-context (-A 1) prints one line after match
CMD: WORK:grep -A 1 TARGET WORK:test-grep-context.txt
EXPECT: TARGET LINE
EXPECT_RC: 0

TEST: Before-context (-B 1) prints one line before match
CMD: WORK:grep -B 1 TARGET WORK:test-grep-context.txt
EXPECT_CONTAINS: before two
EXPECT_RC: 0

TEST: Combined context (-A 1 -B 1) prints one line before and after match
CMD: WORK:grep -A 1 -B 1 TARGET WORK:test-grep-context.txt
EXPECT_CONTAINS: before two
EXPECT_RC: 0

TEST: Pattern from file (-f) reads patterns from file
CMD: WORK:grep -f WORK:test-grep-patterns.txt WORK:test-grep-input.txt
EXPECT: foo bar
EXPECT_RC: 0

TEST: Explicit pattern with -e flag matches correctly
CMD: WORK:grep -e hello WORK:test-grep-input.txt
EXPECT: hello world
EXPECT_RC: 0

TEST: Whole-line match (-x) only matches complete line
CMD: WORK:grep -x "exactly this line" WORK:test-grep-xflag.txt
EXPECT: exactly this line
EXPECT_RC: 0

TEST: Force filename header (-H) on single-file search
CMD: WORK:grep -H hello WORK:test-grep-input.txt
EXPECT: WORK:test-grep-input.txt:hello world
EXPECT_RC: 0

TEST: Suppress filename header (-h) in multi-file search
CMD: WORK:grep -h match WORK:test-grep-multifile-a.txt WORK:test-grep-multifile-b.txt
EXPECT: match in file A
EXPECT_RC: 0

TEST: Byte offset (-b) prints byte offset before each match
CMD: WORK:grep -b hello WORK:test-grep-input.txt
EXPECT: 0:hello world
EXPECT_RC: 0

TEST: Quiet mode (-q) produces no output on match
CMD: WORK:grep -q hello WORK:test-grep-input.txt
EXPECT:
EXPECT_RC: 0

TEST: Suppress error messages (-s) on missing file silences stderr
CMD: WORK:grep -s hello WORK:nosuchfile-grep2.txt
EXPECT:
EXPECT_RC: 10

TEST: Treat binary as text (-a) processes binary file as text
CMD: WORK:grep -a hello WORK:test-grep-input.txt
EXPECT: hello world
EXPECT_RC: 0

TEST: Binary-files option text treats binary as text (no match in dat = RC 5)
CMD: WORK:grep --binary-files=text hello WORK:test-grep-binary.dat
EXPECT_RC: 5

TEST: Binary-files option without-match skips binary file (no binary file notice)
CMD: WORK:grep --binary-files=without-match hello WORK:test-grep-binary.dat
EXPECT_RC: 5

TEST: Line-buffered output flag does not break normal matching
CMD: WORK:grep --line-buffered hello WORK:test-grep-input.txt
EXPECT: hello world
EXPECT_RC: 0

TEST: Null-separated output (--null) with -l produces NUL after filename
CMD: WORK:grep -l --null hello WORK:test-grep-input.txt
EXPECT_RC: 0

TEST: Custom label (--label) replaces filename in multi-file output
CMD: WORK:grep -H --label=MYFILE hello WORK:test-grep-input.txt
EXPECT_CONTAINS: MYFILE:hello world
EXPECT_RC: 0

TEST: Anchored pattern match on line start using BRE caret
CMD: WORK:grep "^start" WORK:test-grep-anchored.txt
EXPECT: start of line match
EXPECT_RC: 0

TEST: Flag combination -n -i case-insensitive with line numbers
CMD: WORK:grep -n -i hello WORK:test-grep-input.txt
EXPECT: 1:hello world
EXPECT_RC: 0

TEST: Flag combination -c -v count of non-matching lines
CMD: WORK:grep -c -v hello WORK:test-grep-input.txt
EXPECT_RC: 0

TEST: Multiple -e patterns matches lines with either pattern
CMD: WORK:grep -e hello -e "quick brown" WORK:test-grep-input.txt
EXPECT: hello world
EXPECT_RC: 0

# -----------------------------------------------------------------------
# CATEGORY 2: ERROR PATH TESTS
# -----------------------------------------------------------------------

TEST: Nonexistent file returns RETURN_ERROR (RC=10)
CMD: WORK:grep hello WORK:nosuchfile-grep.txt
EXPECT_RC: 10

TEST: No pattern argument returns RETURN_ERROR
CMD: WORK:grep
EXPECT_RC: 10

TEST: Invalid regex pattern returns RETURN_ERROR
CMD: WORK:grep "[invalid" WORK:test-grep-input.txt
EXPECT_RC: 10

TEST: Bad --binary-files value returns RETURN_ERROR
CMD: WORK:grep --binary-files=badvalue hello WORK:test-grep-input.txt
EXPECT_RC: 10

# -----------------------------------------------------------------------
# CATEGORY 3: EXIT CODE TESTS
# -----------------------------------------------------------------------

TEST: Match found returns RETURN_OK (RC=0)
CMD: WORK:grep hello WORK:test-grep-input.txt
EXPECT_RC: 0

TEST: No match returns RETURN_WARN (RC=5)
CMD: WORK:grep NOMATCHSTRING WORK:test-grep-input.txt
EXPECT_RC: 5

TEST: Empty file produces no output and returns RETURN_WARN (RC=5)
CMD: WORK:grep hello WORK:test-grep-empty.txt
EXPECT_RC: 5

TEST: Nonexistent file returns RETURN_ERROR (RC=10)
CMD: WORK:grep hello WORK:nosuchfile-grep3.txt
EXPECT_RC: 10

# -----------------------------------------------------------------------
# CATEGORY 4: EDGE CASE TESTS
# -----------------------------------------------------------------------

TEST: Empty file produces no output on any pattern
CMD: WORK:grep pattern WORK:test-grep-empty.txt
EXPECT:
EXPECT_RC: 5

TEST: Binary file prints binary file notice when match found
CMD: WORK:grep -U a WORK:test-grep-binary.dat
EXPECT_CONTAINS: Binary file
EXPECT_RC: 0

TEST: Very long line (>1024 chars) is processed correctly
CMD: WORK:grep MARKER WORK:test-longline.txt
EXPECT_CONTAINS: MARKER
EXPECT_RC: 0

TEST: Pattern matching special regex chars in -F mode (literal bracket)
CMD: WORK:grep -F "special.chars[here]" WORK:test-grep-input.txt
EXPECT: line eight: special.chars[here]
EXPECT_RC: 0

TEST: Count on empty file returns zero
CMD: WORK:grep -c hello WORK:test-grep-empty.txt
EXPECT: 0
EXPECT_RC: 5

TEST: Whole-line (-x) does not match partial line
CMD: WORK:grep -x hello WORK:test-grep-input.txt
EXPECT_RC: 5

TEST: Case-insensitive (-i) with -c counts both cases
CMD: WORK:grep -c -i hello WORK:test-grep-input.txt
EXPECT: 2
EXPECT_RC: 0

TEST: -o with multiple matches on one line prints each on own line
CMD: WORK:grep -o matches WORK:test-grep-input.txt
EXPECT_CONTAINS: matches
EXPECT_RC: 0

# -----------------------------------------------------------------------
# CATEGORY 5: AMIGA-SPECIFIC TESTS
# -----------------------------------------------------------------------

TEST: Amiga WORK: volume path resolves correctly
CMD: WORK:grep quick WORK:test-grep-input.txt
EXPECT: the quick brown fox
EXPECT_RC: 0

TEST: Amiga path with colon in volume name does not confuse pattern parser
CMD: WORK:grep "line eight" WORK:test-grep-input.txt
EXPECT: line eight: special.chars[here]
EXPECT_RC: 0

TEST: Multiple Amiga WORK: path arguments in one invocation
CMD: WORK:grep match WORK:test-grep-multifile-a.txt WORK:test-grep-multifile-b.txt
EXPECT_CONTAINS: match in file A
EXPECT_RC: 0

TEST: -H flag prints full WORK: path in multi-file output
CMD: WORK:grep -H match WORK:test-grep-multifile-a.txt WORK:test-grep-multifile-b.txt
EXPECT_CONTAINS: WORK:test-grep-multifile-a.txt:match in file A
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
passed=53
failed=0
total=53
```

---
Generated by `make test-fsemu TARGET=ports/grep`
Report template: `toolchain/templates/test-report.md.template`
