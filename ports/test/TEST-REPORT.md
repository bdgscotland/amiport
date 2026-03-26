# FS-UAE Test Report: test

## Summary

| Field | Value |
|-------|-------|
| Port | test |
| Date | 2026-03-26 13:44:27 |
| Duration | 70s |
| Platform | FS-UAE 3.2.35 (A1200, Kickstart 3.1) |
| Binary | `WORK:test` (37K) |
| Test method | ARexx harness → TAP output |
| Result | **PASS** — 97/97 passed |

## Test Results

```
1..97
ok 1 - -z returns false for non-empty string
ok 2 - -n returns true for non-empty string
ok 3 - -n returns true for single character string
ok 4 - = returns true when strings are equal
ok 5 - = returns false when strings differ
ok 6 - != returns true when strings differ
ok 7 - != returns false when strings are equal
ok 8 - -eq returns true when integers are equal
ok 9 - -eq returns false when integers differ
ok 10 - -ne returns true when integers are not equal
ok 11 - -ne returns false when integers are equal
ok 12 - -gt returns true when left is greater than right
ok 13 - -gt returns false when left is not greater
ok 14 - -ge returns true when equal (boundary case)
ok 15 - -ge returns true when left is greater
ok 16 - -ge returns false when left is less
ok 17 - -lt returns true when left is less than right
ok 18 - -lt returns false when equal (boundary case)
ok 19 - -le returns true when equal (boundary case)
ok 20 - -le returns false when left is greater
ok 21 - -f returns true for regular file
ok 22 - -f returns false for directory
ok 23 - -d returns true for directory
ok 24 - -d returns false for regular file
ok 25 - -e returns true for existing file
ok 26 - -e returns false for nonexistent path
ok 27 - -s returns true for non-empty file
ok 28 - -s returns false for empty file (zero bytes)
ok 29 - -r returns true for readable file
ok 30 - -r returns false for nonexistent file
ok 31 - -w returns true for writable file on AmigaOS
ok 32 - -w returns false for nonexistent file
ok 33 - -x returns true for executable binary
ok 34 - -a AND returns true when both sides are true
ok 35 - -a AND returns false when second side is false
ok 36 - -o OR returns true when second side is true
ok 37 - -o OR returns false when both sides are false
ok 38 - Logical NOT inverts false to true
ok 39 - Logical NOT inverts true to false
ok 40 - Parentheses group OR before AND -- (true OR false) AND true = true
ok 41 - Parentheses group OR before AND -- (false OR false) AND true = false
ok 42 - Parentheses group OR before AND -- (false OR true) AND true = true
ok 43 - No arguments returns false (POSIX special case argc=1)
ok 44 - Single non-empty string argument returns true (POSIX special case argc=2)
ok 45 - NOT with single non-empty string returns false (POSIX special case argc=3)
ok 46 - POSIX optimised 4-argument path for binary comparison
ok 47 - POSIX optimised 5-argument path NOT with binary comparison (true becomes false)
ok 48 - POSIX optimised 5-argument path NOT with binary comparison (false becomes true)
ok 49 - -nt returns false when file is compared to itself
ok 50 - -ot returns false when file is compared to itself
ok 51 - -nt returns false when comparison file does not exist
ok 52 - -ot returns false when base file does not exist
ok 53 - -ef returns true when file is compared to itself (same device and inode)
ok 54 - -ef returns false when files are different
ok 55 - -O returns true for any file (AmigaOS single-user geteuid always 0)
ok 56 - -G returns true for any file (AmigaOS single-user getegid always 0)
ok 57 - -L returns false (no symbolic links on AmigaOS)
ok 58 - -h returns false (alias for -L, no symbolic links on AmigaOS)
ok 59 - -c returns false (no character devices on AmigaOS FFS)
ok 60 - -b returns false (no block devices on AmigaOS FFS)
ok 61 - -p returns false (no FIFOs on AmigaOS FFS)
ok 62 - -S returns false (no sockets on AmigaOS)
ok 63 - -u returns false (no setuid bit on AmigaOS)
ok 64 - -g returns false (no setgid bit on AmigaOS)
ok 65 - -k returns false (no sticky bit on AmigaOS)
ok 66 - -t 1 returns false when stdout is redirected (not a tty in harness)
ok 67 - Unknown extra operand returns RC=20
ok 68 - Extra operand after complete expression returns RC=20
ok 69 - Non-numeric argument to -eq returns RC=20
ok 70 - Non-numeric argument to -gt returns RC=20
ok 71 - Missing right operand for binary operator returns RC=20
ok 72 - Integer zero equality check
ok 73 - Negative integer comparison -5 -lt 0
ok 74 - Negative integer comparison -5 not greater than 0
ok 75 - Integer with leading zeros 007 equals 7
ok 76 - POSIX case 2 treats operator-like flag as plain non-empty string (true)
ok 77 - -f returns false for nonexistent file (not an error, RC=1 not RC=20)
ok 78 - -e returns true for empty file (exists, zero bytes is still existence)
ok 79 - -s returns false for empty file (zero size)
ok 80 - Real-world file guard check (exists AND regular AND non-empty) all true
ok 81 - Real-world version string equality 1.23 = 1.23
ok 82 - Real-world version string inequality 1.22 != 1.23
ok 83 - Real-world range check 50 in range 1 to 100
ok 84 - Real-world range check 150 out of range 1 to 100
ok 85 - Real-world negated existence check for nonexistent file
ok 86 - Real-world directory existence check for known directory
ok 87 - Real-world parenthesised (regular AND non-empty) OR directory
ok 88 - Stress deep -a chain with four integer comparisons all true
ok 89 - Stress deep -o chain first-true short-circuits rest
ok 90 - Stress deep all-false -o chain returns false
ok 91 - Stress NOT wrapping true integer equality returns false
ok 92 - Stress large integer at 32-bit boundary 2147483647 -gt 2147483646
ok 93 - Precision different-digit-count integers 9999 -lt 10000
ok 94 - Precision same-digit-count integers 12345 -lt 12346
ok 95 - Precision sign-based comparison positive 1 -gt negative 1
ok 96 - Precision negative integer -ge boundary -5 -ge -5
ok 97 - Precision negative integer -le boundary -5 -le -5
# passed: 97 failed: 0 total: 97
```

### Breakdown

| # | Test | Status | Details |
|---|------|--------|---------|
| 1 | -z returns false for non-empty string | PASS | |
| 2 | -n returns true for non-empty string | PASS | |
| 3 | -n returns true for single character string | PASS | |
| 4 | = returns true when strings are equal | PASS | |
| 5 | = returns false when strings differ | PASS | |
| 6 | != returns true when strings differ | PASS | |
| 7 | != returns false when strings are equal | PASS | |
| 8 | -eq returns true when integers are equal | PASS | |
| 9 | -eq returns false when integers differ | PASS | |
| 10 | -ne returns true when integers are not equal | PASS | |
| 11 | -ne returns false when integers are equal | PASS | |
| 12 | -gt returns true when left is greater than right | PASS | |
| 13 | -gt returns false when left is not greater | PASS | |
| 14 | -ge returns true when equal (boundary case) | PASS | |
| 15 | -ge returns true when left is greater | PASS | |
| 16 | -ge returns false when left is less | PASS | |
| 17 | -lt returns true when left is less than right | PASS | |
| 18 | -lt returns false when equal (boundary case) | PASS | |
| 19 | -le returns true when equal (boundary case) | PASS | |
| 20 | -le returns false when left is greater | PASS | |
| 21 | -f returns true for regular file | PASS | |
| 22 | -f returns false for directory | PASS | |
| 23 | -d returns true for directory | PASS | |
| 24 | -d returns false for regular file | PASS | |
| 25 | -e returns true for existing file | PASS | |
| 26 | -e returns false for nonexistent path | PASS | |
| 27 | -s returns true for non-empty file | PASS | |
| 28 | -s returns false for empty file (zero bytes) | PASS | |
| 29 | -r returns true for readable file | PASS | |
| 30 | -r returns false for nonexistent file | PASS | |
| 31 | -w returns true for writable file on AmigaOS | PASS | |
| 32 | -w returns false for nonexistent file | PASS | |
| 33 | -x returns true for executable binary | PASS | |
| 34 | -a AND returns true when both sides are true | PASS | |
| 35 | -a AND returns false when second side is false | PASS | |
| 36 | -o OR returns true when second side is true | PASS | |
| 37 | -o OR returns false when both sides are false | PASS | |
| 38 | Logical NOT inverts false to true | PASS | |
| 39 | Logical NOT inverts true to false | PASS | |
| 40 | Parentheses group OR before AND -- (true OR false) AND true = true | PASS | |
| 41 | Parentheses group OR before AND -- (false OR false) AND true = false | PASS | |
| 42 | Parentheses group OR before AND -- (false OR true) AND true = true | PASS | |
| 43 | No arguments returns false (POSIX special case argc=1) | PASS | |
| 44 | Single non-empty string argument returns true (POSIX special case argc=2) | PASS | |
| 45 | NOT with single non-empty string returns false (POSIX special case argc=3) | PASS | |
| 46 | POSIX optimised 4-argument path for binary comparison | PASS | |
| 47 | POSIX optimised 5-argument path NOT with binary comparison (true becomes false) | PASS | |
| 48 | POSIX optimised 5-argument path NOT with binary comparison (false becomes true) | PASS | |
| 49 | -nt returns false when file is compared to itself | PASS | |
| 50 | -ot returns false when file is compared to itself | PASS | |
| 51 | -nt returns false when comparison file does not exist | PASS | |
| 52 | -ot returns false when base file does not exist | PASS | |
| 53 | -ef returns true when file is compared to itself (same device and inode) | PASS | |
| 54 | -ef returns false when files are different | PASS | |
| 55 | -O returns true for any file (AmigaOS single-user geteuid always 0) | PASS | |
| 56 | -G returns true for any file (AmigaOS single-user getegid always 0) | PASS | |
| 57 | -L returns false (no symbolic links on AmigaOS) | PASS | |
| 58 | -h returns false (alias for -L, no symbolic links on AmigaOS) | PASS | |
| 59 | -c returns false (no character devices on AmigaOS FFS) | PASS | |
| 60 | -b returns false (no block devices on AmigaOS FFS) | PASS | |
| 61 | -p returns false (no FIFOs on AmigaOS FFS) | PASS | |
| 62 | -S returns false (no sockets on AmigaOS) | PASS | |
| 63 | -u returns false (no setuid bit on AmigaOS) | PASS | |
| 64 | -g returns false (no setgid bit on AmigaOS) | PASS | |
| 65 | -k returns false (no sticky bit on AmigaOS) | PASS | |
| 66 | -t 1 returns false when stdout is redirected (not a tty in harness) | PASS | |
| 67 | Unknown extra operand returns RC=20 | PASS | |
| 68 | Extra operand after complete expression returns RC=20 | PASS | |
| 69 | Non-numeric argument to -eq returns RC=20 | PASS | |
| 70 | Non-numeric argument to -gt returns RC=20 | PASS | |
| 71 | Missing right operand for binary operator returns RC=20 | PASS | |
| 72 | Integer zero equality check | PASS | |
| 73 | Negative integer comparison -5 -lt 0 | PASS | |
| 74 | Negative integer comparison -5 not greater than 0 | PASS | |
| 75 | Integer with leading zeros 007 equals 7 | PASS | |
| 76 | POSIX case 2 treats operator-like flag as plain non-empty string (true) | PASS | |
| 77 | -f returns false for nonexistent file (not an error, RC=1 not RC=20) | PASS | |
| 78 | -e returns true for empty file (exists, zero bytes is still existence) | PASS | |
| 79 | -s returns false for empty file (zero size) | PASS | |
| 80 | Real-world file guard check (exists AND regular AND non-empty) all true | PASS | |
| 81 | Real-world version string equality 1.23 = 1.23 | PASS | |
| 82 | Real-world version string inequality 1.22 != 1.23 | PASS | |
| 83 | Real-world range check 50 in range 1 to 100 | PASS | |
| 84 | Real-world range check 150 out of range 1 to 100 | PASS | |
| 85 | Real-world negated existence check for nonexistent file | PASS | |
| 86 | Real-world directory existence check for known directory | PASS | |
| 87 | Real-world parenthesised (regular AND non-empty) OR directory | PASS | |
| 88 | Stress deep -a chain with four integer comparisons all true | PASS | |
| 89 | Stress deep -o chain first-true short-circuits rest | PASS | |
| 90 | Stress deep all-false -o chain returns false | PASS | |
| 91 | Stress NOT wrapping true integer equality returns false | PASS | |
| 92 | Stress large integer at 32-bit boundary 2147483647 -gt 2147483646 | PASS | |
| 93 | Precision different-digit-count integers 9999 -lt 10000 | PASS | |
| 94 | Precision same-digit-count integers 12345 -lt 12346 | PASS | |
| 95 | Precision sign-based comparison positive 1 -gt negative 1 | PASS | |
| 96 | Precision negative integer -ge boundary -5 -ge -5 | PASS | |
| 97 | Precision negative integer -le boundary -5 -le -5 | PASS | |

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
# test FS-UAE test suite
# OpenBSD test 1.23 -- Category 1 (CLI), minimum 8 tests required
#
# POSIX semantics: RC=0 = TRUE, RC=1 = FALSE (correct condition evaluation)
# Only syntax/usage errors return RC=20 (RETURN_FAIL via errx(20,...))
# RC=10 is never returned by test(1) -- error path is RC=20
#
# test produces NO stdout output -- all assertions use EXPECT_RC only
#
# AmigaDOS notes:
#   - Empty string args ("") are stripped by AmigaDOS shell; avoid them in CMD
#   - String < and > operators omitted: AmigaDOS shell treats bare < and > as
#     I/O redirectors; cannot be passed as arguments through Execute scripts
#   - ( and ) are NOT special AmigaDOS shell metacharacters: pass through fine
#   - __nowild=1 is set in test.c so * is not expanded as a wildcard

# =======================================================================
# SECTION 1: STRING OPERATORS
# =======================================================================

# Native: /bin/test -z hello ; echo $?  -> 1 (non-empty string has non-zero length)
TEST: -z returns false for non-empty string
CMD: WORK:test -z hello
EXPECT_RC: 1

# Native: /bin/test -n hello ; echo $?  -> 0 (non-empty string has non-zero length)
TEST: -n returns true for non-empty string
CMD: WORK:test -n hello
EXPECT_RC: 0

# Native: /bin/test -n x ; echo $?  -> 0
TEST: -n returns true for single character string
CMD: WORK:test -n x
EXPECT_RC: 0

# Native: /bin/test hello = hello ; echo $?  -> 0
TEST: = returns true when strings are equal
CMD: WORK:test hello = hello
EXPECT_RC: 0

# Native: /bin/test hello = world ; echo $?  -> 1
TEST: = returns false when strings differ
CMD: WORK:test hello = world
EXPECT_RC: 1

# Native: /bin/test hello != world ; echo $?  -> 0
TEST: != returns true when strings differ
CMD: WORK:test hello != world
EXPECT_RC: 0

# Native: /bin/test hello != hello ; echo $?  -> 1
TEST: != returns false when strings are equal
CMD: WORK:test hello != hello
EXPECT_RC: 1

# =======================================================================
# SECTION 2: INTEGER COMPARISON OPERATORS
# =======================================================================

# Native: /bin/test 5 -eq 5 ; echo $?  -> 0
TEST: -eq returns true when integers are equal
CMD: WORK:test 5 -eq 5
EXPECT_RC: 0

# Native: /bin/test 5 -eq 6 ; echo $?  -> 1
TEST: -eq returns false when integers differ
CMD: WORK:test 5 -eq 6
EXPECT_RC: 1

# Native: /bin/test 3 -ne 5 ; echo $?  -> 0
TEST: -ne returns true when integers are not equal
CMD: WORK:test 3 -ne 5
EXPECT_RC: 0

# Native: /bin/test 3 -ne 3 ; echo $?  -> 1
TEST: -ne returns false when integers are equal
CMD: WORK:test 3 -ne 3
EXPECT_RC: 1

# Native: /bin/test 7 -gt 5 ; echo $?  -> 0
TEST: -gt returns true when left is greater than right
CMD: WORK:test 7 -gt 5
EXPECT_RC: 0

# Native: /bin/test 4 -gt 5 ; echo $?  -> 1
TEST: -gt returns false when left is not greater
CMD: WORK:test 4 -gt 5
EXPECT_RC: 1

# Native: /bin/test 5 -ge 5 ; echo $?  -> 0  (boundary: equal satisfies >=)
TEST: -ge returns true when equal (boundary case)
CMD: WORK:test 5 -ge 5
EXPECT_RC: 0

# Native: /bin/test 6 -ge 5 ; echo $?  -> 0
TEST: -ge returns true when left is greater
CMD: WORK:test 6 -ge 5
EXPECT_RC: 0

# Native: /bin/test 4 -ge 5 ; echo $?  -> 1
TEST: -ge returns false when left is less
CMD: WORK:test 4 -ge 5
EXPECT_RC: 1

# Native: /bin/test 3 -lt 5 ; echo $?  -> 0
TEST: -lt returns true when left is less than right
CMD: WORK:test 3 -lt 5
EXPECT_RC: 0

# Native: /bin/test 5 -lt 5 ; echo $?  -> 1  (boundary: equal does NOT satisfy <)
TEST: -lt returns false when equal (boundary case)
CMD: WORK:test 5 -lt 5
EXPECT_RC: 1

# Native: /bin/test 5 -le 5 ; echo $?  -> 0  (boundary: equal satisfies <=)
TEST: -le returns true when equal (boundary case)
CMD: WORK:test 5 -le 5
EXPECT_RC: 0

# Native: /bin/test 6 -le 5 ; echo $?  -> 1
TEST: -le returns false when left is greater
CMD: WORK:test 6 -le 5
EXPECT_RC: 1

# =======================================================================
# SECTION 3: FILE TEST OPERATORS
# =======================================================================

# Native: /bin/test -f /etc/hosts ; echo $?  -> 0
TEST: -f returns true for regular file
CMD: WORK:test -f WORK:test-oneline.txt
EXPECT_RC: 0

# Native: /bin/test -f /etc ; echo $?  -> 1  (directory is not a regular file)
TEST: -f returns false for directory
CMD: WORK:test -f SYS:Rexxc
EXPECT_RC: 1

# Native: /bin/test -d /etc ; echo $?  -> 0
TEST: -d returns true for directory
CMD: WORK:test -d SYS:Rexxc
EXPECT_RC: 0

# Native: /bin/test -d /etc/hosts ; echo $?  -> 1
TEST: -d returns false for regular file
CMD: WORK:test -d WORK:test-oneline.txt
EXPECT_RC: 1

# Native: /bin/test -e /etc/hosts ; echo $?  -> 0
TEST: -e returns true for existing file
CMD: WORK:test -e WORK:test-oneline.txt
EXPECT_RC: 0

# Native: /bin/test -e /nonexistent ; echo $?  -> 1
TEST: -e returns false for nonexistent path
CMD: WORK:test -e WORK:nonexistent-file-xyz.txt
EXPECT_RC: 1

# Native: /bin/test -s /etc/hosts ; echo $?  -> 0  (hosts has content)
TEST: -s returns true for non-empty file
CMD: WORK:test -s WORK:test-oneline.txt
EXPECT_RC: 0

# Native: comparable zero-size file check -> 1  (test-empty.txt is 0 bytes)
TEST: -s returns false for empty file (zero bytes)
CMD: WORK:test -s WORK:test-empty.txt
EXPECT_RC: 1

# Native: /bin/test -r /etc/hosts ; echo $?  -> 0
TEST: -r returns true for readable file
CMD: WORK:test -r WORK:test-oneline.txt
EXPECT_RC: 0

# Native: /bin/test -r /nonexistent ; echo $?  -> 1
TEST: -r returns false for nonexistent file
CMD: WORK:test -r WORK:no-such-file.txt
EXPECT_RC: 1

# On AmigaOS, WORK: files are writable by default
TEST: -w returns true for writable file on AmigaOS
CMD: WORK:test -w WORK:test-oneline.txt
EXPECT_RC: 0

# Native: /bin/test -w /nonexistent ; echo $?  -> 1
TEST: -w returns false for nonexistent file
CMD: WORK:test -w WORK:no-such-file.txt
EXPECT_RC: 1

# The test binary itself is executable on AmigaOS
TEST: -x returns true for executable binary
CMD: WORK:test -x WORK:test
EXPECT_RC: 0

# =======================================================================
# SECTION 4: LOGICAL OPERATORS
# =======================================================================

# Native: /bin/test "a" = "a" -a "b" = "b" ; echo $?  -> 0
TEST: -a AND returns true when both sides are true
CMD: WORK:test hello = hello -a world = world
EXPECT_RC: 0

# Native: /bin/test "a" = "a" -a "b" = "c" ; echo $?  -> 1
TEST: -a AND returns false when second side is false
CMD: WORK:test hello = hello -a hello = world
EXPECT_RC: 1

# Native: /bin/test "a" = "b" -o "c" = "c" ; echo $?  -> 0
TEST: -o OR returns true when second side is true
CMD: WORK:test hello = world -o hello = hello
EXPECT_RC: 0

# Native: /bin/test "a" = "b" -o "b" = "c" ; echo $?  -> 1
TEST: -o OR returns false when both sides are false
CMD: WORK:test hello = world -o foo = bar
EXPECT_RC: 1

# Native: /bin/test ! "a" = "b" ; echo $?  -> 0  (NOT false = true)
TEST: Logical NOT inverts false to true
CMD: WORK:test ! hello = world
EXPECT_RC: 0

# Native: /bin/test ! "a" = "a" ; echo $?  -> 1  (NOT true = false)
TEST: Logical NOT inverts true to false
CMD: WORK:test ! hello = hello
EXPECT_RC: 1

# =======================================================================
# SECTION 5: PARENTHESES GROUPING
# =======================================================================

# Native: /bin/test \( "a" = "a" -o "b" = "c" \) -a "d" = "d" ; echo $?  -> 0
# AmigaDOS: ( and ) pass through as literals -- no shell quoting needed
TEST: Parentheses group OR before AND -- (true OR false) AND true = true
CMD: WORK:test ( hello = hello -o foo = bar ) -a world = world
EXPECT_RC: 0

# Native: /bin/test \( "a" = "b" -o "b" = "c" \) -a "d" = "d" ; echo $?  -> 1
TEST: Parentheses group OR before AND -- (false OR false) AND true = false
CMD: WORK:test ( hello = world -o foo = bar ) -a world = world
EXPECT_RC: 1

# Native: /bin/test \( "a" = "b" -o "b" = "b" \) -a "c" = "c" ; echo $?  -> 0
TEST: Parentheses group OR before AND -- (false OR true) AND true = true
CMD: WORK:test ( hello = world -o world = world ) -a hello = hello
EXPECT_RC: 0

# =======================================================================
# SECTION 6: POSIX SPECIAL ARGC CASES
# =======================================================================

# Native: /bin/test ; echo $?  -> 1  (argc=1, always false per POSIX)
# Source: case 1: return 1; -- immediate return, no stdin read
TEST: No arguments returns false (POSIX special case argc=1)
CMD: WORK:test
EXPECT_RC: 1

# Native: /bin/test hello ; echo $?  -> 0  (non-empty string is true)
# Source: case 2: return (*argv[1] == '\0'); -- non-empty returns 0 = TRUE
TEST: Single non-empty string argument returns true (POSIX special case argc=2)
CMD: WORK:test hello
EXPECT_RC: 0

# Native: /bin/test ! hello ; echo $?  -> 1
# Source: case 3: argc=3, argv[1]="!" -> return !(*argv[2] == '\0')
# argv[2]="hello", 'h' != '\0' so !false = NOT(false) = true... wait:
# return !(*argv[2] == '\0') = !('h' == '\0') = !(0) = 1 -> RC=1 (false)
TEST: NOT with single non-empty string returns false (POSIX special case argc=3)
CMD: WORK:test ! hello
EXPECT_RC: 1

# POSIX case 4 (optimised binop path): argc=4 with binary operator
# Source: case 4 checks t_wp_op->op_type == BINOP and calls binop() directly
TEST: POSIX optimised 4-argument path for binary comparison
CMD: WORK:test 10 -eq 10
EXPECT_RC: 0

# POSIX case 5 (! + binop): argc=5 with ! and binary operator
# Source: case 5 checks argv[1]=="!" and argv[3] is BINOP, calls !(binop())
TEST: POSIX optimised 5-argument path NOT with binary comparison (true becomes false)
CMD: WORK:test ! 5 -eq 5
EXPECT_RC: 1

TEST: POSIX optimised 5-argument path NOT with binary comparison (false becomes true)
CMD: WORK:test ! 5 -eq 6
EXPECT_RC: 0

# =======================================================================
# SECTION 7: FILE TIMESTAMP AND IDENTITY OPERATORS
# =======================================================================

# Native: /bin/test /etc/hosts -nt /etc/hosts ; echo $?  -> 1 (not newer than itself)
TEST: -nt returns false when file is compared to itself
CMD: WORK:test WORK:test-oneline.txt -nt WORK:test-oneline.txt
EXPECT_RC: 1

# Native: /bin/test /etc/hosts -ot /etc/hosts ; echo $?  -> 1 (not older than itself)
TEST: -ot returns false when file is compared to itself
CMD: WORK:test WORK:test-oneline.txt -ot WORK:test-oneline.txt
EXPECT_RC: 1

# Native: /bin/test /etc/hosts -nt /nonexistent ; echo $?  -> 1 (stat fails = false)
# Source: newerf() returns 0 if either full_stat_get() fails
TEST: -nt returns false when comparison file does not exist
CMD: WORK:test WORK:test-oneline.txt -nt WORK:no-such-file.txt
EXPECT_RC: 1

# Native: /bin/test /nonexistent -ot /etc/hosts ; echo $?  -> 1 (stat fails = false)
TEST: -ot returns false when base file does not exist
CMD: WORK:test WORK:no-such-file.txt -ot WORK:test-oneline.txt
EXPECT_RC: 1

# Native: /bin/test /etc/hosts -ef /etc/hosts ; echo $?  -> 0 (same st_dev + st_ino)
TEST: -ef returns true when file is compared to itself (same device and inode)
CMD: WORK:test WORK:test-oneline.txt -ef WORK:test-oneline.txt
EXPECT_RC: 0

# Native: /bin/test /etc/hosts -ef /etc/passwd ; echo $?  -> 1 (different inodes)
TEST: -ef returns false when files are different
CMD: WORK:test WORK:test-oneline.txt -ef WORK:test-multiline.txt
EXPECT_RC: 1

# =======================================================================
# SECTION 8: AMIGA-SPECIFIC OPERATOR BEHAVIOUR
# =======================================================================

# -O: geteuid() stubbed to 0, all AmigaOS files have uid=0 -> always true
TEST: -O returns true for any file (AmigaOS single-user geteuid always 0)
CMD: WORK:test -O WORK:test-oneline.txt
EXPECT_RC: 0

# -G: getegid() stubbed to 0, all AmigaOS files have gid=0 -> always true
TEST: -G returns true for any file (AmigaOS single-user getegid always 0)
CMD: WORK:test -G WORK:test-oneline.txt
EXPECT_RC: 0

# -L: AmigaOS FFS/OFS/SFS has no POSIX symlinks -- always false
TEST: -L returns false (no symbolic links on AmigaOS)
CMD: WORK:test -L WORK:test-oneline.txt
EXPECT_RC: 1

# -h: alias for -L -- always false on AmigaOS
TEST: -h returns false (alias for -L, no symbolic links on AmigaOS)
CMD: WORK:test -h WORK:test-oneline.txt
EXPECT_RC: 1

# -c: No character devices on AmigaOS FFS -- always false
TEST: -c returns false (no character devices on AmigaOS FFS)
CMD: WORK:test -c WORK:test-oneline.txt
EXPECT_RC: 1

# -b: No block devices on AmigaOS FFS -- always false
TEST: -b returns false (no block devices on AmigaOS FFS)
CMD: WORK:test -b WORK:test-oneline.txt
EXPECT_RC: 1

# -p: No FIFOs on AmigaOS FFS -- always false
TEST: -p returns false (no FIFOs on AmigaOS FFS)
CMD: WORK:test -p WORK:test-oneline.txt
EXPECT_RC: 1

# -S: No UNIX domain sockets on AmigaOS -- always false
TEST: -S returns false (no sockets on AmigaOS)
CMD: WORK:test -S WORK:test-oneline.txt
EXPECT_RC: 1

# -u: No setuid bit on AmigaOS -- always false
TEST: -u returns false (no setuid bit on AmigaOS)
CMD: WORK:test -u WORK:test-oneline.txt
EXPECT_RC: 1

# -g: No setgid bit on AmigaOS -- always false
TEST: -g returns false (no setgid bit on AmigaOS)
CMD: WORK:test -g WORK:test-oneline.txt
EXPECT_RC: 1

# -k: No sticky bit on AmigaOS -- always false
TEST: -k returns false (no sticky bit on AmigaOS)
CMD: WORK:test -k WORK:test-oneline.txt
EXPECT_RC: 1

# -t: isatty() -- in the test harness stdout is redirected to a file, so -t 1 = false
TEST: -t 1 returns false when stdout is redirected (not a tty in harness)
CMD: WORK:test -t 1
EXPECT_RC: 1

# =======================================================================
# SECTION 9: ERROR PATH TESTS -- RC=20 (RETURN_FAIL via errx(20,...))
# =======================================================================

# "test a b" -> after parsing "a" as expression, "b" is leftover
# Source: syntax(*t_wp, "unknown operand") -> errx(20, "b: unknown operand")
TEST: Unknown extra operand returns RC=20
CMD: WORK:test a b
EXPECT_RC: 20

# "test a = b c" -> after "a = b" is parsed, "c" is leftover
TEST: Extra operand after complete expression returns RC=20
CMD: WORK:test a = b c
EXPECT_RC: 20

# "test abc -eq def" -> getnstr() sees non-digit chars -> errx(20, "abc: invalid")
TEST: Non-numeric argument to -eq returns RC=20
CMD: WORK:test abc -eq def
EXPECT_RC: 20

# "test foo -gt bar" -> same path through getnstr()
TEST: Non-numeric argument to -gt returns RC=20
CMD: WORK:test foo -gt bar
EXPECT_RC: 20

# "test 5 -eq" -> binop() sees NULL opnd2 -> syntax("-eq", "argument expected")
# Source: if ((opnd2 = *++t_wp) == NULL) syntax(op->op_text, "argument expected")
TEST: Missing right operand for binary operator returns RC=20
CMD: WORK:test 5 -eq
EXPECT_RC: 20

# =======================================================================
# SECTION 10: EDGE CASE TESTS
# =======================================================================

# Integer zero
# Native: /bin/test 0 -eq 0 ; echo $?  -> 0
TEST: Integer zero equality check
CMD: WORK:test 0 -eq 0
EXPECT_RC: 0

# Negative integers -- getnstr() accepts leading '-' as sign
# Native: /bin/test -5 -lt 0 ; echo $?  -> 0
TEST: Negative integer comparison -5 -lt 0
CMD: WORK:test -5 -lt 0
EXPECT_RC: 0

# Native: /bin/test -5 -gt 0 ; echo $?  -> 1
TEST: Negative integer comparison -5 not greater than 0
CMD: WORK:test -5 -gt 0
EXPECT_RC: 1

# Leading zeros: getnstr() skips leading zeros, treats as decimal
# Native: /bin/test 007 -eq 7 ; echo $?  -> 0
TEST: Integer with leading zeros 007 equals 7
CMD: WORK:test 007 -eq 7
EXPECT_RC: 0

# POSIX case 2: operator-like string treated as plain string operand
# "test -f" with argc=2: returns (*argv[1] == '\0') = ('-' == '\0') = false -> RC=0 (true)
# Wait: '-' != '\0' so returns 0 which means...
# case 2: return (*argv[1] == '\0');
# *argv[1] = '-', '-' == '\0' is 0 (false), function returns 0 = TRUE (shell exit code 0)
TEST: POSIX case 2 treats operator-like flag as plain non-empty string (true)
CMD: WORK:test -f
EXPECT_RC: 0

# Nonexistent file for -f returns false (NOT an error)
TEST: -f returns false for nonexistent file (not an error, RC=1 not RC=20)
CMD: WORK:test -f WORK:no-such-file.txt
EXPECT_RC: 1

# Empty file: -e true, -s false
TEST: -e returns true for empty file (exists, zero bytes is still existence)
CMD: WORK:test -e WORK:test-empty.txt
EXPECT_RC: 0

TEST: -s returns false for empty file (zero size)
CMD: WORK:test -s WORK:test-empty.txt
EXPECT_RC: 1

# =======================================================================
# SECTION 11: REAL-WORLD AND STRESS TESTS
# =======================================================================

# Real-world: Shell script file processing guard
# Check a file exists, is regular, and is non-empty before processing
TEST: Real-world file guard check (exists AND regular AND non-empty) all true
CMD: WORK:test -e WORK:test-oneline.txt -a -f WORK:test-oneline.txt -a -s WORK:test-oneline.txt
EXPECT_RC: 0

# Real-world: Version string equality comparison used in scripts
TEST: Real-world version string equality 1.23 = 1.23
CMD: WORK:test 1.23 = 1.23
EXPECT_RC: 0

# Real-world: Version string inequality
TEST: Real-world version string inequality 1.22 != 1.23
CMD: WORK:test 1.22 != 1.23
EXPECT_RC: 0

# Real-world: Numeric range check (is count in acceptable range 1-100?)
# 50 -ge 1 AND 50 -le 100 = true
TEST: Real-world range check 50 in range 1 to 100
CMD: WORK:test 50 -ge 1 -a 50 -le 100
EXPECT_RC: 0

# Real-world: Range check with out-of-range value
# 150 -ge 1 AND 150 -le 100 = true AND false = false
TEST: Real-world range check 150 out of range 1 to 100
CMD: WORK:test 150 -ge 1 -a 150 -le 100
EXPECT_RC: 1

# Real-world: Negated existence check (file does NOT exist -- clean state)
TEST: Real-world negated existence check for nonexistent file
CMD: WORK:test ! -e WORK:no-such-file-xyz.txt
EXPECT_RC: 0

# Real-world: Directory existence check before creating a file in it
TEST: Real-world directory existence check for known directory
CMD: WORK:test -d SYS:Rexxc
EXPECT_RC: 0

# Real-world: Complex parenthesised expression
# (regular AND non-empty) OR directory -- true for test-oneline.txt
TEST: Real-world parenthesised (regular AND non-empty) OR directory
CMD: WORK:test ( -f WORK:test-oneline.txt -a -s WORK:test-oneline.txt ) -o -d WORK:test-oneline.txt
EXPECT_RC: 0

# Stress: Deep -a chain -- four integer comparisons all true
# Tests that expression parser handles multiple BAND tokens correctly
# Native: test 1 -lt 2 -a 3 -lt 4 -a 5 -lt 6 -a 7 -lt 8 -> 0
TEST: Stress deep -a chain with four integer comparisons all true
CMD: WORK:test 1 -lt 2 -a 3 -lt 4 -a 5 -lt 6 -a 7 -lt 8
EXPECT_RC: 0

# Stress: Deep -o chain -- first true short-circuits evaluation
# Native: test 1 -eq 1 -o 2 -eq 3 -o 4 -eq 5 -o 6 -eq 7 -> 0
TEST: Stress deep -o chain first-true short-circuits rest
CMD: WORK:test 1 -eq 1 -o 2 -eq 3 -o 4 -eq 5 -o 6 -eq 7
EXPECT_RC: 0

# Stress: Deep -o chain where all are false
# Native: test 1 -eq 2 -o 3 -eq 4 -o 5 -eq 6 -o 7 -eq 8 -> 1
TEST: Stress deep all-false -o chain returns false
CMD: WORK:test 1 -eq 2 -o 3 -eq 4 -o 5 -eq 6 -o 7 -eq 8
EXPECT_RC: 1

# Stress: Nested NOT with integer comparison
# ! (5 -eq 5) -> NOT true -> false (RC=1)
TEST: Stress NOT wrapping true integer equality returns false
CMD: WORK:test ! 5 -eq 5
EXPECT_RC: 1

# Stress: Large integer near 32-bit boundary
# 2147483647 is INT_MAX on 32-bit systems; test's getnstr handles it
# Native: /bin/test 2147483647 -gt 2147483646 -> 0
TEST: Stress large integer at 32-bit boundary 2147483647 -gt 2147483646
CMD: WORK:test 2147483647 -gt 2147483646
EXPECT_RC: 0

# Precision: getnstr() length-based comparison algorithm
# 9999 vs 10000: different digit counts; 9999 has fewer digits -> smaller
# Native: /bin/test 9999 -lt 10000 -> 0
TEST: Precision different-digit-count integers 9999 -lt 10000
CMD: WORK:test 9999 -lt 10000
EXPECT_RC: 0

# Precision: Same digit count, lexicographic comparison via strncmp()
# 12345 vs 12346: same length, strncmp("12345","12346",5) < 0
# Native: /bin/test 12345 -lt 12346 -> 0
TEST: Precision same-digit-count integers 12345 -lt 12346
CMD: WORK:test 12345 -lt 12346
EXPECT_RC: 0

# Precision: getnstr() sign-based comparison
# Positive vs negative: sig1=1, sig2=-1, different signs -> c = sig1 = 1 (positive is greater)
# Native: /bin/test 1 -gt -1 -> 0
TEST: Precision sign-based comparison positive 1 -gt negative 1
CMD: WORK:test 1 -gt -1
EXPECT_RC: 0

# Precision: Zero is always treated as positive by getnstr()
# Source: "turn 0 always positive" -> *signum = 1 for "0"
# Native: /bin/test 0 -eq -0 -> may differ? Test 0 = 0 with sign handling
# Actually -0 would be parsed as neg sign + 0, then "turn 0 always positive"
# so sig=-1 gets overridden to sig=1. Both 0 and -0 have sig=1, same length=1
# strncmp("0","0",1) = 0. So 0 -eq -0 should return true.
# Native: /bin/test 0 -eq -0 ... hmm, -0 might error on some systems
# Let's use a safer precision test: test that -ge boundary works at -5
# Native: /bin/test -5 -ge -5 -> 0
TEST: Precision negative integer -ge boundary -5 -ge -5
CMD: WORK:test -5 -ge -5
EXPECT_RC: 0

TEST: Precision negative integer -le boundary -5 -le -5
CMD: WORK:test -5 -le -5
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
passed=97
failed=0
total=97
```

---
Generated by `make test-fsemu TARGET=ports/test`
Report template: `toolchain/templates/test-report.md.template`
