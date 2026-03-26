/*	$OpenBSD: mkdir.c,v 1.31 2019/06/28 13:34:59 deraadt Exp $	*/
/*	$NetBSD: mkdir.c,v 1.14 1995/06/25 21:59:21 mycroft Exp $	*/

/*
 * Copyright (c) 1983, 1992, 1993
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

/* amiport: AmigaOS 3.x port of mkdir 1.31 (OpenBSD) */

/* amiport: Amiga version string */
static const char *verstag = "$VER: mkdir 1.31 (26.03.2026)";

/* amiport: stack cookie -- default 4KB is too small for ported software */
long __stack = 8192;

/* amiport: replaced <stdlib.h> with amiport shim FIRST -- defines exit() macro
 * before <amiport/err.h> static functions are emitted, so err()/errx() use
 * amiport_exit() not libnix exit() */
#include <amiport/stdlib.h>

/* amiport: replaced <sys/stat.h> with amiport shim -- provides stat(),
 * amiport_stat struct, S_ISDIR(), mkdir() via CreateDir() */
#include <amiport/sys/stat.h>

/* amiport: replaced <sys/types.h> -- mode_t not defined on AmigaOS;
 * typedef as unsigned int since Unix permissions are not used */
typedef unsigned int mode_t;

/* amiport: replaced <err.h> with amiport shim -- err(), errx(), warn(), warnx() */
#include <amiport/err.h>

#include <errno.h>
#include <stdio.h>

#include <string.h>

/* amiport: replaced <unistd.h> -- getopt() available via amiport/getopt.h.
 * Using amiport header to avoid libnix getopt_long breakage (crash-patterns #17)
 * even though this program only uses short options. */
#include <amiport/getopt.h>

/* amiport: glob.h provides amiport_expand_argv/free_argv and __progname */
#include <amiport/glob.h>

/* amiport: dirent.h provides mkdir() -> amiport_mkdir() via CreateDir() */
#include <amiport/dirent.h>

/* amiport: pledge/unveil not available on AmigaOS -- stubbed as no-ops */
#define pledge(p, e) (0)
#define unveil(p, f) (0)

/* amiport: chmod() not available on AmigaOS -- AmigaDOS has no Unix permission
 * model. Stubbed as no-op. The -m flag's mode bits are accepted for syntax
 * compatibility but have no effect. */
#define chmod(p, m)  (0)

/* amiport: umask() not available on AmigaOS -- stubbed to return 0.
 * This means mode = 0777 & ~0 = 0777 (all bits), which is fine since
 * amiport_mkdir() ignores the mode argument entirely. */
#define umask(m)     (0)

/* amiport: Unix permission constants not defined on AmigaOS.
 * These are used for the -m flag logic which is a no-op anyway (chmod is
 * stubbed). Defined to their standard POSIX values for code correctness. */
#ifndef S_IWUSR
#define S_IWUSR  0000200
#define S_IXUSR  0000100
#define S_IRWXU  0000700
#define S_IRWXG  0000070
#define S_IRWXO  0000007
#define S_ISUID  0004000
#define S_ISGID  0002000
#define S_ISTXT  0001000
#endif

/* amiport: setmode()/getmode() are OpenBSD-specific functions for parsing
 * symbolic mode strings (e.g. "u+rwx"). Not available on AmigaOS.
 * Since chmod() is a no-op, the -m flag is accepted for syntax compatibility
 * but has no effect. setmode() returns a non-NULL dummy so the NULL check
 * does not trip; getmode() returns the base mode unchanged. */
#define setmode(s)         ((void *)1)
#define getmode(set, base) (base)

/* amiport: __dead is OpenBSD's noreturn attribute spelling */
#ifndef __dead
#define __dead __attribute__((noreturn))
#endif

int	mkpath(char *, mode_t, mode_t);
static void __dead usage(void);

/* amiport: cleanup function registered with atexit() to free expanded argv */
static void
cleanup(void)
{
	amiport_free_argv();
	(void)fflush(stdout);
}

int
main(int argc, char *argv[])
{
	int ch, rv, exitval, pflag;
	void *set;
	mode_t mode, dir_mode;

	/* amiport: expand wildcard args -- AmigaOS shell does not glob */
	amiport_expand_argv(&argc, &argv);
	/* amiport: register cleanup for all exit paths including err()/errx() */
	atexit(cleanup);

	/*
	 * The default file mode is a=rwx (0777) with selected permissions
	 * removed in accordance with the file mode creation mask.  For
	 * intermediate path name components, the mode is the default modified
	 * by u+wx so that the subdirectories can always be created.
	 */
	/* amiport: umask() stubbed to 0 -- mode = 0777, which is fine since
	 * amiport_mkdir() ignores mode entirely */
	mode = 0777 & ~umask(0);
	dir_mode = mode | S_IWUSR | S_IXUSR;

	pflag = 0;
	while ((ch = getopt(argc, argv, "m:p")) != -1)
		switch(ch) {
		case 'p':
			pflag = 1;
			break;
		case 'm':
			/* amiport: setmode()/getmode() stubbed -- -m flag accepted
			 * but mode bits have no effect on AmigaOS (no Unix permissions) */
			if ((set = setmode(optarg)) == NULL)
				errx(10, "invalid file mode: %s", optarg); /* amiport: exit 1->10 RETURN_ERROR */
			mode = getmode(set, S_IRWXU | S_IRWXG | S_IRWXO);
			free(set);
			break;
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	/* amiport: pledge() stubbed as no-op -- condition block preserved for
	 * correctness but the pledge call always succeeds */
	if ((mode & (S_ISUID | S_ISGID | S_ISTXT)) == 0) {
		if (pledge("stdio rpath cpath fattr", NULL) == -1)
			err(10, "pledge"); /* amiport: exit 1->10 RETURN_ERROR */
	}

	if (*argv == NULL)
		usage();

	for (exitval = 0; *argv != NULL; ++argv) {
		char *slash;

		/* Remove trailing slashes, per POSIX. */
		slash = strrchr(*argv, '\0');
		while (--slash > *argv && *slash == '/')
			*slash = '\0';

		if (pflag) {
			rv = mkpath(*argv, mode, dir_mode);
		} else {
			/* amiport: mkdir() mapped to amiport_mkdir() via <amiport/dirent.h> macro */
			rv = mkdir(*argv, mode);
			/*
			 * The mkdir() and umask() calls both honor only the
			 * low nine bits, so if you try to set a mode including
			 * the sticky, setuid, setgid bits you lose them. Don't
			 * do this unless the user has specifically requested
			 * a mode as chmod will (obviously) ignore the umask.
			 */
			/* amiport: chmod() stubbed as no-op -- mode > 0777 branch
			 * is a dead code path on AmigaOS */
			if (rv == 0 && mode > 0777)
				rv = chmod(*argv, mode);
		}
		if (rv == -1) {
			warn("%s", *argv);
			exitval = 1;
		}
	}
	return exitval;
}

/*
 * mkpath -- create directories.
 *	path     - path
 *	mode     - file mode of terminal directory
 *	dir_mode - file mode of intermediate directories
 */
int
mkpath(char *path, mode_t mode, mode_t dir_mode)
{
	struct amiport_stat sb; /* amiport: use amiport_stat struct directly */
	char *slash;
	int done;

	slash = path;

	for (;;) {
		slash += strspn(slash, "/");
		slash += strcspn(slash, "/");

		done = (*slash == '\0');
		*slash = '\0';

		/* amiport: mkdir() mapped to amiport_mkdir() via <amiport/dirent.h> macro */
		if (mkdir(path, done ? mode : dir_mode) == 0) {
			/* amiport: chmod() stubbed as no-op -- always returns 0 */
			if (mode > 0777 && chmod(path, mode) == -1)
				return (-1);
		} else {
			int mkdir_errno = errno;

			/* amiport: stat() mapped to amiport_stat() via <amiport/sys/stat.h> macro */
			if (stat(path, &sb) == -1) {
				/* Not there; use mkdir()s errno */
				errno = mkdir_errno;
				return (-1);
			}
			/* amiport: S_ISDIR() mapped to AMIPORT_S_ISDIR() via macro */
			if (!S_ISDIR(sb.st_mode)) {
				/* Is there, but isn't a directory */
				errno = ENOTDIR;
				return (-1);
			}
		}

		if (done)
			break;

		*slash = '/';
	}

	return (0);
}

static void __dead
usage(void)
{
	(void)fprintf(stderr, "usage: %s [-p] [-m mode] directory ...\n",
	    __progname);
	exit(10); /* amiport: exit 1->10 RETURN_ERROR */
}
