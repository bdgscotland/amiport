# FS-UAE Test Report: dropbear

## Summary

| Field | Value |
|-------|-------|
| Port | dropbear |
| Date | 2026-04-15 21:18:20 |
| Duration | 85s |
| Platform | FS-UAE 3.2.35 (A1200, Kickstart 3.1) |
| Binary | `WORK:dropbear` (unknown) |
| Test method | ARexx harness → TAP output |
| Result | **PASS** — 23/23 passed |

## Test Results

```
1..23
ok 1 - version flag exits cleanly
ok 2 - help flag exits cleanly
ok 3 - no args shows usage
ok 4 - bad flag returns error
ok 5 - missing host argument
ok 6 - missing host entirely (command treated as host)
ok 7 - invalid user@host syntax
ok 8 - SSH echo hello
ok 9 - SSH uname returns OS name
ok 10 - SSH cat /etc/hosts returns localhost
ok 11 - SSH pwd returns home directory
ok 12 - SSH whoami returns username
ok 13 - SSH date produces current year
ok 14 - SSH multi-word echo preserved
ok 15 - SSH very long command line (200 chars)
ok 16 - remote exit 0 propagates
ok 17 - remote exit 1 propagates
ok 18 - remote exit 5 propagates
ok 19 - remote true returns 0
ok 20 - remote false returns nonzero
ok 21 - connection to bad port fails
ok 22 - command producing no output still exits cleanly
ok 23 - exit command terminates session cleanly
# passed: 23 failed: 0 total: 23
```

### Breakdown

| # | Test | Status | Details |
|---|------|--------|---------|
| 1 | version flag exits cleanly | PASS | |
| 2 | help flag exits cleanly | PASS | |
| 3 | no args shows usage | PASS | |
| 4 | bad flag returns error | PASS | |
| 5 | missing host argument | PASS | |
| 6 | missing host entirely (command treated as host) | PASS | |
| 7 | invalid user@host syntax | PASS | |
| 8 | SSH echo hello | PASS | |
| 9 | SSH uname returns OS name | PASS | |
| 10 | SSH cat /etc/hosts returns localhost | PASS | |
| 11 | SSH pwd returns home directory | PASS | |
| 12 | SSH whoami returns username | PASS | |
| 13 | SSH date produces current year | PASS | |
| 14 | SSH multi-word echo preserved | PASS | |
| 15 | SSH very long command line (200 chars) | PASS | |
| 16 | remote exit 0 propagates | PASS | |
| 17 | remote exit 1 propagates | PASS | |
| 18 | remote exit 5 propagates | PASS | |
| 19 | remote true returns 0 | PASS | |
| 20 | remote false returns nonzero | PASS | |
| 21 | connection to bad port fails | PASS | |
| 22 | command producing no output still exits cleanly | PASS | |
| 23 | exit command terminates session cleanly | PASS | |

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
# Dropbear SSH client (dbclient) -- FS-UAE test cases
# Category 4 (Network) -- uses FS-UAE bsdsocket passthrough
#
# Password auth uses DROPBEAR_PASSWORD env var via test-dbclient-run.rexx
# wrapper (SetEnv + dbclient in same ARexx context). Host key auto-accepted
# via -y flag. Version/help output goes to stderr (not captured by harness).
#
# All network tests assume a live SSH server at 127.0.0.1 with user duncan,
# password Zimacs501482. Non-interactive commands only.

# === ARGUMENT PARSING (no network required) ===

TEST: version flag exits cleanly
CMD: WORK:dbclient -V
EXPECT_RC: 0

TEST: help flag exits cleanly
CMD: WORK:dbclient --help
EXPECT_RC: 0

TEST: no args shows usage
CMD: WORK:dbclient
EXPECT_RC: 1

TEST: bad flag returns error
CMD: WORK:dbclient -Z
EXPECT_RC: 1

TEST: missing host argument
CMD: WORK:dbclient -l duncan
EXPECT_RC: 1

TEST: missing host entirely (command treated as host)
CMD: WORK:dbclient echo hello
EXPECT_RC: 1

TEST: invalid user@host syntax
CMD: WORK:dbclient -y @127.0.0.1 echo hello
EXPECT_RC: 1

# === NON-INTERACTIVE COMMAND EXECUTION (SSH required) ===

TEST: SSH echo hello
CMD: SYS:Rexxc/rx WORK:test-dbclient-run.rexx Zimacs501482 127.0.0.1 echo hello
EXPECT_CONTAINS: hello
EXPECT_RC: 0

TEST: SSH uname returns OS name
CMD: SYS:Rexxc/rx WORK:test-dbclient-run.rexx Zimacs501482 127.0.0.1 uname
EXPECT_CONTAINS: Darwin
EXPECT_RC: 0

TEST: SSH cat /etc/hosts returns localhost
CMD: SYS:Rexxc/rx WORK:test-dbclient-run.rexx Zimacs501482 127.0.0.1 cat /etc/hosts
EXPECT_CONTAINS: localhost
EXPECT_RC: 0

TEST: SSH pwd returns home directory
CMD: SYS:Rexxc/rx WORK:test-dbclient-run.rexx Zimacs501482 127.0.0.1 pwd
EXPECT_CONTAINS: /Users/duncan
EXPECT_RC: 0

TEST: SSH whoami returns username
CMD: SYS:Rexxc/rx WORK:test-dbclient-run.rexx Zimacs501482 127.0.0.1 whoami
EXPECT_CONTAINS: duncan
EXPECT_RC: 0

TEST: SSH date produces current year
CMD: SYS:Rexxc/rx WORK:test-dbclient-run.rexx Zimacs501482 127.0.0.1 date
EXPECT_CONTAINS: 202
EXPECT_RC: 0

TEST: SSH multi-word echo preserved
CMD: SYS:Rexxc/rx WORK:test-dbclient-run.rexx Zimacs501482 127.0.0.1 echo hello world from amiga
EXPECT_CONTAINS: hello world from amiga
EXPECT_RC: 0

TEST: SSH very long command line (200 chars)
CMD: SYS:Rexxc/rx WORK:test-dbclient-run.rexx Zimacs501482 127.0.0.1 echo 12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890
EXPECT_CONTAINS: 1234567890
EXPECT_RC: 0

# === EXIT CODE PROPAGATION ===

TEST: remote exit 0 propagates
CMD: SYS:Rexxc/rx WORK:test-dbclient-run.rexx Zimacs501482 127.0.0.1 exit 0
EXPECT_RC: 0

TEST: remote exit 1 propagates
CMD: SYS:Rexxc/rx WORK:test-dbclient-run.rexx Zimacs501482 127.0.0.1 exit 1
EXPECT_RC: 1

TEST: remote exit 5 propagates
CMD: SYS:Rexxc/rx WORK:test-dbclient-run.rexx Zimacs501482 127.0.0.1 exit 5
EXPECT_RC: 5

TEST: remote true returns 0
CMD: SYS:Rexxc/rx WORK:test-dbclient-run.rexx Zimacs501482 127.0.0.1 true
EXPECT_RC: 0

TEST: remote false returns nonzero
CMD: SYS:Rexxc/rx WORK:test-dbclient-run.rexx Zimacs501482 127.0.0.1 false
EXPECT_RC: 1

# === ERROR HANDLING (network errors) ===

TEST: connection to bad port fails
CMD: WORK:dbclient -y -p 1 duncan@127.0.0.1 echo hello
EXPECT_RC: 1

TEST: command producing no output still exits cleanly
CMD: SYS:Rexxc/rx WORK:test-dbclient-run.rexx Zimacs501482 127.0.0.1 true
EXPECT_RC: 0

TEST: exit command terminates session cleanly
CMD: SYS:Rexxc/rx WORK:test-dbclient-run.rexx Zimacs501482 127.0.0.1 exit
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
Generated by `make test-fsemu TARGET=ports/dropbear`
Report template: `toolchain/templates/test-report.md.template`
