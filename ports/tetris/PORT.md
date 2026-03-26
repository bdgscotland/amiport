# Port: tetris

## Overview

| Field | Value |
|-------|-------|
| Program | tetris |
| Version | 1.35 (port revision: 1) |
| Source | OpenBSD games/tetris (openbsd/src rev 1.35) |
| Category | 3 -- Console UI |
| License | BSD 3-clause |
| Original Author | Chris Torek, Darren F. Provine (UC Berkeley) |
| Port Date | 2026-03-26 |

## Description

BSD Tetris is a terminal-mode tetris game from the OpenBSD games collection. It uses termcap for display rendering and raw console input for real-time gameplay. This is the first terminal-based tetris for AmigaOS -- all existing Amiga tetris games are graphical Intuition applications.

## Prior Art on Aminet

No terminal/console tetris exists on Aminet. Multiple graphical tetris clones exist (tetris-ik, SimpleTetris, amigang-tetris1, Tetris 1200) but all use Intuition windows. This is a novel port category for the platform.

## Portability Analysis

Verdict: MODERATE -- Core game engine is fully portable. The main redesign is input.c (ppoll/clock_gettime game timer replaced with WaitForChar). The fdopen(open()) pattern in scores.c is a known crash-patterns #12 blocker.

| Issue | Tier | Resolution |
|-------|------|------------|
| pledge()/unveil() | 1 | Stub macros |
| strtonum() | 1 | amiport_strtonum() |
| errc() | 1 | amiport_errc() |
| getlogin() | 1 | amiport_getlogin() |
| getprogname() | 1 | __progname via amiport_expand_argv() |
| tcgetattr()/tcsetattr() | 1 | amiport/termios.h |
| ioctl(TIOCGWINSZ) | 1 | amiport_ioctl() |
| signal(SIGINT) | 1 | amiport/signal.h |
| sigprocmask/sigemptyset/sigaddset | 1 | amiport/signal.h (no-ops) |
| ppoll() + clock_gettime() | 3 | Redesign: WaitForChar(Input(), timeout_us) |
| fdopen(open()) in scores.c | 3 | Replace with fopen() -- crash-patterns #12 |
| kill(getpid(), sig) | 3 | Remove -- no job control on AmigaOS |
| SIGTSTP/SIGTTOU handlers | 3 | Remove -- no job control on AmigaOS |
| LOGIN_NAME_MAX | arch | Define as 64 |
| OXTABS termios flag | arch | #ifdef guard |
| HOME env fallback | arch | PROGDIR: fallback |
| __dead attribute | arch | #define to __attribute__((noreturn)) |

## Transformations Applied

| Line(s) | Original | Transformed | Comment |
|----------|----------|-------------|---------|
| | | | (filled by code-transformer agent) |

## Shim Functions Exercised

- amiport_strtonum()
- amiport_errc()
- amiport_getlogin()
- amiport_getenv()
- amiport_expand_argv()
- amiport_tcgetattr() / amiport_tcsetattr()
- amiport_ioctl(TIOCGWINSZ)
- amiport_signal()
- amiport_check_break()
- tgetent() / tgetstr() / tgetnum() / tgetflag() / tgoto() / tputs() (console-shim)

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68000+ |
| CFLAGS | `-O2 -noixemul -m68000 -Wall` |
| Libraries | `-lamiport-console -lamiport` |
| Binary size | |

## Test Results

Interactive game -- requires FS-UAE testing. See test-fsemu-cases.txt and test-fsemu-visual-cases.txt.

## Platform Compatibility Notes

- LOGIN_NAME_MAX set to 64 (determines binary score file format)
- WaitForChar() timing granularity is 20ms (1 Amiga tick = 1/50s). At level 9 (111ms fallrate), this introduces ~18% timing jitter. Game remains playable.
- All scores recorded under username "amiga" unless LOGNAME/USER env vars are set
- Score file stored in PROGDIR:tetris.scores (no leading dot)

## Known Limitations

- No job control (SIGTSTP/Ctrl-Z) -- AmigaOS has no job control
- Score file username always "amiga" on single-user systems
- Timer granularity coarser than POSIX original (20ms vs nanosecond)

## Review

<!-- /review-amiga score summary. -->
