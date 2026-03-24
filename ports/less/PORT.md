# Port: less

## Overview

| Field | Value |
|-------|-------|
| Program | less |
| Version | 692 |
| Source | GNU less 692 (GPL-3.0 / Less License) |
| Category | 3 — Console UI |
| License | GPL-3.0 or Less License (dual-licensed) |
| Original Author | Mark Nudelman |
| Port Date | 2026-03-24 |

## Description

GNU less is the standard Unix pager. Displays files one screen at a time with forward/backward scrolling, regex search, line numbers, and many other features. First Category 3 (console UI) port in amiport — the first program to use the console-shim termcap API.

## Prior Art on Aminet

No existing less port found for classic AmigaOS 3.x on 68k. AmigaDOS includes `MORE` which is forward-only with no search. The jffabre Unix commands collection may have contained an obsolete version, but it's inaccessible.

## Portability Analysis

Verdict: **MODERATE** — 36 source files, termcap-based (not ncurses), most POSIX deps handled by defines.h configuration.

| Issue | Tier | Resolution |
|-------|------|------------|
| termcap API (tgetent/tgetstr/etc) | 1 | console-shim libamiport-console.a |
| termios (tcgetattr/tcsetattr) | 1 | posix-shim amiport/termios.h |
| /dev/tty keyboard input | 2 | Replaced with "*" (AmigaDOS star device) |
| poll() for follow mode | 2 | Disabled (HAVE_POLL=0), blocking read fallback |
| TIOCGWINSZ window size | 2 | CSI cursor position report in scrsize() |
| nl_langinfo locale | 2 | Disabled (HAVE_LOCALE=0), latin1 default |
| nanosleep/usleep | 2 | Disabled, sleep() fallback |
| popen/pclose | 2 | Disabled (HAVE_POPEN=0), not in libnix |
| SIGTSTP job control | 3 | Disabled (#ifdef guarded), no job control on AmigaOS |

## Transformations Applied

| File | Original | Transformed | Comment |
|------|----------|-------------|---------|
| screen.c | `#include <termcap.h>` | `#include <amiport-console/term.h>` | Termcap via console-shim |
| screen.c | `#include <termios.h>` | `#include <amiport/termios.h>` | Termios via posix-shim |
| screen.c | TIOCGWINSZ ioctl | CSI cursor position report | AmigaOS window size detection |
| ttyin.c | `open("/dev/tty")` | `open("*")` | AmigaDOS star device = console |
| main.c | — | `long __stack = 65536` | Stack cookie for recursive operations |
| main.c | — | `$VER: less 692 (24.03.2026)` | AmigaOS version string |
| main.c | — | `char *__progname = "less"` | Prevent weak symbol linker issue |
| main.c | — | Cleanup in quit() | Free leaked globals on exit |
| less.h | `QUIT_ERROR 1` | `QUIT_ERROR 10` | Amiga RC: RETURN_ERROR |
| less.h | `QUIT_INTERRUPT 2` | `QUIT_INTERRUPT 5` | Amiga RC: RETURN_WARN |
| lang.h | `#endif //` | `#endif /* */` | C89 comment fix |
| defines.h | — | Hand-crafted | AmigaOS configuration (150+ values) |
| edit.c | `close_pipe()` | `#if HAVE_POPEN` guard | pclose not in libnix |
| tags.c | `pclose()` | `#if HAVE_POPEN` guard | pclose not in libnix |
| tags.c | `char buf[1024]` | `static char buf[1024]` | Crash-patterns #10 |
| line.c | `ecalloc(ansi_state)` | static pool | Perf: eliminate malloc on hot path |
| line.c | `strchr(ansi_chars)` | 256-byte lookup table | Perf: O(1) char classification |
| line.c | `% NUM_LAST_ANSIS` | conditional increment | Perf: avoid 68000 DIVU |
| defines.h | `OUTBUF_SIZE 1024` | `OUTBUF_SIZE 4096` | Perf: fewer write() syscalls |

## Shim Functions Exercised

- `tgetent()`, `tgetstr()`, `tgetnum()`, `tgetflag()`, `tgoto()`, `tputs()`, `tparm()` — console-shim termcap
- `tcgetattr()`, `tcsetattr()`, `cfmakeraw()`, `cfgetospeed()` — posix-shim termios

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68000 |
| CFLAGS | `-O2 -noixemul -m68000 -Wall -std=gnu99` |
| Libraries | `-lamiport-console -lamiport` |
| Binary size | ~239 KB |
| Source files | 37 .c files |

## Test Results

### Automated (FS-UAE): 20/20 pass

See TEST-REPORT.md for full TAP output.

### Interactive Testing (Category 3 — MANDATORY)

Launch: `make install-emu && make emu`, then `WORK:less WORK:test-interactive.txt`

| Test | Expected | Result |
|------|----------|--------|
| SPACE scrolls forward | Shows next screenful | PASS |
| b scrolls backward | Shows previous screenful | PASS |
| /FINDME + ENTER | Jumps to match, highlights | PASS |
| n finds next match | Jumps to line 75 | PASS |
| q quits cleanly | Shell prompt returns | PASS |
| Status line at bottom | : on last row | PASS |
| Backspace during search | Deletes char | PASS |
| G jumps to end | Shows last line | PASS |
| Screen restores on quit | No garbage | PASS |

## Platform Compatibility Notes

- Console-shim compiled with `-m68000` for vamos compatibility (was `-m68020`)
- Console-shim globals separated to `globals.c` to prevent intuition.library linkage in termcap-only programs

## Known Limitations

- No follow mode (`less +F`) — `poll()` disabled, falls back to blocking read
- No pipe commands (`|`) — `popen()`/`pclose()` not in libnix
- No locale/UTF-8 auto-detection — defaults to latin1, set `LESSCHARSET=utf-8` manually
- No window resize detection — size queried once at startup
- No LESSOPEN/LESSCLOSE preprocessor — disabled with HAVE_POPEN=0
- Screen size detection via CSI may not work in all console environments

## Knowledge Capture

- **m68020 vs m68000**: Console-shim was originally `-m68020`, causing vamos crashes. Fixed to `-m68000`. Added to known-pitfalls awareness.
- **globals.c separation**: COLS/LINES in initscr.o pulled intuition.library into termcap-only programs. New pattern: put shared globals in a dependency-free .c file.
- **port-coordinator deprecated**: Cannot dispatch subagents. Orchestrate multi-file ports from main session instead.
- **Interactive testing gap**: Category 3+ ports need manual interactive verification — automated harness can't send keystrokes.

## Review

Memory-checker: 4 CRITICAL leaks found and fixed (every_first_cmd, tagoption in quit()).
Perf-optimizer: 2 HIGH, 2 MEDIUM findings applied (ansi pool, lookup tables, DIVU avoidance, OUTBUF_SIZE).
