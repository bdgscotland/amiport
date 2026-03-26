# Port: unexpand

## Overview

| Field | Value |
|-------|-------|
| Program | unexpand |
| Version | 1.13 (port revision: 1) |
| Source | OpenBSD usr.bin/unexpand (v1.13, Oct 2016) |
| Category | 1 -- CLI tool |
| License | BSD-3-Clause |
| Original Author | The Regents of the University of California |
| Port Date | 2026-03-26 |

## Description

unexpand converts leading spaces in each line of input to tabs, using 8-column tab stops. It is the inverse of expand(1). By default, only leading whitespace is converted; with the `-a` flag, spaces throughout each line that span tab stops are also converted. This is the OpenBSD implementation, commonly used for reformatting source code indentation and cleaning up files with inconsistent whitespace.

## Prior Art on Aminet

No existing unexpand port found on Aminet for AmigaOS 3.x.

## Portability Analysis

Verdict: **PORTABLE** -- Pure Tier 1 POSIX dependencies. Single-file program with no system calls beyond standard I/O and string operations.

| Issue | Tier | Resolution |
|-------|------|------------|
| `<stdbool.h>` | C99 | Removed; `bool` replaced with `int`, `true`/`false` with `1`/`0` |
| `<stdlib.h>` | Tier 1 | Replaced with `<amiport/stdlib.h>` for exit() macro |
| `<unistd.h>` | Tier 1 | Replaced with `<amiport/unistd.h>` |
| `strlcpy()` | Tier 1 | Provided via `<amiport/string.h>` shim |
| `pledge()` / `unveil()` | Stub | Macro stubs: `#define pledge(p, e) (0)` / `#define unveil(p, f) (0)` |
| Exit codes | Tier 1 | All `exit(1)` changed to `exit(10)` for Amiga RETURN_ERROR |
| Wildcard expansion | Tier 1 | Added `amiport_expand_argv()` -- Amiga shell does not glob |

## Transformations Applied

| File | Line(s) | Original | Transformed | Comment |
|------|----------|----------|-------------|---------|
| unexpand.c | 37 | `#include <stdbool.h>` | removed | C99 header not available in C89 |
| unexpand.c | 39 | `<stdlib.h>` | `<amiport/stdlib.h>` | Activates exit() macro |
| unexpand.c | 41 | `<unistd.h>` | `<amiport/unistd.h>` | Amiga unistd shim |
| unexpand.c | 42 | (none) | `<amiport/string.h>` | Provides strlcpy() shim |
| unexpand.c | 43 | (none) | `<amiport/glob.h>` | Provides amiport_expand_argv() |
| unexpand.c | 46-47 | (none) | pledge/unveil stubs | Not available on AmigaOS |
| unexpand.c | 50 | (none) | `$VER: unexpand 1.13 (26.03.2026)` | Amiga version string |
| unexpand.c | 53 | (none) | `long __stack = 16384` | Stack cookie -- Amiga default 4KB too small |
| unexpand.c | 59 | `void tabify(bool)` | `void tabify(int)` | C89 has no bool type |
| unexpand.c | 62-67 | (none) | `cleanup()` function | atexit cleanup for argv expansion + stdout flush |
| unexpand.c | 72 | `bool all = false` | `int all = 0` | C89 bool replacement |
| unexpand.c | 76-78 | (none) | `amiport_expand_argv()` + `atexit(cleanup)` | Wildcard expansion + cleanup registration |
| unexpand.c | 82-83 | `exit(1)` | `exit(10)` | Amiga RETURN_ERROR |
| unexpand.c | 89 | `exit(1)` | `exit(10)` | Amiga RETURN_ERROR |
| unexpand.c | 91 | `all = true` | `all = 1` | C89 bool replacement |
| unexpand.c | 98 | `exit(1)` | `exit(10)` | Amiga RETURN_ERROR |
| unexpand.c | 103-106 | `for (cp=linebuf; *cp; cp++) continue; cp[-1]=0;` | removed | Dead code -- perf-optimizer finding: loop scanned linebuf before tabify() populated it |
| unexpand.c | 106 | `printf("%s", linebuf)` | `fputs(linebuf, stdout)` | Perf-optimizer: avoids printf format overhead |
| unexpand.c | 114 | `void tabify(bool all)` | `void tabify(int all)` | C89 parameter type |
| unexpand.c | 158 | `strlcpy(dp, cp, len)` | same (shimmed) | strlcpy via `<amiport/string.h>` |

## Shim Functions Exercised

- `amiport_strlcpy()`
- `amiport_expand_argv()`, `amiport_free_argv()`

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68000+ |
| CFLAGS | `-O2 -noixemul -m68000 -Wall` |
| LDFLAGS | `-lamiport` |
| Stack cookie | 16384 (16 KB) |
| VAMOS_STACK | default |

## Test Results

31 tests in `test-fsemu-cases.txt`. Categories tested:

| Category | Count | Description |
|----------|-------|-------------|
| Flag coverage | 8 | Default mode, -a flag, leading/mid spaces |
| Exit codes | 3 | RC=0 success, RC=10 on missing file and unknown flag |
| Error paths | 2 | Non `-a` flags, unrecognized options |
| Edge cases | 7 | Empty file, only-spaces line, only-tabs, mixed tab+spaces, no trailing newline, 15 spaces, empty lines |
| Multi-line | 2 | 3-line file, multi-file arguments |
| Real-world | 3 | C source with 8-space indentation (unindented, singly-indented, doubly-indented) |
| Stress | 3 | 500-line file (first + last line), 500-space long line near BUFSIZ boundary |
| Precision | 1 | Tab stop arithmetic at non-aligned column position |
| Amiga-specific | 2 | WORK: volume path resolution |

Notable edge cases tested:
- 7-space line (less than tab stop) is NOT converted -- verifies boundary arithmetic
- File without trailing newline handled correctly (strlcpy copies tail)
- Mixed leading tab + 8 spaces correctly collapses to 2 tabs
- 500-space line near BUFSIZ boundary (501 bytes with newline) tests buffer limits

## Memory Safety

**Verdict: CLEAN -- Approved for shipping. Exemplary memory safety.**

Memory-checker audit (2026-03-26) found zero dynamic allocations. The program uses only static global buffers (`genbuf[BUFSIZ]` and `linebuf[BUFSIZ]`) and local stack variables. File handles are managed via `freopen()` reassignment of stdin -- no explicit close needed. All exit paths covered by `atexit(cleanup)` which frees the expanded argv.

## Performance Notes

Perf-optimizer review (2026-03-26) -- stream filter, I/O-bound.

- **HIGH** [dead linebuf scan removed] -- Original code had a `for (cp = linebuf; *cp; cp++) continue; cp[-1] = 0;` loop scanning linebuf BEFORE `tabify()` populated it. This was scanning stale/uninitialized data from the previous iteration -- logically dead code wasting cycles per input line. Removed in the ported source.
- **MEDIUM** [printf to fputs] -- Changed `printf("%s", linebuf)` to `fputs(linebuf, stdout)`. Eliminates format-string parsing overhead, one call per input line.
- **LOW** -- Global static buffers (`genbuf`, `linebuf`) are already correctly allocated at file scope, avoiding stack pressure.

## Known Limitations

1. **No `-t` flag** -- The OpenBSD 1.13 source does not implement the `-t tablist` option for custom tab stops. Any flag other than `-a` returns RC=10.
2. **BUFSIZ line length limit** -- Lines longer than BUFSIZ (1024 bytes on AmigaOS) are silently truncated. The `strlcpy()` in `tabify()` ensures no buffer overflow, but trailing content is lost.
3. **Stdin mode not testable** -- When no file arguments are given, unexpand reads from stdin. This mode cannot be tested in the ARexx harness (no EOF injection).

## Review

Reviewed by memory-checker and perf-optimizer agents. Score: **READY**.
