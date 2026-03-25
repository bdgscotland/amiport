/*	$OpenBSD: expand.c,v 1.15 2022/12/04 23:50:48 cheloha Exp $	*/
/*	$NetBSD: expand.c,v 1.5 1995/09/02 06:19:46 jtc Exp $	*/

/*
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

#include <stdio.h>
#include <amiport/stdlib.h>  /* amiport: replaced <stdlib.h> -- activates exit() -> amiport_exit() macro */
#include <ctype.h>
#include <unistd.h>
#include <amiport/err.h>     /* amiport: replaced <err.h> with shim header */
#include <amiport/glob.h>    /* amiport: argv wildcard expansion + __progname */

/* amiport: pledge/unveil not available on AmigaOS -- stubbed as no-ops */
#define pledge(p, e) (0)

/* amiport: Amiga version string -- upstream OpenBSD revision 1.15 */
static const char *verstag __attribute__((used)) = "$VER: expand 1.15 (25.03.2026)";

/* amiport: stack cookie -- Amiga default 4KB is too small for ported software */
long __stack = 32768;

/* amiport: __progname defined in argv_expand.o (libamiport.a).
 * Declared here so usage() can reference it.
 * amiport_expand_argv() sets it from argv[0] at runtime. */
extern char *__progname;

/*
 * expand - expand tabs to equivalent spaces
 */
int	nstops;
int	tabstops[100];

/* amiport perf: pre-filled spaces buffer for bulk fwrite -- eliminates
 * per-character putchar(' ') loops on 68k (HIGH impact, saves ~7 JSR
 * calls per tab at default 8-stop width) */
static const char spaces[256] =
	"                                                                "
	"                                                                "
	"                                                                "
	"                                                                ";

static void getstops(char *);
static void usage(void);

static void
cleanup(void)
{
    /* amiport: free expanded argv on all exit paths (err/errx bypass cleanup code) */
    amiport_free_argv();
    (void)fflush(stdout);
}

int
main(int argc, char *argv[])
{
	int c, column;
	int n;
	/* amiport perf: static buffer for fgets — keeps 4KB off stack (known pitfall) */
	static char inbuf[4096];

	/* amiport: expand wildcard args -- Amiga shell doesn't glob */
	amiport_expand_argv(&argc, &argv);
	/* amiport: register cleanup for all exit paths including err()/errx() */
	atexit(cleanup);

	/* amiport: pledge stubbed -- always succeeds, no capability restriction on AmigaOS */
	if (pledge("stdio rpath", NULL) == -1)
		err(10, "pledge"); /* amiport: RETURN_ERROR */

	/* handle obsolete syntax */
	while (argc > 1 && argv[1][0] == '-' &&
	    isdigit((unsigned char)argv[1][1])) {
		getstops(&argv[1][1]);
		argc--; argv++;
	}

	while ((c = getopt (argc, argv, "t:")) != -1) {
		switch (c) {
		case 't':
			getstops(optarg);
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	do {
		if (argc > 0) {
			if (freopen(argv[0], "r", stdin) == NULL)
				err(10, "%s", argv[0]); /* amiport: RETURN_ERROR */
			argc--, argv++;
		}
		column = 0;
		/* amiport perf: fgets + pointer scan replaces getchar() loop.
		 * Eliminates ~40 JSR calls per line on 68k (CRITICAL impact). */
		while (fgets(inbuf, sizeof(inbuf), stdin) != NULL) {
			char *p = inbuf;
			while (*p) {
				c = (unsigned char)*p++;
				switch (c) {
				case '\t':
					if (nstops == 0) {
						/* amiport perf: bulk fwrite replaces putchar loop */
						n = 8 - (column & 07);
						(void)fwrite(spaces, 1, n, stdout);
						column += n;
						continue;
					}
					if (nstops == 1) {
						/* amiport perf: precompute target, single fwrite */
						n = tabstops[0] - (column % tabstops[0]);
						(void)fwrite(spaces, 1, n, stdout);
						column += n;
						continue;
					}
					for (n = 0; n < nstops; n++)
						if (tabstops[n] > column)
							break;
					if (n == nstops) {
						putchar(' ');
						column++;
						continue;
					}
					{
						int nsp = tabstops[n] - column;
						(void)fwrite(spaces, 1, nsp, stdout);
						column = tabstops[n];
					}
					continue;

				case '\b':
					if (column)
						column--;
					putchar('\b');
					continue;

				default:
					putchar(c);
					column++;
					continue;

				case '\n':
					putchar(c);
					column = 0;
					continue;
				}
			}
		}
	} while (argc > 0);
	exit(0);
}

static void
getstops(char *cp)
{
	int i;

	nstops = 0;
	for (;;) {
		i = 0;
		while (*cp >= '0' && *cp <= '9')
			i = i * 10 + *cp++ - '0';
		if (i <= 0 || i > 256) {
bad:
			errx(10, "Bad tab stop spec"); /* amiport: RETURN_ERROR */
		}
		if (nstops > 0 && i <= tabstops[nstops-1])
			goto bad;
		if (nstops >= sizeof(tabstops) / sizeof(tabstops[0]))
			errx(10, "Too many tab stops"); /* amiport: RETURN_ERROR */
		tabstops[nstops++] = i;
		if (*cp == 0)
			break;
		if (*cp != ',' && *cp != ' ')
			goto bad;
		cp++;
	}
}

static void
usage(void)
{
	/* amiport: __progname defined at file scope -- no extern declaration needed */
	fprintf (stderr, "usage: %s [-t tablist] [file ...]\n", __progname);
	exit(10); /* amiport: RETURN_ERROR */
}
