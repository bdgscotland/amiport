---
name: vim_analysis
description: Vim 9.1.0 portability analysis for AmigaOS 3.x — HARD verdict, existing Amiga OS layer must be adapted, C99 pervasive, clib2→libnix, 87 files, key blockers documented
type: project
---

# Vim 9.1.0 AmigaOS 3.x Portability Analysis

**Why:** Vim already has built-in Amiga support (os_amiga.c + os_amiga.h) targeting OS4/AROS/MorphOS. The existing Make_ami.mak uses -mcrt=clib2 for OS4. We need to adapt for OS3.x with bebbo-gcc -noixemul (libnix).

**Verdict: HARD** — Not INFEASIBLE. The Amiga OS layer already exists and is largely correct for OS3, but these blockers must be resolved:

## Key Blockers

1. **C99 pervasive** — 29,488 `//` comment lines, 130+ `for (int i = 0;...)` for-loop declarations spread across build-list files. Require `-std=gnu99` flag. NOT a manual conversion problem.

2. **clib2 vs libnix** — Make_ami.mak uses `-mcrt=clib2`. libnix is missing: `fchown()`, `fchmod()`, `fsync()` (used in bufwrite.c, fileio.c, viminfo.c), `ftruncate()` (bufwrite.c). These will need stubs or `-DHAVE_FSYNC` removed.

3. **open()/read()/write()/lseek() needed for swap file** — memfile.c uses POSIX fd-level I/O via mch_open (maps to open()), lseek (vim_lseek), read/write. These are provided by amiport_open/read/write/lseek shims but mch_open_rw uses mode_t (0600) which doesn't translate. Need `#define mch_open_rw(n,f) mch_open((n),(f),0)` path (already in macros.h for non-UNIX non-MSWIN).

4. **`long long` in crypt.c** — FEAT_SODIUM uses `unsigned long long` extensively. Must disable FEAT_SODIUM (`-DNO_SODIUM` or just never define `HAVE_SODIUM`). FEAT_CRYPT (blowfish/zip) is self-contained, no `long long`.

5. **`for (int i=0;...)` in term.c, eval.c, buffer.c etc** — -std=gnu99 required. 130 instances.

6. **`__attribute__((used))` on stackcookie** — OS3 path doesn't set a stack cookie (correct: leave stack to OS). The existing code already `#ifdef __amigaos4__`-gates this. We add `unsigned long __stack = 1048576;` for OS3.

7. **`gethostname()` in os_amiga.c** — used in `mch_get_host_name()`. Need `amiport_gethostname()` shim.

8. **`getpwuid()/getuid()` in os_amiga.c line 676** — already `#if defined(__amigaos4__)` guarded, falls through to return FAIL on OS3. No action needed.

9. **`fchown()` in bufwrite.c and viminfo.c** — needs to be stubbed as no-op when `!UNIX`. Guard with `#ifdef HAVE_FCHOWN`.

10. **`ssize_t` in main.c line 3723** — inside `#ifdef PROC_EXE_LINK` block which won't be defined on Amiga. No action needed.

11. **if_cscope.c fork/exec** — FEAT_CSCOPE only defined on UNIX. Already gated by `#if defined(FEAT_CSCOPE)`. Add `-DNO_ARP -DFEAT_NORMAL` (not HUGE) to avoid FEAT_CSCOPE being enabled. Or just don't define it.

12. **iconv in mbyte.c** — guarded by `USE_ICONV` which is only set on UNIX. No action needed on Amiga build.

13. **`__progname`** — vim doesn't use __progname; no conflict with amiport __progname.

14. **`ACTION_SCREEN_MODE` vs `SetMode()`** — OS3 path uses `dos_packet(MP(raw_in), ACTION_SCREEN_MODE, -1L)` which is correct for OS3. MorphOS uses SetMode(). Our build is neither, so OS3 path is used.

15. **`mch_settmode` on OS3** — uses dos_packet with ACTION_SCREEN_MODE. This requires raw_in to be a file handle, not a libnix FILE*. The vim Amiga layer opens raw_in/raw_out via AmigaDOS Open()/Input()/Output() directly. NO conflict with console-shim — vim bypasses it entirely.

## Terminal Handling: No conflict with console-shim

os_amiga.c does its own direct console I/O:
- `raw_in` / `raw_out` are AmigaDOS BPTRs from Input()/Output()/Open()
- All reads/writes use AmigaDOS `Read()`/`Write()` directly
- RAW/COOK mode via `dos_packet(ACTION_SCREEN_MODE)` on OS3
- Window size via `dos_packet(ACTION_DISK_INFO)` + ConUnit struct

**DO NOT link console-shim. vim handles its own terminal.**

## Feature Set Recommendation: FEAT_NORMAL (not HUGE)

FEAT_HUGE enables on UNIX only: FEAT_CSCOPE (fork/exec), FEAT_CLIENTSERVER (X11/named pipes), FEAT_TERMINAL (pty.c), FEAT_JOB_CHANNEL (channel.c with sockets).

FEAT_NORMAL gives: folding, syntax, eval, visual, viminfo, digraphs, wildmenu, diff, crypt (blowfish), spell, etc. — fully usable vim.

## Build Flags for OS3

```
-DAMIGA -DFEAT_NORMAL -DNO_ARP -DUSE_TMPNAM -DHAVE_STDARG_H
-DHAVE_TGETENT -DHAVE_TERMCAP -DNEW_SHELLSIZE
-std=gnu99 -noixemul -m68020
```

Do NOT define: HAVE_FSYNC, HAVE_FCHOWN, HAVE_SODIUM, USE_ICONV, FEAT_CSCOPE, FEAT_HUGE

## POSIX shim requirements

- `open()`/`read()`/`write()`/`close()`/`lseek()` — amiport shims (for memfile.c swap file)
- `stat()`/`access()` — amiport shims (bufwrite.c, fileio.c, misc2.c)
- `lstat()` — stub to stat() (no symlinks on OFS/FFS) — macros.h fallback already exists
- `gethostname()` — needs amiport shim (os_amiga.c mch_get_host_name)
- `getenv()`/`setenv()` — NOT needed; os_amiga.c has mch_getenv/mch_setenv using GetVar/SetVar directly
- `mkdir()`/`rmdir()` — amiport shims
- `rename()`/`remove()` — libnix has these
- `dirent.h`/`opendir()`/`readdir()` — amiport shims (for file browsing)
- `iconv` — NOT needed, USE_ICONV not set

## Stack

Set `unsigned long __stack = 1048576;` (1MB) in os_amiga.c for OS3 path. The existing `#elif defined(__AROS__) || defined(__MORPHOS__)` block does this; add OS3 as default (remove #elif).

## C99 Status

All `//` comments and `for (int i=0;...)` patterns require `-std=gnu99`. Do NOT attempt manual conversion — 130 for-loop instances + 29,000 comment lines.

**How to apply:** CLAUDE.md and port Makefile use `-std=gnu99` per ADR-022.
