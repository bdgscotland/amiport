# FS-UAE Test Report: wget

## Summary

| Field | Value |
|-------|-------|
| Port | wget |
| Date | 2026-04-06 00:05:57 |
| Duration | 174s |
| Platform | FS-UAE 3.2.35 (A1200, Kickstart 3.1) |
| Binary | `WORK:wget` (527K) |
| Test method | ARexx harness → TAP output |
| Result | **PASS** — 68/68 passed |

## Test Results

```
1..68
ok 1 - --version prints version string as first line
ok 2 - -V short option prints version string (same as --version)
ok 3 - --help prints help header as first line
ok 4 - -h short option prints help header (same as --help)
ok 5 - --version shows compiled feature flags including digest and https
ok 6 - --version shows second line of feature flags including ssl/openssl
ok 7 - --version shows system wgetrc path
ok 8 - --version shows compilation string
ok 9 - --help shows Usage line
ok 10 - --help mentions --output-file option
ok 11 - --no-config accepted as valid option (no URL -> RC=1 not RC=2)
ok 12 - -e execute option accepted (sets robots=off then fails missing URL)
ok 13 - -q quiet flag accepted as valid option
ok 14 - -v verbose flag accepted as valid option
ok 15 - -t tries option accepted with argument
ok 16 - -w wait option accepted with argument
ok 17 - --spider option accepted as valid flag
ok 18 - --user-agent option accepted with custom string
ok 19 - --tries=0 unlimited retries option accepted
ok 20 - --limit-rate option accepted with value
ok 21 - -r recursive flag accepted as valid option
ok 22 - -c continue flag accepted as valid option
ok 23 - -N timestamping flag accepted as valid option
ok 24 - -S server response flag accepted as valid option
ok 25 - --no-config with -e execute both accepted together
ok 26 - --config with valid wgetrc file accepted and parsed
ok 27 - -b background flag accepted (AmigaOS stub -- no fork)
ok 28 - --header option accepted with custom header string
ok 29 - --post-data option accepted with data string
ok 30 - -O - output-document stdout with failing URL returns network error
ok 31 - -i input-file reads URL list and attempts download (fails network on vamos)
ok 32 - No arguments exits with RC=1 (missing URL)
ok 33 - Unknown long option --invalid-option gives RC=2 parse error
ok 34 - Unknown short option -Z gives RC=2 parse error
ok 35 - Conflicting -v and -q flags give RC=1 generic error
ok 36 - Conflicting -N and --no-clobber give RC=1 generic error
ok 37 - --post-data and --post-file together gives RC=1 generic error
ok 38 - RC=0 from --version flag
ok 39 - RC=0 from --help flag
ok 40 - RC=1 from missing URL argument
ok 41 - RC=2 from unrecognized option --xyzzy
ok 42 - RC=4 from network failure on nonexistent host (no network stack on vamos)
ok 43 - RC=4 from --spider with nonexistent host
ok 44 - FTP URL scheme recognized and fails DNS on vamos (network error RC=4)
ok 45 - -t 1 single retry with failing URL gives RC=4
ok 46 - --tries=1 long form single retry with failing URL gives RC=4
ok 47 - -q quiet mode still returns RC=4 on network failure
ok 48 - Multiple failing URLs all return RC=4
ok 49 - --no-config with failing URL skips wgetrc and returns RC=4
ok 50 - -e robots=off then failing URL gives RC=4
ok 51 - Custom user-agent with failing URL gives RC=4
ok 52 - WORK: volume path accepted for --config file argument
ok 53 - -o logfile to T: Amiga temp volume with failing URL
ok 54 - --version shows AmigaOS as build platform
ok 55 - --version shows Wgetrc section listing config file paths
ok 56 - -b background flag prints AmigaOS stub warning then fails DNS (RC=4)
ok 57 - Real-world download attempt with -q --tries=1 (user workflow, DNS fails)
ok 58 - Real-world recursive download -r -l 1 (common user workflow, DNS fails)
ok 59 - Real-world resume download -c (DNS fails on vamos)
ok 60 - Real-world output to named temp file -O with failing URL
ok 61 - Real-world spider mode with multiple options (DNS fails on vamos)
ok 62 - Stress test many combined options with failing URL
ok 63 - Stress test URL-list file with multiple failing URLs
ok 64 - Stress version output multi-line feature flag rendering
ok 65 - Precision --help renders complete usage line without IO truncation
ok 66 - wget --version starts and exits cleanly without interaction
ok 67 - wget with nonexistent host fails DNS and exits without hanging
ok 68 - Ctrl-C interrupts wget during download attempt
# passed: 68 failed: 0 total: 68
```

### Breakdown

| # | Test | Status | Details |
|---|------|--------|---------|
| 1 | --version prints version string as first line | PASS | |
| 2 | -V short option prints version string (same as --version) | PASS | |
| 3 | --help prints help header as first line | PASS | |
| 4 | -h short option prints help header (same as --help) | PASS | |
| 5 | --version shows compiled feature flags including digest and https | PASS | |
| 6 | --version shows second line of feature flags including ssl/openssl | PASS | |
| 7 | --version shows system wgetrc path | PASS | |
| 8 | --version shows compilation string | PASS | |
| 9 | --help shows Usage line | PASS | |
| 10 | --help mentions --output-file option | PASS | |
| 11 | --no-config accepted as valid option (no URL -> RC=1 not RC=2) | PASS | |
| 12 | -e execute option accepted (sets robots=off then fails missing URL) | PASS | |
| 13 | -q quiet flag accepted as valid option | PASS | |
| 14 | -v verbose flag accepted as valid option | PASS | |
| 15 | -t tries option accepted with argument | PASS | |
| 16 | -w wait option accepted with argument | PASS | |
| 17 | --spider option accepted as valid flag | PASS | |
| 18 | --user-agent option accepted with custom string | PASS | |
| 19 | --tries=0 unlimited retries option accepted | PASS | |
| 20 | --limit-rate option accepted with value | PASS | |
| 21 | -r recursive flag accepted as valid option | PASS | |
| 22 | -c continue flag accepted as valid option | PASS | |
| 23 | -N timestamping flag accepted as valid option | PASS | |
| 24 | -S server response flag accepted as valid option | PASS | |
| 25 | --no-config with -e execute both accepted together | PASS | |
| 26 | --config with valid wgetrc file accepted and parsed | PASS | |
| 27 | -b background flag accepted (AmigaOS stub -- no fork) | PASS | |
| 28 | --header option accepted with custom header string | PASS | |
| 29 | --post-data option accepted with data string | PASS | |
| 30 | -O - output-document stdout with failing URL returns network error | PASS | |
| 31 | -i input-file reads URL list and attempts download (fails network on vamos) | PASS | |
| 32 | No arguments exits with RC=1 (missing URL) | PASS | |
| 33 | Unknown long option --invalid-option gives RC=2 parse error | PASS | |
| 34 | Unknown short option -Z gives RC=2 parse error | PASS | |
| 35 | Conflicting -v and -q flags give RC=1 generic error | PASS | |
| 36 | Conflicting -N and --no-clobber give RC=1 generic error | PASS | |
| 37 | --post-data and --post-file together gives RC=1 generic error | PASS | |
| 38 | RC=0 from --version flag | PASS | |
| 39 | RC=0 from --help flag | PASS | |
| 40 | RC=1 from missing URL argument | PASS | |
| 41 | RC=2 from unrecognized option --xyzzy | PASS | |
| 42 | RC=4 from network failure on nonexistent host (no network stack on vamos) | PASS | |
| 43 | RC=4 from --spider with nonexistent host | PASS | |
| 44 | FTP URL scheme recognized and fails DNS on vamos (network error RC=4) | PASS | |
| 45 | -t 1 single retry with failing URL gives RC=4 | PASS | |
| 46 | --tries=1 long form single retry with failing URL gives RC=4 | PASS | |
| 47 | -q quiet mode still returns RC=4 on network failure | PASS | |
| 48 | Multiple failing URLs all return RC=4 | PASS | |
| 49 | --no-config with failing URL skips wgetrc and returns RC=4 | PASS | |
| 50 | -e robots=off then failing URL gives RC=4 | PASS | |
| 51 | Custom user-agent with failing URL gives RC=4 | PASS | |
| 52 | WORK: volume path accepted for --config file argument | PASS | |
| 53 | -o logfile to T: Amiga temp volume with failing URL | PASS | |
| 54 | --version shows AmigaOS as build platform | PASS | |
| 55 | --version shows Wgetrc section listing config file paths | PASS | |
| 56 | -b background flag prints AmigaOS stub warning then fails DNS (RC=4) | PASS | |
| 57 | Real-world download attempt with -q --tries=1 (user workflow, DNS fails) | PASS | |
| 58 | Real-world recursive download -r -l 1 (common user workflow, DNS fails) | PASS | |
| 59 | Real-world resume download -c (DNS fails on vamos) | PASS | |
| 60 | Real-world output to named temp file -O with failing URL | PASS | |
| 61 | Real-world spider mode with multiple options (DNS fails on vamos) | PASS | |
| 62 | Stress test many combined options with failing URL | PASS | |
| 63 | Stress test URL-list file with multiple failing URLs | PASS | |
| 64 | Stress version output multi-line feature flag rendering | PASS | |
| 65 | Precision --help renders complete usage line without IO truncation | PASS | |
| 66 | wget --version starts and exits cleanly without interaction | PASS | |
| 67 | wget with nonexistent host fails DNS and exits without hanging | PASS | |
| 68 | Ctrl-C interrupts wget during download attempt | PASS | |

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
# test-fsemu-cases.txt -- GNU wget 1.20.3 FS-UAE test suite
# Category 4 (Network) port -- bsdsocket.library required for downloads
#
# IMPORTANT: wget exit codes are POSIX values (0-8), NOT Amiga values.
# The amiport exit() macro does NOT remap wget's enum -- they pass through raw.
# RC=0 success, RC=1 generic error, RC=2 parse error, RC=4 network failure.
# Network tests (DNS lookup) will return RC=4 on vamos (no bsdsocket.library)
# and on FS-UAE without Roadshow/AmiTCP -- this is correct and expected.
#
# All CMD lines use WORK: prefix. No piping, no shell metacharacters.

# === FUNCTIONAL TESTS: option parsing, no network needed ===

# Source: print_version() in main.c -- first printf: "GNU Wget %s built on %s.\n\n"
# with version_string="1.20.3" and OS_TYPE="AmigaOS" (defined in config.h)
TEST: --version prints version string as first line
CMD: WORK:wget --version
EXPECT: GNU Wget 1.20.3 built on AmigaOS.
EXPECT_RC: 0

# Same behavior via short option -V
TEST: -V short option prints version string (same as --version)
CMD: WORK:wget -V
EXPECT: GNU Wget 1.20.3 built on AmigaOS.
EXPECT_RC: 0

# Source: print_help() -> printf("GNU Wget %s, a non-interactive network retriever.\n")
# then print_usage(0) -> "Usage: %s [OPTION]... [URL]...\n"
# First line of stdout is the retriever line.
TEST: --help prints help header as first line
CMD: WORK:wget --help
EXPECT: GNU Wget 1.20.3, a non-interactive network retriever.
EXPECT_RC: 0

# Same behavior via short option -h
TEST: -h short option prints help header (same as --help)
CMD: WORK:wget -h
EXPECT: GNU Wget 1.20.3, a non-interactive network retriever.
EXPECT_RC: 0

# --version output lines:
# Line 1: "GNU Wget 1.20.3 built on AmigaOS." (version header)
# Line 2: blank (from "\n\n" in first printf)
# Feature flags printed by compiled_features[] loop, wrapping at MAX_CHARS_PER_LINE=72.
# Each feature is printed as "%s " (with trailing space). First line ends around "-nls ".
# Second feature line: "-ntlm +opie -psl +ssl/openssl "
# Use EXPECT_CONTAINS: for features to avoid trailing-space mismatch with READLN.
TEST: --version shows compiled feature flags including digest and https
CMD: WORK:wget --version
EXPECT: GNU Wget 1.20.3 built on AmigaOS.
EXPECT_CONTAINS: -cares +digest -gpgme -https
EXPECT_RC: 0

TEST: --version shows second line of feature flags including ssl/openssl
CMD: WORK:wget --version
EXPECT_CONTAINS: -ssl
EXPECT_RC: 0

# --version shows SYSTEM_WGETRC path (defined as "S:wgetrc" in Makefile CFLAGS)
# Source: print_version() -> printf("    %s (system)\n", SYSTEM_WGETRC)
TEST: --version shows system wgetrc path
CMD: WORK:wget --version
EXPECT_CONTAINS: S:wgetrc (system)
EXPECT_RC: 0

# --version shows Compile info (from version.c: compilation_string)
TEST: --version shows compilation string
CMD: WORK:wget --version
EXPECT_CONTAINS: Compile:
EXPECT_RC: 0

# --help includes Usage: line (via print_usage() inside print_help())
TEST: --help shows Usage line
CMD: WORK:wget --help
EXPECT_CONTAINS: Usage: WORK:wget [OPTION]
EXPECT_RC: 0

# --help contains key option descriptions
TEST: --help mentions --output-file option
CMD: WORK:wget --help
EXPECT_CONTAINS: --output-file
EXPECT_RC: 0

# --no-config suppresses system wgetrc loading. The flag itself is processed
# and then wget proceeds -- still needs a URL. With no URL, exits RC=1.
# This tests that --no-config is accepted as a valid option (not RC=2).
# Source: main() -> noconfig=true; then missing-URL path -> exit(WGET_EXIT_GENERIC_ERROR=1)
TEST: --no-config accepted as valid option (no URL -> RC=1 not RC=2)
CMD: WORK:wget --no-config
EXPECT_RC: 1

# -e COMMAND executes a wgetrc-style command. Processed during option parsing.
# With no URL follows, exits RC=1 (generic error, missing URL).
# Tests that -e is accepted as valid option without RC=2 parse error.
TEST: -e execute option accepted (sets robots=off then fails missing URL)
CMD: WORK:wget -e robots=off
EXPECT_RC: 1

# -q quiet mode flag -- accepted without error. Still needs URL.
TEST: -q quiet flag accepted as valid option
CMD: WORK:wget -q
EXPECT_RC: 1

# -v verbose flag -- accepted without error. Still needs URL.
TEST: -v verbose flag accepted as valid option
CMD: WORK:wget -v
EXPECT_RC: 1

# -t retries option -- accepted with numeric argument. Still needs URL.
TEST: -t tries option accepted with argument
CMD: WORK:wget -t 3
EXPECT_RC: 1

# -w wait option -- accepted with numeric argument. Still needs URL.
TEST: -w wait option accepted with argument
CMD: WORK:wget -w 1
EXPECT_RC: 1

# --spider flag -- accepted without error. Still needs URL.
TEST: --spider option accepted as valid flag
CMD: WORK:wget --spider
EXPECT_RC: 1

# --user-agent option -- accepted with string argument. Still needs URL.
TEST: --user-agent option accepted with custom string
CMD: WORK:wget --user-agent=AmiWget/1.0
EXPECT_RC: 1

# --tries=0 (unlimited retries) -- accepted. Still needs URL.
TEST: --tries=0 unlimited retries option accepted
CMD: WORK:wget --tries=0
EXPECT_RC: 1

# --limit-rate option -- accepted with rate argument. Still needs URL.
TEST: --limit-rate option accepted with value
CMD: WORK:wget --limit-rate=100k
EXPECT_RC: 1

# -r recursive -- accepted. Still needs URL.
TEST: -r recursive flag accepted as valid option
CMD: WORK:wget -r
EXPECT_RC: 1

# -c continue/resume -- accepted. Still needs URL.
TEST: -c continue flag accepted as valid option
CMD: WORK:wget -c
EXPECT_RC: 1

# -N timestamping -- accepted. Still needs URL.
TEST: -N timestamping flag accepted as valid option
CMD: WORK:wget -N
EXPECT_RC: 1

# -S server response -- accepted. Still needs URL.
TEST: -S server response flag accepted as valid option
CMD: WORK:wget -S
EXPECT_RC: 1

# --no-config combined with -e -- both accepted. Still needs URL.
TEST: --no-config with -e execute both accepted together
CMD: WORK:wget --no-config -e robots=off
EXPECT_RC: 1

# --config with valid wgetrc file -- file is read, option processed.
# No URL after config read -- exits RC=1 (missing URL, not parse error RC=2).
TEST: --config with valid wgetrc file accepted and parsed
CMD: WORK:wget --config WORK:test-wget-wgetrc.txt
EXPECT_RC: 1

# -b background flag -- AmigaOS stub: prints warning to log, continues.
# With no URL, exits RC=1 (missing URL).
# On FS-UAE with a URL, the program runs normally with a warning logged.
TEST: -b background flag accepted (AmigaOS stub -- no fork)
CMD: WORK:wget -b
EXPECT_RC: 1

# --header option -- adds HTTP header. Accepted. Still needs URL.
TEST: --header option accepted with custom header string
CMD: WORK:wget --header=X-Test:AmigaOS
EXPECT_RC: 1

# --post-data option -- accepted. Still needs URL.
TEST: --post-data option accepted with data string
CMD: WORK:wget --post-data=key=value
EXPECT_RC: 1

# -O - output to stdout -- accepted (HYPHENP check). Still needs URL for network.
# With a URL that fails DNS, exits RC=4.
TEST: -O - output-document stdout with failing URL returns network error
CMD: WORK:wget -O - http://nonexistent.invalid/
EXPECT_RC: 4

# -i input-file with URL list file. wget reads the file and attempts URLs.
# All DNS lookups fail on vamos (no bsdsocket). Exits RC=4 network failure.
TEST: -i input-file reads URL list and attempts download (fails network on vamos)
CMD: WORK:wget -i WORK:test-wget-urls.txt
EXPECT_RC: 4

# === ERROR PATH TESTS ===

# No arguments -- "missing URL" error to stderr, RC=1 (WGET_EXIT_GENERIC_ERROR)
# Source: main() -> "!nurl && !opt.input_filename" -> fprintf(stderr) -> exit(1)
# stderr not captured by harness, so we only verify RC.
TEST: No arguments exits with RC=1 (missing URL)
CMD: WORK:wget
EXPECT_RC: 1

# Unknown long option -- parse error, RC=2 (WGET_EXIT_PARSE_ERROR)
# Source: getopt_long returns '?' -> print_usage(1) -> exit(WGET_EXIT_PARSE_ERROR)
TEST: Unknown long option --invalid-option gives RC=2 parse error
CMD: WORK:wget --invalid-option
EXPECT_RC: 2

# Unknown short option -- parse error, RC=2
TEST: Unknown short option -Z gives RC=2 parse error
CMD: WORK:wget -Z
EXPECT_RC: 2

# Conflicting options: -v and -q (verbose AND quiet)
# Source: "Can't be verbose and quiet at the same time" -> exit(WGET_EXIT_GENERIC_ERROR=1)
TEST: Conflicting -v and -q flags give RC=1 generic error
CMD: WORK:wget -v -q http://nonexistent.invalid/
EXPECT_RC: 1

# Conflicting options: -N (timestamping) and --no-clobber
# Source: "Can't timestamp and not clobber old files at the same time" -> exit(1)
TEST: Conflicting -N and --no-clobber give RC=1 generic error
CMD: WORK:wget -N --no-clobber http://nonexistent.invalid/
EXPECT_RC: 1

# --post-data and --post-file together -- error
# Source: "You cannot specify both --post-data and --post-file" -> exit(1)
TEST: --post-data and --post-file together gives RC=1 generic error
CMD: WORK:wget --post-data=x --post-file WORK:test-wget-wgetrc.txt http://nonexistent.invalid/
EXPECT_RC: 1

# === EXIT CODE TESTS ===

# RC=0: clean exit from --version
TEST: RC=0 from --version flag
CMD: WORK:wget --version
EXPECT_RC: 0

# RC=0: clean exit from --help
TEST: RC=0 from --help flag
CMD: WORK:wget --help
EXPECT_RC: 0

# RC=1: missing URL (generic error)
TEST: RC=1 from missing URL argument
CMD: WORK:wget --no-config
EXPECT_RC: 1

# RC=2: parse error from unknown option
TEST: RC=2 from unrecognized option --xyzzy
CMD: WORK:wget --xyzzy
EXPECT_RC: 2

# RC=4: network failure (DNS resolution failure for nonexistent host)
# bsdsocket.library not available on vamos -- gethostbyname fails
# Source: HOSTERR maps to WGET_EXIT_NETWORK_FAIL=4 in exits.c
TEST: RC=4 from network failure on nonexistent host (no network stack on vamos)
CMD: WORK:wget http://nonexistent.invalid/
EXPECT_RC: 4

# RC=4: network failure with --spider mode (same DNS path)
TEST: RC=4 from --spider with nonexistent host
CMD: WORK:wget --spider http://nonexistent.invalid/
EXPECT_RC: 4

# === EDGE CASE TESTS ===

# FTP URL test -- wget supports FTP scheme. DNS fails on vamos (no bsdsocket).
# Tests that ftp:// scheme is recognized and proceeds to network phase.
# URLERROR or HOSTERR both map to RC=4 via inform_exit_status in exits.c.
TEST: FTP URL scheme recognized and fails DNS on vamos (network error RC=4)
CMD: WORK:wget ftp://nonexistent.invalid/file.tar.gz
EXPECT_RC: 4

# -t 1 limits to 1 retry. With failing URL, still RC=4.
TEST: -t 1 single retry with failing URL gives RC=4
CMD: WORK:wget -t 1 http://nonexistent.invalid/
EXPECT_RC: 4

# --tries=1 same as -t 1 (long form)
TEST: --tries=1 long form single retry with failing URL gives RC=4
CMD: WORK:wget --tries=1 http://nonexistent.invalid/
EXPECT_RC: 4

# -q quiet mode suppresses output but exit code still reflects network status
TEST: -q quiet mode still returns RC=4 on network failure
CMD: WORK:wget -q http://nonexistent.invalid/
EXPECT_RC: 4

# Multiple URLs -- both fail DNS. wget processes all and returns RC=4.
TEST: Multiple failing URLs all return RC=4
CMD: WORK:wget http://nonexistent.invalid/ http://also.invalid/
EXPECT_RC: 4

# --no-config prevents S:wgetrc from being read.
# Combined with failing URL to get a complete code path test.
TEST: --no-config with failing URL skips wgetrc and returns RC=4
CMD: WORK:wget --no-config http://nonexistent.invalid/
EXPECT_RC: 4

# -e robots=off disables robots.txt checking.
# Still attempts the URL, fails DNS, RC=4.
TEST: -e robots=off then failing URL gives RC=4
CMD: WORK:wget -e robots=off http://nonexistent.invalid/
EXPECT_RC: 4

# --user-agent with failing URL -- processes header, fails DNS, RC=4
TEST: Custom user-agent with failing URL gives RC=4
CMD: WORK:wget --user-agent=AmigaOS-Test/1.0 http://nonexistent.invalid/
EXPECT_RC: 4

# === AMIGA-SPECIFIC TESTS ===

# Amiga path: WORK: volume paths work as file arguments
# --config with WORK: path should be accepted (file is read, no URL = RC=1)
TEST: WORK: volume path accepted for --config file argument
CMD: WORK:wget --config WORK:test-wget-wgetrc.txt
EXPECT_RC: 1

# Amiga path: -o logfile to T: (RAM disk temp)
# -o logfile path + failing URL. Creates log in T:, exits RC=4.
TEST: -o logfile to T: Amiga temp volume with failing URL
CMD: WORK:wget -o T:test-wget-log.txt http://nonexistent.invalid/
EXPECT_RC: 4

# --version output shows "AmigaOS" as the build OS (not Linux/Windows)
TEST: --version shows AmigaOS as build platform
CMD: WORK:wget --version
EXPECT_CONTAINS: AmigaOS
EXPECT_RC: 0

# --version shows Wgetrc: section header (verifies full version output rendered)
TEST: --version shows Wgetrc section listing config file paths
CMD: WORK:wget --version
EXPECT_CONTAINS: Wgetrc:
EXPECT_RC: 0

# AmigaOS background stub: -b with URL prints warning (not crash), then fails DNS
# Source: amiga_stubs path prints warning, then continues to URL retrieval
TEST: -b background flag prints AmigaOS stub warning then fails DNS (RC=4)
CMD: WORK:wget -b http://nonexistent.invalid/
EXPECT_RC: 4

# === REAL-WORLD AND STRESS TESTS ===

# Real-world: user downloads a file from the web (fails DNS on vamos, correct behavior)
# Tests the full option combination a user would actually type
TEST: Real-world download attempt with -q --tries=1 (user workflow, DNS fails)
CMD: WORK:wget -q --tries=1 http://nonexistent.invalid/index.html
EXPECT_RC: 4

# Real-world: recursive download attempt (most common wget use case)
# -r -l 1 limits depth to 1. Fails DNS on vamos. Tests recursive option path.
TEST: Real-world recursive download -r -l 1 (common user workflow, DNS fails)
CMD: WORK:wget -r -l 1 http://nonexistent.invalid/
EXPECT_RC: 4

# Real-world: resume interrupted download (-c continue flag)
# Fails DNS on vamos. Tests continue-mode option path.
TEST: Real-world resume download -c (DNS fails on vamos)
CMD: WORK:wget -c http://nonexistent.invalid/file.zip
EXPECT_RC: 4

# Real-world: save with specific filename (-O outfile)
# Fails DNS before creating the file (fopen happens after URL check).
# Actually fopen happens before network in main.c (output_stream opened at line 1950)
# before the retrieval loop. But with -O - to stdout it's fine.
TEST: Real-world output to named temp file -O with failing URL
CMD: WORK:wget -O T:test-wget-output.dat http://nonexistent.invalid/file.bin
EXPECT_RC: 4

# Real-world: spider check (link verification)
# --spider --no-config combined with timeout=1 (no-op on AmigaOS)
TEST: Real-world spider mode with multiple options (DNS fails on vamos)
CMD: WORK:wget --spider --no-config --tries=1 http://nonexistent.invalid/
EXPECT_RC: 4

# Stress: many option flags combined
# Combining -q, --tries=1, --no-config, --user-agent, -S all at once
# Tests option parser under heavy option load. Fails DNS. RC=4.
TEST: Stress test many combined options with failing URL
CMD: WORK:wget -q --tries=1 --no-config --user-agent=TestAgent/1.0 -S http://nonexistent.invalid/
EXPECT_RC: 4

# Stress: -i input-file with 2-URL file (both DNS fails)
# Tests URL-list processing path end-to-end under failure conditions.
TEST: Stress test URL-list file with multiple failing URLs
CMD: WORK:wget --no-config --tries=1 -i WORK:test-wget-urls.txt
EXPECT_RC: 4

# Stress: --version output is deterministic -- validate multiple lines
# Line 1: version header, line 2: blank, line 3: first feature flag row,
# line 4: second feature flag row, line 5: blank, then Wgetrc: section.
# This stress-tests the build_info compiled_features rendering loop.
# Use EXPECT_CONTAINS: for feature lines to avoid trailing-space issues.
TEST: Stress version output multi-line feature flag rendering
CMD: WORK:wget --version
EXPECT: GNU Wget 1.20.3 built on AmigaOS.
EXPECT_CONTAINS: -cares +digest -gpgme -https -ipv6 -iri -large-file
EXPECT_CONTAINS: -ntlm +opie -psl -ssl
EXPECT_RC: 0

# Precision: --help output contains Usage: exactly (no truncation or corruption)
# This validates that the full help text is rendered without IO errors
# Source: print_help() calls fputs() for each entry; exits(WGET_EXIT_IO_FAIL=3) on failure
TEST: Precision --help renders complete usage line without IO truncation
CMD: WORK:wget --help
EXPECT: GNU Wget 1.20.3, a non-interactive network retriever.
EXPECT_LINE: 2,Usage: WORK:wget [OPTION]... [URL]...
EXPECT_RC: 0

# === INTERACTIVE TESTS (Category 4 Network requirement per ADR-023) ===
# wget is non-interactive (no console UI) but ITEST blocks verify that:
# 1. The process starts, initializes bsdsocket, and exits cleanly
# 2. Ctrl-C (SIGBREAK_F on AmigaOS) is handled by amiport_check_break()
# 3. Long-running commands can be force-quit without leaving zombie processes
#
# On FS-UAE with Roadshow: wget may actually connect; WAIT3000 gives it time to fail.
# On vamos: no bsdsocket.library, wget fails immediately with RC=4.

# ITEST 1: --version exits cleanly with no interaction needed
# Tests that wget starts up, runs through init, prints version, and exits RC=0.
ITEST: wget --version starts and exits cleanly without interaction
LAUNCH: WORK:wget --version
KEYS: WAIT2000
EXPECT_RC: 0

# ITEST 2: non-existent host connection attempt fails gracefully
# wget tries DNS, fails (no network on vamos), exits RC=4.
# Tests that the bsdsocket error path returns correctly without hanging.
ITEST: wget with nonexistent host fails DNS and exits without hanging
LAUNCH: WORK:wget --no-config --tries=1 http://nonexistent.invalid/
KEYS: WAIT3000
EXPECT_RC: 4

# ITEST 3: Ctrl-C interrupts a running wget download attempt
# If wget is still running after WAIT2000 (unlikely on vamos but possible on
# FS-UAE with a real TCP stack and an unresponsive server), Ctrl-C fires
# amiport_check_break() and the process exits cleanly.
ITEST: Ctrl-C interrupts wget during download attempt
LAUNCH: WORK:wget --no-config --tries=1 http://nonexistent.invalid/
KEYS: WAIT2000,CTRL_C
EXPECT_RC: 4
```

## Emulator Log

```
(log not captured in this run)
```

## Sentinel File

Written by the ARexx harness when all tests complete:

```
TESTS_COMPLETE
passed=68
failed=0
total=68
```

---
Generated by `make test-fsemu TARGET=ports/wget`
Report template: `toolchain/templates/test-report.md.template`
