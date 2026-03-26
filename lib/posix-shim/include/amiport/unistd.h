/*
 * amiport/unistd.h — POSIX unistd.h shim for AmigaOS
 *
 * Provides file I/O wrappers (open, close, read, write, lseek)
 * and miscellaneous POSIX functions mapped to AmigaOS equivalents.
 */

#ifndef AMIPORT_UNISTD_H
#define AMIPORT_UNISTD_H

#include <exec/types.h>

/* Standard file descriptor numbers */
#ifndef STDIN_FILENO
#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
#endif

/* POSIX types not always available on AmigaOS.
 * Guard each with the same macros used by bebbo-gcc libnix sys/types.h
 * so there is no conflict when both headers are included. */
#ifndef _OFF_T_DECLARED
typedef long off_t;
#define _OFF_T_DECLARED
#endif

#ifndef _SSIZE_T_DECLARED
typedef long ssize_t;
#define _SSIZE_T_DECLARED
#endif

#ifndef __u_char_defined
typedef unsigned char u_char;
#define __u_char_defined
#endif

#ifndef __u_int_defined
typedef unsigned int u_int;
#define __u_int_defined
#endif

/* File access mode flags (matching POSIX values) */
#ifndef O_RDONLY
#define O_RDONLY    0x0000
#define O_WRONLY    0x0001
#define O_RDWR      0x0002
#define O_CREAT     0x0040
#define O_TRUNC     0x0200
#define O_APPEND    0x0400
#endif

/* Seek whence values */
#ifndef SEEK_SET
#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2
#endif

/* File descriptor operations */
int  amiport_open(const char *path, int flags);
int  amiport_close(int fd);
LONG amiport_read(int fd, void *buf, LONG count);
LONG amiport_write(int fd, const void *buf, LONG count);
LONG amiport_lseek(int fd, LONG offset, int whence);

/* File descriptor duplication */
int  amiport_dup(int oldfd);
int  amiport_dup2(int oldfd, int newfd);

/* File operations */
int  amiport_unlink(const char *path);
int  amiport_rename(const char *oldpath, const char *newpath);
int  amiport_access(const char *path, int mode);

/*
 * amiport: chmod() — no-op stub; Amiga protection bits have inverted
 * semantics and most ports just need the call to succeed silently.
 */
int  amiport_chmod(const char *path, unsigned int mode);

/*
 * amiport: realpath() — resolve canonical path via Lock()+NameFromLock().
 * If resolved is NULL, malloc's a 256-byte buffer; caller must free().
 */
char *amiport_realpath(const char *path, char *resolved);

/* Environment — setenv/unsetenv via AmigaDOS SetVar/DeleteVar (ENV:) */
int amiport_setenv(const char *name, const char *value, int overwrite);
int amiport_unsetenv(const char *name);

/* Process-like operations */
unsigned int amiport_sleep(unsigned int seconds);
char *amiport_getcwd(char *buf, int size);
int   amiport_chdir(const char *path);

/* Environment — returns malloc'd string, caller must free() */
char *amiport_getenv(const char *name);

/* Misc */
LONG  amiport_getpid(void);

/* access() mode flags */
#ifndef F_OK
#define F_OK    0   /* Test for existence */
#define R_OK    4   /* Test for read permission */
#define W_OK    2   /* Test for write permission */
#define X_OK    1   /* Test for execute permission */
#endif

/* isatty equivalent */
int amiport_isatty(int fd);

/*
 * readlink — read the target of an AmigaDOS soft link (OS 2.0+)
 *
 * Returns the number of bytes written to buf (not NUL-terminated per POSIX),
 * or -1 on error.  errno = EINVAL if path is not a soft link.
 * Available only on dos.library 36+ (AmigaOS 2.0+).
 */
#include <stddef.h>
LONG amiport_readlink(const char *path, char *buf, size_t bufsiz);

/*
 * ftruncate — truncate or extend an open file to exactly length bytes
 *
 * Uses AmigaDOS SetFileSize() (dos.library 36+).  Returns 0 on success or -1.
 * Caveat: extension does NOT guarantee zero-fill of new bytes on AmigaOS.
 */
int amiport_ftruncate(int fd, LONG length);

/* Convenience macros */
#ifndef AMIPORT_NO_READLINK_MACROS
#define readlink(p, b, n)   amiport_readlink(p, b, n)
#endif

#ifndef AMIPORT_NO_FTRUNCATE_MACROS
#define ftruncate(fd, len)  amiport_ftruncate(fd, len)
#endif

/* setlocale -- Tier 1 shim, returns "C" locale.
 * AmigaOS has locale.library (OpenLocale/GetLocaleStr/StrnCmp) but
 * libnix's C runtime is hardcoded to C locale behavior. Returning "C"
 * is correct: it prevents multibyte code paths from activating while
 * the C library can't support them. See ADCD: locale-library-openlocale.
 * Returns NULL for unsupported locale names (POSIX-correct behavior). */
char *amiport_setlocale(int category, const char *locale);
#ifndef AMIPORT_NO_LOCALE_MACROS
#ifndef LC_ALL
#define LC_ALL      0
#define LC_COLLATE  1
#define LC_CTYPE    2
#define LC_MONETARY 3
#define LC_NUMERIC  4
#define LC_TIME     5
#define LC_MESSAGES 6
#endif
#define setlocale   amiport_setlocale
#endif

/* localeconv -- return "C" locale numeric formatting info */
struct amiport_lconv {
    char *decimal_point;
    char *thousands_sep;
    char *grouping;
    char *int_curr_symbol;
    char *currency_symbol;
    char *mon_decimal_point;
    char *mon_thousands_sep;
    char *mon_grouping;
    char *positive_sign;
    char *negative_sign;
};

struct amiport_lconv *amiport_localeconv(void);

#ifndef AMIPORT_NO_LOCALE_MACROS
#define lconv          amiport_lconv
#define localeconv()   amiport_localeconv()
#endif

/* timegm -- convert struct tm (UTC) to time_t (pure C) */
#include <time.h>
long amiport_timegm(struct tm *tm);
#ifndef AMIPORT_NO_LOCALE_MACROS
#define timegm(t)   amiport_timegm(t)
#endif

/*
 * symlink -- create a soft link via AmigaDOS MakeLink()
 * Returns 0 on success, -1 on error.
 */
int amiport_symlink(const char *target, const char *linkpath);

/* fchmod/fchown/lchown -- no-op stubs (single-user, no real perms) */
int amiport_fchmod(int fd, unsigned int mode);
int amiport_fchown(int fd, int owner, int group);
int amiport_lchown(const char *path, int owner, int group);

/* Forward-declare amiport_timespec if signal.h not yet included */
#ifndef AMIPORT_SIGNAL_H
struct amiport_timespec {
    long tv_sec;
    long tv_nsec;
};
#endif

/* utimensat -- set file timestamps via SetFileDate() (dos.library V36+)
 *
 * amiport: Only modification time is stored (AmigaOS has one timestamp).
 * dirfd is ignored (AT_FDCWD assumed). UTIME_NOW and UTIME_OMIT supported.
 * Precision limited to 1/50s ticks (20ms).
 * See ADCD: dos-library-setfiledate-2 */
#define AMIPORT_UTIME_NOW   ((long)((1L << 30) - 1L))
#define AMIPORT_UTIME_OMIT  ((long)((1L << 30) - 2L))
#define AMIPORT_AT_FDCWD     -100

int amiport_utimensat(int dirfd, const char *path,
                      const struct amiport_timespec times[2], int flags);
int amiport_futimens(int fd, const struct amiport_timespec times[2]);

/*
 * ioctl -- I/O control (TIOCGWINSZ only)
 *
 * amiport: Queries console window dimensions using CSI Window Status
 * Request escape sequence. Falls back to 80x24 for non-interactive fds.
 * Only TIOCGWINSZ is supported; all other requests return -1/ENOTTY.
 */
#define AMIPORT_TIOCGWINSZ  0x5413

struct amiport_winsize {
    unsigned short ws_row;      /* rows in characters */
    unsigned short ws_col;      /* columns in characters */
    unsigned short ws_xpixel;   /* horizontal size in pixels (0) */
    unsigned short ws_ypixel;   /* vertical size in pixels (0) */
};

int amiport_ioctl(int fd, unsigned long request, void *arg);

#ifndef AMIPORT_NO_FILEOPS_MACROS
#define symlink(t, l)     amiport_symlink(t, l)
#define fchmod(f, m)      amiport_fchmod(f, m)
#define fchown(f, o, g)   amiport_fchown(f, o, g)
#define lchown(p, o, g)   amiport_lchown(p, o, g)
#define UTIME_NOW         AMIPORT_UTIME_NOW
#define UTIME_OMIT        AMIPORT_UTIME_OMIT
#define AT_FDCWD          AMIPORT_AT_FDCWD
#define utimensat(d, p, t, f) amiport_utimensat(d, p, t, f)
#define futimens(f, t)    amiport_futimens(f, t)
#define TIOCGWINSZ        AMIPORT_TIOCGWINSZ
#define winsize           amiport_winsize
#endif

/* Thread-safe strtok (not provided by all Amiga C runtimes) */
char *amiport_strtok_r(char *str, const char *delim, char **saveptr);

/* Create a temporary file using T: assign.
 * Returns FILE* opened for read/write, or NULL on failure. */
#include <stdio.h>
FILE *amiport_tmpfile(void);

/* Convenience macros — drop-in POSIX compatibility */
#ifndef AMIPORT_NO_OPEN_MACROS
#define open(p, f)      amiport_open(p, f)
#define close(fd)       amiport_close(fd)
#define read(fd, b, n)  amiport_read(fd, b, n)
#define write(fd, b, n) amiport_write(fd, b, n)
#define lseek(fd, o, w) amiport_lseek(fd, o, w)
#endif

#ifndef AMIPORT_NO_CHMOD_MACROS
#define chmod(p, m)     amiport_chmod(p, m)
#endif

#ifndef AMIPORT_NO_REALPATH_MACROS
#define realpath(p, r)  amiport_realpath(p, r)
#endif

#ifndef AMIPORT_NO_SETENV_MACROS
#define setenv(n, v, o)  amiport_setenv(n, v, o)
#define unsetenv(n)      amiport_unsetenv(n)
#endif

/*
 * AMIPORT_STRICT — Tier 3 function guards
 *
 * When AMIPORT_STRICT is defined, calls to known Tier 3 functions
 * produce link-time errors with readable symbol names instead of
 * cryptic "undefined reference" messages.
 *
 * Example error: undefined reference to `AMIPORT_TIER3_fork_requires_redesign'
 */
#ifdef AMIPORT_STRICT

/* Process model — no equivalent on AmigaOS */
extern int AMIPORT_TIER3_fork_requires_redesign(void);
extern int AMIPORT_TIER3_execve_requires_redesign(void);
extern int AMIPORT_TIER3_execvp_requires_redesign(void);
extern int AMIPORT_TIER3_execlp_requires_redesign(void);
extern int AMIPORT_TIER3_waitpid_requires_redesign(void);
extern int AMIPORT_TIER3_wait_requires_redesign(void);

#define fork()         AMIPORT_TIER3_fork_requires_redesign()
#define execve(...)    AMIPORT_TIER3_execve_requires_redesign()
#define execvp(...)    AMIPORT_TIER3_execvp_requires_redesign()
#define execlp(...)    AMIPORT_TIER3_execlp_requires_redesign()
#define waitpid(...)   AMIPORT_TIER3_waitpid_requires_redesign()
#define wait(...)      AMIPORT_TIER3_wait_requires_redesign()

/* Threads — no equivalent on classic AmigaOS */
extern int AMIPORT_TIER3_pthread_create_requires_redesign(void);
extern int AMIPORT_TIER3_pthread_join_requires_redesign(void);

#define pthread_create(...)  AMIPORT_TIER3_pthread_create_requires_redesign()
#define pthread_join(...)    AMIPORT_TIER3_pthread_join_requires_redesign()

/* Signals — kill() still requires redesign; sigaction is now shimmed */
extern int AMIPORT_TIER3_kill_requires_redesign(void);

#define kill(...)       AMIPORT_TIER3_kill_requires_redesign()

#endif /* AMIPORT_STRICT */

#endif /* AMIPORT_UNISTD_H */
