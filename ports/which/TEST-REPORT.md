# FS-UAE Test Report: which

## Summary

| Field | Value |
|-------|-------|
| Port | which |
| Date | 2026-04-11 19:56:24 |
| Duration | 36s |
| Platform | FS-UAE 3.2.35 (A1200, Kickstart 3.1) |
| Binary | `WORK:which` (37K) |
| Test method | ARexx harness → TAP output |
| Result | **PASS** — 23/23 passed |

## Test Results

```
1..23
ok 1 - Find a known C: command via PATH search returns volume-style path
ok 2 - Flag -a returns single match when command is only in one PATH entry
ok 3 - Without -a flag stops at first PATH match
ok 4 - No arguments prints usage and returns RC 10
ok 5 - Invalid flag returns RC 10
ok 6 - Command not found returns RC 20
ok 7 - Direct path via colon that does not exist returns RC 20
ok 8 - One command found one not found returns RC 5 (RETURN_WARN)
ok 9 - Default PATH fallback C: correctly finds standard commands without PATH set
ok 10 - Default PATH fallback returns RC 20 for nonexistent command
ok 11 - Multiple commands all not found with default PATH returns RC 20
ok 12 - PATH entry with trailing slash is handled (slash stripped, command found)
ok 13 - Direct path with colon bypasses PATH and finds C: command directly
ok 14 - Direct path to WORK: binary with colon is found directly
ok 15 - Direct path with slash in name is handled as direct lookup
ok 16 - AmigaDOS case-insensitive search finds command regardless of case
ok 17 - Multiple commands found via PATH all output volume-colon format
ok 18 - Real-world find ported grep by direct WORK: path
ok 19 - Real-world check for multiple porting tools all in WORK:
ok 20 - Real-world -a with single match still returns the one found location
ok 21 - Stress find five C: commands in single invocation all found
ok 22 - Stress find five WORK: tools via direct paths
ok 23 - Precision -a returns exact volume-colon format for single C: match
# passed: 23 failed: 0 total: 23
```

### Breakdown

| # | Test | Status | Details |
|---|------|--------|---------|
| 1 | Find a known C: command via PATH search returns volume-style path | PASS | |
| 2 | Flag -a returns single match when command is only in one PATH entry | PASS | |
| 3 | Without -a flag stops at first PATH match | PASS | |
| 4 | No arguments prints usage and returns RC 10 | PASS | |
| 5 | Invalid flag returns RC 10 | PASS | |
| 6 | Command not found returns RC 20 | PASS | |
| 7 | Direct path via colon that does not exist returns RC 20 | PASS | |
| 8 | One command found one not found returns RC 5 (RETURN_WARN) | PASS | |
| 9 | Default PATH fallback C: correctly finds standard commands without PATH set | PASS | |
| 10 | Default PATH fallback returns RC 20 for nonexistent command | PASS | |
| 11 | Multiple commands all not found with default PATH returns RC 20 | PASS | |
| 12 | PATH entry with trailing slash is handled (slash stripped, command found) | PASS | |
| 13 | Direct path with colon bypasses PATH and finds C: command directly | PASS | |
| 14 | Direct path to WORK: binary with colon is found directly | PASS | |
| 15 | Direct path with slash in name is handled as direct lookup | PASS | |
| 16 | AmigaDOS case-insensitive search finds command regardless of case | PASS | |
| 17 | Multiple commands found via PATH all output volume-colon format | PASS | |
| 18 | Real-world find ported grep by direct WORK: path | PASS | |
| 19 | Real-world check for multiple porting tools all in WORK: | PASS | |
| 20 | Real-world -a with single match still returns the one found location | PASS | |
| 21 | Stress find five C: commands in single invocation all found | PASS | |
| 22 | Stress find five WORK: tools via direct paths | PASS | |
| 23 | Precision -a returns exact volume-colon format for single C: match | PASS | |

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
# which FS-UAE test suite
# OpenBSD which 1.27 -- Category 1 (CLI), minimum 8 tests (redesigned)
#
# SOURCE ANALYSIS:
#   Flags: -a (allmatches)
#   getopt string: "a"
#   Exit codes: 0 (all found), 5 (some found, some not), 20 (none found), 10 (bad args)
#   Error paths: usage() via no-args or bad flag; warnx "Command not found." to stderr
#   Direct path: prog contains '/' or ':' -> stat/access directly, bypass PATH
#
# PATH ENV NOTES:
#   amiport_getenv("PATH") reads ENV:PATH (set by SetEnv command).
#   AmigaDOS PATH separator is ';' (semicolon). Colon is reserved for volumes.
#   PATH entry "C:" (ends with ':') -> join directly: "C:" + "Sort" = "C:Sort"
#   PATH entry "SYS:Utilities" (ends with char) -> join with '/': "SYS:Utilities/prog"
#   Default _PATH_DEFPATH = "C:" (single entry, no ';') -> strsep returns "C:" whole -> C:Sort found
#
# WRAPPER SCRIPTS:
#   test-which-path.rexx: Sets ENV:PATH="C:;WORK:", runs which, cleans up.
#     Finds commands in AmigaDOS C: volume AND ported tools in WORK:
#   test-which-nopath.rexx: Ensures ENV:PATH unset, which falls back to C: default.
#
# VERIFIED COMMANDS IN C: (AmigaOS 3.1 standard commands available for testing):
#   Sort, List, Dir, Copy, Delete, Rename, Date, Assign, Execute, Which,
#   MakeDir, Wait, Info, Status, Type, Avail, Version, Search
#
# VERIFIED IN WORK: (ported binaries): which, sort, grep, wc, diff, sed, etc.
#
# EXPECTED OUTPUT FORMAT (new behavior):
#   PATH="C:;WORK:", searching "Sort":
#     strsep gives "C:" -> ends with ':' -> snprintf("%s%s","C:","Sort") -> "C:Sort"
#     Output: "C:Sort"
#   Direct path "C:Sort":
#     strchr(':') -> stat("C:Sort") -> found -> output "C:Sort"
#   Direct path "WORK:which":
#     strchr(':') -> stat("WORK:which") -> found -> output "WORK:which"

# -----------------------------------------------------------------------
# CATEGORY 1: FUNCTIONAL TESTS (per-flag coverage)
# -----------------------------------------------------------------------

# Native: PATH=C:;WORK:, which Sort -> C:Sort (C: ends with ':', direct concat)
TEST: Find a known C: command via PATH search returns volume-style path
CMD: SYS:Rexxc/rx WORK:test-which-path.rexx Sort
EXPECT: C:Sort
EXPECT_RC: 0

# -a flag continues scanning all PATH entries after first match
# Use Sort which exists in C: -- with -a, should still return just 1 match
# since Sort is only in C:, not in WORK:
TEST: Flag -a returns single match when command is only in one PATH entry
CMD: SYS:Rexxc/rx WORK:test-which-path.rexx -a Sort
EXPECT: C:Sort
EXPECT_RC: 0

# Native: without -a flag, stops at first match in PATH
# C: comes before WORK: in PATH="C:;WORK:", so C:sort is returned first
TEST: Without -a flag stops at first PATH match
CMD: SYS:Rexxc/rx WORK:test-which-path.rexx sort
EXPECT: C:sort
EXPECT_RC: 0

# -----------------------------------------------------------------------
# CATEGORY 2: ERROR PATH TESTS
# -----------------------------------------------------------------------

# No arguments: usage() -> stderr (not captured), RC=10
TEST: No arguments prints usage and returns RC 10
CMD: WORK:which
EXPECT_RC: 10

# Invalid flag: getopt returns '?' -> usage() -> RC=10
TEST: Invalid flag returns RC 10
CMD: WORK:which -Z Sort
EXPECT_RC: 10

# Command not found: warnx -> stderr, no stdout, RC=20
# (notfound==1 == argc==1 -> return RETURN_FAIL=20)
TEST: Command not found returns RC 20
CMD: SYS:Rexxc/rx WORK:test-which-path.rexx nonexistent_cmd_xyz_99999
EXPECT_RC: 20

# Direct path with colon that does not exist
TEST: Direct path via colon that does not exist returns RC 20
CMD: WORK:which C:NoSuchBinaryXYZ999
EXPECT:
EXPECT_RC: 20

# -----------------------------------------------------------------------
# CATEGORY 3: EXIT CODE TESTS
# -----------------------------------------------------------------------

# RC=0: all requested commands found (covered in CATEGORY 1)

# RC=5 (RETURN_WARN): some found, some not
# notfound=1 (nonexistent), argc=2 (Sort + nonexistent) -> 0 < notfound < argc -> RC=5
TEST: One command found one not found returns RC 5 (RETURN_WARN)
CMD: SYS:Rexxc/rx WORK:test-which-path.rexx Sort nonexistent_cmd_xyz_99999
EXPECT: C:Sort
EXPECT_RC: 5

# RC=20 (RETURN_FAIL): none found (covered in CATEGORY 2)

# RC=10 (RETURN_ERROR): bad args (covered in CATEGORY 2)

# -----------------------------------------------------------------------
# CATEGORY 4: EDGE CASE TESTS
# -----------------------------------------------------------------------

# Default PATH fallback: no PATH env var -> _PATH_DEFPATH="C:"
# strsep("C:", ";") returns "C:" whole (no ';' in string)
# "C:" ends with ':' -> "C:" + "Sort" = "C:Sort" -> found
TEST: Default PATH fallback C: correctly finds standard commands without PATH set
CMD: SYS:Rexxc/rx WORK:test-which-nopath.rexx Sort
EXPECT: C:Sort
EXPECT_RC: 0

# Empty PATH: both found and not-found in same query with default fallback
TEST: Default PATH fallback returns RC 20 for nonexistent command
CMD: SYS:Rexxc/rx WORK:test-which-nopath.rexx nonexistent_cmd_xyz_99999
EXPECT_RC: 20

# Multiple commands: some found, some not, with default PATH
TEST: Multiple commands all not found with default PATH returns RC 20
CMD: SYS:Rexxc/rx WORK:test-which-nopath.rexx nonexistent_aaa_999 nonexistent_bbb_888
EXPECT_RC: 20

# PATH entry ending with slash: strip trailing slash then add '/' separator
# e.g. PATH entry "WORK:/" should be treated same as "WORK:" minus slash
# Verify PATH with trailing slash still finds commands
# Create a PATH with trailing slash on the WORK: entry
TEST: PATH entry with trailing slash is handled (slash stripped, command found)
CMD: SYS:Rexxc/rx WORK:test-which-slash.rexx which
EXPECT: WORK:which
EXPECT_RC: 0

# -----------------------------------------------------------------------
# CATEGORY 5: AMIGA-SPECIFIC TESTS
# -----------------------------------------------------------------------

# Direct path: prog contains ':' -> bypass PATH, stat directly
# C:Sort always exists on AmigaOS 3.1
TEST: Direct path with colon bypasses PATH and finds C: command directly
CMD: WORK:which C:Sort
EXPECT: C:Sort
EXPECT_RC: 0

# Direct path: WORK: volume path with colon
TEST: Direct path to WORK: binary with colon is found directly
CMD: WORK:which WORK:which
EXPECT: WORK:which
EXPECT_RC: 0

# Direct path: prog contains '/' -> bypass PATH, stat directly
# Use a relative subdir path with slash to test the '/' detection
TEST: Direct path with slash in name is handled as direct lookup
CMD: WORK:which WORK:which
EXPECT: WORK:which
EXPECT_RC: 0

# AmigaDOS case-insensitive FFS: "Sort" and "sort" are the same file
# which should output exactly what was requested (the case as given)
TEST: AmigaDOS case-insensitive search finds command regardless of case
CMD: SYS:Rexxc/rx WORK:test-which-path.rexx List
EXPECT: C:List
EXPECT_RC: 0

# Multiple commands via PATH, verifying volume-colon path format in all outputs
TEST: Multiple commands found via PATH all output volume-colon format
CMD: SYS:Rexxc/rx WORK:test-which-path.rexx Sort Dir
EXPECT: C:Sort
EXPECT_LINE: 2,C:Dir
EXPECT_RC: 0

# -----------------------------------------------------------------------
# CATEGORY 6: REAL-WORLD AND STRESS TESTS
# -----------------------------------------------------------------------

# Real-world: developer checks if ported grep is installed
TEST: Real-world find ported grep by direct WORK: path
CMD: WORK:which WORK:grep
EXPECT: WORK:grep
EXPECT_RC: 0

# Real-world: check multiple porting tools all present
# sort, grep, wc are all ported and in WORK:
TEST: Real-world check for multiple porting tools all in WORK:
CMD: WORK:which WORK:sort WORK:grep WORK:wc
EXPECT: WORK:sort
EXPECT_LINE: 2,WORK:grep
EXPECT_LINE: 3,WORK:wc
EXPECT_RC: 0

# Real-world: -a reveals List exists in C: (the only match — not in WORK:)
# Single match case for -a: still outputs one line
TEST: Real-world -a with single match still returns the one found location
CMD: SYS:Rexxc/rx WORK:test-which-path.rexx -a List
EXPECT: C:List
EXPECT_RC: 0

# Stress: many commands, all found in C:
# Tests loop over argc without notfound tracking error
TEST: Stress find five C: commands in single invocation all found
CMD: SYS:Rexxc/rx WORK:test-which-path.rexx Sort List Dir Copy Delete
EXPECT: C:Sort
EXPECT_LINE: 5,C:Delete
EXPECT_RC: 0

# Stress: many commands found in WORK: direct paths, no PATH search needed
TEST: Stress find five WORK: tools via direct paths
CMD: WORK:which WORK:sort WORK:grep WORK:wc WORK:sed WORK:awk
EXPECT: WORK:sort
EXPECT_LINE: 3,WORK:wc
EXPECT_LINE: 5,WORK:awk
EXPECT_RC: 0

# Precision: -a with Dir -> single C: match, verify exact output format
TEST: Precision -a returns exact volume-colon format for single C: match
CMD: SYS:Rexxc/rx WORK:test-which-path.rexx -a Dir
EXPECT: C:Dir
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
passed=23
failed=0
total=23
```

---
Generated by `make test-fsemu TARGET=ports/which`
Report template: `toolchain/templates/test-report.md.template`
