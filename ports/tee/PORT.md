# Port: tee

## Overview

| Field | Value |
|-------|-------|
| Program | tee |
| Version | 1.15 |
| Source | OpenBSD tee.c v1.15 (2023-03-04) |
| Category | 1 — CLI tool |
| License | BSD-3-Clause |
| Original Author | University of California, Berkeley |
| Port Date | 2026-03-22 |

## Description

tee copies standard input to standard output, making a copy in zero or more files. Supports append mode (-a) and SIGINT ignore (-i). A fundamental Unix pipeline utility.

## Prior Art on Aminet

No functional 68k tee exists for AmigaOS 3.x. GNU coreutils-bin on Aminet (2006) is PPC-only (AmigaOS 4). Geek Gadgets (~1998) may have included one but requires ixemul.library and is 25+ years old. AmigaOS 4 has a native TEE command but only for PPC systems.

## Portability Analysis

Verdict: **EASY** — All issues are mechanical Tier 1 shim replacements.

| Issue | Tier | Resolution |
|-------|------|------------|
| `pledge()` calls | 1 | Stub as `#define pledge(p,e) (0)` |
| `signal(SIGINT, SIG_IGN)` | 1 | `amiport_signal()` via `<amiport/signal.h>` |
| `getopt()` | 1 | `amiport_getopt()` via `<amiport/getopt.h>` |
| `open()` with O_WRONLY/O_CREAT/O_TRUNC/O_APPEND | 1 | `amiport_open()` via `<amiport/unistd.h>` |
| `DEFFILEMODE` constant | 1 | `#define DEFFILEMODE 0666` (Amiga ignores mode bits) |
| `read()` / `write()` | 1 | `amiport_read()` / `amiport_write()` |
| `close()` | 1 | `amiport_close()` (protects fd 0/1/2) |
| `err()` / `warn()` | 1 | `amiport_err()` / `amiport_warn()` via `<amiport/err.h>` |
| Exit codes `1` | 1 | Remap to `10` (RETURN_ERROR) |

## Transformations Applied

| Line(s) | Original | Transformed | Comment |
|----------|----------|-------------|---------|
| 33-34 | `<sys/types.h>`, `<sys/stat.h>` | Removed | Not needed; DEFFILEMODE defined locally |
| 38 | `<err.h>` | `<amiport/err.h>` | BSD err/warn family |
| 40 | `<fcntl.h>` | Removed | O_* constants from `<amiport/unistd.h>` |
| 42 | `<signal.h>` | `<amiport/signal.h>` | SIGINT/SIG_IGN via macros |
| 45 | `<stdlib.h>` | `<amiport/stdlib.h>` | Activates exit macro |
| 50 | `<unistd.h>` | `<amiport/unistd.h>` | open/close/read/write/ssize_t |
| 52 | *(new)* | `<amiport/getopt.h>` | getopt not in libnix |
| 54 | *(new)* | `<amiport/glob.h>` | argv wildcard expansion |
| 57 | *(new)* | `#define pledge(p, e) (0)` | OpenBSD security stub |
| 60 | *(new)* | `#define DEFFILEMODE 0666` | BSD constant not in amiport headers |
| 63 | *(new)* | `$VER: tee 1.15` | Amiga version string |
| 66 | *(new)* | `long __stack = 32768` | Stack cookie |
| 71 | `BSIZE (64 * 1024)` | `BSIZE (8 * 1024)` | Reduced: 64KB wastes 12.5% of A500 RAM |
| 81 | `void add()` | `int add()` | Returns error for malloc failure cleanup |
| 84,110,167 | `err(1, ...)` | `err(10, ...)` | RETURN_ERROR exit codes |
| 129,154,etc | `exitval = 1` / `return 1` | `exitval = 10` / `return 10` | RETURN_ERROR exit codes |
| 105 | *(new)* | `amiport_expand_argv()` | Amiga shell wildcard expansion |
| 150 | `open(*argv, ...)` | `amiport_open(*argv, ...)` | 2-arg form, mode bits dropped |
| 155 | `add(fd, *argv)` | `add(fd, *argv) == -1` check | Close fd on malloc failure |
| 174 | `read(STDIN_FILENO, ...)` | `amiport_read(STDIN_FILENO, ...)` | AmigaDOS Read() |
| 176 | *(new)* | `amiport_check_break()` | Ctrl-C check in read loop |
| 183 | `write(p->fd, ...)` | `amiport_write(p->fd, ...)` | AmigaDOS Write() |
| 202 | `close(p->fd)` | `amiport_close(p->fd)` | AmigaDOS Close() |
| 213-217 | *(new)* | SLIST cleanup loop | Free all list nodes before exit |
| 220 | *(new)* | `amiport_free_argv()` | Free expanded argv |
| 223-224 | `return exitval` | `fflush(stdout); _exit(exitval)` | Avoid libnix exit hang |

## Shim Functions Exercised

- `amiport_open()` — file opening with O_* flag translation
- `amiport_read()` — stdin read via AmigaDOS Read()
- `amiport_write()` — file/stdout write via AmigaDOS Write()
- `amiport_close()` — file close with fd 0/1/2 protection
- `amiport_signal()` — SIGINT → SIGBREAKF_CTRL_C mapping
- `amiport_check_break()` — Ctrl-C polling in read loop
- `amiport_expand_argv()` / `amiport_free_argv()` — wildcard expansion
- `amiport_getopt()` — argument parsing
- `err()` / `warn()` — BSD error reporting

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68000+ |
| CFLAGS | `-O2 -noixemul -m68000 -Wall` |
| Libraries | `-lamiport` (posix-shim) |
| Binary size | 30,844 bytes |

## Test Results

11/11 FS-UAE tests passed. See TEST-REPORT.md for full results.

| Test | Result |
|------|--------|
| basic stdin passthrough | PASS |
| write to single file | PASS |
| append mode (-a) | PASS |
| multiple output files | PASS |
| -i flag accepted | PASS |
| invalid flag → RC 10 | PASS |
| nonexistent dir → RC 10 | PASS |
| empty input | PASS |
| long line (>1024 chars) | PASS |
| RAM: volume path | PASS |
| T: volume path | PASS |

## Known Limitations

- `-i` flag (ignore SIGINT) maps to Ctrl-C via amiport_signal() shim — works between read() calls but cannot interrupt a blocking read
- DEFFILEMODE (0666) is passed to open() but Amiga has no Unix permission model — files get default protection bits
- Buffer reduced from 64KB to 8KB for A500 compatibility; no throughput impact on AmigaDOS

## Review

Reviewed with `/review-amiga`. Score: **READY**. No critical issues.

- Stack safety: OK (`__stack = 32768`, no large stack allocations)
- Memory handling: OK (all allocations freed, add() returns error on OOM)
- Path handling: OK (no hardcoded Unix paths)
- Library cleanup: OK (no direct Amiga library opens)
- Conventions: OK (`$VER` string, exit codes 0/10, `fflush` before `_exit`)
- Logic bugs: OK (all 6 automated checks clean)

Memory-checker: PASS after fixes (add() returns error, caller closes leaked fd).
Perf-optimizer: BSIZE reduced 64KB→8KB, amiport_check_break() added.
