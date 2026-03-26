/*	$OpenBSD: ln.c,v 1.25 2019/06/28 13:34:59 deraadt Exp $	*/
/*	$NetBSD: ln.c,v 1.10 1995/03/21 09:06:10 cgd Exp $	*/

/*
 * Copyright (c) 1987, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* amiport: Amiga version string */
static const char *verstag = "$VER: ln 1.25 (26.03.2026)";

/* amiport: stack cookie -- Amiga default 4KB is too small */
long __stack = 16384;

/* amiport: pledge/unveil not available on AmigaOS -- stubbed */
#define pledge(p, e) (0)
#define unveil(p, f) (0)

/* amiport: __dead is OpenBSD noreturn annotation -- map to GCC attribute */
#define __dead __attribute__((noreturn))

#include <sys/types.h>
#include <errno.h>
/* amiport: do NOT include <fcntl.h> here -- it transitively includes
 * sys/stat.h which declares int stat(), conflicting with the
 * amiport/sys/stat.h typedef. AT_SYMLINK_FOLLOW defined below instead. */
#include <libgen.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

/* amiport: replaced <sys/stat.h> with amiport shim */
#include <amiport/sys/stat.h>
/* amiport: replaced <err.h> with amiport shim providing err/errx/warn/warnx/warnc */
#include <amiport/err.h>
/* amiport: replaced <stdlib.h> with amiport shim (activates exit -> amiport_exit) */
#include <amiport/stdlib.h>
/* amiport: replaced <unistd.h> with amiport shim (symlink, unlink, etc.) */
#include <amiport/unistd.h>
/* amiport: getopt/optind via amiport shim (libnix getopt_long is broken) */
#include <amiport/getopt.h>

/* amiport: argv wildcard expansion support */
#include <amiport/glob.h>

/* amiport: AmigaOS dos.library for MakeLink() (hard link via Lock) */
#include <proto/dos.h>

/*
 * amiport-emu: amiport_hard_link() -- create a hard link via AmigaDOS MakeLink()
 *
 * AmigaOS MakeLink(name, lock, TRUE) creates a hard link. The second argument
 * for hard links must be a Lock on the target file (not a string path).
 * AT_SYMLINK_FOLLOW is a no-op on AmigaOS because lstat() aliases to stat()
 * and soft links are transparent to Lock().
 *
 * Caveat: Hard links (LINK_HARD) require FFS with international mode or SFS.
 * On plain OFS/FFS they may fail with IoErr() = ERROR_NOT_IMPLEMENTED.
 * Both Pflag=0 (follow symlink) and Pflag=1 (link to symlink) behave
 * identically on AmigaOS since there is no lstat vs stat distinction at
 * the Lock level.
 */
static int
amiport_hard_link(const char *target, const char *linkname)
{
    BPTR lock;
    LONG ok;

    /* Obtain a shared lock on the target file */
    lock = Lock((CONST_STRPTR)target, SHARED_LOCK);
    if (!lock) {
        errno = ENOENT;
        return -1;
    }

    /* MakeLink(name, lock, TRUE) -- TRUE = hard link */
    ok = MakeLink((CONST_STRPTR)linkname, (LONG)lock, TRUE);
    UnLock(lock);

    if (!ok) {
        /* amiport-emu: map IoErr() to errno for hard link failure */
        errno = EACCES; /* generic; IoErr() may be ERROR_NOT_IMPLEMENTED */
        return -1;
    }
    return 0;
}

/* amiport: AT_SYMLINK_FOLLOW -- not needed at runtime (linkat replaced with
 * amiport_hard_link), but defined for source compatibility. On AmigaOS,
 * lstat == stat, so following symlinks is the only behavior anyway. */
#ifndef AT_SYMLINK_FOLLOW
#define AT_SYMLINK_FOLLOW 0x400
#endif

int	fflag;				/* Unlink existing files. */
int	hflag;				/* Check new name for symlink first. */
int	Pflag;				/* Hard link to symlink. */
int	sflag;				/* Symbolic, not hard, link. */

int	linkit(char *, char *, int);
void	usage(void) __dead;

/* amiport: atexit cleanup -- frees expanded argv on all exit paths */
static void
cleanup(void)
{
    amiport_free_argv();
    (void)fflush(stdout);
}

int
main(int argc, char *argv[])
{
    struct stat sb;
    int ch, exitval;
    char *sourcedir;

    /* amiport: expand wildcard args -- Amiga shell doesn't glob */
    amiport_expand_argv(&argc, &argv);
    /* amiport: register cleanup for all exit paths including err()/errx() */
    atexit(cleanup);

    /* amiport: pledge stubbed as macro -- always returns 0 */
    if (pledge("stdio rpath cpath", NULL) == -1)
        err(10, "pledge"); /* amiport: RETURN_ERROR */

    /* amiport: replaced getopt() with amiport_getopt() via macro in <amiport/unistd.h>;
     * libnix getopt() is available for short options, but using amiport header for
     * consistency -- short options only, no long-option variant needed here */
    while ((ch = getopt(argc, argv, "fhLnPs")) != -1)
        switch (ch) {
        case 'f':
            fflag = 1;
            break;
        case 'h':
        case 'n':
            hflag = 1;
            break;
        case 'L':
            Pflag = 0;
            break;
        case 'P':
            Pflag = 1;
            break;
        case 's':
            sflag = 1;
            break;
        default:
            usage();
        }

    argv += optind;
    argc -= optind;

    switch(argc) {
    case 0:
        usage();
    case 1:				/* ln target */
        exit(linkit(argv[0], ".", 1));
    case 2:				/* ln target source */
        exit(linkit(argv[0], argv[1], 0));
    }
                    /* ln target1 target2 directory */
    sourcedir = argv[argc - 1];
    if (stat(sourcedir, &sb)) /* nosemgrep: cpp.lang.security.filesystem.path-manipulation.path-manipulation */
        err(10, "%s", sourcedir); /* amiport: RETURN_ERROR */
    if (!S_ISDIR(sb.st_mode))
        usage();
    for (exitval = 0; *argv != sourcedir; ++argv)
        exitval |= linkit(*argv, sourcedir, 1);
    exit(exitval);
}

 /*
  * Nomenclature warning!
  *
  * In this source "target" and "source" are used the opposite way they
  * are used in the ln(1) manual.  Here "target" is the existing file and
  * "source" specifies the to-be-created link to "target".
  */
int
linkit(char *target, char *source, int isdir)
{
    struct stat sb;
    char *p, path[PATH_MAX];
    int exists, n;

    if (!sflag) {
        /* If target doesn't exist, quit now. */
        /* amiport: lstat == stat on AmigaOS (no real symlinks in FFS/SFS);
         * Pflag (hard-link-to-symlink) is a no-op. Use amiport_stat directly. */
        if (amiport_stat(target, &sb)) {
            warn("%s", target);
            return (1);
        }
        /* Only symbolic links to directories. */
        if (S_ISDIR(sb.st_mode)) {
            /* amiport: warnc() provided by <amiport/err.h> */
            warnc(EISDIR, "%s", target);
            return (1);
        }
    }

    /* amiport: hflag (stat-vs-lstat for source) is a no-op since lstat == stat */

    /* If the source is a directory, append the target's name. */
    if (isdir || (!amiport_stat(source, &sb) && S_ISDIR(sb.st_mode))) {
        /* amiport: basename() from libnix <libgen.h> -- works correctly on AmigaOS */
        if ((p = basename(target)) == NULL) {
            warn("%s", target);
            return (1);
        }
        n = snprintf(path, sizeof(path), "%s/%s", source, p); /* nosemgrep: path-traversal -- ln intentionally constructs paths from user args; bounded by PATH_MAX */
        if (n < 0 || (size_t)n >= sizeof(path)) {
            warnc(ENAMETOOLONG, "%s/%s", source, p);
            return (1);
        }
        source = path;
    }

    /* amiport: lstat aliases to amiport_stat -- no symlink distinction on AmigaOS */
    exists = (amiport_stat(source, &sb) == 0);
    /*
     * If doing hard links and the source (destination) exists and it
     * actually is the same file like the target (existing file), we
     * complain that the files are identical.  If -f is specified, we
     * accept the job as already done and return with success.
     */
    if (exists && !sflag) {
        struct stat tsb;

        /* amiport: lstat == stat on AmigaOS -- use amiport_stat directly */
        if (amiport_stat(target, &tsb)) {
            warn("%s: disappeared", target);
            return (1);
        }

        /* amiport: st_dev and st_ino populated by amiport_stat() via Lock()+Examine() */
        if (tsb.st_dev == sb.st_dev && tsb.st_ino == sb.st_ino) {
            if (fflag)
                return (0);
            else {
                warnx("%s and %s are identical (nothing done).",
                    target, source);
                return (1);
            }
        }
    }
    /*
     * If the file exists, and -f was specified, unlink it.
     * Attempt the link.
     */
    /* amiport: unlink() -> amiport_unlink() (declared in <amiport/unistd.h>)
     * amiport: symlink() -> amiport_symlink() via macro in <amiport/unistd.h>
     * amiport-emu: linkat() replaced with amiport_hard_link() -- AmigaOS MakeLink()
     *   with Lock; AT_SYMLINK_FOLLOW is a no-op (lstat==stat on AmigaOS).
     *   Hard links require FFS with international mode or SFS. */
    if ((fflag && amiport_unlink(source) == -1 && errno != ENOENT) ||
        (sflag ? symlink(target, source) :
        amiport_hard_link(target, source))) {
        warn("%s", source);
        return (1);
    }

    return (0);
}

void
usage(void)
{
    extern char *__progname;

    (void)fprintf(stderr,
        "usage: %s [-fhLnPs] source [target]\n"
        "       %s [-fLPs] source ... [directory]\n",
        __progname, __progname);
    exit(10); /* amiport: RETURN_ERROR */
}
