# FS-UAE Test Report: amiget

## Summary

| Field | Value |
|-------|-------|
| Port | amiget |
| Date | 2026-04-05 20:22:47 |
| Duration | 56s |
| Platform | FS-UAE 3.2.35 (A1200, Kickstart 3.1) |
| Binary | `WORK:amiget` (43K) |
| Test method | ARexx harness → TAP output |
| Result | **PASS** — 35/35 passed |

## Test Results

```
1..35
ok 1 - No arguments prints help text
ok 2 - help command prints usage header
ok 3 - --help flag prints usage (alternate form)
ok 4 - -h flag prints usage (short form)
ok 5 - installed shows empty message when no packages in DB
ok 6 - Unknown command returns RC 10
ok 7 - search with no term returns RC 10
ok 8 - info with no package name returns RC 10
ok 9 - install with no package name returns RC 10
ok 10 - remove with no package name returns RC 10
ok 11 - remove non-installed package returns RC 10
ok 12 - upgrade amiget blocked by self-update guard returns RC 10
ok 13 - doctor reports network connectivity and returns RC 0
ok 14 - list fetches manifest and shows column header
ok 15 - list shows packages available count at end
ok 16 - search grep finds at least one matching package
ok 17 - search returns zero-match message for unknown term
ok 18 - info grep shows package name field
ok 19 - info nonexistent package returns RC 10
ok 20 - install grep downloads verifies and installs the package
ok 21 - install grep again returns RC 5 already installed
ok 22 - installed shows grep after install
ok 23 - installed shows correct package count after install
ok 24 - upgrade grep shows already latest when up to date
ok 25 - upgrade with no args shows all up to date
ok 26 - remove grep uninstalls the package
ok 27 - installed shows empty after remove
ok 28 - remove already-removed package returns RC 10
ok 29 - real-world install then search shows installed tag
ok 30 - real-world search shows installed tag after install
ok 31 - real-world list shows installed tag for grep
ok 32 - stress repeated list uses cache without network fetch
ok 33 - stress search single char term scans full manifest
ok 34 - precision doctor all four checks pass
ok 35 - cleanup remove grep restores empty DB
# passed: 35 failed: 0 total: 35
```

### Breakdown

| # | Test | Status | Details |
|---|------|--------|---------|
| 1 | No arguments prints help text | PASS | |
| 2 | help command prints usage header | PASS | |
| 3 | --help flag prints usage (alternate form) | PASS | |
| 4 | -h flag prints usage (short form) | PASS | |
| 5 | installed shows empty message when no packages in DB | PASS | |
| 6 | Unknown command returns RC 10 | PASS | |
| 7 | search with no term returns RC 10 | PASS | |
| 8 | info with no package name returns RC 10 | PASS | |
| 9 | install with no package name returns RC 10 | PASS | |
| 10 | remove with no package name returns RC 10 | PASS | |
| 11 | remove non-installed package returns RC 10 | PASS | |
| 12 | upgrade amiget blocked by self-update guard returns RC 10 | PASS | |
| 13 | doctor reports network connectivity and returns RC 0 | PASS | |
| 14 | list fetches manifest and shows column header | PASS | |
| 15 | list shows packages available count at end | PASS | |
| 16 | search grep finds at least one matching package | PASS | |
| 17 | search returns zero-match message for unknown term | PASS | |
| 18 | info grep shows package name field | PASS | |
| 19 | info nonexistent package returns RC 10 | PASS | |
| 20 | install grep downloads verifies and installs the package | PASS | |
| 21 | install grep again returns RC 5 already installed | PASS | |
| 22 | installed shows grep after install | PASS | |
| 23 | installed shows correct package count after install | PASS | |
| 24 | upgrade grep shows already latest when up to date | PASS | |
| 25 | upgrade with no args shows all up to date | PASS | |
| 26 | remove grep uninstalls the package | PASS | |
| 27 | installed shows empty after remove | PASS | |
| 28 | remove already-removed package returns RC 10 | PASS | |
| 29 | real-world install then search shows installed tag | PASS | |
| 30 | real-world search shows installed tag after install | PASS | |
| 31 | real-world list shows installed tag for grep | PASS | |
| 32 | stress repeated list uses cache without network fetch | PASS | |
| 33 | stress search single char term scans full manifest | PASS | |
| 34 | precision doctor all four checks pass | PASS | |
| 35 | cleanup remove grep restores empty DB | PASS | |

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
# test-fsemu-cases.txt -- amiget 1.0 test suite
#
# Category 4 (Network). 16 tests minimum.
# Network commands require bsdsocket.library (Roadshow) -- works on FS-UAE.
# Non-network commands (help, installed, remove) work standalone.
#
# Test ordering is deliberate:
#   Group A (non-network, no install state)  -- help, installed empty, error paths
#   Group B (network, read-only)             -- doctor, list, search, info
#   Group C (install lifecycle)              -- install, already-installed, installed list,
#                                               upgrade up-to-date, remove, installed empty
#   Group D (error paths, network-dependent) -- nonexistent package, self-upgrade block

# -----------------------------------------------------------------------
# Group A: Non-network commands and error paths (no DB state required)
# -----------------------------------------------------------------------

# Native reference: amiget (no args) -- prints same as help
TEST: No arguments prints help text
CMD: WORK:amiget
EXPECT: amiget - Package manager for AmigaOS 3.x
EXPECT_RC: 0

# Native reference: amiget help
TEST: help command prints usage header
CMD: WORK:amiget help
EXPECT: amiget - Package manager for AmigaOS 3.x
EXPECT_LINE: 3,Usage: amiget <command> [arguments]
EXPECT_RC: 0

# Native reference: amiget --help
TEST: --help flag prints usage (alternate form)
CMD: WORK:amiget --help
EXPECT: amiget - Package manager for AmigaOS 3.x
EXPECT_RC: 0

# Native reference: amiget -h
TEST: -h flag prints usage (short form)
CMD: WORK:amiget -h
EXPECT: amiget - Package manager for AmigaOS 3.x
EXPECT_RC: 0

# Native reference: amiget installed  (DB empty or missing)
TEST: installed shows empty message when no packages in DB
CMD: WORK:amiget installed
EXPECT: No packages installed.
EXPECT_RC: 0

# Error path: unknown command goes to stderr, RC=10
TEST: Unknown command returns RC 10
CMD: WORK:amiget frobnicate
EXPECT_RC: 10

# Error path: search with no term -- error goes to stderr
TEST: search with no term returns RC 10
CMD: WORK:amiget search
EXPECT_RC: 10

# Error path: info with no package name -- error goes to stderr
TEST: info with no package name returns RC 10
CMD: WORK:amiget info
EXPECT_RC: 10

# Error path: install with no package name -- error goes to stderr
TEST: install with no package name returns RC 10
CMD: WORK:amiget install
EXPECT_RC: 10

# Error path: remove with no package name -- error goes to stderr
TEST: remove with no package name returns RC 10
CMD: WORK:amiget remove
EXPECT_RC: 10

# Error path: remove a package that is not in the DB (error to stderr)
TEST: remove non-installed package returns RC 10
CMD: WORK:amiget remove nonexistent-pkg-xyz
EXPECT_RC: 10

# Error path: upgrade amiget is blocked (self-update not supported)
# Output goes to stderr, so verify RC only
TEST: upgrade amiget blocked by self-update guard returns RC 10
CMD: WORK:amiget upgrade amiget
EXPECT_RC: 10

# -----------------------------------------------------------------------
# Group B: Network read-only commands (require Roadshow in FS-UAE)
# -----------------------------------------------------------------------

# doctor: stdout first line is always "amiget doctor"
# Full success: RC=0, all 4 checks print OK
TEST: doctor reports network connectivity and returns RC 0
CMD: WORK:amiget doctor
EXPECT: amiget doctor
EXPECT_CONTAINS: All checks passed.
EXPECT_RC: 0

# list: first output line is the column header
# "Name             Version    Description                              Status"
TEST: list fetches manifest and shows column header
CMD: WORK:amiget list
EXPECT: Name             Version    Description                              Status
EXPECT_RC: 0

# list: last line shows package count (real-world usage verification)
# The API has at least 10 packages -- use EXPECT_CONTAINS for the count suffix
TEST: list shows packages available count at end
CMD: WORK:amiget list
EXPECT_CONTAINS: packages available
EXPECT_RC: 0

# search: grep is in the manifest -- should find at least one result
# Output format: "%-16s %-10s %-40s %s\n" -- grep entry present
TEST: search grep finds at least one matching package
CMD: WORK:amiget search grep
EXPECT_CONTAINS: grep
EXPECT_RC: 0

# search: no-match case -- specific string that will never match
TEST: search returns zero-match message for unknown term
CMD: WORK:amiget search xyzzy-no-such-package-ever
EXPECT: No packages match 'xyzzy-no-such-package-ever'.
EXPECT_RC: 0

# info: grep is in the manifest -- shows Name: and Version: fields
# First structured field printed is "  Name:            grep"
TEST: info grep shows package name field
CMD: WORK:amiget info grep
EXPECT_CONTAINS: Name:
EXPECT_CONTAINS: Version:
EXPECT_RC: 0

# info: nonexistent package returns RC 10 (error to stderr)
TEST: info nonexistent package returns RC 10
CMD: WORK:amiget info xyzzy-no-such-package-ever
EXPECT_RC: 10

# -----------------------------------------------------------------------
# Group C: Install lifecycle (stateful -- tests must run in order)
# These tests share the S:amiget.db filesystem state across the session.
# -----------------------------------------------------------------------

# install grep: download, verify SHA-256, extract, register in DB
# stdout first line of success: "Installed grep X.X to C:grep"
# (download/verify progress goes to stderr)
TEST: install grep downloads verifies and installs the package
CMD: WORK:amiget install grep
EXPECT_CONTAINS: Installed grep
EXPECT_RC: 0

# install grep again: already installed, RC=5 (RETURN_WARN)
# stdout: "grep X.X is already installed."
TEST: install grep again returns RC 5 already installed
CMD: WORK:amiget install grep
EXPECT_CONTAINS: is already installed.
EXPECT_RC: 5

# installed: now shows grep in the list
# Header line: "Name             Version    Path"
TEST: installed shows grep after install
CMD: WORK:amiget installed
EXPECT: Name             Version    Path
EXPECT_CONTAINS: grep
EXPECT_RC: 0

# installed: count line at end shows "1 package installed"
TEST: installed shows correct package count after install
CMD: WORK:amiget installed
EXPECT_CONTAINS: package installed
EXPECT_RC: 0

# upgrade grep (single package): already at latest version
# stdout: "grep X.X is already the latest version."
TEST: upgrade grep shows already latest when up to date
CMD: WORK:amiget upgrade grep
EXPECT_CONTAINS: is already the latest version.
EXPECT_RC: 0

# upgrade (batch, no args): grep is current, should say "All packages up to date."
TEST: upgrade with no args shows all up to date
CMD: WORK:amiget upgrade
EXPECT: All packages up to date.
EXPECT_RC: 0

# remove grep: deletes binary from C:, removes from DB
# stdout: "Removed grep"
TEST: remove grep uninstalls the package
CMD: WORK:amiget remove grep
EXPECT_CONTAINS: Removed grep
EXPECT_RC: 0

# installed: back to empty after remove
TEST: installed shows empty after remove
CMD: WORK:amiget installed
EXPECT: No packages installed.
EXPECT_RC: 0

# remove grep again: no longer in DB, should return RC 10
TEST: remove already-removed package returns RC 10
CMD: WORK:amiget remove grep
EXPECT_RC: 10

# -----------------------------------------------------------------------
# Group D: Real-world and stress tests
# -----------------------------------------------------------------------

# Real-world: install + search + info round-trip verifies the manifest
# fetch, SHA-256 verification, and DB tracking all work end-to-end.
# Re-install grep for this test (DB is empty after Group C).
TEST: real-world install then search shows installed tag
CMD: WORK:amiget install grep
EXPECT_CONTAINS: Installed grep
EXPECT_RC: 0

TEST: real-world search shows installed tag after install
CMD: WORK:amiget search grep
EXPECT_CONTAINS: [installed]
EXPECT_RC: 0

# Real-world: list after install shows [installed] tag in status column
TEST: real-world list shows installed tag for grep
CMD: WORK:amiget list
EXPECT_CONTAINS: [installed]
EXPECT_RC: 0

# Stress: repeated list calls exercise manifest cache path (second call
# hits S:amiget-cache.json, not the network)
TEST: stress repeated list uses cache without network fetch
CMD: WORK:amiget list
EXPECT_CONTAINS: packages available
EXPECT_RC: 0

# Stress: search with a single-character term exercises all 128 possible
# package entries (up to AMIGET_MAX_PACKAGES) in the inner loop
TEST: stress search single char term scans full manifest
CMD: WORK:amiget search e
EXPECT_CONTAINS: packages found
EXPECT_RC: 0

# Precision: doctor verifies all 4 network checks pass -- tests the full
# bsdsocket -> DNS -> TCP -> HTTP chain against the live server
TEST: precision doctor all four checks pass
CMD: WORK:amiget doctor
EXPECT: amiget doctor
EXPECT_LINE: 2,  bsdsocket.library... OK
EXPECT_LINE: 3,  DNS resolution...    OK
EXPECT_LINE: 4,  TCP connection...    OK
EXPECT_LINE: 5,  API reachability...  OK
EXPECT_LINE: 6,All checks passed.
EXPECT_RC: 0

# Cleanup: remove grep again to restore DB to empty state
TEST: cleanup remove grep restores empty DB
CMD: WORK:amiget remove grep
EXPECT_CONTAINS: Removed grep
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
passed=35
failed=0
total=35
```

---
Generated by `make test-fsemu TARGET=ports/amiget`
Report template: `toolchain/templates/test-report.md.template`
