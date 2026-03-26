/*	$OpenBSD: column.c,v 1.27 2022/12/26 19:16:00 jmc Exp $	*/
/*	$NetBSD: column.c,v 1.4 1995/09/02 05:53:03 jtc Exp $	*/

/*
 * Copyright (c) 1989, 1993, 1994
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
static const char *verstag = "$VER: column 1.27 (26.03.2026)";

/* amiport: stack cookie -- Amiga default 4KB is too small */
long __stack = 16384;

/* amiport: removed <sys/types.h> -- not needed on AmigaOS */
/* amiport: removed <sys/ioctl.h> -- using <amiport/unistd.h> for ioctl */
/* amiport: removed <locale.h> -- setlocale() not needed on AmigaOS */
/* amiport: removed <wchar.h> -- no wchar support on AmigaOS 3.x */
/* amiport: removed <wctype.h> -- no wctype support on AmigaOS 3.x */

#include <ctype.h>
/* amiport: removed <err.h> -- err/warn/errx/warnx provided by <amiport/err.h> below */
#include <limits.h>
#include <stdio.h>
#include <amiport/stdlib.h>  /* amiport: replaced <stdlib.h> -- activates exit() macro */
#include <string.h>
#include <amiport/unistd.h>  /* amiport: replaced <unistd.h> -- provides amiport_ioctl + TIOCGWINSZ */

#include <amiport/err.h>       /* amiport: BSD err/warn/strtonum for AmigaOS */
#include <amiport/getopt.h>    /* amiport: getopt/optarg/optind -- libnix lacks the header */
#include <amiport/stdio_ext.h> /* amiport: getline() -- not in libnix stdio.h */
#include <amiport/string.h>    /* amiport: reallocarray -> amiport_reallocarray macro */
#include <amiport/glob.h>      /* amiport: amiport_expand_argv / amiport_free_argv */

/* amiport: pledge/unveil not available on AmigaOS -- stubbed */
#define pledge(p, e) (0)
#define unveil(p, f) (0)

void  c_columnate(void);
void *ereallocarray(void *, size_t, size_t);
void  input(FILE *);
void  maketbl(void);
void  print(void);
void  r_columnate(void);
/* amiport: removed __dead -- OpenBSD attribute not available on bebbo-gcc */
void usage(void);

struct field {
	char *content;
	int width;
};

int termwidth;			/* default terminal width */
int entries;			/* number of records */
int eval;			/* exit value */
int *maxwidths;			/* longest record per column */
struct field **table;		/* one array of pointers per line */
/* amiport: replaced wchar_t *separator with char * -- no wchar on AmigaOS 3.x */
char *separator = "\t ";	/* field separator for table option */
/* amiport: global tracking for input() allocations -- needed for atexit cleanup */
static char *input_buf = NULL;
static char *sep_alloc = NULL;  /* tracks strdup'd separator for cleanup */

static void
cleanup(void)
{
	/* amiport: free input() getline buffer, separator strdup, argv expansion */
	free(input_buf);
	input_buf = NULL;
	free(sep_alloc);
	sep_alloc = NULL;
	amiport_free_argv();
	(void)fflush(stdout);
}

int
main(int argc, char *argv[])
{
	/* amiport: replaced struct winsize with amiport_winsize via macro */
	struct winsize win;
	FILE *fp;
	int ch, tflag, xflag;
	/* amiport: removed char *p -- was used for wchar processing, not needed in byte-only path */
	const char *errstr;
	/* amiport: track getenv result for free() -- amiport_getenv returns malloc'd string */
	char *columns_env;

	/* amiport: expand wildcard args -- Amiga shell doesn't glob */
	amiport_expand_argv(&argc, &argv);
	/* amiport: register cleanup for all exit paths including err()/errx() */
	atexit(cleanup);

	/* amiport: removed setlocale(LC_CTYPE, "") -- no locale support on AmigaOS */

	termwidth = 0;
	/* amiport: replaced getenv() with amiport_getenv() -- returns malloc'd string */
	columns_env = amiport_getenv("COLUMNS");
	if (columns_env != NULL) {
		termwidth = strtonum(columns_env, 1, INT_MAX, NULL);
		free(columns_env);
	}
	/* amiport: replaced ioctl() with amiport_ioctl() -- queries console window via CSI */
	if (termwidth == 0 && amiport_ioctl(STDOUT_FILENO, TIOCGWINSZ, &win) == 0 &&
	    win.ws_col > 0)
		termwidth = win.ws_col;
	if (termwidth == 0)
		termwidth = 80;

	if (pledge("stdio rpath", NULL) == -1)
		err(10, "pledge");  /* amiport: RETURN_ERROR */

	tflag = xflag = 0;
	while ((ch = getopt(argc, argv, "c:s:tx")) != -1) {
		switch(ch) {
		case 'c':
			termwidth = strtonum(optarg, 1, INT_MAX, &errstr);
			if (errstr != NULL)
				errx(10, "%s: %s", errstr, optarg);  /* amiport: RETURN_ERROR */
			break;
		case 's':
			/* amiport: replaced mbstowcs/wchar_t separator with plain char* strdup --
			 * no wchar support on AmigaOS 3.x; separator comparison uses strchr below */
			/* amiport: track strdup'd separator for atexit cleanup */
			if ((separator = strdup(optarg)) == NULL)
				err(10, NULL);  /* amiport: RETURN_ERROR */
			sep_alloc = separator;
			break;
		case 't':
			tflag = 1;
			break;
		case 'x':
			xflag = 1;
			break;
		default:
			usage();
		}
	}

	if (!tflag)
		separator = "";
	argv += optind;

	if (*argv == NULL) {
		input(stdin);
	} else {
		for (; *argv; ++argv) {
			if ((fp = fopen(*argv, "r"))) {
				input(fp);
				(void)fclose(fp);
			} else {
				warn("%s", *argv);
				eval = 10;  /* amiport: RETURN_ERROR */
			}
		}
	}

	if (pledge("stdio", NULL) == -1)
		err(10, "pledge");  /* amiport: RETURN_ERROR */

	if (!entries)
		return eval;

	if (tflag)
		maketbl();
	else if (*maxwidths >= termwidth)
		print();
	else if (xflag)
		c_columnate();
	else
		r_columnate();
	return eval;
}

#define	INCR_NEXTTAB(x)	(x = (x + 8) & ~7)
void
c_columnate(void)
{
	int col, numcols;
	struct field **row;

	INCR_NEXTTAB(*maxwidths);
	if ((numcols = termwidth / *maxwidths) == 0)
		numcols = 1;
	for (col = 0, row = table;; ++row) {
		fputs((*row)->content, stdout);
		if (!--entries)
			break;
		if (++col == numcols) {
			col = 0;
			putchar('\n');
		} else {
			while (INCR_NEXTTAB((*row)->width) <= *maxwidths)
				putchar('\t');
		}
	}
	putchar('\n');
}

void
r_columnate(void)
{
	int base, col, numcols, numrows, row;

	INCR_NEXTTAB(*maxwidths);
	if ((numcols = termwidth / *maxwidths) == 0)
		numcols = 1;
	numrows = entries / numcols;
	if (entries % numcols)
		++numrows;

	for (base = row = 0; row < numrows; base = ++row) {
		for (col = 0; col < numcols; ++col, base += numrows) {
			fputs(table[base]->content, stdout);
			if (base + numrows >= entries)
				break;
			while (INCR_NEXTTAB(table[base]->width) <= *maxwidths)
				putchar('\t');
		}
		putchar('\n');
	}
}

void
print(void)
{
	int row;

	for (row = 0; row < entries; row++)
		puts(table[row]->content);
}


void
maketbl(void)
{
	struct field **row;
	int col;

	for (row = table; entries--; ++row) {
		for (col = 0; (*row)[col + 1].content != NULL; ++col)
			printf("%s%*s  ", (*row)[col].content,
			    maxwidths[col] - (*row)[col].width, "");
		puts((*row)[col].content);
	}
}

#define	DEFNUM		1000
#define	DEFCOLS		25

void
input(FILE *fp)
{
	static int maxentry = 0;
	static int maxcols = 0;
	static struct field *cols = NULL;
	int col, width;
	size_t blen;
	/* amiport: ssize_t -- libnix provides this via getline() */
	ssize_t llen;
	/* amiport: buf aliased to file-scope input_buf for atexit cleanup */
	char *p, *s;
#define buf input_buf

	/*
	 * amiport: wchar_t wc, int wlen removed -- multibyte processing
	 * disabled on AmigaOS 3.x (no wchar/mbtowc/wcschr/wcwidth support).
	 * Input is processed as single-byte ASCII/Latin-1.
	 */

	while ((llen = getline(&buf, &blen, fp)) > -1) {
		if (buf[llen - 1] == '\n')
			buf[llen - 1] = '\0';

		p = buf;
		for (col = 0;; col++) {

			/* Skip lines containing nothing but whitespace. */

			/* amiport: replaced mbtowc/iswspace loop with isspace byte loop --
			 * no wchar support on AmigaOS 3.x */
			for (s = p; *s != '\0' && isspace((unsigned char)*s); s++)
				;
			if (*s == '\0')
				break;

			/* Skip leading, multiple, and trailing separators. */

			/* amiport: replaced mbtowc/wcschr loop with strchr byte check --
			 * separator is now a plain char* on AmigaOS 3.x */
			while (*p != '\0' && strchr(separator, (unsigned char)*p) != NULL)
				p++;
			if (*p == '\0')
				break;

			/*
			 * Found a non-empty field.
			 * Remember the start and measure the width.
			 */

			s = p;
			width = 0;
			while (*p != '\0') {
				/* amiport: replaced mbtowc/wcschr/wcwidth with byte loop --
				 * single-byte only on AmigaOS 3.x */
				if (separator[0] != '\0' &&
				    strchr(separator, (unsigned char)*p) != NULL)
					break;
				if (*p == '\t')
					INCR_NEXTTAB(width);
				else
					width++;
				p++;
			}

			if (col + 1 >= maxcols) {
				if (maxcols > INT_MAX - DEFCOLS)
					err(10, "too many columns");  /* amiport: RETURN_ERROR */
				maxcols += DEFCOLS;
				cols = ereallocarray(cols, maxcols,
				    sizeof(*cols));
				maxwidths = ereallocarray(maxwidths, maxcols,
				    sizeof(*maxwidths));
				memset(maxwidths + col, 0,
				    DEFCOLS * sizeof(*maxwidths));
			}

			/*
			 * Remember the width of the field,
			 * NUL-terminate and remember the content,
			 * and advance beyond the separator, if any.
			 */

			cols[col].width = width;
			if (maxwidths[col] < width)
				maxwidths[col] = width;
			if (*p != '\0') {
				*p = '\0';
				p++;  /* amiport: was p += wlen; single-byte advance */
			}
			if ((cols[col].content = strdup(s)) == NULL)
				err(10, NULL);  /* amiport: RETURN_ERROR */
		}
		if (col == 0)
			continue;

		/* Found a non-empty line; remember it. */

		if (entries == maxentry) {
			if (maxentry > INT_MAX - DEFNUM)
				errx(10, "too many input lines");  /* amiport: RETURN_ERROR */
			maxentry += DEFNUM;
			table = ereallocarray(table, maxentry, sizeof(*table));
		}
		table[entries] = ereallocarray(NULL, col + 1,
		    sizeof(*(table[entries])));
		table[entries][col].content = NULL;
		while (col--)
			table[entries][col] = cols[col];
		entries++;
	}
}

void *
ereallocarray(void *ptr, size_t nmemb, size_t size)
{
	if ((ptr = reallocarray(ptr, nmemb, size)) == NULL)
		err(10, NULL);  /* amiport: RETURN_ERROR */
	return ptr;
}

void
usage(void)
{
	(void)fprintf(stderr,
	    "usage: column [-tx] [-c columns] [-s sep] [file ...]\n");
	exit(10);  /* amiport: RETURN_ERROR */
}
