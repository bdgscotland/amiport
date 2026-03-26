# Port: tty

## Overview

| Field | Value |
|-------|-------|
| Program | tty |
| Version | 1.14 |
| Source | OpenBSD usr.bin/tty (v1.14, May 2024) |
| Category | 1 -- CLI tool |
| License | BSD 3-Clause |
| Original Author | The Regents of the University of California |
| Port Date | 2026-03-26 |

## Description

tty prints the name of the terminal connected to standard input. If stdin is not a terminal (e.g., redirected from a file), it prints "not a tty" and exits with a non-zero status. The `-s` flag suppresses output, making it useful in shell scripts that only need the exit code. This is the OpenBSD implementation -- a minimal, single-file utility.

## Prior Art on Aminet

Researched via aminet-researcher agent. No POSIX-compatible tty utility found on Aminet. AmigaOS shell scripts can use `Status` to check process state, but there is no direct equivalent to the Unix `tty` command.

## Portability Analysis

Verdict: **SIMPLE** -- Single file, minimal dependencies. The main challenge is replacing `ttyname()` with AmigaOS `IsInteractive()`.

| Issue | Tier | Resolution |
|-------|------|------------|
| `ttyname(STDIN_FILENO)` | Tier 2 | Replaced with `IsInteractive(Input())` from `<proto/dos.h>` |
| `<paths.h>` / `_PATH_DEVDB` | Stub | Removed -- OpenBSD-specific, only used by stubbed `unveil()` |
| `err()` | Tier 1 | `<amiport/err.h>` |
| `getopt()` | Tier 1 | `<amiport/getopt.h>` |
| `pledge()` / `unveil()` | Stub | `#define pledge(p, e) (0)` / `#define unveil(p, f) (0)` |
| `<proto/dos.h>` | N/A | Added for `IsInteractive()` and `Input()` |
| Exit codes | Tier 1 | 0=tty (OK), 1->10 (RETURN_ERROR/not a tty), 2->20 (RETURN_FAIL/usage) |

## Transformations Applied

| File | Line(s) | Original | Transformed | Comment |
|------|----------|----------|-------------|---------|
| tty.c | 34 | `<stdlib.h>` | `<amiport/stdlib.h>` | Activates exit() macro |
| tty.c | 37 | `<unistd.h>` | `<amiport/unistd.h>` | Shim replacement |
| tty.c | 38 | `<paths.h>` | removed | OpenBSD-specific |
| tty.c | 39-41 | (none) | `<amiport/err.h>`, `<amiport/getopt.h>` | Shim headers |
| tty.c | 43 | (none) | `#include <proto/dos.h>` | AmigaOS IsInteractive/Input |
| tty.c | 45 | (none) | `<amiport/glob.h>` | Wildcard expansion |
| tty.c | 48-49 | (none) | `#define pledge/unveil` | Stubbed as no-ops |
| tty.c | 52 | (none) | `$VER: tty 1.14` | Amiga version string |
| tty.c | 55 | (none) | `long __stack = 16384` | Stack cookie |
| tty.c | 60-66 | (none) | `cleanup()` with `amiport_free_argv()` | atexit cleanup |
| tty.c | 76-78 | (none) | `amiport_expand_argv()` + `atexit(cleanup)` | Wildcard expansion |
| tty.c | 99-110 | `t = ttyname(STDIN_FILENO)` | `IsInteractive(Input())` check | Returns "CON:" or NULL |
| tty.c | 115 | `exit(t ? 0 : 1)` | `exit(t ? 0 : 10)` | RETURN_ERROR for non-tty |
| tty.c | 123 | `exit(2)` | `exit(20)` | RETURN_FAIL for usage |

## Shim Functions Exercised

- `amiport_err()`
- `amiport_expand_argv()`, `amiport_free_argv()`
- AmigaOS direct: `IsInteractive()`, `Input()` via `<proto/dos.h>`

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68000+ |
| CFLAGS | `-O2 -noixemul -m68000 -Wall` |
| Stack | 16384 bytes |
| Libraries | `-lamiport` (Tier 1 posix-shim) |
| Binary size | ~29KB |

## Test Results

**11/11 passed** (100%) on FS-UAE (A1200, Kickstart 3.1).

| Category | Count | Description |
|----------|-------|-------------|
| Functional | 2 | Default output "CON:", silent flag -s |
| Error paths | 3 | Invalid flags -Z, -x, -f |
| Redirect (non-tty) | 2 | stdin from file: "not a tty" + RC=10, -s with redirect |
| Resource safety | 2 | Repeated invocations succeed without leaks |
| Interactive (ITEST) | 2 | KeyInject verification: default and -s from console |

## Memory Safety

- `amiport_free_argv()` via `atexit(cleanup)` on all exit paths
- No dynamic allocations beyond argv expansion -- tty uses only static strings and AmigaOS API calls
- `IsInteractive()` and `Input()` are read-only queries with no cleanup needed

## Performance Notes

No performance concerns. Single-call utility: one `IsInteractive()` query, one `puts()`, one `exit()`. The entire program runs in microseconds.

## Known Limitations

1. **Reports "CON:" instead of /dev/ttyN** -- On Unix, tty prints the device path (e.g., `/dev/tty0`). On AmigaOS, there is no `/dev/` device tree. When the process has an interactive console, this port reports `"CON:"` -- the canonical AmigaOS console device name. This is the most meaningful equivalent.
2. **No ttyname() mapping** -- AmigaOS does not provide `ttyname()`. The port uses `IsInteractive(Input())` which returns a boolean, not a device path. All interactive consoles report the same `"CON:"` string regardless of which CON: window they are attached to.
3. **Exit code mapping** -- POSIX tty returns 1 for "not a tty"; this port returns 10 (RETURN_ERROR) to be visible to AmigaDOS `IF ERROR` tests. Usage errors return 20 (RETURN_FAIL).
4. **Input() handle depends on launch context** -- When launched via `Run >clinumfile Execute scriptfile` (the ITEST harness pattern), `Input()` returns the script file handle. `IsInteractive()` correctly returns FALSE in that case. Direct console launch returns TRUE.

## Review

Reviewed with `/review-amiga`. Score: **READY**.

| Dimension | Score |
|-----------|-------|
| Stack safety | OK (16KB cookie, no local buffers) |
| Memory handling | OK (atexit cleanup, no dynamic allocations) |
| Path handling | N/A (tty does not open files) |
| Library cleanup | OK (no library opens -- uses dos.library via process base) |
| Conventions | OK (version string, exit codes, stack cookie) |
