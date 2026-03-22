/*	$OpenBSD: head.c,v 1.24 2022/02/07 17:19:57 cheloha Exp $	*/

/*
 * Copyright (c) 1980, 1987 Regents of the University of California.
 * All rights reserved.
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
#include <string.h>           /* perf: strlen() used in fgets/fputs line loop */
#include <amiport/stdlib.h>   /* amiport: replaced <stdlib.h> — activates exit() macro */
#include <ctype.h>
#include <limits.h>
#include <amiport/err.h>      /* amiport: replaced <err.h> — provides err/errx/warn/strtonum */
#include <errno.h>
#include <amiport/getopt.h>   /* amiport: replaced <unistd.h> — provides getopt, optarg, optind */
#include <amiport/glob.h>     /* amiport: argv wildcard expansion for AmigaOS shell */

/* amiport: pledge/unveil not available on AmigaOS — stubbed as macros per known-pitfalls.md */
#define pledge(p, e) (0)

/* amiport: Amiga version string — upstream OpenBSD revision 1.24 */
static const char *verstag = "$VER: head 1.24 (22.03.2026)";

/* amiport: debug-agent — increased from 16384 to 65536.
 * FS-UAE crash: ACPU_LineF (0x8000000B) on real AmigaOS.
 * Real AmigaOS call chains are deeper than vamos (console.device,
 * filesystem handler, amiport shim layers). With buf[4096] on the
 * stack plus DOS/console call depth, 16KB was marginal on real HW.
 * 64KB gives safe headroom (crash-patterns #3). */
long __stack = 65536;

int head_file(const char *, long, int);
static void usage(void);
static void cleanup(void);

/*
 * head - give the first few lines of a stream or of each of a set of files
 *
 * Bill Joy UCB August 24, 1977
 */

int
main(int argc, char *argv[])
{
	const char *errstr;
	int	ch;
	long	linecnt = 10;
	int	status = 0;

	/* amiport: expand wildcard args — Amiga shell doesn't glob */
	amiport_expand_argv(&argc, &argv);

	/* amiport: register cleanup so err()/errx()/exit() free expanded argv */
	atexit(cleanup);

	if (pledge("stdio rpath", NULL) == -1)
		err(10, "pledge"); /* amiport: RETURN_ERROR */

	/* handle obsolete -number syntax */
	if (argc > 1 && argv[1][0] == '-' &&
	    isdigit((unsigned char)argv[1][1])) {
		linecnt = (long)strtonum(argv[1] + 1, 1, LONG_MAX, &errstr); /* amiport: cast to long */
		if (errstr != NULL)
			errx(10, "count is %s: %s", errstr, argv[1] + 1); /* amiport: RETURN_ERROR */
		argc--;
		argv++;
	}

	while ((ch = getopt(argc, argv, "n:")) != -1) {
		switch (ch) {
		case 'n':
			linecnt = (long)strtonum(optarg, 1, LONG_MAX, &errstr); /* amiport: cast to long */
			if (errstr != NULL)
				errx(10, "count is %s: %s", errstr, optarg); /* amiport: RETURN_ERROR */
			break;
		default:
			usage();
		}
	}
	argc -= optind, argv += optind;

	if (argc == 0) {
		if (pledge("stdio", NULL) == -1)
			err(10, "pledge"); /* amiport: RETURN_ERROR */

		status = head_file(NULL, linecnt, 0);
	} else {
		for (; *argv != NULL; argv++)
			status |= head_file(*argv, linecnt, argc > 1);
	}

	return status;
}

int
head_file(const char *path, long count, int need_header)
{
	const char *name;
	FILE *fp;
	int status = 0;
	static int first = 1;
	/* amiport: debug-agent — changed from auto (stack) to static.
	 * FS-UAE crash: ACPU_LineF on real AmigaOS with buf[4096] on stack.
	 * Real AmigaOS DOS+console call chains add ~3KB of hidden stack depth
	 * that vamos does not have (vamos uses simpler code paths). Moving
	 * buf[] to static avoids placing 4096 bytes in the stack frame.
	 * Static is safe here: head_file() is never called recursively. */
	static char buf[4096];
	size_t buflen;

	if (path != NULL) {
		name = path;
		fp = fopen(name, "r");
		if (fp == NULL) {
			warn("%s", name);
			return 10; /* amiport: RETURN_ERROR */
		}
		if (need_header) {
			printf("%s==> %s <==\n", first ? "" : "\n", name);
			if (ferror(stdout))
				err(10, "stdout"); /* amiport: RETURN_ERROR */
			first = 0;
		}
	} else {
		name = "stdin";
		fp = stdin;
	}

	/* perf: replaced per-character getc/putchar loop with fgets+fputs.
	 *
	 * Original loop paid ~3 branch instructions per byte:
	 *   (1) getc EOF check, (2) putchar EOF check, (3) newline + count test.
	 * On 68000 @ 7 MHz with chip RAM that is ~30 cycles per byte — for a
	 * typical 80-char line that is 2400 cycles per line in loop overhead
	 * alone, before any actual I/O.
	 *
	 * fgets() moves the tight byte-scan into libnix's inner loop (no
	 * per-iteration putchar EOF check). fputs() writes the whole line in
	 * one call. Per-line overhead drops to: one fgets, one strlen, one
	 * fputs — roughly 40x fewer branch instructions for 80-char lines.
	 *
	 * Correctness note: fgets stops at '\n' OR at buf-1 bytes. If a line
	 * exceeds the buffer, buf will NOT end with '\n'. We only decrement
	 * count when we see '\n' — so partial lines do not advance the counter.
	 * This correctly handles lines longer than 4095 bytes. */
	while (count > 0 && fgets(buf, (int)sizeof(buf), fp) != NULL) {
		if (fputs(buf, stdout) == EOF)
			err(10, "stdout"); /* amiport: RETURN_ERROR */
		buflen = strlen(buf);
		if (buflen > 0 && buf[buflen - 1] == '\n')
			count--;
	}
	if (ferror(fp)) {
		warn("%s", name);
		status = 10; /* amiport: RETURN_ERROR */
	}

	/* amiport: NEVER fclose(stdin) on AmigaOS — closes the shell's input handle,
	 * crashes the console handler, and can take down Workbench */
	if (fp != stdin)
		fclose(fp);

	return status;
}


/* amiport: atexit cleanup — frees expanded argv on ALL exit paths (err/errx/exit).
 * Without this, every error exit leaks argv memory permanently on AmigaOS. */
static void
cleanup(void)
{
	amiport_free_argv();
	fflush(stdout);
}

static void
usage(void)
{
	fputs("usage: head [-count | -n count] [file ...]\n", stderr);
	exit(10); /* amiport: RETURN_ERROR — visible to Amiga IF ERROR scripts */
}
