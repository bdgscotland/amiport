/*	$OpenBSD: colrm.c,v 1.14 2022/12/04 23:50:47 cheloha Exp $	*/
/*	$NetBSD: colrm.c,v 1.4 1995/09/02 05:51:37 jtc Exp $	*/

/*-
 * Copyright (c) 1991, 1993
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
static const char *verstag = "$VER: colrm 1.14 (26.03.2026)";

/* amiport: stack cookie -- Amiga default 4KB is too small */
long __stack = 16384;

#include <sys/types.h>

#include <errno.h>
#include <limits.h>
/* amiport: removed <locale.h> -- setlocale() stub is in <amiport/unistd.h> */
#include <stdio.h>
/* amiport: replaced <stdlib.h> with <amiport/stdlib.h> -- activates exit() -> amiport_exit() macro */
#include <amiport/stdlib.h>
#include <string.h>
/* amiport: replaced <unistd.h> with <amiport/unistd.h> */
#include <amiport/unistd.h>
/* amiport: getopt/optind not in unistd.h -- use amiport/getopt.h (libnix getopt_long broken) */
#include <amiport/getopt.h>
/* amiport: removed <wchar.h> -- no wchar support on AmigaOS 3.x; multibyte paths guarded below */

/* amiport: replaced <err.h> with <amiport/err.h> */
#include <amiport/err.h>

/* amiport: getline() not in libnix -- provided by <amiport/stdio_ext.h> */
#include <amiport/stdio_ext.h>

/* amiport: argv wildcard expansion */
#include <amiport/glob.h>

/* amiport: pledge/unveil not available on AmigaOS -- stubbed */
#define pledge(p, e) (0)

#define	TAB	8

void usage(void);

/* amiport: cleanup for all exit paths including err()/errx() */
static char *getline_buf = NULL;

static void
cleanup(void)
{
	if (getline_buf != NULL) {
		free(getline_buf);
		getline_buf = NULL;
	}
	amiport_free_argv();
	(void)fflush(stdout);
}

int
main(int argc, char *argv[])
{
	char	 *line, *p;
	size_t	  linesz; /* amiport: replaced ssize_t with size_t -- matches amiport_getline() signature */
	u_long	  column, newcol, start, stop;
	int	  ch, len, width;
#ifndef __AMIGA__
	/* amiport: wchar_t not available on AmigaOS 3.x -- ASCII-only path used instead */
	wchar_t	  wc;
#endif

	/* amiport: expand wildcard args -- Amiga shell doesn't glob */
	amiport_expand_argv(&argc, &argv);
	/* amiport: register cleanup for all exit paths including err()/errx() */
	atexit(cleanup);

	/* amiport: setlocale() is a no-op stub on AmigaOS -- removed call */
	/* setlocale(LC_CTYPE, ""); */

	/* amiport: pledge stubbed as macro returning 0 -- no-op on AmigaOS */
	if (pledge("stdio", NULL) == -1)
		err(10, "pledge"); /* amiport: RETURN_ERROR */

	while ((ch = getopt(argc, argv, "")) != -1)
		switch(ch) {
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	start = stop = 0;
	switch(argc) {
	case 2:
		stop = strtol(argv[1], &p, 10);
		if (stop <= 0 || *p)
			errx(10, "illegal column -- %s", argv[1]); /* amiport: RETURN_ERROR */
		/* FALLTHROUGH */
	case 1:
		start = strtol(argv[0], &p, 10);
		if (start <= 0 || *p)
			errx(10, "illegal column -- %s", argv[0]); /* amiport: RETURN_ERROR */
		break;
	case 0:
		break;
	default:
		usage();
	}

	if (stop && start > stop)
		err(10, "illegal start and stop columns"); /* amiport: RETURN_ERROR */

	line = NULL;
	linesz = 0;
	/* amiport: track getline buffer for atexit cleanup (double-free prevention) */
	/* amiport: getline() via <amiport/stdio_ext.h> macro -> amiport_getline() */
	while (getline(&line, &linesz, stdin) != -1) {
		getline_buf = line;
		column = 0;
		width = 0;
		for (p = line; *p != '\0'; p += len) {
			len = 1;
			switch (*p) {
			case '\n':
				putchar('\n');
				continue;
			case '\b':
				/*
				 * Pass it through if the previous character
				 * was in scope, still represented by the
				 * current value of "column".
				 * Allow an optional second backspace
				 * after a double-width character.
				 */
				if (start == 0 || column < start ||
				    (stop > 0 &&
				     column > stop + (width > 1))) {
					putchar('\b');
					if (width > 1 && p[1] == '\b')
						putchar('\b');
				}
				if (width > 1 && p[1] == '\b')
					p++;
				column -= width;
				continue;
			case '\t':
				newcol = (column + TAB) & ~(TAB - 1);
				if (start == 0 || newcol < start) {
					putchar('\t');
					column = newcol;
				} else
					/*
					 * Expand tabs that intersect or
					 * follow deleted columns.
					 */
					while (column < newcol)
						if (++column < start ||
						    (stop > 0 &&
						     column > stop))
				 			putchar(' ');
				continue;
			default:
				break;
			}

			/*
			 * Handle the three cases of invalid bytes,
			 * non-printable, and printable characters.
			 *
			 * amiport: On AmigaOS 3.x, wchar/mbtowc/wcwidth are not
			 * available. All characters are treated as single-byte
			 * (ASCII) with width 1. The mbtowc/wcwidth code path is
			 * compiled out with #ifndef __AMIGA__.
			 */

#ifndef __AMIGA__
			if ((len = mbtowc(&wc, p, MB_CUR_MAX)) == -1) {
				(void)mbtowc(NULL, NULL, MB_CUR_MAX);
				len = 1;
				width = 1;
			} else if ((width = wcwidth(wc)) == -1)
				width = 1;
#else
			/* amiport: ASCII-only -- every byte has width 1 */
			len = 1;
			width = 1;
#endif

			/*
			 * If the character completely fits before or
			 * after the cut, keep it; otherwise, skip it.
			 */

			if ((start == 0 || column + width < start ||
			    (stop > 0 && column + (width > 0) > stop)))
			    	fwrite(p, 1, len, stdout);

			/*
			 * If the cut cuts the character in half
			 * and no backspace follows,
			 * print a blank for correct columnation.
			 */

			else if (width > 1 && p[len] != '\b' &&
			    (start == 0 || column + 1 < start ||
			    (stop > 0 && column + width > stop)))
				putchar(' ');

			column += width;
		}
	}
	/* amiport: NULL out tracking pointer -- atexit cleanup must not double-free */
	getline_buf = NULL;
	if (ferror(stdin))
		err(10, "stdin"); /* amiport: RETURN_ERROR */
	if (ferror(stdout))
		err(10, "stdout"); /* amiport: RETURN_ERROR */
	return 0;
}

void
usage(void)
{
	(void)fprintf(stderr, "usage: colrm [start [stop]]\n");
	exit(10); /* amiport: RETURN_ERROR -- visible to Amiga IF ERROR scripts */
}
