/*
 * amigaos_compat.h -- amiport force-include for libgit2 on AmigaOS
 *
 * This header is injected into every libgit2 translation unit via the
 * Makefile's `-include amigaos_compat.h` flag. It activates the amiport
 * posix-shim for functions that bebbo-gcc's libnix does not provide.
 *
 * The shim uses #define-based aliasing (e.g. `#define pread amiport_pread`)
 * so libgit2 source code keeps its POSIX call signatures unchanged -- the
 * preprocessor retargets them to amiport_* entry points at compile time.
 *
 * Functions retargeted via this header (from the amiport shim):
 *   amiport/stdio_ext.h  -> pread, pwrite (seek+read+seek emulation)
 *   amiport/unistd.h     -> realpath, readlink, ftruncate, symlink
 *   amiport/sys/stat.h   -> lstat (aliased to stat; no symlinks on FFS)
 *
 * libnix provides natively:
 *   open, close, read, write, lseek, fopen, fclose, fdopen, fileno,
 *   stat, fstat, access, mkdir, rmdir, chdir, getpid, gettimeofday
 *
 * Missing from both libnix AND amiport shim (patched in libgit2 source):
 *   getppid, getpgid, getsid, getentropy  -- rand.c has __AMIGA__ branch
 *   mmap, munmap                          -- NO_MMAP activates posix.c fallback
 *   getpwuid_r                            -- see note below (likely dead code)
 *   futimes, utimes                       -- see note below (GIT_USE_FUTIMENS off)
 *
 * `getpwuid_r` is called only from sysdir.c when `getuid() != geteuid()`.
 * On AmigaOS both always return 0, so the call site is dead code at
 * runtime but still needs to link. If the first build surfaces it as an
 * undefined symbol, extend the shim (/extend-shim skill) or add an
 * ifdef guard to sysdir.c.
 *
 * `futimes`/`utimes` are only reached via the `!GIT_USE_FUTIMENS` path
 * in posix.c. With `GIT_USE_FUTIMENS` unset, `p_futimes` may still be
 * referenced. Handle at first-build time, not speculatively.
 */

#ifndef AMIPORT_LIBGIT2_COMPAT_H
#define AMIPORT_LIBGIT2_COMPAT_H

#ifdef __AMIGA__

/* Pull the POSIX surface the shim provides. These headers install the
 * #define macros that retarget pread/pwrite/realpath/ftruncate/readlink/
 * lstat/symlink to their amiport_* equivalents. Include order matters:
 * stdio_ext.h first (for pread/pwrite), then unistd.h (for realpath and
 * friends), then sys/stat.h (for lstat). */
#include <amiport/stdio_ext.h>
#include <amiport/unistd.h>
#include <amiport/sys/stat.h>

#endif /* __AMIGA__ */

#endif /* AMIPORT_LIBGIT2_COMPAT_H */
