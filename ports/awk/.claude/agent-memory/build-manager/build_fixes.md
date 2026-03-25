---
name: awk build fixes
description: All compile/link fixes applied to get awk 2024.12.25 building for AmigaOS
type: project
---

Build succeeded on 2026-03-25 after 4 fix iterations. Binary: ports/awk/awk (172KB, AmigaOS loadseg executable).

## Fix 1: SA_SIGINFO present but sa_sigaction missing

The AmigaOS toolchain defines SA_SIGINFO (0x4) in signal.h but does NOT provide sa_sigaction
or siginfo_t in the sigaction struct. Added `#undef SA_SIGINFO` under `#ifdef __AMIGA__` in
main.c before the fpecatch function. This collapses the SA_SIGINFO branch entirely on Amiga,
using plain signal(SIGFPE, fpecatch) instead.

**Why:** bebbo toolchain sys-include has SA_SIGINFO flag constant but not the extended sigaction fields.

## Fix 2: libnix concat clashes with awk concat

libnix string.h declares `char *concat(const char *, ...)`. awk defines `Node *concat(Node *)`.
Incompatible types cause a hard error in every TU that includes both.
Fix: rename awk's concat to awk_concat in proto.h (definition + #define alias) and b.c (definition + recursive calls).
The #define alias in proto.h covers all call sites automatically.

## Fix 3: amiport/sys/stat.h typedef conflict after fcntl.h

run.c includes <fcntl.h> which transitively pulls in sys/stat.h, declaring `int stat()`.
Then <amiport/sys/stat.h> tries `typedef struct amiport_stat stat` -- conflicts with the function decl.
Fix: define AMIPORT_NO_STAT_MACROS before including amiport/sys/stat.h, then manually
`#define stat amiport_stat` and `#undef/#define S_ISDIR/S_ISREG` to AMIPORT_S_ISDIR/ISREG.

## Fix 4: amiport-emu/popen.h macros clashing with proto.h extern declarations

popen.h defines `#define popen(cmd, mode) amiport_emu_popen((cmd), (mode))`.
proto.h has `extern FILE *popen(const char *, const char *)` -- macro expands inside the declaration.
Fix: guard the extern declarations with `#ifndef popen` / `#ifndef pclose`.

## Fix 5: towupper/towlower static stubs conflicting with wctype.h

run.c had an `#elif defined(__AMIGA__)` block with static towupper/towlower stubs.
The AmigaOS toolchain wctype.h already provides these as non-static extern functions.
Static redeclaration after extern = hard error. Fix: remove the __AMIGA__ stub block entirely.

## Fix 6: FOPEN_MAX not in libnix stdio.h

libnix stdio.h does not define FOPEN_MAX. The sys-include stdio.h has it (via __FOPEN_MAX__)
but libnix does not pull that in. Fix: `#ifndef FOPEN_MAX / #define FOPEN_MAX 20 / #endif`
in run.c after the stat includes. 20 is the ANSI C minimum.

## Fix 7: environ undefined at link time

libnix does not provide a global `environ` variable. awk needs it for envinit().
Fix: define a NULL-terminated empty array and assign to `char **environ` under `#ifdef __AMIGA__`
in main.c. ENVIRON AWK built-in works correctly (empty) on AmigaOS.

## Fix 8: Missing -I and -L for posix-emu

The port Makefile had -lamiport-emu in LDFLAGS but no -L path for it, and no -I for its headers.
common.mk only covers posix-shim. Fix: add `-I../../lib/posix-emu/include` to CFLAGS and
`-L../../lib/posix-emu` to LDFLAGS in ports/awk/Makefile.

**Why:** Any port using posix-emu (popen, pipe, mmap, select, etc.) must add both paths explicitly.
