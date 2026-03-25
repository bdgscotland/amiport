/*	$OpenBSD: rev.c,v 1.16 2022/02/08 17:44:18 cheloha Exp $	*/
/*	$NetBSD: rev.c,v 1.5 1995/09/28 08:49:40 tls Exp $	*/

/*-
 * Copyright (c) 1987, 1992, 1993
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

#include <sys/types.h>

/* amiport: replaced <err.h> with <amiport/err.h> */
#include <amiport/err.h>
#include <errno.h>
#include <locale.h>
#include <stdio.h>
/* amiport: replaced <stdlib.h> with <amiport/stdlib.h> (activates amiport_exit macro) */
#include <amiport/stdlib.h>
#include <string.h>
#include <unistd.h>
/* amiport: added <amiport/stdio_ext.h> for amiport_getline() shim (maps getline -> amiport_getline) */
#include <amiport/stdio_ext.h>

/* amiport: AmigaOS boilerplate -- version string and stack cookie */
static const char *verstag __attribute__((used)) = "$VER: rev 1.16 (25.03.2026)";
long __stack = 16384;

/* amiport: define __progname here to prevent weak symbol stripping by linker (crash-patterns #14) */
char *__progname = "rev";

/* amiport: pledge() is a no-op on AmigaOS -- no kernel sandboxing available */
#define pledge(p, e) (0)

/* amiport: atexit cleanup for getline buffer -- leaked on err() paths without this */
static char *getline_buf = NULL;

static void cleanup(void)
{
	free(getline_buf);
	(void)fflush(stdout);
}

int multibyte;

int isu8cont(unsigned char);
int rev_file(const char *);
void usage(void);

int
main(int argc, char *argv[])
{
	int ch, rval;

	atexit(cleanup);
	setlocale(LC_CTYPE, "");

	/*
	 * amiport: MB_CUR_MAX pitfall (crash-patterns #11) -- on libnix,
	 * MB_CUR_MAX expands to a runtime function call that returns >1 even
	 * without real locale/wchar support. Guard with __AMIGA__ to force
	 * multibyte off; AmigaOS has no UTF-8 support.
	 */
#ifndef __AMIGA__
	multibyte = MB_CUR_MAX > 1;
#else
	multibyte = 0; /* amiport: AmigaOS has no wchar/UTF-8 support -- always single-byte */
#endif

	/* amiport: pledge() is a no-op macro -- err(1,...) replaced with err(10,...) */
	if (pledge("stdio rpath", NULL) == -1)
		err(10, "pledge");

	while ((ch = getopt(argc, argv, "")) != -1)
		switch(ch) {
		default:
			usage();
		}

	argc -= optind;
	argv += optind;

	rval = 0;
	if (argc == 0) {
		/* amiport: pledge() is a no-op macro */
		if (pledge("stdio", NULL) == -1)
			err(10, "pledge");

		rval = rev_file(NULL);
	} else {
		for (; *argv != NULL; argv++)
			rval |= rev_file(*argv);
	}
	return rval;
}

int
isu8cont(unsigned char c)
{
	return (c & (0x80 | 0x40)) == 0x80;
}

int
rev_file(const char *path)
{
	/*
	 * amiport perf: removed multibyte-path locals (t, te, u) -- the
	 * single-byte path only needs lo/hi for the in-place reversal swap.
	 * Fewer locals = better register allocation on 68k with its 8 Dregs.
	 */
	char *p = NULL, *lo, *hi;
	const char *filename;
	FILE *fp;
	size_t ps = 0;
	/* amiport: changed ssize_t to long -- amiport_getline() returns long; ssize_t not available in C89 */
	long len;
	int rval = 0;
	char tmp;

	if (path != NULL) {
		fp = fopen(path, "r");
		if (fp == NULL) {
			warn("%s", path);
			/* amiport: return RETURN_WARN (5) -- visible to AmigaDOS IF WARN scripts */
			return 5;
		}
		filename = path;
	} else {
		fp = stdin;
		filename = "stdin";
	}

	/* amiport: getline() maps to amiport_getline() via macro in <amiport/stdio_ext.h> */
	/* amiport: track buffer pointer for atexit cleanup on err() paths */
	while ((len = getline(&p, &ps, fp)) != -1) {
		getline_buf = p;
		if (p[len - 1] == '\n')
			--len;
#ifndef __AMIGA__
		/*
		 * Multibyte (UTF-8) path: character-at-a-time with UTF-8
		 * continuation byte detection. Only compiled on non-Amiga
		 * targets where multibyte locales are possible.
		 */
		if (multibyte) {
			char *t, *te, *u;
			for (t = p + len - 1; t >= p; --t) {
				te = t;
				while (t > p && isu8cont(*t))
					--t;
				for (u = t; u <= te; ++u)
					if (putchar(*u) == EOF)
						err(10, "stdout");
			}
			if (putchar('\n') == EOF)
				err(10, "stdout");
			continue;
		}
#endif
		/*
		 * amiport perf: single-byte fast path (always taken on AmigaOS
		 * since multibyte=0 and the #ifndef __AMIGA__ above removes
		 * the branch entirely).
		 *
		 * Strategy: reverse the line content in-place with a two-pointer
		 * swap, then emit the whole line with one fwrite() call + one
		 * putchar('\n'). This reduces N putchar() calls (each a JSR +
		 * stack frame + EOF check through libnix) to a single fwrite()
		 * that hands a contiguous buffer to the OS.
		 *
		 * For an 80-char line: 81 putchar() -> 1 fwrite() + 1 putchar().
		 * On 7MHz 68000: each putchar() ~50 cycles JSR overhead.
		 * Saving ~80 * 50 = 4000 cycles per line.
		 */
		lo = p;
		hi = p + len - 1;
		while (lo < hi) {
			tmp = *lo;
			*lo = *hi;
			*hi = tmp;
			++lo;
			--hi;
		}
		/* amiport: err(1,...) -> err(10,...) -- AmigaOS exit code convention */
		if (fwrite(p, 1, (size_t)len, stdout) != (size_t)len ||
		    putchar('\n') == EOF)
			err(10, "stdout");
	}
	free(p);
	getline_buf = NULL;  /* amiport: prevent double-free in atexit cleanup */
	if (ferror(fp)) {
		warn("%s", filename);
		/* amiport: RETURN_WARN (5) -- visible to AmigaDOS IF WARN scripts */
		rval = 5;
	}

	/* amiport: fclose(stdin) pitfall (crash-patterns #19 / known-pitfalls) -- closing stdin
	 * crashes the Workbench console handler. Guard with fp != stdin check. */
	if (fp != stdin)
		(void)fclose(fp);

	return rval;
}

void
usage(void)
{
	/* amiport: removed 'extern char *__progname' -- defined as global above to prevent
	 * weak symbol stripping by bebbo-gcc linker (see known-pitfalls: __progname weak symbol) */
	(void)fprintf(stderr, "usage: %s [file ...]\n", __progname);
	/* amiport: exit(1) -> exit(10) -- AmigaOS RETURN_ERROR convention */
	exit(10);
}
