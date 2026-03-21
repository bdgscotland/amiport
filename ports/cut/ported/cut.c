/*	$OpenBSD: cut.c,v 1.28 2023/03/08 04:43:10 guenther Exp $	*/
/*	$NetBSD: cut.c,v 1.9 1995/09/02 05:59:23 jtc Exp $	*/

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Adam S. Moskowitz of Menlo Consulting and Marciano Pitargue.
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
static const char *verstag = "$VER: cut 1.28 (20.03.2026)";

/* amiport: Stack size cookie */
long __stack = 32768;

#include <assert.h>
#include <ctype.h>
#include <amiport/err.h>    /* amiport: replaced <err.h> — provides err, errx, warn, strtonum */
#include <errno.h>
#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <amiport/unistd.h> /* amiport: replaced <unistd.h> */
#include <amiport/getopt.h> /* amiport: provides getopt, optarg, optind */

/* amiport: pledge/unveil not available on AmigaOS — stubbed */
#define pledge(p, e) (0)
#define unveil(p, f) (0)

/* amiport: _POSIX2_LINE_MAX not defined on AmigaOS */
#ifndef _POSIX2_LINE_MAX
#define _POSIX2_LINE_MAX 2048
#endif

/* amiport: strsep() not available in libnix — inline implementation */
static char *
amiport_strsep(char **stringp, const char *delim)
{
	char *s, *tok;
	const char *spanp;
	int c, sc;

	if ((s = *stringp) == NULL)
		return (NULL);
	for (tok = s;;) {
		c = *s++;
		spanp = delim;
		do {
			if ((sc = *spanp++) == c) {
				if (c == 0)
					s = NULL;
				else
					s[-1] = 0;
				*stringp = s;
				return (tok);
			}
		} while (sc != 0);
	}
	/* NOTREACHED */
}
#define strsep amiport_strsep

/* amiport: getline() not available in libnix — inline implementation */
static long  /* amiport: ssize_t replaced with long */
amiport_getline(char **lineptr, size_t *n, FILE *stream)
{
	char *buf, *p;
	size_t alloc;
	int c;
	long len;  /* amiport: ssize_t replaced with long */

	if (lineptr == NULL || n == NULL) {
		errno = EINVAL;
		return -1;
	}
	buf = *lineptr;
	alloc = *n;
	if (buf == NULL || alloc == 0) {
		alloc = 128;
		buf = (char *)realloc(buf, alloc);
		if (buf == NULL)
			return -1;
		*lineptr = buf;
		*n = alloc;
	}
	p = buf;
	len = 0;
	for (;;) {
		c = fgetc(stream);
		if (c == EOF) {
			if (len == 0)
				return -1;
			break;
		}
		if ((size_t)(len + 1) >= alloc) {
			char *newbuf;
			alloc = alloc * 2;
			newbuf = (char *)realloc(buf, alloc);
			if (newbuf == NULL) {
				/* amiport: preserve old pointer so caller can free on failure */
				return -1;
			}
			buf = newbuf;
			*lineptr = buf;
			*n = alloc;
			p = buf + len;
		}
		*p++ = (char)c;
		len++;
		if (c == '\n')
			break;
	}
	*p = '\0';
	return len;
}
#define getline amiport_getline

char	dchar[5];
int	dlen;

/* amiport: line buffers lifted from static locals to globals so main() can
 * free them before exit(). On AmigaOS there is no OS-level memory reclaim on
 * process exit — explicit free is required to avoid permanent heap leaks. */
char	*cut_line = NULL;
size_t	 cut_linesz = 0;

int	bflag;
int	cflag;
int	dflag;
int	fflag;
int	nflag;
int	sflag;

void	b_cut(FILE *, char *);
void	c_cut(FILE *, char *);
void	f_cut(FILE *, char *);
void	get_list(char *);
void	usage(void);

int
main(int argc, char *argv[])
{
	FILE *fp;
	void (*fcn)(FILE *, char *);
	int ch, rval;

	/* amiport: setlocale not fully supported on AmigaOS — skip */
	/* setlocale(LC_CTYPE, ""); */

	if (pledge("stdio rpath", NULL) == -1)
		err(10, "pledge");  /* amiport: exit code 1 → 10 (RETURN_ERROR) */

	dchar[0] = '\t';		/* default delimiter */
	dchar[1] = '\0';
	dlen = 1;

	while ((ch = getopt(argc, argv, "b:c:d:f:sn")) != -1)
		switch(ch) {
		case 'b':
			get_list(optarg);
			bflag = 1;
			break;
		case 'c':
			get_list(optarg);
			cflag = 1;
			break;
		case 'd':
			/* amiport: replaced mblen with strlen for Amiga compatibility */
			dlen = strlen(optarg);
			if (dlen == 0 || dlen >= (int)sizeof(dchar)) usage();
			(void)memcpy(dchar, optarg, dlen);
			dchar[dlen] = '\0';
			dflag = 1;
			break;
		case 'f':
			get_list(optarg);
			fflag = 1;
			break;
		case 'n':
			nflag = 1;
			break;
		case 's':
			sflag = 1;
			break;
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (bflag + cflag + fflag != 1 ||
	    (nflag && !bflag) ||
	    ((dflag || sflag) && !fflag))
		usage();

	if (MB_CUR_MAX == 1) {
		nflag = 0;
		if (cflag) {
			bflag = 1;
			cflag = 0;
		}
	}

	fcn = fflag ? f_cut : (cflag || nflag) ? c_cut : b_cut;

	rval = 0;
	if (*argv)
		for (; *argv; ++argv) {
			if (strcmp(*argv, "-") == 0)
				fcn(stdin, "stdin");
			else {
				if ((fp = fopen(*argv, "r"))) {
					fcn(fp, *argv);
					(void)fclose(fp);
				} else {
					rval = 10;  /* amiport: exit code 1 → 10 (RETURN_ERROR) */
					warn("%s", *argv);
				}
			}
		}
	else {
		if (pledge("stdio", NULL) == -1)
			err(10, "pledge");  /* amiport: exit code 1 → 10 (RETURN_ERROR) */

		fcn(stdin, "stdin");
	}
	/* amiport: free line buffer explicitly before exit — AmigaOS does not
	 * reclaim malloc'd memory at process exit the way Unix does; relying on
	 * libnix cleanup is implementation-dependent and not safe across all
	 * toolchain configurations (-noixemul has no cleanup walk). */
	if (cut_line != NULL) {
		free(cut_line);
		cut_line = NULL;
	}
	exit(rval);
}

int autostart, autostop, maxval;

char positions[_POSIX2_LINE_MAX + 1];

int
read_number(char **p)
{
	int dash, n;
	const char *errstr;
	char *q;

	q = *p + strcspn(*p, "-");
	dash = *q == '-';
	*q = '\0';
	n = strtonum(*p, 1, _POSIX2_LINE_MAX, &errstr);
	if (errstr != NULL)
		errx(10, "[-bcf] list: %s %s (allowed 1-%d)", *p, errstr,
		    _POSIX2_LINE_MAX);  /* amiport: exit code 1 → 10 (RETURN_ERROR) */
	if (dash)
		*q = '-';
	*p = q;

	return n;
}

void
get_list(char *list)
{
	int setautostart, start, stop;
	char *p;

	/*
	 * set a byte in the positions array to indicate if a field or
	 * column is to be selected; use +1, it's 1-based, not 0-based.
	 * This parser is less restrictive than the Draft 9 POSIX spec.
	 * POSIX doesn't allow lists that aren't in increasing order or
	 * overlapping lists.  We also handle "-3-5" although there's no
	 * real reason too.
	 */
	while ((p = strsep(&list, ", \t"))) {
		setautostart = start = stop = 0;
		if (*p == '-') {
			++p;
			setautostart = 1;
		}
		if (isdigit((unsigned char)*p)) {
			start = stop = read_number(&p);
			if (setautostart && start > autostart)
				autostart = start;
		}
		if (*p == '-') {
			if (isdigit((unsigned char)p[1])) {
				++p;
				stop = read_number(&p);
			}
			if (*p == '-') {
				++p;
				if (!autostop || autostop > stop)
					autostop = stop;
			}
		}
		if (*p != '\0' || !stop || !start)
			errx(10, "[-bcf] list: illegal list value");  /* amiport: exit code 1 → 10 (RETURN_ERROR) */
		if (maxval < stop)
			maxval = stop;
		if (start <= stop)
			memset(positions + start, 1, stop - start + 1);
	}

	/* overlapping ranges */
	if (autostop && maxval > autostop)
		maxval = autostop;

	/* set autostart */
	/* amiport: was '1' (ASCII 49) — corrected to 1 to match line 314 style;
	 * both are non-zero so boolean tests pass, but consistency prevents
	 * future confusion if the test is ever changed to == 1 */
	if (autostart)
		memset(positions + 1, 1, autostart);
}

void
b_cut(FILE *fp, char *fname)
{
	int ch = 0, col;
	char *pos;

	for (;;) {
		pos = positions + 1;
		for (col = maxval; col; --col) {
			if ((ch = getc(fp)) == EOF)
				return;
			if (ch == '\n')
				break;
			if (*pos++)
				(void)putchar(ch);
		}
		if (ch != '\n') {
			if (autostop)
				while ((ch = getc(fp)) != EOF && ch != '\n')
					(void)putchar(ch);
			else
				while ((ch = getc(fp)) != EOF && ch != '\n')
					;
		}
		(void)putchar('\n');
	}
}

void
c_cut(FILE *fp, char *fname)
{
	/* amiport: use shared globals cut_line/cut_linesz so main() can free */
	long		 linelen;  /* amiport: ssize_t replaced with long */
	char		*cp, *pos, *maxpos;
	int		 len;

	while ((linelen = getline(&cut_line, &cut_linesz, fp)) != -1) {
		if (cut_line[linelen - 1] == '\n')
			cut_line[linelen - 1] = '\0';

		cp = cut_line;
		pos = positions + 1;
		maxpos = pos + maxval;
		while(pos < maxpos && *cp != '\0') {
			/* amiport: mblen not reliable on Amiga, use 1 for ASCII */
			len = 1;
			pos += nflag ? len : 1;
			if (pos[-1] == '\0')
				cp += len;
			else
				while (len--)
					putchar(*cp++);
		}
		if (autostop)
			puts(cp);
		else
			putchar('\n');
	}
}

void
f_cut(FILE *fp, char *fname)
{
	/* amiport: use shared globals cut_line/cut_linesz so main() can free */
	long		 linelen;  /* amiport: ssize_t replaced with long */
	char		*sp, *ep, *pos, *maxpos;
	int		 output;

	while ((linelen = getline(&cut_line, &cut_linesz, fp)) != -1) {
		if (cut_line[linelen - 1] == '\n')
			cut_line[linelen - 1] = '\0';

		if ((ep = strstr(cut_line, dchar)) == NULL) {
			if (!sflag)
				puts(cut_line);
			continue;
		}

		pos = positions + 1;
		maxpos = pos + maxval;
		output = 0;
		sp = cut_line;
		for (;;) {
			if (*pos++) {
				if (output)
					fputs(dchar, stdout);
				/* amiport: fwrite replaces putchar loop — one AmigaDOS
				 * Write call per field instead of one per character */
				if (ep > sp)
					(void)fwrite(sp, 1, (size_t)(ep - sp), stdout);
				sp = ep;
				output = 1;
			} else
				sp = ep;
			if (*sp == '\0' || pos == maxpos)
				break;
			sp += dlen;
			if ((ep = strstr(sp, dchar)) == NULL)
				ep = strchr(sp, '\0');
		}
		if (autostop)
			puts(sp);
		else
			putchar('\n');
	}
}

void
usage(void)
{
	(void)fprintf(stderr,
	    "usage: cut -b list [-n] [file ...]\n"
	    "       cut -c list [file ...]\n"
	    "       cut -f list [-s] [-d delim] [file ...]\n");
	exit(10);  /* amiport: exit code 1 → 10 (RETURN_ERROR) */
}
