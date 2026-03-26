# FS-UAE Test Report: column

## Summary

| Field | Value |
|-------|-------|
| Port | column |
| Date | 2026-03-26 19:25:50 |
| Duration | 42s |
| Platform | FS-UAE 3.2.35 (A1200, Kickstart 3.1) |
| Binary | `WORK:column` (44K) |
| Test method | ARexx harness → TAP output |
| Result | **PASS** — 25/25 passed |

## Test Results

```
1..25
ok 1 - Default columnation fills columns first (r_columnate)
ok 2 - Default columnation fits all 8 items on one line
ok 3 - -x flag fills rows first (c_columnate)
ok 4 - -x with -c 40 fills rows first with narrow width
ok 5 - -c 40 uses 40-column terminal width for r_columnate
ok 6 - -c 1 minimum width forces single column output
ok 7 - -c 40 with 12 items produces 3-row 4-column layout
ok 8 - -t -s comma creates aligned table from CSV
ok 9 - -t with default separator aligns space-separated words
ok 10 - -t -s comma with 2-column data aligns headers and rows
ok 11 - -s sets field separator used by table mode
ok 12 - Two WORK file arguments are combined and columnated
ok 13 - Empty input file produces no output with RC 0
ok 14 - Blank and whitespace-only lines are skipped
ok 15 - Item wider than terminal width is printed one per line
ok 16 - Invalid flag -Z produces usage error RC 10
ok 17 - Non-existent file argument sets RC 10
ok 18 - -c 0 is out of strtonum range and exits RC 10
ok 19 - Real-world narrow terminal list fits 4 columns per row
ok 20 - Real-world CSV formatted as aligned table
ok 21 - Real-world -x produces left-to-right row fill like ls -x
ok 22 - Stress 500 items default r_columnate 80 columns first line correct
ok 23 - Stress 500 items -x c_columnate 80 columns first line correct
ok 24 - Stress 500 items -t table mode processes all lines without error
ok 25 - Stress -x c_columnate last row contains items 496-500 in sequential order
# passed: 25 failed: 0 total: 25
```

### Breakdown

| # | Test | Status | Details |
|---|------|--------|---------|
| 1 | Default columnation fills columns first (r_columnate) | PASS | |
| 2 | Default columnation fits all 8 items on one line | PASS | |
| 3 | -x flag fills rows first (c_columnate) | PASS | |
| 4 | -x with -c 40 fills rows first with narrow width | PASS | |
| 5 | -c 40 uses 40-column terminal width for r_columnate | PASS | |
| 6 | -c 1 minimum width forces single column output | PASS | |
| 7 | -c 40 with 12 items produces 3-row 4-column layout | PASS | |
| 8 | -t -s comma creates aligned table from CSV | PASS | |
| 9 | -t with default separator aligns space-separated words | PASS | |
| 10 | -t -s comma with 2-column data aligns headers and rows | PASS | |
| 11 | -s sets field separator used by table mode | PASS | |
| 12 | Two WORK file arguments are combined and columnated | PASS | |
| 13 | Empty input file produces no output with RC 0 | PASS | |
| 14 | Blank and whitespace-only lines are skipped | PASS | |
| 15 | Item wider than terminal width is printed one per line | PASS | |
| 16 | Invalid flag -Z produces usage error RC 10 | PASS | |
| 17 | Non-existent file argument sets RC 10 | PASS | |
| 18 | -c 0 is out of strtonum range and exits RC 10 | PASS | |
| 19 | Real-world narrow terminal list fits 4 columns per row | PASS | |
| 20 | Real-world CSV formatted as aligned table | PASS | |
| 21 | Real-world -x produces left-to-right row fill like ls -x | PASS | |
| 22 | Stress 500 items default r_columnate 80 columns first line correct | PASS | |
| 23 | Stress 500 items -x c_columnate 80 columns first line correct | PASS | |
| 24 | Stress 500 items -t table mode processes all lines without error | PASS | |
| 25 | Stress -x c_columnate last row contains items 496-500 in sequential order | PASS | |

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
# test-fsemu-cases.txt -- column 1.27 FS-UAE test suite
# column columnates lists of items from files
# Flags: -c columns, -s sep, -t (table mode), -x (fill rows first)
# All EXPECT: values derived from running native column on macOS

# ==========================================================================
# FUNCTIONAL: Default behavior (r_columnate -- fill columns first)
# ==========================================================================

# Native: column test-column-list12.txt
TEST: Default columnation fills columns first (r_columnate)
CMD: WORK:column WORK:test-column-list12.txt
EXPECT: apple	cherry	fig	kiwi	mango	peach
EXPECT_LINE: 2,banana	date	grape	lemon	orange	pear
EXPECT_RC: 0

# Native: column test-column-list8.txt
TEST: Default columnation fits all 8 items on one line
CMD: WORK:column WORK:test-column-list8.txt
EXPECT: alpha	beta	gamma	delta	epsilon	zeta	eta	theta
EXPECT_RC: 0

# ==========================================================================
# FUNCTIONAL: -x flag (fill rows before columns, c_columnate)
# ==========================================================================

# Native: column -x test-column-list12.txt
TEST: -x flag fills rows first (c_columnate)
CMD: WORK:column -x WORK:test-column-list12.txt
EXPECT: apple	banana	cherry	date	fig	grape	kiwi	lemon	mango	orange
EXPECT_LINE: 2,peach	pear
EXPECT_RC: 0

# Native: column -x -c 40 test-column-list8.txt
TEST: -x with -c 40 fills rows first with narrow width
CMD: WORK:column -x -c 40 WORK:test-column-list8.txt
EXPECT: alpha	beta	gamma	delta	epsilon
EXPECT_LINE: 2,zeta	eta	theta
EXPECT_RC: 0

# ==========================================================================
# FUNCTIONAL: -c flag (custom column width)
# ==========================================================================

# Native: column -c 40 test-column-list8.txt
TEST: -c 40 uses 40-column terminal width for r_columnate
CMD: WORK:column -c 40 WORK:test-column-list8.txt
EXPECT: alpha	gamma	epsilon	eta
EXPECT_LINE: 2,beta	delta	zeta	theta
EXPECT_RC: 0

# Native: column -c 1 test-column-list8.txt
TEST: -c 1 minimum width forces single column output
CMD: WORK:column -c 1 WORK:test-column-list8.txt
EXPECT: alpha
EXPECT_LINE: 2,beta
EXPECT_RC: 0

# Native: column -c 40 test-column-list12.txt
TEST: -c 40 with 12 items produces 3-row 4-column layout
CMD: WORK:column -c 40 WORK:test-column-list12.txt
EXPECT: apple	date	kiwi	orange
EXPECT_LINE: 2,banana	fig	lemon	peach
EXPECT_LINE: 3,cherry	grape	mango	pear
EXPECT_RC: 0

# ==========================================================================
# FUNCTIONAL: -t flag (table mode with alignment)
# ==========================================================================

# Native: column -t -s, test-column-csv.txt
TEST: -t -s comma creates aligned table from CSV
CMD: WORK:column -t -s, WORK:test-column-csv.txt
EXPECT: Name   Age  City
EXPECT_LINE: 2,Alice  30   London
EXPECT_LINE: 3,Bob    25   Paris
EXPECT_RC: 0

# Native: column -t test-column-words.txt
TEST: -t with default separator aligns space-separated words
CMD: WORK:column -t WORK:test-column-words.txt
EXPECT: one   two   three
EXPECT_LINE: 2,four  five  six
EXPECT_RC: 0

# Native: column -t -s, test-column-scores.txt
TEST: -t -s comma with 2-column data aligns headers and rows
CMD: WORK:column -t -s, WORK:test-column-scores.txt
EXPECT: Name   Score
EXPECT_LINE: 2,Alice  100
EXPECT_LINE: 3,Bob    95
EXPECT_RC: 0

# ==========================================================================
# FUNCTIONAL: -s flag (custom field separator for -t mode)
# ==========================================================================

# Native: column -t -s, test-column-csv.txt (same as above)
# -s only takes effect when -t is also active (source resets separator to "" without -t)
TEST: -s sets field separator used by table mode
CMD: WORK:column -t -s, WORK:test-column-csv.txt
EXPECT: Name   Age  City
EXPECT_RC: 0

# ==========================================================================
# AMIGA-SPECIFIC: WORK: path handling and multiple file arguments
# ==========================================================================

# Native: column test-column-list8.txt test-column-animals.txt
# (8 items + 3 items = 11 items combined, then columnated)
TEST: Two WORK file arguments are combined and columnated
CMD: WORK:column WORK:test-column-list8.txt WORK:test-column-animals.txt
EXPECT: alpha	gamma	epsilon	eta	cat	bird
EXPECT_LINE: 2,beta	delta	zeta	theta	dog
EXPECT_RC: 0

# ==========================================================================
# EDGE CASES: Empty file, whitespace-only lines, long items
# ==========================================================================

# Native: column test-empty.txt; echo RC=$?
TEST: Empty input file produces no output with RC 0
CMD: WORK:column WORK:test-empty.txt
EXPECT_RC: 0

# Native: column test-column-whitespace.txt (blank lines skipped by input())
TEST: Blank and whitespace-only lines are skipped
CMD: WORK:column WORK:test-column-whitespace.txt
EXPECT: apple	banana	cherry
EXPECT_RC: 0

# Native: column test-column-longitem.txt
# Source: when *maxwidths >= termwidth, print() is called -- one item per line
TEST: Item wider than terminal width is printed one per line
CMD: WORK:column WORK:test-column-longitem.txt
EXPECT: AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
EXPECT_RC: 0

# ==========================================================================
# ERROR PATH TESTS (EXPECT_RC: 10)
# ==========================================================================

# Source: usage() at default: case, exits with 10
TEST: Invalid flag -Z produces usage error RC 10
CMD: WORK:column -Z WORK:test-empty.txt
EXPECT_RC: 10

# Source: warn("%s", *argv) + eval=10 when fopen() fails on a non-existent file
TEST: Non-existent file argument sets RC 10
CMD: WORK:column WORK:nonexistent-xyz-99.txt
EXPECT_RC: 10

# Source: strtonum(optarg, 1, INT_MAX, &errstr) -- 0 is below minimum range of 1
# errstr is set, errx(10, ...) is called
TEST: -c 0 is out of strtonum range and exits RC 10
CMD: WORK:column -c 0 WORK:test-empty.txt
EXPECT_RC: 10

# ==========================================================================
# REAL-WORLD TESTS
# ==========================================================================

# Real-world: display list with specified width (like column(1) in shell scripts)
# Native: column -c 40 test-column-list12.txt
TEST: Real-world narrow terminal list fits 4 columns per row
CMD: WORK:column -c 40 WORK:test-column-list12.txt
EXPECT: apple	date	kiwi	orange
EXPECT_RC: 0

# Real-world: format structured CSV into aligned columns (like /etc/passwd viewer)
# Native: column -t -s, test-column-csv.txt
TEST: Real-world CSV formatted as aligned table
CMD: WORK:column -t -s, WORK:test-column-csv.txt
EXPECT: Name   Age  City
EXPECT_LINE: 2,Alice  30   London
EXPECT_LINE: 3,Bob    25   Paris
EXPECT_RC: 0

# Real-world: -x to fill left-to-right (like ls -x)
# Native: column -x -c 40 test-column-list12.txt
TEST: Real-world -x produces left-to-right row fill like ls -x
CMD: WORK:column -x -c 40 WORK:test-column-list12.txt
EXPECT: apple	banana	cherry	date	fig
EXPECT_LINE: 2,grape	kiwi	lemon	mango	orange
EXPECT_LINE: 3,peach	pear
EXPECT_RC: 0

# ==========================================================================
# STRESS TESTS
# ==========================================================================

# Stress: 500 items, default r_columnate at 80 columns (100 output rows)
# Native: column -c 80 test-column-stress.txt | head -1
TEST: Stress 500 items default r_columnate 80 columns first line correct
CMD: WORK:column -c 80 WORK:test-column-stress.txt
EXPECT: item0001	item0101	item0201	item0301	item0401
EXPECT_RC: 0

# Stress: 500 items, -x c_columnate at 80 columns
# Native: column -x -c 80 test-column-stress.txt | head -1
TEST: Stress 500 items -x c_columnate 80 columns first line correct
CMD: WORK:column -x -c 80 WORK:test-column-stress.txt
EXPECT: item0001	item0002	item0003	item0004	item0005
EXPECT_RC: 0

# Stress: 500 items through -t table mode (each item is a single-word field)
# With single-word-per-line and default separator, each row has 1 field
# maketbl() simply puts each item on its own line
# Native: printf 'item0001\nitem0002\n' | column -t
TEST: Stress 500 items -t table mode processes all lines without error
CMD: WORK:column -t -c 80 WORK:test-column-stress.txt
EXPECT: item0001
EXPECT_RC: 0

# Stress precision: verify -x row fill order is exactly sequential including final row
# With 500 items at 80 cols: 100 rows of 5 items each; row 100 = items 496-500
# Native: column -x -c 80 test-column-stress.txt (line 100 = last line)
TEST: Stress -x c_columnate last row contains items 496-500 in sequential order
CMD: WORK:column -x -c 80 WORK:test-column-stress.txt
EXPECT: item0001	item0002	item0003	item0004	item0005
EXPECT_LINE: 100,item0496	item0497	item0498	item0499	item0500
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
passed=25
failed=0
total=25
```

---
Generated by `make test-fsemu TARGET=ports/column`
Report template: `toolchain/templates/test-report.md.template`
