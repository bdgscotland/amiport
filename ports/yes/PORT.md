# Port: yes

## Overview

| Field | Value |
|-------|-------|
| Program | yes |
| Version | 1.9 |
| Source | OpenBSD yes.c (rev 1.9, 2015-10-13) |
| Category | 1 — CLI tool |
| License | BSD-3-Clause |
| Original Author | The Regents of the University of California |
| Port Date | 2026-03-21 |

## Description

`yes` repeatedly outputs a string until killed. With no arguments it outputs "y"; with arguments it outputs the first argument. Commonly used to pipe affirmative responses to interactive programs.

## Prior Art on Aminet

No 68k AmigaOS port of `yes` exists. A PPC-only port (GNU coreutils 5.2.1, 2006) exists on Aminet but targets AmigaOS 4.x only. This is the first standalone 68k port.

## Portability Analysis

Verdict: **EASY** — 17 executable lines, all Tier 1.

| Issue | Tier | Resolution |
|-------|------|------------|
| `pledge()` call | 1 | Stub as `#define pledge(p,e) (0)` |
| `<err.h>` / `err()` | 1 | Replace with `<amiport/err.h>` |
| `<unistd.h>` | 1 | Replace with `<amiport/unistd.h>` |
| `err(1, ...)` exit code | 1 | Change to `err(10, ...)` for RETURN_ERROR |
| Infinite `for(;;)` loops | 1 | Add `amiport_check_break()` for Ctrl-C |

## Transformations Applied

| Line(s) | Original | Transformed | Comment |
|----------|----------|-------------|---------|
| 35 | `#include <err.h>` | `#include <amiport/err.h>` | amiport: use shim err.h |
| 37 | `#include <unistd.h>` | `#include <amiport/unistd.h>` | amiport: use shim unistd.h |
| — | (none) | `#include <amiport/signal.h>` | amiport: added for amiport_check_break() |
| — | (none) | `#define pledge(p,e) (0)` | amiport: stub OpenBSD pledge |
| — | (none) | `#define unveil(p,f) (0)` | amiport: stub OpenBSD unveil |
| 42 | `err(1, "pledge")` | `err(10, "pledge")` | amiport: Amiga RETURN_ERROR |
| 44–45 | `for(;;) puts(...)` | Added `amiport_check_break()` | amiport: Ctrl-C support |
| 46–47 | `for(;;) puts(...)` | Added `amiport_check_break()` | amiport: Ctrl-C support |
| — | (none) | `long __stack = 8192;` | amiport: stack cookie |
| — | (none) | `$VER: yes 1.9 (21.03.2026)` | amiport: version string |

## Shim Functions Exercised

- `amiport_err()` (via `<amiport/err.h>` macro)
- `amiport_check_break()` (via `<amiport/signal.h>`)

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68000+ |
| CFLAGS | `-O2 -noixemul -m68000 -Wall` |
| Libraries | `-lamiport` (posix-shim) |
| Binary size | 8,104 bytes |

## Test Results

Tested via vamos — all pass:

| Test | Command | Expected | Result |
|------|---------|----------|--------|
| Default output | `yes \| head -5` | 5 lines of "y" | PASS |
| Custom string | `yes hello \| head -5` | 5 lines of "hello" | PASS |
| Version cookie | `strings yes \| grep VER` | `$VER: yes 1.9 (21.03.2026)` | PASS |

11 FS-UAE test cases generated in `test-fsemu-cases.txt`.

## Known Limitations

- `yes` outputs only the first argument when multiple arguments are given (matches OpenBSD behavior; GNU `yes` concatenates all arguments). This is a design choice of the OpenBSD source, not a porting limitation.

## Review

Reviewed with `/review-amiga`. Score: **READY**. No critical issues.

- Stack safety: OK (`__stack = 8192`)
- Memory handling: OK (no dynamic allocations)
- Path handling: OK (no file I/O)
- Library cleanup: OK (no Amiga libraries opened)
- Conventions: OK (version string, exit codes, Ctrl-C)
- Logic bugs: OK (all automated checks clean)
- Memory checker: CLEAN (no allocations to leak)
