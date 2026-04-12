/*	$OpenBSD: logname.c,v 1.10 2016/10/13 11:51:02 schwarze Exp $	*/
/*	$NetBSD: logname.c,v 1.6 1994/12/22 06:39:32 jtc Exp $	*/

/*-
 * Copyright (c) 1991, 1993, 1994
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

/* amiport: replaced <err.h> with <amiport/err.h> (libnix has no err.h) */
#include <amiport/err.h>
#include <stdio.h>
/* amiport: replaced <stdlib.h> with <amiport/stdlib.h> (activates exit->amiport_exit macro) */
#include <amiport/stdlib.h>
#include <unistd.h>
/* amiport: added <amiport/getopt.h> -- libnix getopt_long broken, use amiport shim */
#include <amiport/getopt.h>
/* amiport: added <amiport/glob.h> for amiport_expand_argv/amiport_free_argv */
#include <amiport/glob.h>
/* amiport: added <amiport/pwd.h> for getlogin() -> amiport_getlogin() shim */
#include <amiport/pwd.h>

/* amiport: Amiga stack cookie -- default 4KB Amiga stack is insufficient */
long __stack = 4096;

/* amiport: version string for AmigaOS version query */
static const char *verstag = "$VER: logname 1.10 (11.04.2026)";

/* amiport: OpenBSD __dead attribute removed -- not supported by bebbo-gcc */
/* amiport: pledge() stubbed as no-op -- AmigaOS has no pledge() syscall */
#define pledge(p, e) (0)

static void
usage(void)
{
	(void)fprintf(stderr, "usage: logname\n");
	/* amiport: exit(1) -> exit(10) -- Amiga error convention (RETURN_ERROR=10) */
	exit(10);
}

static void
cleanup(void)
{
	/* amiport: atexit cleanup -- free expanded argv, flush stdout */
	amiport_free_argv();
	(void)fflush(stdout);
}

int
main(int argc, char *argv[])
{
	int ch;
	char *p;

	/* amiport: expand wildcards and $vars in argv (AmigaOS shell does not do this) */
	amiport_expand_argv(&argc, &argv);
	/* amiport: register cleanup for all exit paths including err()/errx() */
	atexit(cleanup);

	/* amiport: pledge() is a no-op on AmigaOS */
	if (pledge("stdio", NULL) == -1)
		/* amiport: err(1,...) -> err(10,...) -- Amiga error convention */
		err(10, "pledge");

	while ((ch = getopt(argc, argv, "")) != -1)
		switch (ch) {
		default:
			usage();
		}

	if (argc != optind)
		usage();

	if ((p = getlogin()) == NULL)
		/* amiport: err(1, NULL) -> err(10, NULL) -- Amiga error convention */
		err(10, NULL);

	(void)printf("%s\n", p);
	return 0;
}
