---
name: tetris_analysis
description: OpenBSD tetris portability analysis: MODERATE verdict, Category 3 Console UI, ppoll/clock_gettime redesign, termios shim, sigprocmask no-op, arc4random in libnix, fdopen(open()) crash-patterns #12, LOGIN_NAME_MAX define needed, OXTABS strip, timespecsub/timespecclear local macros, errc/strtonum shims
type: project
---

OpenBSD tetris portability analysis — MODERATE verdict.

**Why:** The game loop timing (ppoll + clock_gettime) requires a Tier 2/3 redesign of input.c. All other issues (termios, signals, score file, display) are Tier 1 shims or trivial stubs. fdopen(open()) in scores.c is crash-patterns #12 and is a linker-namespace blocker.

**How to apply:** Redesign input.c to use timer.device for timeout + WaitForChar() for keyboard. Replace fdopen(sd, mstr) with fopen(scorepath, mstr). All other issues follow standard patterns.

Key items:
- ppoll() — not in libnix, must redesign (Tier 3)
- clock_gettime(CLOCK_MONOTONIC) — not in libnix, must replace with timer.device EClock or Delay()-based counter (Tier 3 as part of input.c redesign)
- timespecclear/timespecsub — OpenBSD sys/time.h macros, define locally
- fdopen(sd, mstr) in scores.c — crash-patterns #12 (amiport fd namespace vs libnix fd namespace)
- LOGIN_NAME_MAX — define locally (~256 or use 64)
- OXTABS — strip from termios c_oflag (AmigaOS has no tab-expansion flag)
- arc4random_uniform() — in libnix! Available directly
- strtonum() — amiport/err.h shim
- errc() — amiport/err.h shim
- pledge()/unveil() — stub as macros
- __dead — strip or #define __dead __attribute__((noreturn))
- getprogname() — macro -> __progname via posix-tiers
- getlogin() — amiport_getlogin() returns "amiga"
- strlcpy() — in libnix directly
- termios (tcgetattr/tcsetattr) — amiport/termios.h shim (works for ECHO/ICANON)
- SIGTSTP/SIGTTOU — not real on AmigaOS, stub signal handlers as no-ops
- sigprocmask() — amiport no-op shim
- ioctl(TIOCGWINSZ) — amiport_ioctl() shim (fallback 80x24)
- kill(getpid(), sig) — must stub (no kill() on AmigaOS)
- scores.c file locking (LOCK_EX/LOCK_SH comment only, uses fdopen) — AmigaOS has exclusive-lock via MODE_NEWFILE; no explicit flock
- exit code: all err/errx uses 1 -> must change to 10
