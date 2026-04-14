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

/* amiport: getline() now provided by amiport/stdio_ext.h (promoted from inline) */
#include <amiport/stdio_ext.h>

/* amiport: sed -i needs a FILE* writing to a unique tmpfile. The obvious
 * mkstemp() + fdopen() combo does not work on AmigaOS: amiport_mkstemp()
 * opens the tmpfile with an EXCLUSIVE write lock (MODE_NEWFILE), and then
 * fdopen()'s internal libnix fopen(name, "w") is blocked by that lock
 * (the AmigaOS "Exclusive Write Lock" pitfall -- crash-patterns #19).
 * Solution: skip the intermediate fd and do a single direct libnix fopen()
 * on a manually-generated unique name. Called from the __AMIGA__ branch
 * in main.c's mf_getline().
 *
 * Template must end with "XXXXXX". The XXXXXX is replaced in place with
 * a unique suffix derived from the task address + counter. Returns a
 * libnix FILE* open for writing, or NULL with errno set. */
#ifdef __AMIGA__
#include <proto/exec.h>    /* FindTask */

static int sed_tmpname_counter = 0;

static FILE *
sed_tmpname_fopen(char *tmpl)
{
    int len;
    char suffix[7];
    struct Task *t;
    FILE *fp;

    if (!tmpl) {
        errno = EINVAL;
        return NULL;
    }
    len = (int)strlen(tmpl);
    if (len < 6 || strcmp(tmpl + len - 6, "XXXXXX") != 0) {
        errno = EINVAL;
        return NULL;
    }
    t = FindTask(NULL);
    sprintf(suffix, "%02lx%04x",
            (unsigned long)((unsigned long)t & 0xFF),
            (unsigned int)(sed_tmpname_counter++ & 0xFFFF));
    memcpy(tmpl + len - 6, suffix, 6);
    fp = fopen(tmpl, "w");
    if (!fp) {
        /* restore template on failure so caller can report original path */
        memcpy(tmpl + len - 6, "XXXXXX", 6);
        return NULL;
    }
    return fp;
}
#endif /* __AMIGA__ */

/* amiport: open() with 3 args (O_CREAT mode) — the shim macro only accepts
 * 2 args.  AmigaOS has no permission model so the mode argument is always
 * discarded.  Undefine the shim macro and replace with a 3-arg inline that
 * forwards to amiport_open(), dropping the mode silently.
 * All open() calls in sed use O_CREAT and supply a mode, so 3-arg form only. */
#ifdef open
#undef open
#endif
static int
amiport_open3(const char *path, int flags, int mode)
{
    (void)mode; /* amiport: no permission model on AmigaOS */
    return amiport_open(path, flags);
}
#define open(p, f, m)  amiport_open3(p, f, m)

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

/* amiport: errc() is provided by amiport/err.h (included above via amiport/unistd.h).
 * The local static definition was removed in REVISION 2 to resolve the duplicate
 * definition error introduced when the shim added amiport_errc() as a real function. */
#ifndef errc
#define errc amiport_errc
#endif

#endif /* SED_COMPAT_H */
