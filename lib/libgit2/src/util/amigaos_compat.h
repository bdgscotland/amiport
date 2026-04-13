/*
 * amigaos_compat.h -- amiport force-include for libgit2 on AmigaOS
 *
 * This header is injected into every libgit2 translation unit via the
 * Makefile's `-include src/util/amigaos_compat.h` flag.
 *
 * Strategy: forward-declare only the amiport functions libgit2 needs that
 * libnix does NOT provide, then define only the target macros. We do NOT
 * include the full amiport/ headers because they redefine read/write/open/
 * stat/fstat/fchown etc. which conflict with libnix's own declarations.
 * libgit2 uses libnix native I/O (open/read/write/stat) -- these must NOT
 * be remapped to amiport_* equivalents.
 *
 * Functions retargeted via this header:
 *   pread, pwrite   -> amiport_pread, amiport_pwrite (seek+read+seek)
 *   realpath        -> amiport_realpath
 *   readlink        -> amiport_readlink
 *   ftruncate       -> amiport_ftruncate
 *   lstat           -> lstat (libnix provides stat/lstat natively)
 *
 * libnix provides natively (DO NOT REMAP):
 *   open, close, read, write, lseek, fopen, fclose, fdopen, fileno,
 *   stat, fstat, lstat, access, mkdir, rmdir, chdir, getpid, gettimeofday
 *
 * Intentionally excluded (not needed or dead code on AmigaOS):
 *   getppid, getpgid, getsid, getentropy -- rand.c has __AMIGA__ branch
 *   mmap, munmap                         -- NO_MMAP activates posix.c fallback
 *   getpwuid_r                           -- added to shim; called from sysdir.c
 *   futimes, utimes                      -- added to shim; called from posix.c
 *
 * amiport: PDR-010 Phase 2, Stage 3 -- surgical macro-only approach.
 * Do not replace with full amiport/unistd.h include without auditing
 * which macros it installs -- it redefines open/read/write which breaks
 * the libnix native fd namespace that libgit2 depends on.
 */

#ifndef AMIPORT_LIBGIT2_COMPAT_H
#define AMIPORT_LIBGIT2_COMPAT_H

#ifdef __AMIGA__

/* Pull system headers first so their declarations are in place before we
 * define any override macros. This prevents the "conflicting types" error
 * that occurs when our #define pread(...) amiport_pread(...) is active
 * while libnix's <unistd.h> later declares `ssize_t pread(...)` -- that
 * system declaration would preprocess into `ssize_t amiport_pread(...)`,
 * conflicting with our own `LONG amiport_pread(...)` declaration. */
#include <unistd.h>   /* pread, pwrite, readlink, ftruncate declarations */
#include <sys/time.h> /* struct timeval for utimes/futimes */
#include <stddef.h>   /* size_t */

/* pread/pwrite -- positional I/O via seek+read/write+seek.
 * libnix declares but does NOT implement pread/pwrite. We undef the
 * libnix declarations and provide our own amiport_* implementation.
 * Signature matches libnix's ssize_t declaration to avoid conflicts. */
#ifdef pread
#undef pread
#endif
#ifdef pwrite
#undef pwrite
#endif
ssize_t amiport_pread(int fd, void *buf, size_t count, off_t offset);
ssize_t amiport_pwrite(int fd, const void *buf, size_t count, off_t offset);
#define pread(fd, buf, count, offset)  amiport_pread(fd, buf, count, offset)
#define pwrite(fd, buf, count, offset) amiport_pwrite(fd, buf, count, offset)

/* realpath -- canonical path resolution via Lock()+NameFromLock().
 * libnix may not provide this; we override to be safe. */
#ifdef realpath
#undef realpath
#endif
char *amiport_realpath(const char *path, char *resolved);
#define realpath(path, resolved)       amiport_realpath(path, resolved)

/* readlink -- libnix provides this natively; no override needed.
 * symlink -- libnix does not provide; add our stub. */
int amiport_symlink(const char *target, const char *linkpath);
#ifndef symlink
#define symlink(t, l)                  amiport_symlink(t, l)
#endif

/* getpwuid_r -- stub (AmigaOS has no user database) */
struct passwd;
int amiport_getpwuid_r(unsigned int uid, struct passwd *pwd,
                       char *buf, size_t buflen, struct passwd **result);
#ifndef getpwuid_r
#define getpwuid_r(u, p, b, l, r)     amiport_getpwuid_r(u, p, b, l, r)
#endif

/* utimes/futimes -- set file timestamps (added to shim 2026-04) */
int amiport_utimes(const char *path, const struct timeval times[2]);
int amiport_futimes(int fd, const struct timeval times[2]);
#ifndef utimes
#define utimes(p, t)                   amiport_utimes(p, t)
#endif
#ifndef futimes
#define futimes(fd, t)                 amiport_futimes(fd, t)
#endif

#endif /* __AMIGA__ */

#endif /* AMIPORT_LIBGIT2_COMPAT_H */
