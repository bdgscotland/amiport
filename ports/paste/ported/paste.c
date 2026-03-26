/*	$OpenBSD: paste.c,v 1.27 2022/12/04 23:50:49 cheloha Exp $	*/

/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Adam S. Moskowitz of Menlo Consulting.
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

/* amiport: sys/queue.h not available on AmigaOS -- inline SIMPLEQ macros */
/* SIMPLEQ (singly-linked tail queue) -- minimal subset used by paste */
#define SIMPLEQ_HEAD(name, type) \
    struct name { struct type *sqh_first; struct type **sqh_last; }
#define SIMPLEQ_HEAD_INITIALIZER(head) \
    { NULL, &(head).sqh_first }
#define SIMPLEQ_ENTRY(type) \
    struct { struct type *sqe_next; }
/* amiport: head is passed as pointer (&head) -- use -> not . */
#define SIMPLEQ_FOREACH(var, head, field) \
    for ((var) = (head)->sqh_first; (var); (var) = (var)->field.sqe_next)
#define SIMPLEQ_INSERT_TAIL(head, elm, field) do { \
    (elm)->field.sqe_next = NULL; \
    *(head)->sqh_last = (elm); \
    (head)->sqh_last = &(elm)->field.sqe_next; \
} while (0)

/* amiport: __dead maps to GCC noreturn attribute */
#define __dead __attribute__((__noreturn__))

/* amiport: pledge/unveil not available on AmigaOS -- stubbed */
#define pledge(p, e) (0)

/* amiport: sys/types.h included via exec/types.h -- keep for ssize_t compat */
#include <sys/types.h>
/* amiport: replaced <err.h> with <amiport/err.h> */
#include <amiport/err.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
/* amiport: replaced <stdlib.h> with <amiport/stdlib.h> -- activates exit() shim */
#include <amiport/stdlib.h>
#include <string.h>
/* amiport: replaced <unistd.h> with <amiport/unistd.h> */
#include <amiport/unistd.h>
/* amiport: replaced <getopt.h> with <amiport/getopt.h> -- libnix getopt broken */
#include <amiport/getopt.h>
/* amiport: <amiport/stdio_ext.h> provides getline() via amiport_getline() */
#include <amiport/stdio_ext.h>
/* amiport: <amiport/glob.h> provides amiport_expand_argv() and __progname */
#include <amiport/glob.h>

/* amiport: AmigaOS version string -- __attribute__((used)) prevents linker removal */
static const char *verstag __attribute__((used)) = "$VER: paste 1.27 (26.03.2026)";

/* amiport: stack cookie -- Amiga default 4KB is too small */
long __stack = 8192;

char *delim;
int delimcnt;

int	tr(char *);
__dead void usage(void);
void	parallel(char **);
void	sequential(char **);

/* amiport: atexit cleanup -- frees expanded argv on all exit paths */
static void
cleanup(void)
{
	amiport_free_argv();
	(void)fflush(stdout);
}

int
main(int argc, char *argv[])
{
	extern char *optarg;
	extern int optind;
	int ch, seq;

	/* amiport: expand wildcard args -- Amiga shell doesn't glob */
	amiport_expand_argv(&argc, &argv);
	/* amiport: register cleanup for all exit paths including err()/errx() */
	atexit(cleanup);

	/* amiport: pledge() stubbed to (0) -- no-op on AmigaOS */
	if (pledge("stdio rpath", NULL) == -1)
		err(10, "pledge"); /* amiport: RETURN_ERROR */

	seq = 0;
	while ((ch = getopt(argc, argv, "d:s")) != -1) {
		switch (ch) {
		case 'd':
			delimcnt = tr(delim = optarg);
			break;
		case 's':
			seq = 1;
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc == 0)
		usage();

	if (delim == NULL) {
		delimcnt = 1;
		delim = "\t";
	}

	if (seq)
		sequential(argv);
	else
		parallel(argv);
	return 0;
}

struct list {
	SIMPLEQ_ENTRY(list) entries;
	FILE *fp;
	int cnt;
	char *name;
};

void
parallel(char **argv)
{
	SIMPLEQ_HEAD(, list) head = SIMPLEQ_HEAD_INITIALIZER(head);
	struct list *lp;
	char *line, *p;
	size_t linesize;
	/* amiport: amiport_getline() returns long, not ssize_t */
	long len;
	int cnt;
	int opencnt, output;
	char ch;

	for (cnt = 0; (p = *argv) != NULL; ++argv, ++cnt) {
		if ((lp = malloc(sizeof(*lp))) == NULL)
			err(10, NULL); /* amiport: RETURN_ERROR */

		if (p[0] == '-' && p[1] == '\0')
			lp->fp = stdin;
		else if ((lp->fp = fopen(p, "r")) == NULL)
			err(10, "%s", p); /* amiport: RETURN_ERROR */
		lp->cnt = cnt;
		lp->name = p;
		SIMPLEQ_INSERT_TAIL(&head, lp, entries);
	}

	line = NULL;
	linesize = 0;

	for (opencnt = cnt; opencnt;) {
		output = 0;
		SIMPLEQ_FOREACH(lp, &head, entries) {
			if (lp->fp == NULL) {
				if (output && lp->cnt &&
				    (ch = delim[(lp->cnt - 1) % delimcnt]))
					putchar(ch);
				continue;
			}
			if ((len = getline(&line, &linesize, lp->fp)) == -1) {
				if (ferror(lp->fp))
					err(10, "%s", lp->fp == stdin ? /* amiport: RETURN_ERROR */
					    "getline" : lp->name);
				if (--opencnt == 0)
					break;
				/* amiport: guard fclose -- never close stdin (known-pitfalls) */
				if (lp->fp != stdin)
					fclose(lp->fp);
				lp->fp = NULL;
				if (output && lp->cnt &&
				    (ch = delim[(lp->cnt - 1) % delimcnt]))
					putchar(ch);
				continue;
			}
			if (line[len - 1] == '\n')
				line[len - 1] = '\0';
			/*
			 * make sure that we don't print any delimiters
			 * unless there's a non-empty file.
			 */
			if (!output) {
				output = 1;
				for (cnt = 0; cnt < lp->cnt; ++cnt)
					if ((ch = delim[cnt % delimcnt]))
						putchar(ch);
			} else if ((ch = delim[(lp->cnt - 1) % delimcnt]))
				putchar(ch);
			fputs(line, stdout);
		}
		if (output)
			putchar('\n');
	}
	free(line);
}

void
sequential(char **argv)
{
	FILE *fp;
	char *line, *p;
	size_t linesize;
	/* amiport: amiport_getline() returns long, not ssize_t */
	long len;
	int cnt;

	line = NULL;
	linesize = 0;
	for (; (p = *argv) != NULL; ++argv) {
		if (p[0] == '-' && p[1] == '\0')
			fp = stdin;
		else if ((fp = fopen(p, "r")) == NULL) {
			warn("%s", p);
			continue;
		}
		cnt = -1;
		while ((len = getline(&line, &linesize, fp)) != -1) {
			if (line[len - 1] == '\n')
				line[len - 1] = '\0';
			if (cnt >= 0)
				putchar(delim[cnt]);
			if (++cnt == delimcnt)
				cnt = 0;
			fputs(line, stdout);
		}
		if (ferror(fp))
			err(10, "%s", fp == stdin ? "getline" : p); /* amiport: RETURN_ERROR */
		if (cnt >= 0)
			putchar('\n');
		/* amiport: guard fclose -- never close stdin (known-pitfalls) */
		if (fp != stdin)
			fclose(fp);
	}
	free(line);
}

int
tr(char *arg)
{
	int cnt;
	char ch, *p;

	for (p = arg, cnt = 0; (ch = *p++) != '\0'; ++arg, ++cnt) {
		if (ch == '\\') {
			switch (ch = *p++) {
			case 'n':
				*arg = '\n';
				break;
			case 't':
				*arg = '\t';
				break;
			case '0':
				*arg = '\0';
				break;
			default:
				*arg = ch;
				break;
			}
		} else
			*arg = ch;
	}

	if (cnt == 0)
		errx(10, "no delimiters specified"); /* amiport: RETURN_ERROR */
	return cnt;
}

__dead void
usage(void)
{
	extern char *__progname;
	fprintf(stderr, "usage: %s [-s] [-d list] file ...\n", __progname);
	exit(10); /* amiport: RETURN_ERROR */
}
