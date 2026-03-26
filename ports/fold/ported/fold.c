/*	$OpenBSD: fold.c,v 1.18 2016/05/23 10:31:42 schwarze Exp $	*/
/*	$NetBSD: fold.c,v 1.6 1995/09/01 01:42:44 jtc Exp $	*/

/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Kevin Ruddy.
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
static const char *verstag __attribute__((used)) = "$VER: fold 1.18 (26.03.2026)";

/* amiport: stack cookie -- Amiga default 4KB is too small */
long __stack = 16384;

#include <ctype.h>
/* amiport: replaced <err.h> with amiport shim */
#include <amiport/err.h>
#include <limits.h>
/* amiport: removed <locale.h> -- setlocale() stub is in <amiport/unistd.h> */
#include <stdio.h>
/* amiport: replaced <stdlib.h> with amiport shim -- activates amiport_exit() */
#include <amiport/stdlib.h>
#include <string.h>
/* amiport: added <amiport/string.h> for reallocarray() */
#include <amiport/string.h>
/* amiport: replaced <unistd.h> with amiport shim */
#include <amiport/unistd.h>
/* amiport: added <amiport/getopt.h> -- provides getopt/optarg/optind macros */
#include <amiport/getopt.h>
/* amiport: removed <wchar.h> -- no wchar support on AmigaOS 3.x */
/* amiport: mbtowc/wcwidth/wchar_t paths guarded with #ifndef __AMIGA__ below */

/* amiport: added <amiport/glob.h> for amiport_expand_argv/amiport_free_argv */
#include <amiport/glob.h>

/* amiport: pledge/unveil not available on AmigaOS -- stubbed */
#define pledge(p, e) (0)
#define unveil(p, f) (0)

#define	DEFLINEWIDTH	80

static void fold(unsigned int);
static int isu8cont(unsigned char);
/* amiport: replaced OpenBSD __dead with __attribute__((noreturn)) */
static void usage(void) __attribute__((noreturn));

int count_bytes = 0;
int split_words = 0;

/* amiport: cleanup function for atexit -- frees argv expansion, flushes stdout */
static void
cleanup(void)
{
	amiport_free_argv();
	(void)fflush(stdout);
}

int
main(int argc, char *argv[])
{
	int ch, lastch, newarg, prevoptind;
	unsigned int width;
	const char *errstr;

	/* amiport: expand wildcard args -- Amiga shell does not glob */
	amiport_expand_argv(&argc, &argv);
	/* amiport: register cleanup for all exit paths including err()/errx() */
	atexit(cleanup);

	/* amiport: setlocale is a no-op stub via amiport/unistd.h */
	setlocale(LC_CTYPE, "");

	/* amiport: pledge stubbed as macro returning 0 */
	if (pledge("stdio rpath", NULL) == -1)
		err(10, "pledge"); /* amiport: RETURN_ERROR */

	width = 0;
	lastch = '\0';
	prevoptind = 1;
	newarg = 1;
	while ((ch = getopt(argc, argv, "0123456789bsw:")) != -1) {
		switch (ch) {
		case 'b':
			count_bytes = 1;
			break;
		case 's':
			split_words = 1;
			break;
		case 'w':
			width = strtonum(optarg, 1, UINT_MAX, &errstr);
			if (errstr != NULL)
				errx(10, "illegal width value, %s: %s", errstr,
					optarg); /* amiport: RETURN_ERROR */
			break;
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			if (newarg)
				width = 0;
			else if (!isdigit(lastch))
				usage();
			if (width > UINT_MAX / 10 - 1)
				errx(10, "illegal width value, too large"); /* amiport: RETURN_ERROR */
			width = (width * 10) + (ch - '0');
			if (width < 1)
				errx(10, "illegal width value, too small"); /* amiport: RETURN_ERROR */
			break;
		default:
			usage();
		}
		lastch = ch;
		newarg = optind != prevoptind;
		prevoptind = optind;
	}
	argv += optind;
	argc -= optind;

	if (width == 0)
		width = DEFLINEWIDTH;

	if (!*argv) {
		/* amiport: pledge stubbed as macro returning 0 */
		if (pledge("stdio", NULL) == -1)
			err(10, "pledge"); /* amiport: RETURN_ERROR */
		fold(width);
	} else {
		for (; *argv; ++argv) {
			if (!freopen(*argv, "r", stdin)) /* nosemgrep: cpp.lang.security.filesystem.path-manipulation.path-manipulation -- CLI tool, argv paths supplied by invoking user */
				err(10, "%s", *argv); /* amiport: RETURN_ERROR */
			else
				fold(width);
		}
	}
	return 0;
}

/*
 * Fold the contents of standard input to fit within WIDTH columns
 * (or bytes) and write to standard output.
 *
 * If split_words is set, split the line at the last space character
 * on the line.  This flag necessitates storing the line in a buffer
 * until the current column > width, or a newline or EOF is read.
 *
 * The buffer can grow larger than WIDTH due to backspaces and carriage
 * returns embedded in the input stream.
 */
static void
fold(unsigned int max_width)
{
	static char	*buf = NULL;
	static size_t	 bufsz = 2048;
	char		*cp;	/* Current mb character. */
	char		*np;	/* Next mb character. */
	char		*sp;	/* To search for the last space. */
	char		*nbuf;	/* For buffer reallocation. */
#ifndef __AMIGA__
	/* amiport: wchar_t not available without <wchar.h> on AmigaOS */
	wchar_t		 wc;	/* Current wide character. */
#endif
	int		 ch;	/* Last byte read. */
	int		 len;	/* Bytes in the current mb character. */
	unsigned int	 col;	/* Current display position. */
	int		 width; /* Display width of wc. */

	if (buf == NULL && (buf = malloc(bufsz)) == NULL)
		err(10, NULL); /* amiport: RETURN_ERROR */

	np = cp = buf;
	ch = 0;
	col = 0;

	while (ch != EOF) {  /* Loop on input characters. */
		while ((ch = getchar()) != EOF) {  /* Loop on input bytes. */
			if (np + 1 == buf + bufsz) {
				/* amiport: reallocarray via <amiport/string.h> */
				nbuf = reallocarray(buf, 2, bufsz);
				if (nbuf == NULL)
					err(10, NULL); /* amiport: RETURN_ERROR */
				bufsz *= 2;
				cp = nbuf + (cp - buf);
				np = nbuf + (np - buf);
				buf = nbuf;
			}
			*np++ = ch;

			/*
			 * Read up to and including the first byte of
			 * the next character, such that we are sure
			 * to have a complete character in the buffer.
			 * There is no need to read more than five bytes
			 * ahead, since UTF-8 characters are four bytes
			 * long at most.
			 */

			if (np - cp > 4 || (np - cp > 1 && !isu8cont(ch)))
				break;
		}

		while (cp < np) {  /* Loop on output characters. */

			/* Handle end of line and backspace. */

			if (*cp == '\n' || (*cp == '\r' && !count_bytes)) {
				fwrite(buf, 1, ++cp - buf, stdout);
				memmove(buf, cp, np - cp);
				np = buf + (np - cp);
				cp = buf;
				col = 0;
				continue;
			}
			if (*cp == '\b' && !count_bytes) {
				if (col)
					col--;
				cp++;
				continue;
			}

			/*
			 * Measure display width.
			 * Process the last byte only if
			 * end of file was reached.
			 */

			if (np - cp > (ch != EOF)) {
				len = 1;
				width = 1;

				if (*cp == '\t') {
					if (count_bytes == 0)
						width = 8 - (col & 7);
#ifndef __AMIGA__
				/* amiport: mbtowc/wcwidth not available on AmigaOS 3.x --
				 * isu8cont() always returns 0 on Amiga (MB_CUR_MAX guarded),
				 * so multibyte characters are treated as single bytes (len=1) */
				} else if ((len = mbtowc(&wc, cp,
				    np - cp)) < 1)
					len = 1;
				else if (count_bytes)
					width = len;
				else if ((width = wcwidth(wc)) < 0)
					width = 1;
#else
				} else {
					/* amiport: no multibyte support -- treat as single byte */
					len = 1;
					width = 1;
				}
#endif

				col += width;
				if (col <= max_width || cp == buf) {
					cp += len;
					continue;
				}
			}

			/* Line break required. */

			if (col > max_width) {
				if (split_words) {
					for (sp = cp; sp > buf; sp--) {
						if (sp[-1] == ' ') {
							cp = sp;
							break;
						}
					}
				}
				fwrite(buf, 1, cp - buf, stdout);
				putchar('\n');
				memmove(buf, cp, np - cp);
				np = buf + (np - cp);
				cp = buf;
				col = 0;
				continue;
			}

			/* Need more input. */

			break;
		}
	}
	fwrite(buf, 1, np - buf, stdout);

	if (ferror(stdin))
		err(10, NULL); /* amiport: RETURN_ERROR */
}

static int
isu8cont(unsigned char c)
{
	/* amiport: MB_CUR_MAX is __locale_mb_cur_max() on libnix -- may return >1
	 * even without locale support. Guard to prevent entering multibyte path
	 * when wchar functions are unavailable. See known-pitfalls.md. */
#ifndef __AMIGA__
	return MB_CUR_MAX > 1 && (c & (0x80 | 0x40)) == 0x80;
#else
	(void)c;
	return 0;
#endif
}

/* amiport: replaced __dead with __attribute__((noreturn)) */
static void
usage(void)
{
	(void)fprintf(stderr, "usage: fold [-bs] [-w width] [file ...]\n");
	exit(10); /* amiport: RETURN_ERROR */
}
