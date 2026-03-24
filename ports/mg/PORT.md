# Port: mg

## Overview

| Field | Value |
|-------|-------|
| Program | mg |
| Version | 3.7 |
| Source | https://github.com/troglobit/mg (v3.7 release) |
| Category | 3 — Console UI |
| License | Public Domain |
| Original Author | Joachim Wiberg (troglobit), OpenBSD |
| Port Date | 2026-03-24 |

## Description

mg (Micro GNU Emacs) is a lightweight, public domain Emacs-like text editor. Originally from the BSD family, it provides core Emacs keybindings (C-x C-s save, C-x C-c quit, C-x C-f open, C-s search) in a small footprint without requiring Lisp or heavy dependencies. The troglobit/mg fork is the portable version maintained for non-OpenBSD systems.

## Prior Art on Aminet

- **MicroEMACS 3.10** (1989): Different editor (ancestor of mg), 37 years old, uses Intuition GUI
- **JASSPA-MEmacs** (2022): PPC-only (AmigaOS 4), not compatible with 68k
- **Nano 1.2.5** (2006): Requires ixemul, 20 years old
- **Emacs 19.25** (1995): Full GNU Emacs, too heavy for 68k

No existing mg port for AmigaOS 3.x 68k. This is the first.

## Portability Analysis

Verdict: **COMPLEX** — 43 source files, ~21K LOC. Core editor portable with shim work; 5 subsystems disabled due to fork/exec requirement.

| Issue | Tier | Resolution |
|-------|------|------------|
| termios (tcgetattr/tcsetattr/cfmakeraw) | 1 | amiport/termios.h shim |
| File I/O (open/read/write/close/stat) | 1 | amiport shim functions |
| Directory ops (opendir/readdir/closedir) | 1 | amiport shim |
| Path ops (getcwd/chdir/mkdir/unlink/rename/realpath) | 1 | amiport shim |
| err/errx with exit codes | 1 | amiport/err.h, codes → 10 |
| getenv returns malloc'd string | 1 | Must free at each call site |
| asprintf/vasprintf | 1 | amiport shim |
| mkstemp | 1 | amiport shim |
| fnmatch | 1 | amiport shim |
| poll() in ttwait/setupterm | 2 | WaitForChar() or stub |
| POSIX regex (regcomp/regexec) | 2 | amiport-emu regex |
| getpwuid/getpwnam (tilde expansion) | 2 | Stub with ENV:HOME |
| fchmod/fchown/umask | 2 | No-op stubs |
| futimens | 2 | No-op stub |
| struct timespec st_mtim | 2 | tv_nsec always 0 |
| Shell pipe (fork+exec+socketpair) | 3 | **DISABLED** — M-x shell-command stubbed |
| Dired (fork+execvp+pipe) | 3 | **DISABLED** — dired.c excluded |
| Spawn/suspend (SIGTSTP) | 3 | **DISABLED** — C-z stubbed |
| Gzip (popen) | 3 | **DISABLED** — .gz support removed |
| Cscope/grep (popen) | 3 | **DISABLED** — cscope.c, grep.c excluded |

## Disabled Features

| Feature | File | Reason |
|---------|------|--------|
| Directory editor (dired) | dired.c | fork/execvp/pipe/SIGCHLD |
| Compile/grep mode | grep.c | popen/getline |
| Cscope integration | cscope.c | popen/getline |
| Shell command (M-x shell-command) | region.c | fork/execv/socketpair |
| Pipe region (M-! on region) | region.c | fork/execv/socketpair |
| Suspend to shell (C-z) | spawn.c | SIGTSTP job control |
| Gzip transparent open | fileio.c | popen("gunzip -c") |

## Transformations Applied

<!-- Filled by code-transformer agent -->

## Shim Functions Exercised

<!-- Filled after transform -->

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68000+ |
| CFLAGS | `-O2 -noixemul -m68000 -Wall -std=gnu99 -DWITHOUT_CURSES -DREGEX` |
| Libraries | `-lamiport` (posix-shim), `-lamiport-emu` (regex) |
| Binary size | TBD |

## Test Results

<!-- Filled after testing -->

## Platform Compatibility Notes

<!-- Filled after build/test -->

## Known Limitations

- No shell integration (M-x shell-command, M-!, C-z suspend) — AmigaOS has no fork()
- No dired (directory editor) — requires fork/exec for shell commands
- No cscope/grep/compile mode — requires popen()
- No transparent .gz file opening — requires popen("gunzip")
- Terminal size fixed at startup (no SIGWINCH resize detection)
- Tilde expansion (~user) only works for current user via ENV:HOME
- File permissions (chmod/chown) are no-ops

## Review

<!-- Filled after review -->
