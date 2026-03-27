---
name: vim_transform_decisions
description: Vim 9.1 OS3 transform decisions -- what changed and what was deliberately left alone
type: project
---

Vim 9.1 has built-in Amiga support (os_amiga.c, os_amiga.h). The transform is surgical.

**Why:** Vim already has correct mch_* implementations for AmigaOS. The goal was minimal,
upstreamable changes -- not replacing OS-layer calls with amiport shims.

**How to apply:** Future transforms on vim ported/ should follow the same minimal approach.
Never add amiport/ headers or replace mch_* functions.

## Changes Applied (2026-03-26)

### os_amiga.c
1. Stack cookie: added `#elif defined(__GNUC__) && defined(AMIGA)` branch with
   `unsigned long __stack = 1048576;` for OS3/bebbo-gcc. The existing comment
   says "leave the stack alone on OS3" but that was written for AROS/OS4 era --
   bebbo-gcc DOES support __stack and Vim needs 1MB for syntax highlighting.

2. mch_get_host_name: replaced the two-branch `#if !defined(__AROS__)` with
   a three-branch structure: OS4/MorphOS call gethostname(), AROS uses "Amiga",
   everything else (OS3/libnix) falls to "amiga". libnix does NOT have gethostname.

### os_amiga.h
3. Added fchown/fchmod/ftruncate stub macros guarded by the OS3 predicate
   `defined(__GNUC__) && defined(AMIGA) && !defined(__amigaos4__) && ...`
   These return 0 (no-op). All call sites in bufwrite.c/viminfo.c/undo.c/fileio.c
   are already inside HAVE_FCHOWN/HAVE_FCHMOD/HAVE_FTRUNCATE guards which are
   NOT defined for the Amiga build -- so the stubs are a belt-and-suspenders measure.

## What Was NOT Changed

- No amiport/ headers added (vim uses AmigaDOS I/O directly)
- No mch_* function bodies modified (all are correct for OS3)
- No exit() replacements (vim has its own exit handling)
- No console-shim linkage (vim does its own terminal I/O)
- C++ comments (//) left intact -- Makefile uses -std=gnu99 which allows them
- Existing $VER string in os_amiga.c left as-is -- it already covers __GNUC__ builds
  (adding a second verstag would cause a linker duplicate-symbol warning)

## Build notes

- Makefile uses -std=gnu99 -m68020, NOT -ansi. C++ comments are fine.
- LDFLAGS links -lamiport for any POSIX I/O paths that go through libnix wrappers.
- fchown/ftruncate are never actually called -- HAVE_* macros are not defined.
