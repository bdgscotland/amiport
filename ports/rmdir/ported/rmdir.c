/*	$OpenBSD: rmdir.c,v 1.15 2025/05/14 15:51:58 schwarze Exp $	*/
/*	$NetBSD: rmdir.c,v 1.13 1995/03/21 09:08:31 cgd Exp $	*/

/*-
 * Copyright (c) 1992, 1993, 1994
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

/* amiport: __dead macro for GCC noreturn attribute (OpenBSD uses __dead from sys/cdefs.h) */
#define __dead __attribute__((__noreturn__))

/* amiport: pledge/unveil not available on AmigaOS -- stubbed as no-ops */
#define pledge(p, e) (0)

#include <assert.h>
/* amiport: replaced <err.h> with amiport shim providing err/warn family */
#include <amiport/err.h>
#include <errno.h>
#include <stdio.h>
/* amiport: replaced <stdlib.h> with amiport shim -- activates amiport_exit() macro */
#include <amiport/stdlib.h>
#include <string.h>
/* amiport: replaced <unistd.h> with amiport shim */
#include <amiport/unistd.h>
/* amiport: include dirent shim -- provides rmdir() -> amiport_rmdir() macro */
#include <amiport/dirent.h>
/* amiport: include glob shim -- provides amiport_expand_argv/amiport_free_argv and extern __progname */
#include <amiport/glob.h>
/* amiport: include getopt shim -- libnix getopt_long is broken; amiport/getopt.h provides working getopt + optind/optarg */
#include <amiport/getopt.h>

/* amiport: Amiga version string */
static const char *verstag = "$VER: rmdir 1.15 (26.03.2026)";

/* amiport: stack cookie -- Amiga default 4KB is too small */
long __stack = 8192;

/* amiport: __progname provided by argv_expand.o via amiport_expand_argv() -- do NOT redefine */
extern char *__progname;

int rm_path(char *);
static void __dead usage(void);

/* amiport: cleanup function registered with atexit() to free expanded argv on all exit paths */
static void
cleanup(void)
{
    amiport_free_argv();
    (void)fflush(stdout);
}

int
main(int argc, char *argv[])
{
	int ch, errors;
	int pflag;

	/* amiport: expand wildcard args -- Amiga shell does not glob */
	amiport_expand_argv(&argc, &argv);
	/* amiport: register cleanup for all exit paths including err()/errx() */
	atexit(cleanup);

	if (pledge("stdio cpath", NULL) == -1)
		err(10, "pledge"); /* amiport: RETURN_ERROR */

	pflag = 0;
	while ((ch = getopt(argc, argv, "p")) != -1)
		switch(ch) {
		case 'p':
			pflag = 1;
			break;
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc == 0)
		usage();

	for (errors = 0; *argv != NULL; argv++) {
		if (rmdir(*argv) == -1) { /* nosemgrep: cpp.lang.security.filesystem.path-manipulation.path-manipulation -- CLI tool, argv paths supplied by invoking user */
			warn("%s", *argv);
			errors = 1;
		} else if (pflag)
			errors |= rm_path(*argv);
	}
	return errors;
}

/*
 * Iteratively remove each pathname component, except the
 * last one, because that one was already removed in main().
 */
int
rm_path(char *path)
{
	char *p;

	assert(*path != '\0');
	p = strchr(path, '\0');
	while (--p > path && *p == '/')
		continue;

	for (; p > path; p--) {
		if (p[0] == '/' && p[-1] != '/') {
			p[0] = '\0';
			if (rmdir(path) == -1) {
				warn("%s", path);
				return 1;
			}
		}
	}
	return 0;
}

static void __dead
usage(void)
{
	fprintf(stderr, "usage: %s [-p] directory ...\n", __progname);
	exit(10); /* amiport: RETURN_ERROR -- visible to Amiga IF ERROR scripts */
}
