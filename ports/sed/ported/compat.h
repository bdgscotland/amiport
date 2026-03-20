/*
 * compat.h — AmigaOS compatibility shims for OpenBSD sed
 *
 * amiport: Centralizes all POSIX/BSD/OpenBSD compatibility definitions
 * needed by the ported sed source files.
 */

#ifndef SED_COMPAT_H
#define SED_COMPAT_H

/* amiport: system headers that libnix provides */
#include <unistd.h>            /* write, close, unlink from libnix */

/* amiport: POSIX shim headers */
#include <amiport/string.h>   /* strlcpy, strlcat, reallocarray */
#include <amiport/err.h>      /* err, errx, warn, warnx, warnc, strtonum */
#include <amiport/stdio_ext.h> /* mkstemp */
#include <amiport/unistd.h>   /* u_char, u_int, ssize_t, file ops */
#include <amiport/getopt.h>   /* getopt, optarg, optind */

/* amiport: Tier 2 regex emulation */
#include <amiport-emu/regex.h>

/* amiport: stub pledge/unveil (OpenBSD security, no-op on Amiga) */
#define pledge(p, e) (0)
#define unveil(p, f) (0)

/* amiport: __dead is OpenBSD's __attribute__((noreturn)) */
#ifndef __dead
#define __dead __attribute__((noreturn))
#endif

/* amiport: regoff_t not defined in our regex emulation */
#ifndef _REGOFF_T_DECLARED
typedef int regoff_t;
#define _REGOFF_T_DECLARED
#endif

/* amiport: _POSIX2_LINE_MAX — standard POSIX constant */
#ifndef _POSIX2_LINE_MAX
#define _POSIX2_LINE_MAX 2048
#endif

/* amiport: DEFFILEMODE and ALLPERMS — BSD file permission macros.
 * AmigaOS has no UNIX permission model; define for compilation only. */
#ifndef DEFFILEMODE
#define DEFFILEMODE 0666
#endif
#ifndef ALLPERMS
#define ALLPERMS 0777
#endif

/* amiport: PATH_MAX — may not be defined on AmigaOS */
#ifndef PATH_MAX
#define PATH_MAX 256
#endif

/* amiport: REG_STARTEND — non-standard regex extension.
 * Our Tier 2 regex doesn't support it directly; we emulate by
 * creating a NUL-terminated substring copy before calling regexec. */
#ifndef REG_STARTEND
#define REG_STARTEND 0x1000
#endif

/* amiport: getline() — GNU extension, not in libnix.
 * Simple implementation using fgets() + realloc(). */
static ssize_t
amiport_getline(char **lineptr, size_t *n, FILE *stream)
{
    char *buf;
    size_t size, len;
    int c;

    if (lineptr == NULL || n == NULL || stream == NULL) {
        return -1;
    }

    if (*lineptr == NULL || *n == 0) {
        *n = 128;
        *lineptr = malloc(*n);
        if (*lineptr == NULL)
            return -1;
    }

    buf = *lineptr;
    size = *n;
    len = 0;

    while ((c = fgetc(stream)) != EOF) {
        if (len + 2 > size) {
            char *newbuf;
            size *= 2;
            /* perf: save old pointer — realloc failure must not leak it */
            newbuf = realloc(buf, size);
            if (newbuf == NULL)
                return -1;
            buf = newbuf;
            *lineptr = buf;
            *n = size;
        }
        buf[len++] = (char)c;
        if (c == '\n')
            break;
    }

    if (len == 0)
        return -1;

    buf[len] = '\0';
    return (ssize_t)len;
}

#ifndef AMIPORT_NO_GETLINE_MACRO
#define getline amiport_getline
#endif

/* amiport: fdopen() — not in libnix.
 * Stub that returns NULL; in-place editing (-i) will fail gracefully.
 * sed's -i mode needs fdopen(fd, "w") after mkstemp(). */
#ifndef fdopen
static FILE *
amiport_fdopen(int fd, const char *mode)
{
    (void)fd;
    (void)mode;
    /* amiport: fdopen not available on AmigaOS; in-place editing limited */
    return NULL;
}
#define fdopen amiport_fdopen
#endif

/* amiport: fchown/fchmod — no UNIX permissions on AmigaOS, no-op */
#define fchown(fd, uid, gid) (0)
#define fchmod(fd, mode)     (0)

/* amiport: dirname() — extract directory part of path.
 * Simple implementation; modifies the input string. */
static char *
amiport_dirname(char *path)
{
    static char dot[] = ".";
    char *p;

    if (path == NULL || *path == '\0')
        return dot;

    /* Find last separator (handle both / and : for AmigaOS) */
    p = path + strlen(path) - 1;

    /* Trim trailing slashes */
    while (p > path && *p == '/')
        *p-- = '\0';

    /* Find the last slash */
    p = strrchr(path, '/');
    if (p == NULL) {
        /* Check for Amiga volume/assign separator */
        p = strrchr(path, ':');
        if (p != NULL) {
            *(p + 1) = '\0';
            return path;
        }
        return dot;
    }

    if (p == path)
        *(p + 1) = '\0';
    else
        *p = '\0';

    return path;
}

#define dirname amiport_dirname

/* amiport: errc() — like err() but with explicit error code.
 * OpenBSD extension, not available on Amiga. */
static void
amiport_errc(int eval, int code, const char *fmt, ...)
{
    va_list ap;
    if (fmt != NULL) {
        va_start(ap, fmt);
        (void)vfprintf(stderr, fmt, ap);
        va_end(ap);
        (void)fprintf(stderr, ": ");
    }
    (void)fprintf(stderr, "%s\n", strerror(code));
    exit(eval);
}
#define errc amiport_errc

#endif /* SED_COMPAT_H */
