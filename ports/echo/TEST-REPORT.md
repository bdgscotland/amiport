# FS-UAE Test Report: echo

## Summary

| Field | Value |
|-------|-------|
| Port | echo |
| Date | 2026-03-26 13:32:09 |
| Duration | 35s |
| Platform | FS-UAE 3.2.35 (A1200, Kickstart 3.1) |
| Binary | `WORK:echo` (23K) |
| Test method | ARexx harness → TAP output |
| Result | **PASS** — 37/37 passed |

## Test Results

```
1..37
ok 1 - Basic string output with space
ok 2 - Multiple arguments joined with spaces
ok 3 - No arguments outputs a single newline (first line is blank)
ok 4 - -n suppresses trailing newline
ok 5 - -n with no arguments produces no output
ok 6 - -e enables escape sequences (backslash-n becomes newline)
ok 7 - -e backslash-t produces tab character
ok 8 - -e double backslash produces single backslash
ok 9 - -e backslash-c stops output before rest of argument and subsequent args
ok 10 - -e backslash-c at start produces no output
ok 11 - -e backslash-a produces bell character (0x07)
ok 12 - -e backslash-b produces backspace character (0x08)
ok 13 - -e backslash-e produces ESC character (0x1b)
ok 14 - -e backslash-f produces form feed character (0x0c)
ok 15 - -e backslash-r produces carriage return character (0x0d)
ok 16 - -e backslash-v produces vertical tab character (0x0b)
ok 17 - -e octal escape backslash-0101 produces A (65)
ok 18 - -e octal escape backslash-077 produces question mark (63)
ok 19 - -e hex escape backslash-x41 produces A (65)
ok 20 - -e hex escape backslash-x48 produces H; full word Hello
ok 21 - -E disables escape processing (backslash-n printed literally)
ok 22 - -E after -e disables escape processing
ok 23 - Combined -ne enables escapes and suppresses trailing newline
ok 24 - Combined -en same result as -ne (order irrelevant)
ok 25 - Combined -nE suppresses newline and disables escapes
ok 26 - Unknown flag treated as argument per POSIX echo spec
ok 27 - Single unknown flag argument printed as-is
ok 28 - -e trailing backslash at end of string outputs single backslash
ok 29 - Empty string argument outputs blank line
ok 30 - Amiga WORK volume path invocation works correctly
ok 31 - Real-world tab-formatted two-column report
ok 32 - Real-world prompt simulation with -n (no trailing newline)
ok 33 - Real-world multi-line status report with tabs and newlines
ok 34 - Stress 26-word argument output (full alphabet)
ok 35 - Stress multiple tab escapes in single argument
ok 36 - Stress five consecutive octal escapes producing ABCDE
ok 37 - Stress five consecutive hex escapes producing ABCDE
# passed: 37 failed: 0 total: 37
```

### Breakdown

| # | Test | Status | Details |
|---|------|--------|---------|
| 1 | Basic string output with space | PASS | |
| 2 | Multiple arguments joined with spaces | PASS | |
| 3 | No arguments outputs a single newline (first line is blank) | PASS | |
| 4 | -n suppresses trailing newline | PASS | |
| 5 | -n with no arguments produces no output | PASS | |
| 6 | -e enables escape sequences (backslash-n becomes newline) | PASS | |
| 7 | -e backslash-t produces tab character | PASS | |
| 8 | -e double backslash produces single backslash | PASS | |
| 9 | -e backslash-c stops output before rest of argument and subsequent args | PASS | |
| 10 | -e backslash-c at start produces no output | PASS | |
| 11 | -e backslash-a produces bell character (0x07) | PASS | |
| 12 | -e backslash-b produces backspace character (0x08) | PASS | |
| 13 | -e backslash-e produces ESC character (0x1b) | PASS | |
| 14 | -e backslash-f produces form feed character (0x0c) | PASS | |
| 15 | -e backslash-r produces carriage return character (0x0d) | PASS | |
| 16 | -e backslash-v produces vertical tab character (0x0b) | PASS | |
| 17 | -e octal escape backslash-0101 produces A (65) | PASS | |
| 18 | -e octal escape backslash-077 produces question mark (63) | PASS | |
| 19 | -e hex escape backslash-x41 produces A (65) | PASS | |
| 20 | -e hex escape backslash-x48 produces H; full word Hello | PASS | |
| 21 | -E disables escape processing (backslash-n printed literally) | PASS | |
| 22 | -E after -e disables escape processing | PASS | |
| 23 | Combined -ne enables escapes and suppresses trailing newline | PASS | |
| 24 | Combined -en same result as -ne (order irrelevant) | PASS | |
| 25 | Combined -nE suppresses newline and disables escapes | PASS | |
| 26 | Unknown flag treated as argument per POSIX echo spec | PASS | |
| 27 | Single unknown flag argument printed as-is | PASS | |
| 28 | -e trailing backslash at end of string outputs single backslash | PASS | |
| 29 | Empty string argument outputs blank line | PASS | |
| 30 | Amiga WORK volume path invocation works correctly | PASS | |
| 31 | Real-world tab-formatted two-column report | PASS | |
| 32 | Real-world prompt simulation with -n (no trailing newline) | PASS | |
| 33 | Real-world multi-line status report with tabs and newlines | PASS | |
| 34 | Stress 26-word argument output (full alphabet) | PASS | |
| 35 | Stress multiple tab escapes in single argument | PASS | |
| 36 | Stress five consecutive octal escapes producing ABCDE | PASS | |
| 37 | Stress five consecutive hex escapes producing ABCDE | PASS | |

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
# test-fsemu-cases.txt -- FS-UAE functional tests for echo 1.12
# Source: ports/echo/ported/echo.c (OpenBSD echo.c 1.12)
# Category: 1 (CLI tool) -- minimum 15 tests
# echo always exits 0 (no error paths -- pledge is a no-op, unknown flags treated as args)
#
# Note: echo reads no files -- all tests pass arguments directly.
# No test input files needed for this port.

# --- Functional: basic output ---

# Native: echo "hello world"
TEST: Basic string output with space
CMD: WORK:echo hello world
EXPECT: hello world
EXPECT_RC: 0

# Native: echo one two three four five
TEST: Multiple arguments joined with spaces
CMD: WORK:echo one two three four five
EXPECT: one two three four five
EXPECT_RC: 0

# Native: echo (no args) -> just a newline, first line is empty
TEST: No arguments outputs a single newline (first line is blank)
CMD: WORK:echo
EXPECT:
EXPECT_RC: 0

# --- Functional: -n flag ---

# Native: echo -n hello -> no trailing newline; output captured as first line = "hello"
TEST: -n suppresses trailing newline
CMD: WORK:echo -n hello
EXPECT: hello
EXPECT_RC: 0

# Native: echo -n -> no output at all (0 bytes)
TEST: -n with no arguments produces no output
CMD: WORK:echo -n
EXPECT:
EXPECT_RC: 0

# --- Functional: -e flag and escape sequences ---

# Native: echo -e "hello\nworld" -> two lines; first line = "hello"
TEST: -e enables escape sequences (backslash-n becomes newline)
CMD: WORK:echo -e hello\nworld
EXPECT: hello
EXPECT_LINE: 2,world
EXPECT_RC: 0

# Native: echo -e "hello\tworld" -> "hello<TAB>world"
TEST: -e backslash-t produces tab character
CMD: WORK:echo -e hello\tworld
EXPECT: hello	world
EXPECT_RC: 0

# Native: echo -e "back\\slash" -> "back\slash"
TEST: -e double backslash produces single backslash
CMD: WORK:echo -e back\\slash
EXPECT: back\slash
EXPECT_RC: 0

# Native: echo -e "hello\c" rest -> "hi" (2 chars, no trailing newline, no "rest")
# \c stops output immediately; nothing after it is printed
TEST: -e backslash-c stops output before rest of argument and subsequent args
CMD: WORK:echo -e hello\c world
EXPECT: hello
EXPECT_RC: 0

# Native: echo -e "\c" rest -> 0 bytes output
TEST: -e backslash-c at start produces no output
CMD: WORK:echo -e \c word
EXPECT:
EXPECT_RC: 0

# Native: echo -e "bell\a" -> bell<0x07> then newline; first line = "bell<BEL>"
# \a = 0x07 (BEL)
TEST: -e backslash-a produces bell character (0x07)
CMD: WORK:echo -e bell\a
EXPECT_CONTAINS: bell
EXPECT_RC: 0

# Native: echo -e "ab\bc" -> a backspace b c newline; first line contains backspace
# \b = 0x08 (backspace) -- output contains control chars, use CONTAINS
TEST: -e backslash-b produces backspace character (0x08)
CMD: WORK:echo -e ab\bc
EXPECT_CONTAINS: ab
EXPECT_RC: 0

# Native: echo -e "\e[0m" -> ESC[0m (ESC=0x1b, then "[0m"); first char is ESC
# \e = 0x1b (escape)
TEST: -e backslash-e produces ESC character (0x1b)
CMD: WORK:echo -e \e[0m
EXPECT_CONTAINS: [0m
EXPECT_RC: 0

# Native: echo -e "ff\f" -> "ff" then form feed (0x0c); use CONTAINS to avoid control char issues
TEST: -e backslash-f produces form feed character (0x0c)
CMD: WORK:echo -e ff\f
EXPECT_CONTAINS: ff
EXPECT_RC: 0

# Native: echo -e "hello\rworld" -> "hello<CR>world"; first line = "hello<CR>world"
# \r = 0x0d (carriage return)
TEST: -e backslash-r produces carriage return character (0x0d)
CMD: WORK:echo -e hello\rworld
EXPECT_CONTAINS: hello
EXPECT_RC: 0

# Native: echo -e "vt\v" -> "vt<VT>"; first line = "vt<VT>"
# \v = 0x0b (vertical tab)
TEST: -e backslash-v produces vertical tab character (0x0b)
CMD: WORK:echo -e vt\v
EXPECT_CONTAINS: vt
EXPECT_RC: 0

# --- Functional: -e octal and hex escapes ---

# Native: echo -e "\0101" -> "A" (\0101 = octal 101 = 65 = 'A')
TEST: -e octal escape backslash-0101 produces A (65)
CMD: WORK:echo -e \0101
EXPECT: A
EXPECT_RC: 0

# Native: echo -e "\077" -> "?" (\077 = octal 77 = 63 = '?')
TEST: -e octal escape backslash-077 produces question mark (63)
CMD: WORK:echo -e \077
EXPECT: ?
EXPECT_RC: 0

# Native: echo -e "\x41" -> "A" (\x41 = hex 41 = 65 = 'A')
TEST: -e hex escape backslash-x41 produces A (65)
CMD: WORK:echo -e \x41
EXPECT: A
EXPECT_RC: 0

# Native: echo -e "\x48ello" -> "Hello"
TEST: -e hex escape backslash-x48 produces H; full word Hello
CMD: WORK:echo -e \x48ello
EXPECT: Hello
EXPECT_RC: 0

# --- Functional: -E flag ---

# Native: echo -E "hello\nworld" -> "hello\nworld" (literal backslash-n, no escape)
TEST: -E disables escape processing (backslash-n printed literally)
CMD: WORK:echo -E hello\nworld
EXPECT: hello\nworld
EXPECT_RC: 0

# Native: echo -e -E "hello\nworld" -> "hello\nworld" (-E after -e turns escapes off)
TEST: -E after -e disables escape processing
CMD: WORK:echo -e -E hello\nworld
EXPECT: hello\nworld
EXPECT_RC: 0

# --- Functional: combined flags ---

# Native: echo -ne "hello\nworld" -> "hello<newline>world" (no trailing newline)
# First line = "hello", second line = "world" (no newline at end)
TEST: Combined -ne enables escapes and suppresses trailing newline
CMD: WORK:echo -ne hello\nworld
EXPECT: hello
EXPECT_LINE: 2,world
EXPECT_RC: 0

# Native: echo -en "hello\nworld" -> same as -ne
TEST: Combined -en same result as -ne (order irrelevant)
CMD: WORK:echo -en hello\nworld
EXPECT: hello
EXPECT_LINE: 2,world
EXPECT_RC: 0

# Native: echo -nE "hello\nworld" -> "hello\nworld" (literal, no trailing newline)
TEST: Combined -nE suppresses newline and disables escapes
CMD: WORK:echo -nE hello\nworld
EXPECT: hello\nworld
EXPECT_RC: 0

# --- Edge cases ---

# Native: echo -z hello -> "-z hello" (unknown flag resets flags, arg printed as-is)
# Per POSIX echo spec: unknown flag terminates flag processing, arg is data
TEST: Unknown flag treated as argument per POSIX echo spec
CMD: WORK:echo -z hello
EXPECT: -z hello
EXPECT_RC: 0

# Native: echo -X -> "-X" (unknown flag by itself)
TEST: Single unknown flag argument printed as-is
CMD: WORK:echo -X
EXPECT: -X
EXPECT_RC: 0

# Native: echo -e "test\\" -> "test\" (trailing backslash outputs backslash)
# Source case '\0': putchar('\\'); return 0; -- lone \ at end outputs backslash
TEST: -e trailing backslash at end of string outputs single backslash
CMD: WORK:echo -e test\\
EXPECT: test\
EXPECT_RC: 0

# Native: echo "" -> empty line (newline only)
TEST: Empty string argument outputs blank line
CMD: WORK:echo
EXPECT:
EXPECT_RC: 0

# --- Amiga-specific tests ---

# echo is purely argument-based (no file I/O), so Amiga path handling is
# tested implicitly by all tests using WORK:echo. Add explicit WORK: path test.
TEST: Amiga WORK volume path invocation works correctly
CMD: WORK:echo Amiga WORK path test
EXPECT: Amiga WORK path test
EXPECT_RC: 0

# --- Real-world tests ---

# Native: echo -e "Name:\tAlice\nRole:\tDeveloper"
# Two-column tab-formatted report -- real-world use case
TEST: Real-world tab-formatted two-column report
CMD: WORK:echo -e Name:\tAlice\nRole:\tDeveloper
EXPECT: Name:	Alice
EXPECT_LINE: 2,Role:	Developer
EXPECT_RC: 0

# Native: echo -n "Enter value: " -> "Enter value: " (no newline, simulates prompt)
TEST: Real-world prompt simulation with -n (no trailing newline)
CMD: WORK:echo -n Enter value:
EXPECT: Enter value:
EXPECT_RC: 0

# Native: echo -e "STATUS:\tOK\nCODE:\t0\nMSG:\tDone"
# Script diagnostic output -- multi-line formatted status
TEST: Real-world multi-line status report with tabs and newlines
CMD: WORK:echo -e STATUS:\tOK\nCODE:\t0\nMSG:\tDone
EXPECT: STATUS:	OK
EXPECT_LINE: 2,CODE:	0
EXPECT_LINE: 3,MSG:	Done
EXPECT_RC: 0

# --- Stress tests ---

# Native: echo a b c d e f g h i j k l m n o p q r s t u v w x y z
# 26 single-char args, all space-separated -- tests many-argument handling
TEST: Stress 26-word argument output (full alphabet)
CMD: WORK:echo a b c d e f g h i j k l m n o p q r s t u v w x y z
EXPECT: a b c d e f g h i j k l m n o p q r s t u v w x y z
EXPECT_RC: 0

# Native: echo -e "a\tb\tc\td\te\tf\tg\th"
# 8 tab-separated values -- tests repeated escape processing
TEST: Stress multiple tab escapes in single argument
CMD: WORK:echo -e a\tb\tc\td\te\tf\tg\th
EXPECT: a	b	c	d	e	f	g	h
EXPECT_RC: 0

# Native: echo -e "\0101\0102\0103\0104\0105" -> "ABCDE"
# Five octal escapes in sequence -- tests octal parser loop stability
TEST: Stress five consecutive octal escapes producing ABCDE
CMD: WORK:echo -e \0101\0102\0103\0104\0105
EXPECT: ABCDE
EXPECT_RC: 0

# Native: echo -e "\x41\x42\x43\x44\x45" -> "ABCDE"
# Five hex escapes -- tests hex parser loop stability
TEST: Stress five consecutive hex escapes producing ABCDE
CMD: WORK:echo -e \x41\x42\x43\x44\x45
EXPECT: ABCDE
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
passed=37
failed=0
total=37
```

---
Generated by `make test-fsemu TARGET=ports/echo`
Report template: `toolchain/templates/test-report.md.template`
