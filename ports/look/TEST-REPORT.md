# FS-UAE Test Report: look

## Summary

| Field | Value |
|-------|-------|
| Port | look |
| Date | 2026-03-26 19:41:15 |
| Duration | 35s |
| Platform | FS-UAE 3.2.35 (A1200, Kickstart 3.1) |
| Binary | `WORK:look` (38K) |
| Test method | ARexx harness → TAP output |
| Result | **PASS** — 36/36 passed |

## Test Results

```
1..36
ok 1 - Basic lookup finds line with exact prefix
ok 2 - -d flag ignores non-alphanumeric chars in comparison
ok 3 - -f flag matches case-insensitively
ok 4 - -t flag truncates search string at termination character
ok 5 - -t flag with mid-string termchar returns all matching lines
ok 6 - Combined -df flag matches case-insensitively and ignores punctuation
ok 7 - Combined -df flag on pure alphanumeric dict entries
ok 8 - Prefix match returns all lines beginning with prefix
ok 9 - Single-character prefix returns all matching lines
ok 10 - Two-character prefix matches multiple words
ok 11 - Prefix match for l returns lemon and lime
ok 12 - Found string returns exit code 0 (RETURN_OK)
ok 13 - String not found returns exit code 5 (RETURN_WARN)
ok 14 - Invalid flag returns exit code 20 (RETURN_FAIL)
ok 15 - Missing file returns exit code 20 (RETURN_FAIL)
ok 16 - No arguments returns exit code 20 (RETURN_FAIL)
ok 17 - Too many arguments returns exit code 20
ok 18 - Empty file returns error (cannot read empty file)
ok 19 - Single-line file matches only line
ok 20 - Single-line file returns 5 when string not present
ok 21 - First line of file is found correctly
ok 22 - Last line of file is found correctly
ok 23 - -t colon truncates string at colon for key:value file lookup
ok 24 - -f flag finds uppercase entry when searching lowercase
ok 25 - -f flag returns 5 when no case-insensitive match exists
ok 26 - Amiga WORK: volume paths work for file argument
ok 27 - Missing default words file returns exit code 20
ok 28 - Real-world case-insensitive prefix search returns multiple results
ok 29 - Real-world key lookup in colon-delimited sorted data
ok 30 - Real-world -df flag on dictionary entries with punctuation
ok 31 - Binary search finds prefix in middle of 192-entry sorted file
ok 32 - Binary search finds entries at start of 192-entry file
ok 33 - Binary search finds entries at end of 192-entry file
ok 34 - Binary search on 192-entry file returns 5 when no match
ok 35 - Exact full-string match finds single entry in 192-entry file
ok 36 - -t flag precision - terminates at first r in apricot leaving apr prefix
# passed: 36 failed: 0 total: 36
```

### Breakdown

| # | Test | Status | Details |
|---|------|--------|---------|
| 1 | Basic lookup finds line with exact prefix | PASS | |
| 2 | -d flag ignores non-alphanumeric chars in comparison | PASS | |
| 3 | -f flag matches case-insensitively | PASS | |
| 4 | -t flag truncates search string at termination character | PASS | |
| 5 | -t flag with mid-string termchar returns all matching lines | PASS | |
| 6 | Combined -df flag matches case-insensitively and ignores punctuation | PASS | |
| 7 | Combined -df flag on pure alphanumeric dict entries | PASS | |
| 8 | Prefix match returns all lines beginning with prefix | PASS | |
| 9 | Single-character prefix returns all matching lines | PASS | |
| 10 | Two-character prefix matches multiple words | PASS | |
| 11 | Prefix match for l returns lemon and lime | PASS | |
| 12 | Found string returns exit code 0 (RETURN_OK) | PASS | |
| 13 | String not found returns exit code 5 (RETURN_WARN) | PASS | |
| 14 | Invalid flag returns exit code 20 (RETURN_FAIL) | PASS | |
| 15 | Missing file returns exit code 20 (RETURN_FAIL) | PASS | |
| 16 | No arguments returns exit code 20 (RETURN_FAIL) | PASS | |
| 17 | Too many arguments returns exit code 20 | PASS | |
| 18 | Empty file returns error (cannot read empty file) | PASS | |
| 19 | Single-line file matches only line | PASS | |
| 20 | Single-line file returns 5 when string not present | PASS | |
| 21 | First line of file is found correctly | PASS | |
| 22 | Last line of file is found correctly | PASS | |
| 23 | -t colon truncates string at colon for key:value file lookup | PASS | |
| 24 | -f flag finds uppercase entry when searching lowercase | PASS | |
| 25 | -f flag returns 5 when no case-insensitive match exists | PASS | |
| 26 | Amiga WORK: volume paths work for file argument | PASS | |
| 27 | Missing default words file returns exit code 20 | PASS | |
| 28 | Real-world case-insensitive prefix search returns multiple results | PASS | |
| 29 | Real-world key lookup in colon-delimited sorted data | PASS | |
| 30 | Real-world -df flag on dictionary entries with punctuation | PASS | |
| 31 | Binary search finds prefix in middle of 192-entry sorted file | PASS | |
| 32 | Binary search finds entries at start of 192-entry file | PASS | |
| 33 | Binary search finds entries at end of 192-entry file | PASS | |
| 34 | Binary search on 192-entry file returns 5 when no match | PASS | |
| 35 | Exact full-string match finds single entry in 192-entry file | PASS | |
| 36 | -t flag precision - terminates at first r in apricot leaving apr prefix | PASS | |

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
# Test suite for look 1.27
# look -- display lines beginning with a given string from a sorted file
# Category 1 (CLI tool) -- minimum 15 tests
# All EXPECT: values derived by running native look on macOS.

# ---------------------------------------------------------------------------
# 1. FUNCTIONAL TESTS -- one per flag/option (-d, -f, -t)
# ---------------------------------------------------------------------------

# Native: look apple test-look-sorted.txt | head -1
TEST: Basic lookup finds line with exact prefix
CMD: WORK:look apple WORK:test-look-sorted.txt
EXPECT: apple
EXPECT_RC: 0

# Native: look -d apple test-look-dict.txt | head -1
TEST: -d flag ignores non-alphanumeric chars in comparison
CMD: WORK:look -d apple WORK:test-look-dict.txt
EXPECT: apple-pie
EXPECT_RC: 0

# Native: look -f APPLE test-look-sorted.txt | head -1
TEST: -f flag matches case-insensitively
CMD: WORK:look -f APPLE WORK:test-look-sorted.txt
EXPECT: apple
EXPECT_RC: 0

# Native: look -t e apple test-look-sorted.txt
# -t e truncates string after first 'e' -> searches "apple" (e is at pos 4)
TEST: -t flag truncates search string at termination character
CMD: WORK:look -t e apple WORK:test-look-sorted.txt
EXPECT: apple
EXPECT_RC: 0

# Native: look -t a banana test-look-sorted.txt
# -t a truncates "banana" after first 'a' -> "ba" -> matches banana
TEST: -t flag with mid-string termchar returns all matching lines
CMD: WORK:look -t a banana WORK:test-look-sorted.txt
EXPECT: banana
EXPECT_RC: 0

# Native: look -df apple test-look-casedict.txt | head -1
TEST: Combined -df flag matches case-insensitively and ignores punctuation
CMD: WORK:look -df apple WORK:test-look-casedict.txt
EXPECT: Apple-Tree
EXPECT_RC: 0

# Native: look -df apple test-look-dict.txt | head -1
TEST: Combined -df flag on pure alphanumeric dict entries
CMD: WORK:look -df apple WORK:test-look-dict.txt
EXPECT: apple-pie
EXPECT_RC: 0

# ---------------------------------------------------------------------------
# 2. MULTI-LINE OUTPUT VERIFICATION
# ---------------------------------------------------------------------------

# Native: look gra test-look-sorted.txt
# Returns: grape, grapefruit
TEST: Prefix match returns all lines beginning with prefix
CMD: WORK:look gra WORK:test-look-sorted.txt
EXPECT: grape
EXPECT_LINE: 2,grapefruit
EXPECT_RC: 0

# Native: look p test-look-sorted.txt
# Returns: peach, pear, plum
TEST: Single-character prefix returns all matching lines
CMD: WORK:look p WORK:test-look-sorted.txt
EXPECT: peach
EXPECT_LINE: 2,pear
EXPECT_LINE: 3,plum
EXPECT_RC: 0

# Native: look ap test-look-sorted.txt
# Returns: apple, apricot
TEST: Two-character prefix matches multiple words
CMD: WORK:look ap WORK:test-look-sorted.txt
EXPECT: apple
EXPECT_LINE: 2,apricot
EXPECT_RC: 0

# Native: look l test-look-sorted.txt
# Returns: lemon, lime
TEST: Prefix match for l returns lemon and lime
CMD: WORK:look l WORK:test-look-sorted.txt
EXPECT: lemon
EXPECT_LINE: 2,lime
EXPECT_RC: 0

# ---------------------------------------------------------------------------
# 3. EXIT CODE TESTS
# ---------------------------------------------------------------------------

# Native: look banana test-look-sorted.txt > /dev/null; echo $?  -> 0
TEST: Found string returns exit code 0 (RETURN_OK)
CMD: WORK:look banana WORK:test-look-sorted.txt
EXPECT: banana
EXPECT_RC: 0

# Native: look zznotfound test-look-sorted.txt > /dev/null; echo $?  -> 1 (maps to 5)
TEST: String not found returns exit code 5 (RETURN_WARN)
CMD: WORK:look zznotfound WORK:test-look-sorted.txt
EXPECT_RC: 5

# Bad option triggers usage() which exits 20
TEST: Invalid flag returns exit code 20 (RETURN_FAIL)
CMD: WORK:look -Z word WORK:test-look-sorted.txt
EXPECT_RC: 20

# Missing file triggers err(20, ...) -> RETURN_FAIL
TEST: Missing file returns exit code 20 (RETURN_FAIL)
CMD: WORK:look word WORK:nonexistent-file-xyz.txt
EXPECT_RC: 20

# No arguments triggers usage() -> exit(20)
# look never reads stdin -- argc=0 means default: usage(); exit(20)
TEST: No arguments returns exit code 20 (RETURN_FAIL)
CMD: WORK:look
EXPECT_RC: 20

# ---------------------------------------------------------------------------
# 4. ERROR PATH TESTS
# ---------------------------------------------------------------------------

# usage() called when argc != 1 or 2 after getopt
# (no args after option parsing = default case in switch)
TEST: Too many arguments returns exit code 20
CMD: WORK:look word WORK:test-look-sorted.txt WORK:test-look-sorted.txt
EXPECT_RC: 20

# ---------------------------------------------------------------------------
# 5. EDGE CASE TESTS
# ---------------------------------------------------------------------------

# Native: look apple /dev/empty -> RC=1 (macOS); empty file is a valid case
TEST: Empty file returns error (cannot read empty file)
CMD: WORK:look apple WORK:test-empty.txt
EXPECT_RC: 20

# Native: look banana test-look-single.txt -> banana; RC=0
TEST: Single-line file matches only line
CMD: WORK:look banana WORK:test-look-single.txt
EXPECT: banana
EXPECT_RC: 0

# Native: look apple test-look-single.txt -> RC=1 (not found)
TEST: Single-line file returns 5 when string not present
CMD: WORK:look apple WORK:test-look-single.txt
EXPECT_RC: 5

# Native: look apple test-look-sorted.txt -> "apple" (first line in file)
TEST: First line of file is found correctly
CMD: WORK:look apple WORK:test-look-sorted.txt
EXPECT: apple
EXPECT_RC: 0

# Native: look water test-look-sorted.txt -> "watermelon" (last line in file)
TEST: Last line of file is found correctly
CMD: WORK:look water WORK:test-look-sorted.txt
EXPECT: watermelon
EXPECT_RC: 0

# -t with colon delimiter (useful for colon-separated data)
# Native: look -t : aaa test-look-colon.txt -> "aaa:first"
TEST: -t colon truncates string at colon for key:value file lookup
CMD: WORK:look -t : aaa WORK:test-look-colon.txt
EXPECT: aaa:first
EXPECT_RC: 0

# -f on mixed-case sorted file
# Native: look -f apple test-look-case.txt -> "Apple"
TEST: -f flag finds uppercase entry when searching lowercase
CMD: WORK:look -f apple WORK:test-look-case.txt
EXPECT: Apple
EXPECT_RC: 0

# -f no match on mixed-case file
# Native: look -f zzz test-look-case.txt -> RC=1
TEST: -f flag returns 5 when no case-insensitive match exists
CMD: WORK:look -f zzz WORK:test-look-case.txt
EXPECT_RC: 5

# ---------------------------------------------------------------------------
# 6. AMIGA-SPECIFIC TESTS
# ---------------------------------------------------------------------------

# WORK: volume paths must work as both program path and file argument
TEST: Amiga WORK: volume paths work for file argument
CMD: WORK:look cherry WORK:test-look-sorted.txt
EXPECT: cherry
EXPECT_RC: 0

# Verify that look handles the Amiga-mapped words path gracefully (no system words file)
# On Amiga, _PATH_WORDS = "WORK:words" -- if not present, expect error
TEST: Missing default words file returns exit code 20
CMD: WORK:look someword
EXPECT_RC: 20

# ---------------------------------------------------------------------------
# 7. REAL-WORLD AND STRESS TESTS
# ---------------------------------------------------------------------------

# Real-world: look used as a dictionary prefix search tool
# Native: look -f Gra test-look-sorted.txt -> "grape", "grapefruit"
TEST: Real-world case-insensitive prefix search returns multiple results
CMD: WORK:look -f Gra WORK:test-look-sorted.txt
EXPECT: grape
EXPECT_LINE: 2,grapefruit
EXPECT_RC: 0

# Real-world: -t as field delimiter for structured data lookup
# Native: look -t : aab test-look-colon.txt -> "aab:second"
TEST: Real-world key lookup in colon-delimited sorted data
CMD: WORK:look -t : aab WORK:test-look-colon.txt
EXPECT: aab:second
EXPECT_RC: 0

# Real-world: -d flag normalizes entries with embedded punctuation
# This is a common Unix use case for look with /usr/share/dict/words
# Native: look -d apple test-look-casedict.txt -> first match ignoring punctuation
TEST: Real-world -df flag on dictionary entries with punctuation
CMD: WORK:look -df apple WORK:test-look-casedict.txt
EXPECT: Apple-Tree
EXPECT_RC: 0

# Stress: binary search correctness on 192-entry sorted file
# The binary search must correctly bisect the file to find "sigma" entries
# Native: look sigma test-look-stress.txt | head -1 -> "sigma01"
TEST: Binary search finds prefix in middle of 192-entry sorted file
CMD: WORK:look sigma WORK:test-look-stress.txt
EXPECT: sigma01
EXPECT_LINE: 8,sigma08
EXPECT_RC: 0

# Stress: binary search on first entries in stress file
# Native: look alpha test-look-stress.txt | head -1 -> "alpha01"
TEST: Binary search finds entries at start of 192-entry file
CMD: WORK:look alpha WORK:test-look-stress.txt
EXPECT: alpha01
EXPECT_LINE: 2,alpha02
EXPECT_RC: 0

# Stress: binary search on last entries in stress file
# Native: look zeta test-look-stress.txt | head -1 -> "zeta01"
TEST: Binary search finds entries at end of 192-entry file
CMD: WORK:look zeta WORK:test-look-stress.txt
EXPECT: zeta01
EXPECT_LINE: 2,zeta02
EXPECT_RC: 0

# Stress: binary search with no match returns RETURN_WARN
# Native: look zznotfound test-look-stress.txt -> RC=1 -> maps to 5
TEST: Binary search on 192-entry file returns 5 when no match
CMD: WORK:look zznotfound WORK:test-look-stress.txt
EXPECT_RC: 5

# Stress: exact single-entry match in stress file
# Native: look omega01 test-look-stress.txt -> "omega01"
TEST: Exact full-string match finds single entry in 192-entry file
CMD: WORK:look omega01 WORK:test-look-stress.txt
EXPECT: omega01
EXPECT_RC: 0

# Precision/real-world: look -t r apricot finds entries with "apr" prefix
# Native: look -t r apricot test-look-sorted.txt -> "apricot"
TEST: -t flag precision - terminates at first r in apricot leaving apr prefix
CMD: WORK:look -t r apricot WORK:test-look-sorted.txt
EXPECT: apricot
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
passed=36
failed=0
total=36
```

---
Generated by `make test-fsemu TARGET=ports/look`
Report template: `toolchain/templates/test-report.md.template`
