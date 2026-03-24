---
name: mg_analysis
description: Mg 3.8-pre (troglobit fork) portability analysis — HARD verdict, fork/exec/socketpair in region.c and dired.c are main Tier 3 blockers, WITHOUT_CURSES mode makes terminal I/O manageable
type: project
---

# Mg 3.8-pre Portability Analysis Summary

**Verdict:** HARD
**Category:** Console UI (Category 3)
**Port strategy:** -DWITHOUT_CURSES, disable dired/grep/compile/cscope

## Key Blockers (Tier 3)

1. **region.c `pipeio()`** — fork()+execv()+socketpair()+poll() for shell pipe-to-region / shell-command features. Redesign to SystemTags() or disable.
2. **dired.c `d_exec()`** — fork()+execvp()+pipe() for dired shell execution. Whole dired subsystem should be disabled (-DENABLE_DIRED=no).
3. **spawn.c `spawncli()`** — SIGTSTP job control (C-z suspend). No job control on AmigaOS. Stub as "feature unavailable".
4. **tty.c** — SIGWINCH/SIGCONT/sigaction() for terminal resize. AmigaOS has no SIGWINCH. Window size fixed at 80x24; stub the handler.

## Tier 2 Issues

- `poll()` in ttyio.c (ttwait) and ansi.c (setupterm) — replace with WaitForChar() / Tier 2 emu
- `struct timespec` / `st_mtim` / `tv_nsec` for mtime change detection — AmigaOS DateStamp resolution is 1/50s (no nanoseconds). Compare tv_sec only, treat tv_nsec as always matching.
- `getpwuid()` / `getpwnam()` in fileio.c expandtilde() — stub: return home from ENV:HOME only
- `fchown()` — stub as no-op (no multi-user on AmigaOS)
- `fchmod()` / `chmod()` — amiport_chmod() no-op stub
- `futimens()` — already in lib/ as fallback; on Amiga, no-op or use SetFileDate()
- `umask()` — no-op on AmigaOS, returns 0

## Tier 1 Issues

- `termios.h` / `tcgetattr()` / `tcsetattr()` — amiport/termios.h covers this
- `stat()` / `fstat()` / `open()` / `read()` / `write()` / `close()` — amiport shims
- `getcwd()` / `chdir()` / `mkdir()` / `opendir()` / `readdir()` — amiport shims
- `getenv()` — amiport_getenv() (caller must free)
- `realpath()` — amiport_realpath()
- `asprintf()` / `vasprintf()` — amiport versions
- `strndup()` — available in libnix
- `getline()` — amiport_getline()
- `mkstemp()` — amiport_mkstemp()
- `TIOCGWINSZ` ioctl — stub, return 80x24 fallback (tty.c already handles this case)
- `FIONREAD` ioctl in ttyio.c charswaiting() — stub returning 0 (conservative, typing still works)
- `fileno()` — libnix provides this
- `fdopen()` — libnix provides this (but see crash-patterns #12 about amiport_open + fdopen)
- `SIGINT` / `signal()` — amiport_signal() covers SIGINT
- `strftime()` / `localtime()` / `time()` — available in libnix

## Features to Disable

- `--disable-dired` — fork/exec in d_exec(), too complex to rewrite
- `--disable-compile` — popen()/getline()/pclose() for grep/compile modes
- `--disable-cscope` — popen()/getline()/pclose() for cscope
- `MGLOG=no` — sys/queue.h usage in log.c uses system header, not queue.h
- batch mode / pty_init() — openpty()/login_tty() for PTY, disable by not using -b flag

## C89/C99 Issues

- VLA in fileio.c:55 — `char cmd[strlen(fn) + sizeof(GUNZIP) + 2]` — use fixed buffer or malloc
- `static inline` in def.h:827 — `static inline int globalwd()` — use `static int` or `#define`
- Designated initializer in tty.c:66 — `struct sigaction sa = { .sa_handler = winchhandler }` — needs C99 or rewrite
- `long long` in def.h and main.c — strtonum signature; the bundled lib/strtonum.c uses it; need to assess
- `__typeof__` in queue.h (XSIMPLEQ_XOR) — acceptable as GCC extension under -ansi

## Path Issues

- `~/.mg`, `~/.mg.d`, `/tmp` hardcoded paths — replace with S:mg, T: equivalents
- `_PATH_BSHELL` from `<paths.h>` — define as "PIPE:sh" or stub
- `/dev/null` in dired.c and grep.c — define as "NIL:"

## Stub Value Warnings

- `st_uid` / `st_gid` always 0 — fileio.c stores them in b_fi.fi_uid/fi_gid; fchown() uses them. Since fchown() is stubbed as no-op, this is fine.
- `st_mtim.tv_nsec` — fchecktime() compares tv_nsec for dirty detection. On AmigaOS, tv_nsec will always be 0 from DateStamp. Compare only tv_sec.
- `getenv("HOME")` returns malloc'd copy via amiport_getenv() — caller must free (currently leaked in fileio.c expandtilde)

## Key Transformation Notes

- `ttyio.c` write() call uses `write(fileno(stdout), ...)` not amiport_write — this must stay as libnix write for stdio fd
- `ansi.c setupterm()` uses scanf() to read terminal response for size detection — this may hang on AmigaOS if the terminal doesn't respond to the ESC[6n query. Must add a tight timeout or skip size query.
- `poll()` in ansi.c:130 — used in the setupterm() size query with 300ms timeout. Replace with WaitForChar(stdout, 300*1000/50) approx.
- The `ansi.c` ANSI backend is well-suited for Amiga console.device which supports ANSI escapes natively.
