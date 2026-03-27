---
name: vim 9.1 initial build fixes
description: All fixes required to get vim 9.1 to compile and link for AmigaOS
type: project
---

## Build Fixes Applied (2026-03-26)

### Makefile Changes
- Removed `-DHAVE_STAT_H` — caused vim.h to include non-existent `<stat.h>`;
  sys/stat.h already included via other ifdef paths. Safe to omit.
- Added `-DWORDS_BIGENDIAN` — 68k is big-endian; needed by blowfish.c
- Added `-DHAVE_ERRNO_H` — enables errno.h inclusion via vim.h
- Removed redundant `-DHAVE_*` flags already in os_amiga.h (HAVE_STDLIB_H,
  HAVE_STRING_H, HAVE_FCNTL_H, etc.)
- Updated vamos test target to note vim requires FS-UAE (68020-only binary)

### ported/blowfish.c
- Added `|| defined(WORDS_BIGENDIAN) || defined(AMIGA)` to the endian guard.
  Without HAVE_CONFIG_H, blowfish.c raised a compile-time #error. The condition
  now accepts any of the three valid "endianness was determined" signals.

### ported/os_amiga.c
- Removed `void Delay(long);` local prototype inside mch_delay(). This clashed
  with the NDK's inline/exec.h declaration of Delay(ULONG). The function is
  available via proto/dos.h already included.

### ported/regexp_bt.c and regexp_nfa.c
- Copied from original/ — these are #include'd sub-files by regexp.c and must
  be present in ported/ as passthrough copies.

### ported/xdiff/xmacros.h
- Added unconditional `#ifndef SIZE_MAX / #define SIZE_MAX / #endif` — the
  original only defined it for hpux/VMS. AmigaOS libnix has no stdint.h.

### ported/vim_stubs.c (new file)
- Provides no-op stubs for 8 symbols not available on AmigaOS:
  - im_get_status() / im_set_active() — X11 Input Method (not on Amiga)
  - set_ref_in_im_funcs() — GC ref tracking for IM closures
  - did_set_imactivatefunc() / did_set_imstatusfunc() — option callbacks
  - mch_rmdir() — uses AmigaDOS DeleteFile() for empty dirs
  - getpwuid() / getgrgid() — POSIX user DB (declared in NDK but not in libnix)
  - getuid() — returns 0 (single-user)

## Build Result
- Binary: ports/vim/vim, 2.6MB AmigaOS loadseg executable
- Compiled with: -O2 -m68020 -std=gnu99 -DFEAT_NORMAL
- Cannot test with vamos (68020 instructions); requires FS-UAE
