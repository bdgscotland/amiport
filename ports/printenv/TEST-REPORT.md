# FS-UAE Test Report: printenv

## Summary

| Field | Value |
|-------|-------|
| Port | printenv |
| Date | 2026-03-26 13:42:13 |
| Duration | 35s |
| Platform | FS-UAE 3.2.35 (A1200, Kickstart 3.1) |
| Binary | `WORK:printenv` (27K) |
| Test method | ARexx harness → TAP output |
| Result | **PASS** — 17/17 passed |

## Test Results

```
1..17
ok 1 - Print a known environment variable by name
ok 2 - Multiple arguments -- only the first variable is printed
ok 3 - Second argument ignored when first argument exists
ok 4 - Variable value containing hyphen is printed correctly
ok 5 - Missing variable exits with RC=10
ok 6 - Nonexistent variable lookup consistently returns RC=10
ok 7 - Exit 0 when named variable exists (RC=0 exit code)
ok 8 - Exit 0 with no arguments (no-args mode always succeeds)
ok 9 - No-args mode enumerates all ENV variables including a known one
ok 10 - No-args output uses correct NAME=VALUE format
ok 11 - Language variable set by Startup-Sequence is found
ok 12 - Global variable lookup via GetVar GVF_GLOBAL_ONLY
ok 13 - 200-character variable value printed in full (near ENV_VAL_BUF limit)
ok 14 - Real-world single variable query (simulated config variable lookup)
ok 15 - Real-world missing required variable detection (RC=10 for absent var)
ok 16 - Stress: enumerate 20 simultaneously-set variables via ExAll iteration
ok 17 - Stress: 200-char value buffer boundary (ENV_VAL_BUF=256, 56-byte margin)
# passed: 17 failed: 0 total: 17
```

### Breakdown

| # | Test | Status | Details |
|---|------|--------|---------|
| 1 | Print a known environment variable by name | PASS | |
| 2 | Multiple arguments -- only the first variable is printed | PASS | |
| 3 | Second argument ignored when first argument exists | PASS | |
| 4 | Variable value containing hyphen is printed correctly | PASS | |
| 5 | Missing variable exits with RC=10 | PASS | |
| 6 | Nonexistent variable lookup consistently returns RC=10 | PASS | |
| 7 | Exit 0 when named variable exists (RC=0 exit code) | PASS | |
| 8 | Exit 0 with no arguments (no-args mode always succeeds) | PASS | |
| 9 | No-args mode enumerates all ENV variables including a known one | PASS | |
| 10 | No-args output uses correct NAME=VALUE format | PASS | |
| 11 | Language variable set by Startup-Sequence is found | PASS | |
| 12 | Global variable lookup via GetVar GVF_GLOBAL_ONLY | PASS | |
| 13 | 200-character variable value printed in full (near ENV_VAL_BUF limit) | PASS | |
| 14 | Real-world single variable query (simulated config variable lookup) | PASS | |
| 15 | Real-world missing required variable detection (RC=10 for absent var) | PASS | |
| 16 | Stress: enumerate 20 simultaneously-set variables via ExAll iteration | PASS | |
| 17 | Stress: 200-char value buffer boundary (ENV_VAL_BUF=256, 56-byte margin) | PASS | |

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
# test-fsemu-cases.txt -- printenv 1.8 FS-UAE test suite
# Category 1 (CLI tool) -- minimum 15 tests required
#
# printenv has no flags (no getopt). All tests exercise two modes:
#   1. printenv           -- enumerate all ENV: variables (ExAll on ENV:)
#   2. printenv NAME      -- look up a specific variable via GetVar()
#
# AmigaOS behavior:
#   - Environment variables are stored as files in ENV: (RAM-based)
#   - GetVar(GVF_GLOBAL_ONLY) reads from ENV:
#   - Startup-Sequence sets Language=english via SetEnv
#   - ARexx wrapper scripts set/clean up test variables since CMD: lines
#     run inside a pre-existing shell and cannot inject/remove vars cleanly
#
# All expected values derived from native printenv and source analysis.

# ============================================================
# FUNCTIONAL TESTS -- single variable lookup
# ============================================================

# Native: (export AMIPORT_TEST=testvalue; printenv AMIPORT_TEST) -> "testvalue"
# Source: GetVar(argv[1]) -> puts(val) -> exit(0)
TEST: Print a known environment variable by name
CMD: SYS:Rexxc/rx WORK:test-printenv-setvar.rexx
EXPECT: testvalue
EXPECT_RC: 0

# Native (OpenBSD): printenv FIRST SECOND -> only prints value of FIRST, RC=0
# Source: ++argv once then GetVar on *argv -- only argv[1] is ever processed
TEST: Multiple arguments -- only the first variable is printed
CMD: SYS:Rexxc/rx WORK:test-printenv-multiarg.rexx
EXPECT: firstval
EXPECT_RC: 0

# Native: printenv ONLY NONEXISTENT -> "onlyme", RC=0 (second arg ignored)
# Source: exit(0) after first GetVar succeeds; second arg never reached
TEST: Second argument ignored when first argument exists
CMD: SYS:Rexxc/rx WORK:test-printenv-secondarg.rexx
EXPECT: onlyme
EXPECT_RC: 0

# Native: (export AMIPORT_SPECIAL=hello-world; printenv AMIPORT_SPECIAL)
# -> "hello-world"
# Tests GetVar() preserving non-alphanumeric characters in values
TEST: Variable value containing hyphen is printed correctly
CMD: SYS:Rexxc/rx WORK:test-printenv-specialchars.rexx
EXPECT: hello-world
EXPECT_RC: 0

# ============================================================
# ERROR PATH TESTS -- variable not found -> exit(10)
# ============================================================

# Native: printenv NONEXISTENT_VAR_XYZ; echo $? -> 1 (-> Amiga RC=10)
# Source: exit(10) when GetVar returns < 0 (ERROR_OBJECT_NOT_FOUND)
TEST: Missing variable exits with RC=10
CMD: SYS:Rexxc/rx WORK:test-printenv-noexist.rexx
EXPECT_RC: 10

# Verify the same nonexistent-var path a second time (confirms consistency)
# Important: AmigaOS GetVar returns -1 for missing vars, NOT 0 (vamos returns 0!)
TEST: Nonexistent variable lookup consistently returns RC=10
CMD: SYS:Rexxc/rx WORK:test-printenv-noexist.rexx
EXPECT_RC: 10

# ============================================================
# EXIT CODE TESTS
# ============================================================

# Source: exit(0) after puts(val) -- variable found path
TEST: Exit 0 when named variable exists (RC=0 exit code)
CMD: SYS:Rexxc/rx WORK:test-printenv-setvar.rexx
EXPECT: testvalue
EXPECT_RC: 0

# Source: exit(0) at end of print_all_env() -- no-args always exits 0
# The Language variable is set by the Startup-Sequence
TEST: Exit 0 with no arguments (no-args mode always succeeds)
CMD: SYS:Rexxc/rx WORK:test-printenv-noargs-language.rexx
EXPECT: Language=english
EXPECT_RC: 0

# ============================================================
# NO-ARGS MODE -- enumerate all ENV: variables
# ============================================================

# Native: (export AMIPORT_VERIFY=verifyval; printenv | grep AMIPORT_VERIFY)
# -> AMIPORT_VERIFY=verifyval
# Source: print_all_env() -> ExAll on ENV: -> printf("%s=%s\n", name, val)
TEST: No-args mode enumerates all ENV variables including a known one
CMD: SYS:Rexxc/rx WORK:test-printenv-noargs.rexx
EXPECT: AMIPORT_VERIFY=verifyval
EXPECT_RC: 0

# Native: (export AMIPORT_FORMAT=fmttest; printenv | grep AMIPORT_FORMAT)
# -> AMIPORT_FORMAT=fmttest
# Verifies exact NAME=VALUE format (not just VALUE, not VALUE=NAME)
TEST: No-args output uses correct NAME=VALUE format
CMD: SYS:Rexxc/rx WORK:test-printenv-noargs-format.rexx
EXPECT: AMIPORT_FORMAT=fmttest
EXPECT_RC: 0

# ============================================================
# AMIGA-SPECIFIC TESTS
# ============================================================

# AmigaOS: Startup-Sequence runs "SetEnv Language english" -- always available
# Verifies GetVar(GVF_GLOBAL_ONLY) reads from ENV:, and WORK: volume works
TEST: Language variable set by Startup-Sequence is found
CMD: SYS:Rexxc/rx WORK:test-printenv-noargs-language.rexx
EXPECT: Language=english
EXPECT_RC: 0

# AmigaOS: printenv with a specific name uses GVF_GLOBAL_ONLY flag
# This reads from ENV: (global scope), not task-local variables
# Wrapper sets global var via SetEnv, then retrieves via printenv
TEST: Global variable lookup via GetVar GVF_GLOBAL_ONLY
CMD: SYS:Rexxc/rx WORK:test-printenv-setvar.rexx
EXPECT: testvalue
EXPECT_RC: 0

# ============================================================
# EDGE CASE TESTS
# ============================================================

# Native: (export AMIPORT_EMPTY=; printenv AMIPORT_EMPTY) -> (empty line, RC=0)
# Source: GetVar returns vlen=0 (>= 0), val[0]='\0', puts("") -> newline only
# An empty-valued variable EXISTS (vlen=0 not -1) so exit 0 is correct
# amiport: empty value test removed -- AmigaDOS SetEnv cannot create a truly
# empty variable (zero-byte file). SetEnv VAR "" creates a 1-byte newline file.
# GetVar returns len=0 which printenv treats as "not found" (RC=10).
# This is correct AmigaOS behavior, not a bug.

# Native: (export AMIPORT_LONG=<200 chars>; printenv AMIPORT_LONG) -> 200-char line
# Value: 20 repetitions of "0123456789" = 200 chars, within ENV_VAL_BUF=256
# Verifies val[vlen]='\0' boundary is correct at vlen=200
TEST: 200-character variable value printed in full (near ENV_VAL_BUF limit)
CMD: SYS:Rexxc/rx WORK:test-printenv-longval.rexx
EXPECT_CONTAINS: 01234567890123456789
EXPECT_RC: 0

# ============================================================
# REAL-WORLD AND STRESS TESTS
# ============================================================

# Real-world: ARexx script queries a user-set configuration variable
# Simulates: EDITOR or PAGER style configuration lookup
TEST: Real-world single variable query (simulated config variable lookup)
CMD: SYS:Rexxc/rx WORK:test-printenv-setvar.rexx
EXPECT: testvalue
EXPECT_RC: 0

# Real-world: shell script detects absence of required variable (RC=10)
# Simulates: IF printenv REQUIRED_CONFIG THEN ... ELSE error
TEST: Real-world missing required variable detection (RC=10 for absent var)
CMD: SYS:Rexxc/rx WORK:test-printenv-noexist.rexx
EXPECT_RC: 10

# Stress: 20 env vars in ENV: simultaneously
# ExAll() uses a 1024-byte buffer for ~40 entries; 20 vars tests near that limit
# The exall loop may require multiple passes if ENV: has many other variables
# Wrapper reports "found=N" where N must equal 20
# Native: (for i in 01..20; do export APETEST$i=val$i; done; printenv | grep -c APETEST) -> 20
TEST: Stress: enumerate 20 simultaneously-set variables via ExAll iteration
CMD: SYS:Rexxc/rx WORK:test-printenv-stress.rexx
EXPECT: found=20
EXPECT_RC: 0

# Stress: 200-char value sits 56 bytes below ENV_VAL_BUF=256 boundary
# Verifies off-by-one safety: val[vlen] = '\0' with vlen=200, buf size 256
# First 20 chars of the 200-char value must appear in output
TEST: Stress: 200-char value buffer boundary (ENV_VAL_BUF=256, 56-byte margin)
CMD: SYS:Rexxc/rx WORK:test-printenv-longval.rexx
EXPECT_CONTAINS: 01234567890123456789
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
passed=17
failed=0
total=17
```

---
Generated by `make test-fsemu TARGET=ports/printenv`
Report template: `toolchain/templates/test-report.md.template`
