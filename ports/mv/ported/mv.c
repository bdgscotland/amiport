/* nosemgrep: path-traversal */
/* mv is a CLI file-manipulation utility. All filesystem calls on user-supplied
 * paths are intentional -- CWE-22 findings in this file are false positives. */
/*	$OpenBSD: mv.c,v 1.47 2021/10/24 21:24:21 deraadt Exp $	*/
/*	$NetBSD: mv.c,v 1.9 1995/03/21 09:06:52 cgd Exp $	*/

/*
 * Copyright (c) 1989, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ken Smith of The State University of New York at Buffalo.
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

/* amiport: AmigaOS 3.x port of OpenBSD mv v1.47 */

/* amiport: AmigaOS version string */
static const char *verstag = "$VER: mv 1.47 (11.04.2026)";

/* amiport: stack cookie -- directory walking possible, use 16KB */
long __stack = 16384;

/* amiport: replaced <sys/time.h> with <amiport/sys/time.h> */
#include <amiport/sys/time.h>
/* amiport: removed <sys/wait.h> -- no fork/wait on AmigaOS */
/* amiport: removed <sys/mount.h> -- no mount points on AmigaOS */

/* amiport: replaced <sys/stat.h> with <amiport/sys/stat.h>
 * provides lstat -> amiport_lstat (aliased to amiport_stat, no symlinks),
 * fchmod -> amiport_fchmod, fchown -> amiport_fchown,
 * futimens -> amiport_futimens */
#include <amiport/sys/stat.h>

/* amiport: replaced <err.h> with <amiport/err.h>
 * provides err(), errx(), warn(), warnx(), warnc(), errc().
 * bare <err.h> does not exist in bebbo-gcc libnix. */
#include <amiport/err.h>

#include <errno.h>

/* amiport: replaced <fcntl.h> with <amiport/unistd.h> which provides
 * O_RDONLY, O_WRONLY, O_CREAT, O_TRUNC flags and access(), fchmod(), fchown(),
 * futimens(), ftruncate() shims.
 * NOTE: open/close/read/write use libnix native versions (NOT amiport_open)
 * so that the standard fd namespace (stdin=0, stdout=1, stderr=2) is
 * consistent and fdopen/fileno interop works (crash-patterns #12).
 * AMIPORT_NO_OPEN_MACROS prevents the amiport_open macros from overriding
 * libnix's native open/close/read/write. */
#define AMIPORT_NO_OPEN_MACROS
#include <amiport/unistd.h>

#include <stdio.h>

/* amiport: replaced <stdlib.h> with <amiport/stdlib.h>
 * activates exit() -> amiport_exit() macro */
#include <amiport/stdlib.h>

#include <string.h>
#include <limits.h>

/* amiport: replaced <pwd.h> with <amiport/pwd.h>
 * provides user_from_uid() -> amiport_user_from_uid() (returns "root") */
#include <amiport/pwd.h>

/* amiport: replaced <grp.h> with <amiport/grp.h>
 * provides group_from_gid() -> amiport_group_from_gid() (returns "wheel") */
#include <amiport/grp.h>

/* amiport: <amiport/getopt.h> replaces <getopt.h> -- libnix getopt_long
 * returns '?' for all options (crash-patterns #17). Short getopt is fine. */
#include <amiport/getopt.h>

/* amiport: <amiport/dirent.h> provides rmdir() -> amiport_rmdir() */
#include <amiport/dirent.h>

/* amiport: <amiport/glob.h> provides amiport_expand_argv / amiport_free_argv
 * for AmigaDOS wildcard expansion (shell does not glob for us) */
#include <amiport/glob.h>

/* amiport: proto/dos.h for IsInteractive(Input()) used instead of
 * isatty(STDIN_FILENO). amiport_isatty() only knows about amiport fd table,
 * not libnix standard descriptors. See known-pitfalls. */
#include <proto/dos.h>

/* amiport: PATH_MAX -- define as 256 if libnix does not provide it */
#ifndef PATH_MAX
#define PATH_MAX 256
#endif

/* amiport: pledge() -- no equivalent on AmigaOS; stub as no-op macro */
#define pledge(promises, execpromises) (0)

/* amiport: S_ISUID / S_ISGID -- mode bit constants not defined by
 * amiport/sys/stat.h (which only has the type/dir/reg bits we need).
 * Values match POSIX / BSD convention. */
#ifndef S_ISUID
#define S_ISUID 04000
#endif
#ifndef S_ISGID
#define S_ISGID 02000
#endif

/* amiport: forward declarations for libnix native open/close/read/write/unlink.
 * These are provided by libnix (sys-include/sys/unistd.h and
 * sys-include/sys/fcntl.h) but including those system headers after the
 * amiport shim headers causes type-redefinition conflicts.  Since
 * AMIPORT_NO_OPEN_MACROS is defined, these symbols are NOT macro-redirected
 * to amiport_*; the declarations here satisfy the compiler. */
extern int    open(const char *path, int flags, ...);
extern int    close(int fd);
extern long   read(int fd, void *buf, unsigned long nbyte);
extern long   write(int fd, const void *buf, unsigned long nbyte);
extern int    unlink(const char *path);

/*
 * amiport: strmode() -- libnix declares strmode(int, char *) in <string.h>
 * but does NOT provide it in libc.a (undefined reference at link time).
 * Provide a local implementation.  AmigaOS has no Unix permission model so
 * the output is a fixed plausible string; buf must be >= 12 bytes.
 */
static void
mv_strmode(int mode, char *p)
{
    (void)mode;
    p[0]  = '-';
    p[1]  = 'r';
    p[2]  = 'w';
    p[3]  = '-';
    p[4]  = 'r';
    p[5]  = '-';
    p[6]  = '-';
    p[7]  = 'r';
    p[8]  = '-';
    p[9]  = '-';
    p[10] = ' ';
    p[11] = '\0';
}
#define strmode(m, p) mv_strmode((int)(m), (p))

/* amiport: fchflags() -- no equivalent on AmigaOS.
 * AmigaOS has protection bits but no BSD file flags (UF_NODUMP etc.).
 * Stub as a macro that returns 0 (success) so the conditional is inert. */
#define fchflags(fd, flags) (0)

/* amiport: __progname -- DO NOT define here.
 * argv_expand.o in libamiport.a provides __progname as a strong symbol,
 * initialized from argv[0] by amiport_expand_argv(). */
extern char *__progname;

int fflg, iflg, vflg;
int stdin_ok;

/* amiport: removed extern cpmain() / rmmain() declarations.
 * These functions are not available on AmigaOS. mvcopy() below
 * returns an error message instead. */

int	mvcopy(char *, char *);
int	do_move(char *, char *);
int	fastcopy(char *, char *, struct amiport_stat *);
void	usage(void);

/* amiport: file-scope globals for fastcopy() buffer -- freed in cleanup() */
static unsigned int fastcopy_blen;
static char *fastcopy_bp;

/* amiport: atexit cleanup -- free argv expansion and fastcopy buffer on all exit paths
 * (err/errx/exit all call atexit handlers) */
static void
cleanup(void)
{
    free(fastcopy_bp); /* amiport: free static fastcopy buffer */
    amiport_free_argv();
    (void)fflush(stdout);
}

int
main(int argc, char *argv[])
{
    int baselen, len, rval;
    char *p, *endp;
    struct stat sb;
    int ch;
    char path[PATH_MAX];

    /* amiport: expand AmigaDOS wildcards in argv.
     * amiport_expand_argv initialises __progname from argv[0]. */
    amiport_expand_argv(&argc, &argv);
    atexit(cleanup);

    /* amiport: use IsInteractive(Input()) instead of isatty(STDIN_FILENO).
     * amiport_isatty() only covers the amiport internal fd table and will
     * return 0 for fd 0 (stdin) -- see known-pitfalls (amiport_isatty). */
    stdin_ok = IsInteractive(Input()) ? 1 : 0;

    while ((ch = getopt(argc, argv, "ifv")) != -1) {
        switch (ch) {
        case 'i':
            fflg = 0;
            iflg = 1;
            break;
        case 'f':
            iflg = 0;
            fflg = 1;
            break;
        case 'v':
            vflg = 1;
            break;
        default:
            usage();
        }
    }
    argc -= optind;
    argv += optind;

    if (argc < 2)
        usage();

    /*
     * If the stat on the target fails or the target isn't a directory,
     * try the move.  More than 2 arguments is an error in this case.
     *
     * Note: Semgrep CWE-22 path-traversal finding here is a false positive.
     * mv is a CLI file-manipulation tool -- user-supplied paths are the
     * intended input. The process has no more filesystem access than the
     * user invoking it.
     */
    if (stat(argv[argc - 1], &sb) || !S_ISDIR(sb.st_mode)) { /* nosemgrep: cpp.lang.security.filesystem.path-manipulation.path-manipulation */
        if (argc > 2)
            usage();
        exit(do_move(argv[0], argv[1]));
    }

    /* It's a directory, move each file into it. */
    if (strlcpy(path, argv[argc - 1], sizeof path) >= sizeof path) {
        /* amiport: exit code 10 = RETURN_ERROR (visible to IF ERROR in scripts) */
        errx(10, "%s: destination pathname too long", *argv);
    }
    baselen = strlen(path);
    endp = &path[baselen];
    if (*(endp - 1) != '/') {
        *endp++ = '/';
        ++baselen;
    }
    for (rval = 0; --argc; ++argv) {
        char *current_arg = *argv;

        /*
         * Get the name of the file to create from
         * the argument. This is a bit tricky because
         * in the case of b/ we actually want b and empty
         * string
         */
        /* amiport: also check ':' as path separator for AmigaDOS volume names
         * e.g., "T:file.txt" -> basename is "file.txt", not "T:file.txt" */
        p = strrchr(current_arg, '/');
        {
            char *colon = strrchr(current_arg, ':');
            if (colon != NULL && (p == NULL || colon > p))
                p = colon;
        }
        if (p == NULL)
            p = current_arg;
        else {
            /* Special case foo/ */
            if (!*(p+1)) {
                while (p >= current_arg && *p == '/')
                    p--;

                while (p >= current_arg && *p != '/')
                    p--;
            }

            p++;
        }

        if ((baselen + (len = strlen(p))) >= PATH_MAX) {
            warnx("%s: destination pathname too long", *argv);
            /* amiport: exit code 10 = RETURN_ERROR */
            rval = 10;
        } else {
            memmove(endp, p, len + 1);
            if (do_move(current_arg, path)) {
                /* amiport: exit code 10 = RETURN_ERROR */
                rval = 10;
            }
        }
    }
    exit(rval);
}

int
do_move(char *from, char *to)
{
    struct stat sb, fsb;
    char modep[15];

    /* Source path must exist (symlink is OK on POSIX; no symlinks on Amiga). */
    /* amiport: lstat() -> amiport_lstat() via macro in <amiport/sys/stat.h>
     * (aliased to amiport_stat; no symlinks on AmigaOS) */
    if (lstat(from, &fsb)) {
        warn("%s", from);
        /* amiport: return 10 = RETURN_ERROR */
        return (10);
    }

    /*
     * (1)	If the destination path exists, the -f option is not specified
     *	and either of the following conditions are true:
     *
     *	(a) The permissions of the destination path do not permit
     *	    writing and the standard input is a terminal.
     *	(b) The -i option is specified.
     *
     *	the mv utility shall write a prompt to standard error and
     *	read a line from standard input.  If the response is not
     *	affirmative, mv shall do nothing more with the current
     *	source file...
     */
    if (!fflg && !access(to, F_OK)) {
        int ask = 1;
        int first;
        int ch;

        if (iflg && !access(from, F_OK)) {
            (void)fprintf(stderr, "overwrite %s? ", to);
        } else if (stdin_ok && access(to, W_OK) && !stat(to, &sb)) {
            /* amiport: strmode() is provided by libnix <string.h>.
             * Cast st_mode (ULONG) to int to match libnix's signature. */
            strmode((int)sb.st_mode, modep);
            (void)fprintf(stderr, "override %s%s%s/%s for %s? ",
                modep + 1, modep[9] == ' ' ? "" : " ",
                /* amiport: user_from_uid() -> amiport_user_from_uid() via
                 * macro in <amiport/pwd.h>; returns "root" */
                user_from_uid(sb.st_uid, 0),
                /* amiport: group_from_gid() -> amiport_group_from_gid() via
                 * macro in <amiport/grp.h>; returns "wheel" */
                group_from_gid(sb.st_gid, 0), to);
        } else {
            ask = 0;
        }
        if (ask) {
            first = ch = getchar();
            while (ch != '\n' && ch != EOF)
                ch = getchar();
            if (first != 'y' && first != 'Y')
                return (0);
        }
    }

    /*
     * (2)	If rename() succeeds, mv shall do nothing more with the
     *	current source file.  If it fails for any other reason than
     *	EXDEV, mv shall write a diagnostic message to the standard
     *	error and do nothing more with the current source file.
     */
    if (!rename(from, to)) {
        if (vflg)
            (void)fprintf(stdout, "%s -> %s\n", from, to);
        return (0);
    }

    if (errno != EXDEV) {
        warn("rename %s to %s", from, to);
        /* amiport: return 10 = RETURN_ERROR */
        return (10);
    }

    /* amiport: removed statfs / realpath mount-point check.
     * sys/mount.h and statfs() are not available on AmigaOS.
     * AmigaOS has no Unix-style mount points; volumes are named devices
     * (e.g. "DH0:"). The EXDEV path falls through directly to the
     * cross-volume copy/delete logic below. */

    /*
     * (4)	If the destination path exists, mv shall attempt to remove it.
     *	If this fails for any reason, mv shall write a diagnostic
     *	message to the standard error and do nothing more with the
     *	current source file...
     */
    if (!lstat(to, &sb)) {
        /* amiport: rmdir() -> amiport_rmdir() via macro in <amiport/dirent.h>
         * amiport: unlink() -> amiport_unlink() via macro in <amiport/unistd.h>
         *   (AMIPORT_NO_OPEN_MACROS does not suppress the unlink macro) */
        if ((S_ISDIR(sb.st_mode)) ? rmdir(to) : unlink(to)) {
            warn("can't remove %s", to);
            /* amiport: return 10 = RETURN_ERROR */
            return (10);
        }
    }

    /*
     * (5)	The file hierarchy rooted in source_file shall be duplicated
     *	as a file hierarchy rooted in the destination path...
     */
    return (S_ISREG(fsb.st_mode) ?
        fastcopy(from, to, &fsb) : mvcopy(from, to));
}

int
fastcopy(char *from, char *to, struct stat *sbp)
{
    struct timespec ts[2];
    /* amiport: u_int32_t -> unsigned int (C89 + exec/types.h compatible) */
    /* amiport: blen/bp promoted to file scope as fastcopy_blen/fastcopy_bp -- freed in cleanup() */
    int nread, from_fd, to_fd;
    int badchown = 0, serrno = 0;

    if (!fastcopy_blen) {
        fastcopy_blen = sbp->st_blksize;
        if ((fastcopy_bp = malloc(fastcopy_blen)) == NULL) {
            warn(NULL);
            fastcopy_blen = 0;
            /* amiport: return 10 = RETURN_ERROR */
            return (10);
        }
    }

    /* amiport: open/close/read/write use libnix native functions.
     * AMIPORT_NO_OPEN_MACROS is defined so the amiport shim macros do not
     * redirect these calls. libnix open() returns fds in the same namespace
     * as stdin/stdout/stderr so the full POSIX fd operations work correctly. */
    if ((from_fd = open(from, O_RDONLY)) == -1) {
        warn("%s", from);
        /* amiport: return 10 = RETURN_ERROR */
        return (10);
    }
    if ((to_fd = open(to, O_CREAT | O_TRUNC | O_WRONLY, 0600)) == -1) {
        warn("%s", to);
        (void)close(from_fd);
        /* amiport: return 10 = RETURN_ERROR */
        return (10);
    }

    /* amiport: fchown() -> amiport_fchown() via macro in <amiport/unistd.h>
     * -- no-op stub; AmigaOS has no user/group ownership model */
    if (fchown(to_fd, sbp->st_uid, sbp->st_gid)) {
        serrno = errno;
        badchown = 1;
    }
    /* amiport: fchmod() -> amiport_fchmod() via macro in <amiport/unistd.h>
     * -- no-op stub; AmigaOS protection bits have inverted semantics */
    (void) fchmod(to_fd, sbp->st_mode & ~(S_ISUID|S_ISGID));

    while ((nread = read(from_fd, fastcopy_bp, fastcopy_blen)) > 0) {
        if (write(to_fd, fastcopy_bp, nread) != nread) {
            warn("%s", to);
            goto err;
        }
    }
    if (nread == -1) {
        warn("%s", from);
err:
        if (unlink(to))
            warn("%s: remove", to);
        (void)close(from_fd);
        (void)close(to_fd);
        /* amiport: return 10 = RETURN_ERROR */
        return (10);
    }
    (void)close(from_fd);

    if (badchown) {
        if ((sbp->st_mode & (S_ISUID|S_ISGID))) {
            /* amiport: warnc() -> amiport_warnc() via macro in <amiport/err.h> */
            warnc(serrno,
                "%s: set owner/group; not setting setuid/setgid",
                to);
            sbp->st_mode &= ~(S_ISUID|S_ISGID);
        } else if (!fflg) {
            warnc(serrno, "%s: set owner/group", to);
        }
    }
    /* amiport: fchmod() no-op stub -- second call to set final mode */
    if (fchmod(to_fd, sbp->st_mode))
        warn("%s: set mode", to);

    /*
     * XXX
     * NFS doesn't support chflags; ignore errors unless there's reason
     * to believe we're losing bits.
     */
    /* amiport: fchflags() -- no equivalent on AmigaOS; stub macro returns 0.
     * AmigaOS has no BSD file flags (UF_NODUMP, SF_IMMUTABLE, etc.).
     * st_flags is not in struct amiport_stat; use 0 as the flags value --
     * the fchflags macro always returns 0 and the errno check is inert. */
    errno = 0;
    if (fchflags(to_fd, 0))
        if (errno != EOPNOTSUPP)
            warn("%s: set flags", to);

    /* amiport: struct amiport_stat uses ULONG st_atime / st_mtime (Unix
     * timestamps), not struct timespec st_atim / st_mtim.
     * Construct struct timespec values from the ULONG timestamps.
     * tv_nsec is 0 -- AmigaOS timestamp resolution is 1/50s (20ms). */
    ts[0].tv_sec  = (long)sbp->st_atime;
    ts[0].tv_nsec = 0;
    ts[1].tv_sec  = (long)sbp->st_mtime;
    ts[1].tv_nsec = 0;
    /* amiport: futimens() -> amiport_futimens() via macro in <amiport/unistd.h>
     * -- AmigaOS stores only modification time at 1/50s (20ms) precision.
     * Access time (ts[0]) is silently ignored. */
    if (futimens(to_fd, ts))
        warn("%s: set times", to);

    if (close(to_fd)) {
        warn("%s", to);
        /* amiport: return 10 = RETURN_ERROR */
        return (10);
    }

    /* amiport: unlink() -> amiport_unlink() via <amiport/sys/stat.h> macro */
    if (unlink(from)) {
        warn("%s: remove", from);
        /* amiport: return 10 = RETURN_ERROR */
        return (10);
    }

    if (vflg)
        (void)fprintf(stdout, "%s -> %s\n", from, to);

    return (0);
}

int
mvcopy(char *from, char *to)
{
    /*
     * amiport-redesign: NEEDS HUMAN REVIEW
     *
     * Original mvcopy() called cpmain(2, argv) + rmmain(1, argv) to move a
     * directory tree across volumes. cpmain() and rmmain() are OpenBSD
     * internal entry points that share the same process -- they are not
     * available on AmigaOS.
     *
     * A complete cross-volume directory move requires a recursive copy+delete
     * loop using AmigaDOS Lock/Examine/ExNext (or a bundled cp -r + rm -r).
     * This is a Tier 3 redesign. See redesign-patterns.md.
     *
     * Workaround for users: copy the directory with AmigaDOS 'Copy ALL',
     * then delete the original with 'Delete ALL'.
     */
    (void)from;
    (void)to;
    warnx("cannot move directories across volumes: cross-volume directory move not supported");
    /* amiport: return 10 = RETURN_ERROR */
    return (10);
}

void
usage(void)
{
    (void)fprintf(stderr, "usage: %s [-fiv] source target\n", __progname);
    (void)fprintf(stderr, "       %s [-fiv] source ... directory\n",
        __progname);
    /* amiport: exit(1) -> exit(10): RETURN_ERROR, visible to IF ERROR in scripts */
    exit(10);
}
