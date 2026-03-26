/*	$OpenBSD: unexpand.c,v 1.13 2016/10/11 16:22:15 millert Exp $	*/
/*	$NetBSD: unexpand.c,v 1.5 1994/12/24 17:08:05 cgd Exp $	*/

/*-
 * Copyright (c) 1980, 1993
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

/*
 * unexpand - put tabs into a file replacing blanks
 */

/* amiport: removed <stdbool.h> -- C99, not available in C89 */
#include <stdio.h>
#include <amiport/stdlib.h>   /* amiport: replaced <stdlib.h> -- activates exit() macro */
#include <string.h>
#include <amiport/unistd.h>   /* amiport: replaced <unistd.h> */
#include <amiport/string.h>   /* amiport: added for strlcpy() shim */
#include <amiport/glob.h>     /* amiport: added for amiport_expand_argv() */

/* amiport: pledge/unveil not available on AmigaOS -- stubbed */
#define pledge(p, e) (0)
#define unveil(p, f) (0)

/* amiport: Amiga version string */
static const char *verstag = "$VER: unexpand 1.13 (26.03.2026)";

/* amiport: stack cookie -- Amiga default 4KB is too small */
long __stack = 16384;

char	genbuf[BUFSIZ];
char	linebuf[BUFSIZ];

/* amiport: changed bool to int -- C89 has no bool type */
void tabify(int);

/* amiport: atexit cleanup for argv expansion and stdout flush */
static void
cleanup(void)
{
	amiport_free_argv();
	(void)fflush(stdout);
}

int
main(int argc, char *argv[])
{
	int all = 0;    /* amiport: changed bool all = false to int all = 0 -- C89 */
	char *cp;

	/* amiport: expand wildcard args -- Amiga shell doesn't glob */
	amiport_expand_argv(&argc, &argv);
	/* amiport: register cleanup for all exit paths including err()/errx() */
	atexit(cleanup);

	if (pledge("stdio rpath", NULL) == -1) {
		perror("pledge");
		exit(10); /* amiport: RETURN_ERROR */
	}

	argc--, argv++;
	if (argc > 0 && argv[0][0] == '-') {
		if (strcmp(argv[0], "-a") != 0) {
			fprintf(stderr, "usage: unexpand [-a] [file ...]\n");
			exit(10); /* amiport: RETURN_ERROR */
		}
		all = 1;    /* amiport: changed true to 1 -- C89 */
		argc--, argv++;
	}
	do {
		if (argc > 0) {
			if (freopen(argv[0], "r", stdin) == NULL) { /* nosemgrep: cpp.lang.security.filesystem.path-manipulation.path-manipulation */
				perror(argv[0]);
				exit(10); /* amiport: RETURN_ERROR */
			}
			argc--, argv++;
		}
		while (fgets(genbuf, BUFSIZ, stdin) != NULL) {
			/* amiport: removed dead linebuf scan (perf-optimizer finding) --
			 * tabify() populates linebuf, so scanning it before tabify() is wasteful */
			tabify(all);
			fputs(linebuf, stdout); /* amiport: fputs avoids printf format overhead */
		}
	} while (argc > 0);
	exit(0);
}

/* amiport: changed bool parameter to int -- C89 */
void
tabify(int all)
{
	char *cp, *dp;
	int dcol;
	int ocol;
	size_t len;

	ocol = 0;
	dcol = 0;
	cp = genbuf;
	dp = linebuf;
	len = sizeof linebuf;

	for (;;) {
		switch (*cp) {

		case ' ':
			dcol++;
			break;

		case '\t':
			dcol += 8;
			dcol &= ~07;
			break;

		default:
			while (((ocol + 8) &~ 07) <= dcol) {
				if (ocol + 1 == dcol)
					break;
				if (len > 1) {
					*dp++ = '\t';
					len--;
				}
				ocol += 8;
				ocol &= ~07;
			}
			while (ocol < dcol) {
				if (len > 1) {
					*dp++ = ' ';
					len--;
				}
				ocol++;
			}
			if (*cp == '\0' || !all) {
				strlcpy(dp, cp, len); /* amiport: strlcpy via <amiport/string.h> */
				return;
			}
			*dp++ = *cp;
			len--;
			ocol++;
			dcol++;
		}
		cp++;
	}
}
