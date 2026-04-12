/*	$OpenBSD: rm.c,v 1.45 2025/04/20 13:47:54 kn Exp $	*/
/*	$NetBSD: rm.c,v 1.19 1995/09/07 06:48:50 jtc Exp $	*/

/*-
 * Copyright (c) 1990, 1993, 1994
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
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* amiport: AmigaOS 3.x port of OpenBSD rm 1.45 */

/* amiport: Amiga boilerplate -- stack cookie and version string */
long __stack = 16384;
static const char *verstag = "$VER: rm 1.45 (11.04.2026)";

#include <sys/types.h>
/* amiport: include <fcntl.h> BEFORE <amiport/sys/stat.h> because fcntl.h
   pulls in sys/stat.h via _default_fcntl.h. Including it first lets sys/stat.h
   parse 'struct stat' before the #define stat amiport_stat macro is in effect,
   preventing "redefinition of struct amiport_stat" errors. */
#include <fcntl.h>
/* amiport: replaced <sys/stat.h> with <amiport/sys/stat.h> */
#include <amiport/sys/stat.h>
/* amiport: removed <sys/mount.h> -- fstatfs() not available on AmigaOS,
   secure overwrite (-P) is disabled below */

/* amiport: replaced <err.h> with <amiport/err.h> */
#include <amiport/err.h>
#include <errno.h>
/* amiport: replaced <fts.h> with <amiport/fts.h> */
#include <amiport/fts.h>
#include <stdio.h>
/* amiport: replaced <stdlib.h> with <amiport/stdlib.h> (provides exit -> amiport_exit) */
#include <amiport/stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
/* amiport: replaced <pwd.h> with <amiport/pwd.h> (provides user_from_uid macro) */
#include <amiport/pwd.h>
/* amiport: replaced <grp.h> with <amiport/grp.h> (provides group_from_gid macro) */
#include <amiport/grp.h>
/* amiport: for amiport_expand_argv / amiport_free_argv */
#include <amiport/glob.h>

/* amiport: pledge() is a no-op on AmigaOS */
#define pledge(p, e) (0)

#define MAXIMUM(a, b)	(((a) > (b)) ? (a) : (b))

extern char *__progname;

/*
 * amiport: local strmode() implementation, renamed rm_strmode() to avoid
 * conflict with libnix's string.h declaration strmode(int, char *).
 * libnix declares strmode but does not provide the body; our signature
 * uses mode_t which differs, so we use a private name.
 * Fills buf with 11 mode chars + NUL (e.g. "-rwxr-xr-x ").
 */
static void
rm_strmode(mode_t mode, char *p)
{
    /* file type */
    if (S_ISREG(mode))       p[0] = '-';
    else if (S_ISDIR(mode))  p[0] = 'd';
    else if (S_ISLNK(mode))  p[0] = 'l';
    else if (S_ISFIFO(mode)) p[0] = 'p';
    else                     p[0] = '?';

    /* owner */
    p[1] = (mode & S_IRUSR) ? 'r' : '-';
    p[2] = (mode & S_IWUSR) ? 'w' : '-';
    p[3] = (mode & S_IXUSR) ? 'x' : '-';
    /* group */
    p[4] = (mode & S_IRGRP) ? 'r' : '-';
    p[5] = (mode & S_IWGRP) ? 'w' : '-';
    p[6] = (mode & S_IXGRP) ? 'x' : '-';
    /* other */
    p[7] = (mode & S_IROTH) ? 'r' : '-';
    p[8] = (mode & S_IWOTH) ? 'w' : '-';
    p[9] = (mode & S_IXOTH) ? 'x' : '-';
    /* trailing space and NUL (OpenBSD strmode produces 11 chars + NUL) */
    p[10] = ' ';
    p[11] = '\0';
}

int dflag, eval, fflag, iflag, Pflag, vflag, stdin_ok;

int	check(char *, char *, struct stat *);
void	checkdot(char **);
void	rm_file(char **);
/* amiport: rm_overwrite and pass are disabled on AmigaOS (no fstatfs, no
   fsync on Amiga filesystems in the same POSIX sense). The -P flag is
   accepted but silently ignored -- the file is still removed. */
int	rm_overwrite(char *, struct stat *);
void	rm_tree(char **);
void	usage(void);

/* amiport: cleanup for atexit -- free expanded argv */
static void
cleanup(void)
{
    fflush(stdout);
    amiport_free_argv();
}

/*
 * rm --
 *	This rm is different from historic rm's, but is expected to match
 *	POSIX 1003.2 behavior.  The most visible difference is that -f
 *	has two specific effects now, ignore non-existent files and force
 * 	file removal.
 */
int
main(int argc, char *argv[])
{
    int ch, rflag;

    /* amiport: expand AmigaDOS wildcards in argv */
    amiport_expand_argv(&argc, &argv);
    /* amiport: register cleanup for all exit paths (err/errx/exit) */
    atexit(cleanup);

    Pflag = rflag = 0;
    /* amiport: replaced getopt() with amiport_getopt() via <amiport/getopt.h>
       (libnix getopt_long is broken -- returns '?' for all options) */
    while ((ch = getopt(argc, argv, "dfiPRrv")) != -1)
        switch(ch) {
        case 'd':
            dflag = 1;
            break;
        case 'f':
            fflag = 1;
            iflag = 0;
            break;
        case 'i':
            fflag = 0;
            iflag = 1;
            break;
        case 'P':
            /* amiport: -P (secure overwrite) is accepted but not performed --
               AmigaOS filesystems have no fsync/fstatfs equivalent. The file
               is still unlinked. */
            Pflag = 1;
            break;
        case 'R':
        case 'r':			/* Compatibility. */
            rflag = 1;
            break;
        case 'v':
            vflag = 1;
            break;
        default:
            usage();
        }
    argc -= optind;
    argv += optind;

    /* amiport: pledge() is a no-op -- macro defined above */
    if (Pflag) {
        if (pledge("stdio rpath wpath cpath getpw", NULL) == -1)
            err(10, "pledge");
    } else {
        if (pledge("stdio rpath cpath getpw", NULL) == -1)
            err(10, "pledge");
    }

    if (argc < 1 && fflag == 0)
        usage();

    checkdot(argv);

    if (*argv) {
        stdin_ok = isatty(STDIN_FILENO);

        if (rflag)
            rm_tree(argv);
        else
            rm_file(argv);
    }

    return (eval);
}

void
rm_tree(char **argv)
{
    FTS *fts;
    FTSENT *p;
    int needstat;
    int flags;

    /*
     * Remove a file hierarchy.  If forcing removal (-f), or interactive
     * (-i) or can't ask anyway (stdin_ok), don't stat the file.
     */
    needstat = !fflag && !iflag && stdin_ok;

    /*
     * If the -i option is specified, the user can skip on the pre-order
     * visit.  The fts_number field flags skipped directories.
     */
#define	SKIPPED	1

    flags = FTS_PHYSICAL;
    if (!needstat)
        flags |= FTS_NOSTAT;
    if (!(fts = fts_open(argv, flags, NULL)))
        /* amiport: exit(1) -> err(10,...) for AmigaOS error convention */
        err(10, NULL);
    while ((p = fts_read(fts)) != NULL) {
        switch (p->fts_info) {
        case FTS_DNR:
            if (!fflag || p->fts_errno != ENOENT) {
                warnc(p->fts_errno, "%s", p->fts_path);
                /* amiport: eval=1 stays -- non-zero but not Amiga error code;
                   eval is returned from main() which is the process RC */
                eval = 1;
            }
            continue;
        case FTS_ERR:
            /* amiport: errc(1,...) -> errc(10,...) for AmigaOS error convention */
            errc(10, p->fts_errno, "%s", p->fts_path);
            /* NOTREACHED */
        case FTS_NS:
            /*
             * FTS_NS: assume that if can't stat the file, it
             * can't be unlinked.
             */
            if (!needstat)
                break;
            if (!fflag || p->fts_errno != ENOENT) {
                warnc(p->fts_errno, "%s", p->fts_path);
                eval = 1;
            }
            continue;
        case FTS_D:
            /* Pre-order: give user chance to skip. */
            if (!fflag && !check(p->fts_path, p->fts_accpath,
                p->fts_statp)) {
                (void)fts_set(fts, p, FTS_SKIP);
                p->fts_number = SKIPPED;
            }
            continue;
        case FTS_DP:
            /* Post-order: see if user skipped. */
            if (p->fts_number == SKIPPED)
                continue;
            break;
        default:
            if (!fflag &&
                !check(p->fts_path, p->fts_accpath, p->fts_statp))
                continue;
        }

        /*
         * If we can't read or search the directory, may still be
         * able to remove it.  Don't print out the un{read,search}able
         * message unless the remove fails.
         */
        switch (p->fts_info) {
        case FTS_DP:
        case FTS_DNR:
            if (!rmdir(p->fts_accpath)) {
                if (vflag)
                    fprintf(stdout, "%s\n", p->fts_path);
                continue;
            }
            if (fflag && errno == ENOENT)
                continue;
            break;

        case FTS_F:
        case FTS_NSOK:
            /* amiport: -P secure overwrite disabled on AmigaOS (no fstatfs/fsync);
               call rm_overwrite stub which does nothing */
            if (Pflag)
                rm_overwrite(p->fts_accpath, p->fts_info ==
                    FTS_NSOK ? NULL : p->fts_statp);
            /* FALLTHROUGH */
        default:
            if (!unlink(p->fts_accpath)) {
                if (vflag)
                    fprintf(stdout, "%s\n", p->fts_path);
                continue;
            }
            if (fflag && errno == ENOENT)
                continue;
        }
        warn("%s", p->fts_path);
        eval = 1;
    }
    if (errno)
        /* amiport: err(1,...) -> err(10,...) for AmigaOS error convention */
        err(10, "fts_read");
    fts_close(fts);
}

void
rm_file(char **argv)
{
    struct stat sb;
    int rval;
    char *f;

    /*
     * Remove a file.  POSIX 1003.2 states that, by default, attempting
     * to remove a directory is an error, so must always stat the file.
     */
    while ((f = *argv++) != NULL) {
        /* Assume if can't stat the file, can't unlink it. */
        if (lstat(f, &sb)) {
            if (!fflag || errno != ENOENT) {
                warn("%s", f);
                eval = 1;
            }
            continue;
        }

        if (S_ISDIR(sb.st_mode) && !dflag) {
            warnx("%s: is a directory", f);
            eval = 1;
            continue;
        }
        if (!fflag && !check(f, f, &sb))
            continue;
        else if (S_ISDIR(sb.st_mode))
            rval = rmdir(f);
        else {
            /* amiport: -P secure overwrite stub -- just skips overwrite */
            if (Pflag)
                rm_overwrite(f, &sb);
            rval = unlink(f);
        }
        if (rval && (!fflag || errno != ENOENT)) {
            warn("%s", f);
            eval = 1;
        } else if (rval == 0 && vflag)
            (void)fprintf(stdout, "%s\n", f);
    }
}

/*
 * rm_overwrite --
 *	amiport: DISABLED on AmigaOS. The original implementation requires
 *	fstatfs() (from <sys/mount.h>) and fsync() which are not available
 *	on AmigaOS 3.x. The -P flag is accepted but the overwrite pass is
 *	skipped. The file will still be unlinked by the caller.
 *	Returns 1 (success) unconditionally so callers proceed to unlink().
 */
int
rm_overwrite(char *file, struct stat *sbp)
{
    /* amiport: secure overwrite not implemented on AmigaOS (no fstatfs/fsync).
       File will still be removed by unlink() in the caller. */
    (void)file;
    (void)sbp;
    return (1);
}

int
check(char *path, char *name, struct stat *sp)
{
    int ch, first;
    char modep[15];

    /* Check -i first. */
    if (iflag)
        (void)fprintf(stderr, "remove %s? ", path);
    else {
        /*
         * If it's not a symbolic link and it's unwritable and we're
         * talking to a terminal, ask.  Symbolic links are excluded
         * because their permissions are meaningless.  Check stdin_ok
         * first because we may not have stat'ed the file.
         */
        if (!stdin_ok || S_ISLNK(sp->st_mode) || !access(name, W_OK) ||
            errno != EACCES)
            return (1);
        /* amiport: rm_strmode() is our local implementation (renamed to avoid
           conflict with libnix string.h's strmode(int, char *) declaration) */
        rm_strmode(sp->st_mode, modep);
        /* amiport: user_from_uid/group_from_gid provided by <amiport/pwd.h>
           and <amiport/grp.h> macros */
        (void)fprintf(stderr, "override %s%s%s/%s for %s? ",
            modep + 1, modep[9] == ' ' ? "" : " ",
            user_from_uid(sp->st_uid, 0),
            group_from_gid(sp->st_gid, 0), path);
    }
    (void)fflush(stderr);

    first = ch = getchar();
    while (ch != '\n' && ch != EOF)
        ch = getchar();
    return (first == 'y' || first == 'Y');
}

/*
 * POSIX.2 requires that if "." or ".." are specified as the basename
 * portion of an operand, a diagnostic message be written to standard
 * error and nothing more be done with such operands.
 *
 * Since POSIX.2 defines basename as the final portion of a path after
 * trailing slashes have been removed, we'll remove them here.
 */
#define ISDOT(a) ((a)[0] == '.' && (!(a)[1] || ((a)[1] == '.' && !(a)[2])))
void
checkdot(char **argv)
{
    char *p, **save, **t;
    int complained;
    struct stat sb;
#ifndef __AMIGA__
    struct stat root;
#endif

    complained = 0;

#ifndef __AMIGA__
    /* amiport: AmigaOS has no POSIX root "/" -- skip root inode check.
       Set root.st_ino to an impossible value to prevent accidental match. */
    stat("/", &root);
#endif

    for (t = argv; *t;) {
#ifdef __AMIGA__
        /* amiport: AmigaOS has no single filesystem root to guard against.
           Skip the root inode comparison entirely. */
        if (0) {
#else
        if (lstat(*t, &sb) == 0 &&
            root.st_ino == sb.st_ino && root.st_dev == sb.st_dev) {
#endif
            if (!complained++)
                warnx("\"/\" may not be removed");
            goto skip;
        }
        /* strip trailing slashes */
        p = strrchr(*t, '\0');
        while (--p > *t && *p == '/')
            *p = '\0';

        /* extract basename */
        if ((p = strrchr(*t, '/')) != NULL)
            ++p;
        else
            p = *t;

        if (ISDOT(p)) {
            if (!complained++)
                warnx("\".\" and \"..\" may not be removed");
skip:
            /* amiport: eval=1 here is used as a boolean flag; main() returns
               it as the process RC. On AmigaOS, the shell's IF WARN tests RC>=5
               so any non-zero RC triggers the warning condition. */
            eval = 1;
            for (save = t; (t[0] = t[1]) != NULL; ++t)
                continue;
            t = save;
        } else
            ++t;
    }
}

void
usage(void)
{
    (void)fprintf(stderr, "usage: %s [-dfiPRrv] file ...\n", __progname);
    /* amiport: exit(1) -> exit(10) for AmigaOS error convention */
    exit(10);
}
