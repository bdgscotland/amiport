/*	$OpenBSD: rs.c,v 1.30 2015/12/03 12:23:15 schwarze Exp $	*/

/*-
 * Copyright (c) 1993
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
 *	rs - reshape a data array
 *	Author:  John Kunze, Office of Comp. Affairs, UCB
 *		BEWARE: lots of unfinished edges
 */

/* amiport: Amiga version string */
static const char *verstag = "$VER: rs 1.30 (26.03.2026)";

/* amiport: stack cookie -- default 4KB is too small for ported software */
long __stack = 16384;

#include <ctype.h>
/* amiport: replaced <err.h> with <amiport/err.h> -- also provides strtonum */
#include <amiport/err.h>
#include <errno.h>
#include <limits.h>
/* amiport: removed <locale.h> -- setlocale() stub is in <amiport/unistd.h> */
#include <stdio.h>
/* amiport: replaced <stdlib.h> with <amiport/stdlib.h> -- activates exit() -> amiport_exit() macro */
#include <amiport/stdlib.h>
#include <string.h>
/* amiport: replaced <unistd.h> with <amiport/unistd.h> -- provides setlocale stub */
#include <amiport/unistd.h>
/* amiport: added <amiport/getopt.h> -- provides getopt(), optarg, optind */
#include <amiport/getopt.h>
/* amiport: added <amiport/string.h> -- provides reallocarray */
#include <amiport/string.h>
/* amiport: added <amiport/stdio_ext.h> -- provides getline */
#include <amiport/stdio_ext.h>
/* amiport: added <amiport/glob.h> -- provides amiport_expand_argv, __progname */
#include <amiport/glob.h>

/* amiport: pledge() not available on AmigaOS -- stubbed */
#define pledge(p, e) (0)

/*
 * mbsavis -- return display width of a multibyte string and store pointer.
 *
 * amiport: AmigaOS 3.x has no multibyte/wchar support.  This is a stub
 * that returns strlen(src) as the display width and sets *dstpp to point
 * to the source string directly (no allocation, caller must not free).
 * On OpenBSD the real mbsavis() calls vis() to handle non-printable bytes;
 * on AmigaOS we treat the input as plain ASCII.
 */
static int
mbsavis(char **dstpp, const char *src)
{
	/* amiport: mbsavis() stub -- AmigaOS has no multibyte/vis support.
	 * Store src pointer directly (no copy) and return strlen as width. */
	*dstpp = (char *)src; /* safe: callers do not free ep->s for these */
	return (int)strlen(src);
}

struct	entry {
	int	 w;  /* Display width. */
	char	*s;  /* Multibyte string. */
};

long	flags;
#define	TRANSPOSE	000001
#define	MTRANSPOSE	000002
#define	ONEPERLINE	000004
#define	ONEISEPONLY	000010
#define	ONEOSEPONLY	000020
#define	NOTRIMENDCOL	000040
#define	SQUEEZE		000100
#define	SHAPEONLY	000200
#define	DETAILSHAPE	000400
#define	RIGHTADJUST	001000
#define	NULLPAD		002000
#define	RECYCLE		004000
#define	SKIPPRINT	010000
#define ONEPERCHAR	0100000
#define NOARGS		0200000

short	*colwidths;
int	nelem;
struct entry *elem;
struct entry *endelem;
char	*curline;
int	allocsize = BUFSIZ;
int	irows, icols;
int	orows, ocols;
int	maxwidth;
int	skip;
int	propgutter;
char	isep = ' ', osep = ' ';
int	owidth = 80, gutter = 2;

void	  usage(void);
void	  getargs(int, char *[]);
void	  getfile(void);
int	  get_line(void);
struct entry *getptrs(struct entry *);
void	  prepfile(void);
void	  prints(struct entry *, int);
void	  putfile(void);

#define INCR(ep) do {			\
	if (++ep >= endelem)		\
		ep = getptrs(ep);	\
} while(0)

/* amiport: cleanup function registered with atexit() */
static void
cleanup(void)
{
	amiport_free_argv();
	(void)fflush(stdout);
}

int
main(int argc, char *argv[])
{
	/* amiport: expand wildcard args -- Amiga shell doesn't glob */
	amiport_expand_argv(&argc, &argv);
	/* amiport: register cleanup for all exit paths including err()/errx() */
	atexit(cleanup);

	/* amiport: setlocale() mapped to amiport_setlocale() via <amiport/unistd.h>
	 * -- always returns "C" locale on AmigaOS */
	setlocale(LC_CTYPE, "");

	if (pledge("stdio", NULL) == -1)
		err(10, "pledge"); /* amiport: RETURN_ERROR */

	getargs(argc, argv);
	getfile();
	if (flags & SHAPEONLY) {
		printf("%d %d\n", irows, icols);
		exit(0);
	}
	prepfile();
	putfile();
	exit(0);
}

void
getfile(void)
{
	const char delim[2] = { isep, '\0' };
	char *p;
	struct entry *ep;
	int multisep = (flags & ONEISEPONLY ? 0 : 1);
	int nullpad = flags & NULLPAD;
	struct entry *padto;

	curline = NULL;
	while (skip--) {
		if (get_line() == EOF)
			return;
		if (flags & SKIPPRINT)
			puts(curline);
	}
	if (get_line() == EOF)
		return;
	if (flags & NOARGS && strlen(curline) < (size_t)owidth)
		flags |= ONEPERLINE;
	if (flags & ONEPERLINE)
		icols = 1;
	else				/* count cols on first line */
		for (p = curline; *p != '\0'; p++) {
			if (*p == isep && multisep)
				continue;
			icols++;
			while (*p && *p != isep)
				p++;
		}
	ep = getptrs(NULL);
	p = curline;
	do {
		if (flags & ONEPERLINE) {
			ep->w = mbsavis(&ep->s, curline);
			if (maxwidth < ep->w)
				maxwidth = ep->w;
			INCR(ep);		/* prepare for next entry */
			irows++;
			continue;
		}
		p = curline;
		while (p != NULL && *p != '\0') {
			if (*p == isep) {
				p++;
				if (multisep)
					continue;
				ep->s = "";	/* empty column */
				ep->w = 0;
			} else
				ep->w = mbsavis(&ep->s, strsep(&p, delim));
			if (maxwidth < ep->w)
				maxwidth = ep->w;
			INCR(ep);		/* prepare for next entry */
		}
		irows++;			/* update row count */
		if (nullpad) {			/* pad missing entries */
			padto = elem + irows * icols;
			while (ep < padto) {
				ep->s = "";
				ep->w = 0;
				INCR(ep);
			}
		}
	} while (get_line() != EOF);
	nelem = ep - elem;
}

void
putfile(void)
{
	struct entry *ep;
	int i, j, n;

	ep = elem;
	if (flags & TRANSPOSE) {
		for (i = 0; i < orows; i++) {
			for (j = i; j < nelem; j += orows)
				prints(ep + j, (j - i) / orows);
			putchar('\n');
		}
	} else {
		for (n = 0, i = 0; i < orows && n < nelem; i++) {
			for (j = 0; j < ocols; j++) {
				if (n++ >= nelem)
					break;
				prints(ep++, j);
			}
			putchar('\n');
		}
	}
}

void
prints(struct entry *ep, int col)
{
	int n;

	n = (flags & ONEOSEPONLY ? 1 : colwidths[col] - ep->w);
	if (flags & RIGHTADJUST)
		while (n-- > 0)
			putchar(osep);
	fputs(ep->s, stdout);
	while (n-- > 0)
		putchar(osep);
}

void
usage(void)
{
	extern char *__progname;

	fprintf(stderr,
	    "usage: %s [-CcSs[x]] [-GgKkw N] [-EeHhjmnTtyz] [rows [cols]]\n",
	    __progname);
	exit(10); /* amiport: RETURN_ERROR */
}

void
prepfile(void)
{
	struct entry *ep;
	int  i;
	int  j;
	struct entry *lp;
	int colw;
	int max = 0;
	int n;

	if (!nelem)
		exit(0);
	gutter += maxwidth * propgutter / 100.0;
	colw = maxwidth + gutter;
	if (flags & MTRANSPOSE) {
		orows = icols;
		ocols = irows;
	}
	else if (orows == 0 && ocols == 0) {	/* decide rows and cols */
		ocols = owidth / colw;
		if (ocols == 0) {
			warnx("Display width %d is less than column width %d",
			    owidth, colw);
			ocols = 1;
		}
		if (ocols > nelem)
			ocols = nelem;
		orows = nelem / ocols + (nelem % ocols ? 1 : 0);
	}
	else if (orows == 0)			/* decide on rows */
		orows = nelem / ocols + (nelem % ocols ? 1 : 0);
	else if (ocols == 0)			/* decide on cols */
		ocols = nelem / orows + (nelem % orows ? 1 : 0);
	while ((lp = elem + orows * ocols) > endelem)
	     (void)getptrs(NULL);
	if (flags & RECYCLE) {
		for (ep = elem + nelem; ep < lp; ep++)
			memcpy(ep, ep - nelem, sizeof(*ep));
		nelem = lp - elem;
	}
	if (!(colwidths = calloc(ocols, sizeof(short))))
		errx(10, "malloc:  No gutter space"); /* amiport: RETURN_ERROR */
	if (flags & SQUEEZE) {
		for (ep = elem, i = 0; i < ocols; i++) {
			max = 0;
			if (flags & TRANSPOSE) {
				for (j = 0; j < orows; j++, ep++)
					if (ep->w > max)
						max = ep->w;
			} else {
				for (j = i; j < nelem; j += ocols)
					if (ep[j].w > max)
						max = ep[j].w;
			}
			colwidths[i] = max + gutter;
		}
	} else {
		for (i = 0; i < ocols; i++)
			colwidths[i] = colw;
	}
	if (!(flags & NOTRIMENDCOL)) {
		if (flags & RIGHTADJUST)
			colwidths[0] -= gutter;
		else
			colwidths[ocols - 1] = 0;
	}
	n = orows * ocols;
	if (n > nelem && (flags & RECYCLE))
		nelem = n;
}

int
get_line(void)
{
	static	size_t	 cursz;
	/* amiport: ssize_t -> long -- amiport_getline() returns long; %zd -> %ld (C89) */
	static	long	 curlen;

	if (irows > 0 && flags & DETAILSHAPE)
		printf(" %ld line %d\n", curlen, irows); /* amiport: %zd -> %ld (C89, no %z support) */

	/* amiport: getline() -> amiport_getline() via <amiport/stdio_ext.h> macro */
	if ((curlen = getline(&curline, &cursz, stdin)) == EOF) {
		if (ferror(stdin))
			err(10, NULL); /* amiport: RETURN_ERROR */
		return EOF;
	}
	if (curlen > 0 && curline[curlen - 1] == '\n')
		curline[--curlen] = '\0';

	return 0;
}

struct entry *
getptrs(struct entry *sp)
{
	struct entry *p;
	int newsize;

	newsize = allocsize * 2;
	/* amiport: reallocarray() -> amiport_reallocarray() via <amiport/string.h> macro */
	p = reallocarray(elem, newsize, sizeof(*p));
	if (p == NULL)
		err(10, "no memory"); /* amiport: RETURN_ERROR */

	allocsize = newsize;
	sp = sp == NULL ? p : p + (sp - elem);
	elem = p;
	endelem = elem + allocsize;
	return(sp);
}

void
getargs(int ac, char *av[])
{
	int ch;
	const char *errstr;

	if (ac == 1)
		flags |= NOARGS | TRANSPOSE;
	/* amiport: getopt() provided by <amiport/unistd.h> */
	while ((ch = getopt(ac, av, "c::C::s::S::k:K:g:G:w:tTeEnyjhHmz")) != -1) {
		switch (ch) {
		case 'T':
			flags |= MTRANSPOSE;
			/* FALLTHROUGH */
		case 't':
			flags |= TRANSPOSE;
			break;
		case 'c':		/* input col. separator */
			flags |= ONEISEPONLY;
			/* FALLTHROUGH */
		case 's':		/* one or more allowed */
			if (optarg == NULL)
				isep = '\t';	/* default is ^I */
			else if (optarg[1] != '\0')
				usage();	/* single char only */
			else
				isep = *optarg;
			break;
		case 'C':
			flags |= ONEOSEPONLY;
			/* FALLTHROUGH */
		case 'S':
			if (optarg == NULL)
				osep = '\t';	/* default is ^I */
			else if (optarg[1] != '\0')
				usage();	/* single char only */
			else
				osep = *optarg;
			break;
		case 'w':		/* window width, default 80 */
			/* amiport: strtonum() -> amiport_strtonum() via <amiport/err.h> macro */
			owidth = strtonum(optarg, 1, INT_MAX, &errstr);
			if (errstr) {
				warnx("width %s", errstr);
				usage();
			}
			break;
		case 'K':			/* skip N lines */
			flags |= SKIPPRINT;
			/* FALLTHROUGH */
		case 'k':			/* skip, do not print */
			/* amiport: strtonum() -> amiport_strtonum() via <amiport/err.h> macro */
			skip = strtonum(optarg, 0, INT_MAX, &errstr);
			if (errstr) {
				warnx("skip value %s", errstr);
				usage();
			}
			if (skip == 0)
				skip = 1;
			break;
		case 'm':
			flags |= NOTRIMENDCOL;
			break;
		case 'g':		/* gutter width */
			/* amiport: strtonum() -> amiport_strtonum() via <amiport/err.h> macro */
			gutter = strtonum(optarg, 0, INT_MAX, &errstr);
			if (errstr) {
				warnx("gutter width %s", errstr);
				usage();
			}
			break;
		case 'G':
			/* amiport: strtonum() -> amiport_strtonum() via <amiport/err.h> macro */
			propgutter = strtonum(optarg, 0, INT_MAX, &errstr);
			if (errstr) {
				warnx("gutter proportion %s", errstr);
				usage();
			}
			break;
		case 'e':		/* each line is an entry */
			flags |= ONEPERLINE;
			break;
		case 'E':
			flags |= ONEPERCHAR;
			break;
		case 'j':			/* right adjust */
			flags |= RIGHTADJUST;
			break;
		case 'n':	/* null padding for missing values */
			flags |= NULLPAD;
			break;
		case 'y':
			flags |= RECYCLE;
			break;
		case 'H':			/* print shape only */
			flags |= DETAILSHAPE;
			/* FALLTHROUGH */
		case 'h':
			flags |= SHAPEONLY;
			break;
		case 'z':			/* squeeze col width */
			flags |= SQUEEZE;
			break;
		default:
			usage();
		}
	}
	ac -= optind;
	av += optind;

	switch (ac) {
	case 2:
		/* amiport: strtonum() -> amiport_strtonum() via <amiport/err.h> macro */
		ocols = strtonum(av[1], 0, INT_MAX, &errstr);
		if (errstr) {
			warnx("columns value %s", errstr);
			usage();
		}
		/* FALLTHROUGH */
	case 1:
		/* amiport: strtonum() -> amiport_strtonum() via <amiport/err.h> macro */
		orows = strtonum(av[0], 0, INT_MAX, &errstr);
		if (errstr) {
			warnx("columns value %s", errstr);
			usage();
		}
		/* FALLTHROUGH */
	case 0:
		break;
	default:
		usage();
	}
}
