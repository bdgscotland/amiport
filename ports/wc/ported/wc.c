/*	$OpenBSD: wc.c,v 1.32 2024/09/11 03:57:14 guenther Exp $	*/

/*
 * Copyright (c) 1980, 1987, 1991, 1993
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

/* amiport: sys/stat.h replaced with amiport shim header */
#include <amiport/sys/stat.h>

/* amiport: removed <fcntl.h> — O_RDONLY covered by <amiport/unistd.h> */
#include <stdio.h>
/* amiport: stdlib.h replaced with amiport shim header — activates exit() → amiport_exit() macro */
#include <amiport/stdlib.h>
/* amiport: removed <locale.h> — setlocale() stub is in <amiport/unistd.h> */
#include <ctype.h>
/* amiport: err.h replaced with amiport shim header */
#include <amiport/err.h>
/* amiport: unistd.h replaced with amiport shim header */
#include <amiport/unistd.h>
/* amiport: removed <util.h> — fmt_scaled() inlined below */
/* amiport: removed <wchar.h> — multibyte path is dead code on AmigaOS (MB_CUR_MAX always 1) */
/* amiport: removed <wctype.h> — iswspace() not needed; multibyte path guarded below */
/* amiport: added <amiport/glob.h> for amiport_expand_argv() / amiport_free_argv() */
#include <amiport/glob.h>
/* amiport: added <amiport/getopt.h> for getopt() / optind / optarg */
#include <amiport/getopt.h>
/* amiport: added <stdint.h> for int64_t */
#include <stdint.h>

/* amiport: Amiga version string — upstream version 1.32 */
static const char *verstag = "$VER: wc 1.32 (22.03.2026)";

/* amiport: stack cookie — Amiga default 4KB is too small */
long __stack = 16384;

/* amiport: pledge/unveil not available on AmigaOS — stubbed as macros */
#define pledge(p, e) (0)
#define unveil(p, f) (0)

/*
 * amiport: fmt_scaled() from OpenBSD util.h — inlined because <util.h> is
 * OpenBSD-specific and not available on AmigaOS.
 */
#define FMT_SCALED_STRSIZE 7
static int
fmt_scaled(long long number, char *result)
{
	const char units[] = " KMGTPE";
	int i = 0;
	double v = (double)number;
	while (v >= 1024.0 && i < 6) {
		v /= 1024.0;
		i++;
	}
	if (i == 0)
		snprintf(result, FMT_SCALED_STRSIZE, "%6lld", number);
	else
		snprintf(result, FMT_SCALED_STRSIZE, "%5.1f%c", v, units[i]);
	return 0;
}

#define	_MAXBSIZE (64 * 1024)

/*
 * amiport: 256-byte whitespace lookup table — replaces isspace() library call
 * in the hot byte-scanning loop. On 68000, a table lookup is 4-8 cycles vs
 * 20-40 cycles for a jsr to isspace(). Estimated 3-5x speedup for doword path.
 */
static unsigned char ws_table[256];

static void
init_ws_table(void)
{
	ws_table[' ']  = 1;
	ws_table['\t'] = 1;
	ws_table['\n'] = 1;
	ws_table['\r'] = 1;
	ws_table['\f'] = 1;
	ws_table['\v'] = 1;
}

int64_t	tlinect, twordct, tcharct;
int	doline, doword, dochar, humanchar, multibyte;
int	rval;
/* amiport: __progname — OpenBSD provides this; define and set from argv[0] */
char *__progname;

/* amiport: argv expansion cleanup — freed via atexit(cleanup) on all exit paths */
static void cleanup(void) { amiport_free_argv(); (void)fflush(stdout); }

static void print_counts(int64_t, int64_t, int64_t, const char *);
static void format_and_print(int64_t);
static void cnt(const char *);

int
main(int argc, char *argv[])
{
	int ch;

	/* amiport: init whitespace lookup table for 68k performance */
	init_ws_table();

	/* amiport: expand wildcard args — Amiga shell doesn't glob */
	amiport_expand_argv(&argc, &argv);
	atexit(cleanup);

	/* amiport: set __progname from argv[0] (OpenBSD provides this automatically) */
	__progname = strrchr(argv[0], '/');
	if (__progname == NULL)
		__progname = strrchr(argv[0], ':');  /* amiport: Amiga volume separator */
	if (__progname != NULL)
		__progname++;
	else
		__progname = argv[0];

	/* amiport: setlocale() is a no-op on AmigaOS — MB_CUR_MAX always 1 */
	/* setlocale(LC_CTYPE, ""); */

	/* amiport: pledge() stubbed as macro above — always returns 0 */
	if (pledge("stdio rpath", NULL) == -1)
		err(10, "pledge"); /* amiport: RETURN_ERROR */

	while ((ch = getopt(argc, argv, "lwchm")) != -1)
		switch(ch) {
		case 'l':
			doline = 1;
			break;
		case 'w':
			doword = 1;
			break;
		case 'm':
			/*
			 * amiport: on AmigaOS, never enable multibyte path.
			 * MB_CUR_MAX is __locale_mb_cur_max() which may return >1
			 * at runtime even without locale support, but the multibyte
			 * code path is compiled out (#ifndef __AMIGA__), so setting
			 * multibyte=1 would skip ALL counting logic.
			 */
#ifndef __AMIGA__
			if (MB_CUR_MAX > 1)
				multibyte = 1;
#endif
			/* FALLTHROUGH */
		case 'c':
			dochar = 1;
			break;
		case 'h':
			humanchar = 1;
			break;
		default:
			fprintf(stderr,
			    "usage: %s [-c | -m] [-hlw] [file ...]\n",
			    __progname);
			return 10; /* amiport: RETURN_ERROR */
		}
	argv += optind;
	argc -= optind;

	/*
	 * wc is unusual in that its flags are on by default, so,
	 * if you don't get any arguments, you have to turn them
	 * all on.
	 */
	if (!doline && !doword && !dochar)
		doline = doword = dochar = 1;

	if (!*argv) {
		cnt(NULL);
	} else {
		int dototal = (argc > 1);

		do {
			cnt(*argv);
		} while(*++argv);

		if (dototal)
			print_counts(tlinect, twordct, tcharct, "total");
	}

	return rval;
}

static void
cnt(const char *path)
{
	static char *buf;
	static size_t bufsz;

	FILE *stream;
	const char *file;
	char *C;
	/* amiport: wchar_t removed — multibyte path is dead code on AmigaOS (MB_CUR_MAX always 1) */
#ifndef __AMIGA__
	wchar_t wc;
#endif
	int gotsp; /* amiport: int not short — 32-bit is natural register width on 68k */
	ssize_t len;
	int64_t linect, wordct, charct;
	struct stat sbuf;
	int fd;

	linect = wordct = charct = 0;
	stream = NULL;
	if (path != NULL) {
		file = path;
		/* amiport: open() macro-remapped to amiport_open() by <amiport/unistd.h> */
		if ((fd = open(file, O_RDONLY)) == -1) {
			warn("%s", file);
			rval = 10; /* amiport: RETURN_ERROR */
			return;
		}
	} else  {
		file = "(stdin)";
		fd = STDIN_FILENO;
	}

	if (!multibyte) {
		if (bufsz < _MAXBSIZE &&
		    (buf = realloc(buf, _MAXBSIZE)) == NULL)
			err(10, NULL); /* amiport: RETURN_ERROR */
		bufsz = _MAXBSIZE; /* amiport: avoid redundant realloc on subsequent cnt() calls */

		/*
		 * According to POSIX, a word is a "maximal string of
		 * characters delimited by whitespace."  Nothing is said
		 * about a character being printing or non-printing.
		 */
		if (doword) {
			gotsp = 1;
			/* amiport: read() macro-remapped to amiport_read() by <amiport/unistd.h> */
			while ((len = read(fd, buf, _MAXBSIZE)) > 0) {
				charct += len;
				for (C = buf; len--; ++C) {
					if (ws_table[(unsigned char)*C]) { /* amiport: lookup table replaces isspace() for 68k perf */
						gotsp = 1;
						if (*C == '\n')
							++linect;
					} else if (gotsp) {
						gotsp = 0;
						++wordct;
					}
				}
			}
			if (len == -1) {
				warn("%s", file);
				rval = 10; /* amiport: RETURN_ERROR */
			}
		}
		/*
		 * Line counting is split out because it's a lot
		 * faster to get lines than to get words, since
		 * the word count requires some logic.
		 */
		else if (doline) {
			/* amiport: read() macro-remapped to amiport_read() by <amiport/unistd.h> */
			while ((len = read(fd, buf, _MAXBSIZE)) > 0) {
				charct += len;
				for (C = buf; len--; ++C)
					if (*C == '\n')
						++linect;
			}
			if (len == -1) {
				warn("%s", file);
				rval = 10; /* amiport: RETURN_ERROR */
			}
		}
		/*
		 * If all we need is the number of characters and
		 * it's a directory or a regular file, just stat
		 * our handle.  We avoid testing for it not being
		 * a special device in case someone adds a new type
		 * of inode.
		 */
		else if (dochar) {
			/* amiport: fstat() macro-remapped to amiport_fstat() by <amiport/sys/stat.h> */
			if (fstat(fd, &sbuf)) {
				warn("%s", file);
				rval = 10; /* amiport: RETURN_ERROR */
			} else {
				if (S_ISREG(sbuf.st_mode) || S_ISDIR(sbuf.st_mode))
					charct = sbuf.st_size;
				else {
					/* amiport: read() macro-remapped to amiport_read() */
					while ((len = read(fd, buf, _MAXBSIZE)) > 0)
						charct += len;
					if (len == -1) {
						warn("%s", file);
						rval = 10; /* amiport: RETURN_ERROR */
					}
				}
			}
		}
	}
#ifndef __AMIGA__
	/*
	 * amiport: multibyte code path — unreachable on AmigaOS because
	 * MB_CUR_MAX is always 1 and multibyte is never set to 1.
	 * Guarded with #ifndef __AMIGA__ to suppress wchar_t / mbtowc /
	 * iswspace compilation errors (no wchar.h / wctype.h on AmigaOS).
	 */
	else {
		if (path == NULL)
			stream = stdin;
		else if ((stream = fdopen(fd, "r")) == NULL) {
			warn("%s", file);
			close(fd);
			rval = 10; /* amiport: RETURN_ERROR */
			return;
		}

		gotsp = 1;
		while ((len = getline(&buf, &bufsz, stream)) > 0) {
			const char *end = buf + len;
			for (C = buf; C < end; C += len) {
				++charct;
				len = mbtowc(&wc, C, MB_CUR_MAX);
				if (len == -1) {
					mbtowc(NULL, NULL,
					    MB_CUR_MAX);
					len = 1;
					wc = L'?';
				} else if (len == 0)
					len = 1;
				if (iswspace(wc)) {
					gotsp = 1;
					if (wc == L'\n')
						++linect;
				} else if (gotsp) {
					gotsp = 0;
					++wordct;
				}
			}
		}
		if (ferror(stream)) {
			warn("%s", file);
			rval = 10; /* amiport: RETURN_ERROR */
		}
	}
#endif /* !__AMIGA__ */

	print_counts(linect, wordct, charct, path);

	/*
	 * Don't bother checking doline, doword, or dochar -- speeds
	 * up the common case
	 */
	tlinect += linect;
	twordct += wordct;
	tcharct += charct;

	/*
	 * amiport: guard close()/fclose() against stdin.
	 * Calling fclose(stdin) on AmigaOS closes the shell's input handle
	 * and can crash the console handler (known-pitfalls: Never fclose(stdin)).
	 * close(STDIN_FILENO) has the same effect — guard both.
	 */
	if (stream == NULL) {
		/* amiport: close() macro-remapped to amiport_close() by <amiport/unistd.h> */
		if (fd != STDIN_FILENO) {
			if (close(fd) != 0) {
				warn("%s", file);
				rval = 10; /* amiport: RETURN_ERROR */
			}
		}
	} else {
		if (stream != stdin) {
			if (fclose(stream) != 0) {
				warn("%s", file);
				rval = 10; /* amiport: RETURN_ERROR */
			}
		}
	}
}

static void
format_and_print(int64_t v)
{
	if (humanchar) {
		char result[FMT_SCALED_STRSIZE];

		/* amiport: fmt_scaled() inlined above — <util.h> not available on AmigaOS */
		fmt_scaled((long long)v, result);
		printf("%7s", result);
	} else {
		printf(" %7lld", v);
	}
}

static void
print_counts(int64_t lines, int64_t words, int64_t chars, const char *name)
{
	if (doline)
		format_and_print(lines);
	if (doword)
		format_and_print(words);
	if (dochar)
		format_and_print(chars);

	if (name)
		printf(" %s\n", name);
	else
		printf("\n");
}
