/*	$OpenBSD: cat.c,v 1.34 2022/02/09 01:58:57 cheloha Exp $	*/
/*	$NetBSD: cat.c,v 1.11 1995/09/07 06:12:54 jtc Exp $	*/

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Kevin Fall.
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
static const char *verstag = "$VER: cat 1.34 (26.03.2026)";

/* amiport: stack cookie -- Amiga default 4KB is too small */
long __stack = 8192;

#include <sys/types.h>
/* amiport: stat.h not needed -- fstat/st_blksize removed, BUFSIZ used directly */

#include <ctype.h>
/* amiport: replaced <err.h> with amiport shim */
#include <amiport/err.h>
#include <errno.h>
/* amiport: <fcntl.h> kept -- O_RDONLY etc. also defined in amiport/unistd.h */
#include <fcntl.h>
#include <stdio.h>
/* amiport: replaced <stdlib.h> with amiport shim (activates exit() -> amiport_exit()) */
#include <amiport/stdlib.h>
#include <string.h>
/* amiport: replaced <unistd.h> with amiport shim */
#include <amiport/unistd.h>
/* amiport: getopt shim -- libnix getopt_long is broken (crash-patterns #17) */
#include <amiport/getopt.h>
/* amiport: glob.h provides amiport_expand_argv, amiport_free_argv, __progname */
#include <amiport/glob.h>
/* amiport: utsname.h provides getprogname() macro */
#include <amiport/utsname.h>

/* amiport: pledge/unveil not available on AmigaOS -- stubbed */
#define pledge(p, e) (0)

#define MAXIMUM(a, b)	(((a) > (b)) ? (a) : (b))

int bflag, eflag, nflag, sflag, tflag, vflag;
int rval;

void cat_file(const char *);
void cook_buf(FILE *, const char *);
void raw_cat(int, const char *);

/* amiport: cleanup for atexit -- frees expanded argv and flushes stdout */
static void
cleanup(void)
{
	amiport_free_argv();
	(void)fflush(stdout);
}

int
main(int argc, char *argv[])
{
	int ch;

	/* amiport: expand wildcard args -- Amiga shell doesn't glob */
	amiport_expand_argv(&argc, &argv);
	/* amiport: register cleanup for all exit paths including err()/errx() */
	atexit(cleanup);

	/* amiport: pledge stubbed -- no-op on AmigaOS */
	if (pledge("stdio rpath", NULL) == -1)
		err(10, "pledge"); /* amiport: RETURN_ERROR */

	while ((ch = getopt(argc, argv, "benstuv")) != -1) {
		switch (ch) {
		case 'b':
			bflag = nflag = 1;	/* -b implies -n */
			break;
		case 'e':
			eflag = vflag = 1;	/* -e implies -v */
			break;
		case 'n':
			nflag = 1;
			break;
		case 's':
			sflag = 1;
			break;
		case 't':
			tflag = vflag = 1;	/* -t implies -v */
			break;
		case 'u':
			setvbuf(stdout, NULL, _IONBF, 0);
			break;
		case 'v':
			vflag = 1;
			break;
		default:
			fprintf(stderr, "usage: %s [-benstuv] [file ...]\n",
			    getprogname());
			return 10; /* amiport: RETURN_ERROR */
		}
	}
	argc -= optind;
	argv += optind;

	if (argc == 0) {
		/* amiport: pledge stubbed -- no-op on AmigaOS */
		if (pledge("stdio", NULL) == -1)
			err(10, "pledge"); /* amiport: RETURN_ERROR */

		cat_file(NULL);
	} else {
		for (; *argv != NULL; argv++)
			cat_file(*argv);
	}
	/* amiport: replaced fclose(stdout) with fflush(stdout).
	 * fclose(stdout) closes the shell's output handle on AmigaOS -- dangerous.
	 * fflush is sufficient to detect write errors. */
	if (fflush(stdout) == EOF)
		err(10, "stdout"); /* amiport: RETURN_ERROR */
	return rval;
}

void
cat_file(const char *path)
{
	FILE *fp;
	int fd;

	if (bflag || eflag || nflag || sflag || tflag || vflag) {
		if (path == NULL || strcmp(path, "-") == 0) {
			cook_buf(stdin, "stdin");
			clearerr(stdin);
		} else {
			if ((fp = fopen(path, "r")) == NULL) {
				warn("%s", path);
				rval = 1;
				return;
			}
			cook_buf(fp, path);
			/* amiport: guard fclose -- never close stdin (known-pitfalls) */
			if (fp != stdin) fclose(fp);
		}
	} else {
		if (path == NULL || strcmp(path, "-") == 0) {
			raw_cat(STDIN_FILENO, "stdin");
		} else {
			/* amiport: open() -> amiport_open() via macro in amiport/unistd.h */
			if ((fd = open(path, O_RDONLY)) == -1) {
				warn("%s", path);
				rval = 1;
				return;
			}
			raw_cat(fd, path);
			/* amiport: close() -> amiport_close() via macro in amiport/unistd.h */
			close(fd);
		}
	}
}

void
cook_buf(FILE *fp, const char *filename)
{
	/* amiport: replaced unsigned long long with unsigned long -- no long long
	 * on 68k (C89, 32-bit). Line count wraps at ~4 billion which is fine. */
	unsigned long line;
	int ch, gobble, prev;

	line = gobble = 0;
	for (prev = '\n'; (ch = getc(fp)) != EOF; prev = ch) {
		if (prev == '\n') {
			if (sflag) {
				if (ch == '\n') {
					if (gobble)
						continue;
					gobble = 1;
				} else
					gobble = 0;
			}
			if (nflag) {
				if (!bflag || ch != '\n') {
					/* amiport: %6llu -> %6lu (unsigned long, C89) */
					fprintf(stdout, "%6lu\t", ++line);
					if (ferror(stdout))
						break;
				} else if (eflag) {
					(void)fprintf(stdout, "%6s\t", "");
					if (ferror(stdout))
						break;
				}
			}
		}
		if (ch == '\n') {
			if (eflag && putchar('$') == EOF)
				break;
		} else if (ch == '\t') {
			if (tflag) {
				if (putchar('^') == EOF || putchar('I') == EOF)
					break;
				continue;
			}
		} else if (vflag) {
			if (!isascii(ch)) {
				if (putchar('M') == EOF || putchar('-') == EOF)
					break;
				ch = toascii(ch);
			}
			if (iscntrl(ch)) {
				if (putchar('^') == EOF ||
				    putchar(ch == '\177' ? '?' :
				    ch | 0100) == EOF)
					break;
				continue;
			}
		}
		if (putchar(ch) == EOF)
			break;
	}
	if (ferror(fp)) {
		warn("%s", filename);
		rval = 1;
		clearerr(fp);
	}
	if (ferror(stdout))
		err(10, "stdout"); /* amiport: RETURN_ERROR */
}

void
raw_cat(int rfd, const char *filename)
{
	/* amiport: replaced ssize_t with LONG (32-bit signed, C89 on 68k) */
	LONG nr, nw, off;
	static size_t bsize;
	static char *buf = NULL;
	/* amiport: struct stat not used for st_blksize -- amiport_stat has no
	 * st_blksize field. Buffer sized to BUFSIZ directly (see below). */

	/* amiport: write to STDOUT_FILENO (fd 1 = Output() in amiport fd table).
	 * Do NOT use fileno(stdout) -- amiport_fileno(stdout) returns -1 because
	 * stdout was opened by libnix startup, not by amiport_fopen(). */
	if (buf == NULL) {
		/* amiport: removed fstat(fileno(stdout), ...) for st_blksize.
		 * amiport_stat struct has no st_blksize field. Use BUFSIZ directly. */
		bsize = BUFSIZ;
		if ((buf = malloc(bsize)) == NULL)
			err(10, NULL); /* amiport: RETURN_ERROR */
	}
	/* amiport: read() -> amiport_read(), write() -> amiport_write() via macros */
	while ((nr = read(rfd, buf, bsize)) != -1 && nr != 0) {
		for (off = 0; nr; nr -= nw, off += nw) {
			if ((nw = write(STDOUT_FILENO, buf + off, nr)) == -1 || nw == 0)
				err(10, "stdout"); /* amiport: RETURN_ERROR */
		}
	}
	if (nr == -1) {
		warn("%s", filename);
		rval = 1;
	}
}
