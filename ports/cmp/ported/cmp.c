/*      $OpenBSD: cmp.c,v 1.19 2021/10/24 21:24:16 deraadt Exp $      */
/*      $NetBSD: cmp.c,v 1.7 1995/09/08 03:22:56 tls Exp $      */

/*
 * Copyright (c) 1987, 1990, 1993, 1994
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

#include <sys/types.h>
/* amiport: replaced <sys/stat.h> with <amiport/sys/stat.h> */
#include <amiport/sys/stat.h>

/* amiport: replaced <err.h> with <amiport/err.h> -- bare <err.h> missing in bebbo-gcc libnix */
#include <amiport/err.h>
#include <errno.h>
/* amiport: replaced <fcntl.h> with <amiport/unistd.h> -- provides O_RDONLY and open() shim */
#include <amiport/unistd.h>
#include <limits.h>
#include <stdio.h>
/* amiport: replaced <stdlib.h> with <amiport/stdlib.h> -- activates exit()->amiport_exit() macro */
#include <amiport/stdlib.h>
#include <string.h>
/* amiport: replaced <unistd.h> with <amiport/unistd.h> -- provides open(), close(), fstat() shims */
/* (already included above) */
/* amiport: <amiport/glob.h> for wildcard argv expansion on AmigaOS */
#include <amiport/glob.h>
/* amiport: <amiport/getopt.h> -- libnix getopt_long is broken; provides getopt + optind */
#include <amiport/getopt.h>

#include "extern.h"

/* amiport: AmigaOS version string for the Version command */
static const char verstag[] __attribute__((used)) = "$VER: cmp 1.19 (11.04.2026)";
/* amiport: stack cookie -- default 4KB stack too small for recursive paths */
long __stack = 16384;

/* amiport: pledge() is an OpenBSD sandbox call -- no-op on AmigaOS */
#define pledge(p, e) (0)

int	lflag, sflag;

static off_t get_skip(const char *, const char *);
/* amiport: removed __attribute__((noreturn)) -- C89 mode */
static void usage(void);
/* amiport: cleanup() registered via atexit() so argv is freed on all exit paths */
static void cleanup(void);

int
main(int argc, char *argv[])
{
	struct stat sb1, sb2;
	off_t skip1, skip2;
	int ch, fd1, fd2, special;
	char *file1, *file2;
	/* amiport: track file paths for c_special() (NULL = stdin) */
	const char *path1, *path2;

	/* amiport: expand AmigaOS wildcards in argv before processing */
	amiport_expand_argv(&argc, &argv);
	atexit(cleanup);

	/* amiport: pledge() no-op on AmigaOS (macro defined above) */
	(void)pledge("stdio rpath", NULL);

	while ((ch = getopt(argc, argv, "ls")) != -1)
		switch (ch) {
		case 'l':		/* print all differences */
			lflag = 1;
			break;
		case 's':		/* silent run */
			sflag = 1;
			break;
		default:
			usage();
		}

	argv += optind;
	argc -= optind;

	if (lflag && sflag)
		errx(ERR_EXIT, "only one of -l and -s may be specified");

	if (argc < 2 || argc > 4)
		usage();

	/* Backward compatibility -- handle "-" meaning stdin. */
	special = 0;
	path1 = argv[0];
	if (strcmp(file1 = argv[0], "-") == 0) {
		special = 1;
		fd1 = 0;
		file1 = "stdin";
		path1 = NULL; /* amiport: NULL path signals stdin to c_special() */
	} else if ((fd1 = open(file1, O_RDONLY)) == -1) {
		fatal("%s", file1);
	}
	path2 = argv[1];
	if (strcmp(file2 = argv[1], "-") == 0) {
		if (special)
			fatalx("standard input may only be specified once");
		special = 1;
		fd2 = 0;
		file2 = "stdin";
		path2 = NULL; /* amiport: NULL path signals stdin to c_special() */
	} else if ((fd2 = open(file2, O_RDONLY)) == -1) {
		fatal("%s", file2);
	}

	/* amiport: pledge() no-op on AmigaOS */
	(void)pledge("stdio", NULL);

	skip1 = (argc > 2) ? get_skip(argv[2], "skip1") : 0;
	skip2 = (argc == 4) ? get_skip(argv[3], "skip2") : 0;

	if (!special) {
		if (fstat(fd1, &sb1) == -1)
			fatal("%s", file1);
		if (!S_ISREG(sb1.st_mode))
			special = 1;
		else {
			if (fstat(fd2, &sb2) == -1)
				fatal("%s", file2);
			if (!S_ISREG(sb2.st_mode))
				special = 1;
		}
	}

	if (special) {
		/*
		 * amiport: c_special() takes paths now (not fds).
		 * For named files: close the amiport fd (c_special reopens via fopen).
		 * For stdin: path is NULL, c_special uses the stdin FILE*.
		 */
		if (path1 != NULL) amiport_close(fd1);
		if (path2 != NULL) amiport_close(fd2);
		c_special(path1, file1, skip1, path2, file2, skip2);
	} else {
		/* amiport: pass path1/path2 so c_regular can fall back to c_special */
		c_regular(fd1, path1, file1, skip1, sb1.st_size,
		    fd2, path2, file2, skip2, sb2.st_size);
	}
	return 0;
}

/* amiport: free expanded argv on all exit paths (including err/errx) */
static void
cleanup(void)
{
	fflush(stdout);
	amiport_free_argv();
}

static off_t
get_skip(const char *arg, const char *name)
{
	off_t skip;
	char *ep;

	errno = 0;
	/*
	 * amiport: strtoll() not reliably available in libnix; off_t is
	 * long (32-bit) on AmigaOS so strtol() covers the full range.
	 * LLONG_MAX replaced with LONG_MAX from <limits.h>.
	 */
	skip = strtol(arg, &ep, 0);
	if (arg[0] == '\0' || *ep != '\0')
		fatalx("%s is invalid: %s", name, arg);
	if (skip < 0)
		fatalx("%s is too small: %s", name, arg);
	/* amiport: LLONG_MAX -> LONG_MAX (off_t is 32-bit long on AmigaOS) */
	if (skip == LONG_MAX && errno == ERANGE)
		fatalx("%s is too large: %s", name, arg);
	return skip;
}

static void
usage(void)
{

	(void)fprintf(stderr,
	    "usage: cmp [-l | -s] file1 file2 [skip1 [skip2]]\n");
	exit(ERR_EXIT);
}
