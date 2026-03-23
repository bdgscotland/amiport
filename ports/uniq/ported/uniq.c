/*	$OpenBSD: uniq.c,v 1.33 2022/01/01 18:20:52 cheloha Exp $	*/
/*	$NetBSD: uniq.c,v 1.7 1995/08/31 22:03:48 jtc Exp $	*/

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Case Larsen.
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
static const char *verstag = "$VER: uniq 1.33 (22.03.2026)";

/* amiport: stack cookie — Amiga default 4KB is too small */
long __stack = 32768;

#include <ctype.h>
/* amiport: replaced <err.h> with <amiport/err.h> — provides err/errx/warn/warnx/strtonum */
#include <amiport/err.h>
#include <limits.h>
/* amiport: removed <locale.h> — setlocale() stub is in <amiport/unistd.h> */
#include <stdio.h>
/* amiport: replaced <stdlib.h> with <amiport/stdlib.h> — activates exit() → amiport_exit() macro */
#include <amiport/stdlib.h>
#include <string.h>
#include <strings.h>
/* amiport: replaced <unistd.h> with <amiport/unistd.h> — provides setlocale stub, file I/O */
#include <amiport/unistd.h>
/* amiport: added <amiport/getopt.h> — provides getopt(), optarg, optind, opterr, optopt */
#include <amiport/getopt.h>
/* amiport: added <amiport/stdio_ext.h> for getline() shim */
#include <amiport/stdio_ext.h>
/* amiport: added <amiport/string.h> for strlcpy() shim */
#include <amiport/string.h>
/* amiport: added <amiport/glob.h> for amiport_expand_argv/free_argv and __progname */
#include <amiport/glob.h>
/* amiport: define __progname locally — weak symbol from libamiport.a does not
 * survive linking (bebbo-gcc strips unreferenced weak data symbols even when
 * the containing object is linked). amiport_expand_argv() writes to this. */
char *__progname = "uniq";
/* amiport: removed <wchar.h> — no wchar support on AmigaOS 3.x; multibyte path guarded below */
/* amiport: removed <wctype.h> — no wctype support on AmigaOS 3.x */

/* amiport: pledge/unveil not available on AmigaOS — stubbed as no-ops */
#define pledge(p, e) (0)
#define unveil(p, f) (0)

/* amiport: __dead may not be defined on all compilers — provide fallback */
#ifndef __dead
#define __dead __attribute__((__noreturn__))
#endif

/* amiport: downgraded from long long to long — 64-bit emulation on 68000 costs
 * two instructions per operation. These values never exceed 32-bit in practice. */
long numchars, numfields;
unsigned long repeats;
int cflag, dflag, iflag, uflag;

void	 show(const char *);
char	*skip(char *);
void	 obsolete(char *[]);
__dead void	usage(void);

/* amiport: track obsolete() malloc'd strings for cleanup — AmigaOS has no
 * automatic process memory cleanup with -noixemul */
#define MAX_OBSOLETE_ALLOCS 4
static char *obsolete_allocs[MAX_OBSOLETE_ALLOCS];
static int obsolete_alloc_count;

/* amiport: cleanup function registered via atexit() — frees expanded argv on all exit paths */
static void
cleanup(void)
{
	int i;
	for (i = 0; i < obsolete_alloc_count; i++)
		free(obsolete_allocs[i]);
	amiport_free_argv();
	(void)fflush(stdout);
}

int
main(int argc, char *argv[])
{
	const char *errstr;
	char *p, *prevline, *t, *thisline, *tmp;
	size_t prevsize, thissize, tmpsize;
	long len; /* amiport: ssize_t → long (getline returns long via shim) */
	int ch;
	/* amiport: hoist comparison function — avoids ternary eval per line in loop */
	int (*cmpfn)(const char *, const char *);

	/* amiport: expand wildcard args — Amiga shell doesn't glob */
	amiport_expand_argv(&argc, &argv);
	/* amiport: register cleanup for all exit paths including err()/errx() */
	atexit(cleanup);

	/* amiport: setlocale() stubbed — always returns "C" on AmigaOS */
	setlocale(LC_CTYPE, "");

	if (pledge("stdio rpath wpath cpath", NULL) == -1)
		err(10, "pledge"); /* amiport: RETURN_ERROR */

	obsolete(argv);
	while ((ch = getopt(argc, argv, "cdf:is:u")) != -1) {
		switch (ch) {
		case 'c':
			cflag = 1;
			break;
		case 'd':
			dflag = 1;
			break;
		case 'f':
			/* amiport: LONG_MAX — numfields is long, not long long */
			numfields = strtonum(optarg, 0, LONG_MAX, &errstr);
			if (errstr)
				errx(10, "fields is %s: %s", errstr, optarg); /* amiport: RETURN_ERROR */
			break;
		case 'i':
			iflag = 1;
			break;
		case 's':
			/* amiport: LONG_MAX — numchars is long, not long long */
			numchars = strtonum(optarg, 0, LONG_MAX, &errstr);
			if (errstr)
				errx(10, "chars is %s: %s", errstr, optarg); /* amiport: RETURN_ERROR */
			break;
		case 'u':
			uflag = 1;
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	/* If neither -d nor -u are set, default is -d -u. */
	if (!dflag && !uflag)
		dflag = uflag = 1;

	/* amiport: set comparison function once, not per-line in the loop */
	cmpfn = iflag ? strcasecmp : strcmp;

	if (argc > 2)
		usage();
	if (argc >= 1 && strcmp(argv[0], "-") != 0) {
		if (freopen(argv[0], "r", stdin) == NULL)
			err(10, "%s", argv[0]); /* amiport: RETURN_ERROR */
	}
	if (argc == 2 && strcmp(argv[1], "-") != 0) {
		if (freopen(argv[1], "w", stdout) == NULL)
			err(10, "%s", argv[1]); /* amiport: RETURN_ERROR */
	}

	if (pledge("stdio", NULL) == -1)
		err(10, "pledge"); /* amiport: RETURN_ERROR */

	prevsize = 0;
	prevline = NULL;
	if ((len = getline(&prevline, &prevsize, stdin)) == -1) {
		free(prevline);
		if (ferror(stdin))
			err(10, "getline"); /* amiport: RETURN_ERROR */
		fflush(stdout);
		exit(0); /* amiport: exit hang debunked — exit() is safe, runs atexit cleanup */
	}
	if (prevline[len - 1] == '\n')
		prevline[len - 1] = '\0';
	if (numfields || numchars)
		p = skip(prevline);
	else
		p = prevline;

	thissize = 0;
	thisline = NULL;
	while ((len = getline(&thisline, &thissize, stdin)) != -1) {
		if (thisline[len - 1] == '\n')
			thisline[len - 1] = '\0';

		/* If requested get the chosen fields + character offsets. */
		if (numfields || numchars)
			t = skip(thisline);
		else
			t = thisline;

		/* If different, print; set previous to new value. */
		if (cmpfn(p, t)) {
			show(prevline);
			tmp = prevline;
			prevline = thisline;
			thisline = tmp;
			tmp = p;
			p = t;
			t = tmp;
			tmpsize = prevsize;
			prevsize = thissize;
			thissize = tmpsize;
			repeats = 0;
		} else
			++repeats;
	}
	free(thisline);
	if (ferror(stdin))
		err(10, "getline"); /* amiport: RETURN_ERROR */

	show(prevline);
	free(prevline);

	fflush(stdout);
	exit(0); /* amiport: exit hang debunked — exit() is safe, runs atexit cleanup */
}

/*
 * show --
 *	Output a line depending on the flags and number of repetitions
 *	of the line.
 */
void
show(const char *str)
{
	if ((dflag && repeats) || (uflag && !repeats)) {
		if (cflag)
			/* amiport: %lu — repeats is unsigned long, not unsigned long long */
			printf("%4lu %s\n", repeats + 1, str);
		else
			printf("%s\n", str);
	}
}

char *
skip(char *str)
{
	long nchars, nfields; /* amiport: downgraded from long long */
	int len;
	int field_started;
	unsigned char uc;

	/*
	 * amiport: replaced multibyte mbtowc/iswblank/mblen path with single-byte
	 * equivalents. AmigaOS is always C locale (single-byte). MB_CUR_MAX is a
	 * runtime function call on libnix that may return >1 even without locale
	 * support, which would enter the (now-removed) wchar code path and crash.
	 * See known-pitfalls.md (MB_CUR_MAX) and crash-patterns.md #11.
	 *
	 * Original wchar_t wc; and mbtowc/iswblank/mblen calls are guarded out:
	 */
#ifndef __AMIGA__
	/* original multibyte path — not compiled on AmigaOS */
#endif

	for (nfields = numfields; nfields && *str; nfields--) {
		/* Skip one field, including preceding blanks. */
		for (field_started = 0; *str != '\0'; str += len) {
			/* amiport: single-byte step; len always 1 on AmigaOS */
			uc = (unsigned char)*str;
			len = 1;
			/* amiport: inlined isblank() — avoids JSR overhead in inner loop on 68000 */
			if (uc == ' ' || uc == '\t') {
				if (field_started)
					break;
			} else
				field_started = 1;
		}
	}

	/* Skip some additional characters. */
	/* amiport: single-byte locale — direct str++ instead of str += len */
	for (nchars = numchars; nchars-- && *str != '\0'; str++)
		;

	return (str);
}

void
obsolete(char *argv[])
{
	size_t len;
	char *ap, *p, *start;

	while ((ap = *++argv)) {
		/* Return if "--" or not an option of any form. */
		if (ap[0] != '-') {
			if (ap[0] != '+')
				return;
		} else if (ap[1] == '-')
			return;
		if (!isdigit((unsigned char)ap[1]))
			continue;
		/*
		 * Digit signifies an old-style option.  Malloc space for dash,
		 * new option and argument.
		 */
		len = strlen(ap) + 3;
		if ((start = p = malloc(len)) == NULL)
			err(10, "malloc"); /* amiport: RETURN_ERROR */
		/* amiport: track for cleanup — AmigaOS leaks permanently without this */
		if (obsolete_alloc_count < MAX_OBSOLETE_ALLOCS)
			obsolete_allocs[obsolete_alloc_count++] = start;
		*p++ = '-';
		*p++ = ap[0] == '+' ? 's' : 'f';
		/* amiport: strlcpy() provided by <amiport/string.h> shim */
		(void)strlcpy(p, ap + 1, len - 2);
		*argv = start;
	}
}

__dead void
usage(void)
{
	fprintf(stderr,
	    "usage: %s [-ci] [-d | -u] [-f fields] [-s chars] [input_file [output_file]]\n",
	    /* amiport: replaced getprogname() with __progname — provided by <amiport/glob.h> */
	    __progname);
	exit(10); /* amiport: RETURN_ERROR — visible to Amiga IF ERROR scripts */
}
