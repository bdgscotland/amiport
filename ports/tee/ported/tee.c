/*	$OpenBSD: tee.c,v 1.15 2023/03/04 00:00:25 cheloha Exp $	*/
/*	$NetBSD: tee.c,v 1.5 1994/12/09 01:43:39 jtc Exp $	*/

/*
 * Copyright (c) 1988, 1993
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

/* amiport: removed <sys/types.h> — not needed on AmigaOS */
/* amiport: removed <sys/stat.h> — not needed; DEFFILEMODE defined below */
#include <sys/queue.h>

/* amiport: replaced <err.h> with amiport/err.h — BSD err/warn family */
#include <amiport/err.h>
#include <errno.h>
/* amiport: <fcntl.h> O_* constants provided by amiport/unistd.h */
/* amiport: replaced <signal.h> with amiport/signal.h — SIGINT/SIG_IGN via macros */
#include <amiport/signal.h>
#include <stdio.h>
/* amiport: replaced <stdlib.h> with amiport/stdlib.h — activates amiport_exit() macro */
#include <amiport/stdlib.h>
/* amiport: _exit() declaration — not in libnix stdlib.h under -noixemul */
void _exit(int status);
#include <string.h>
/* amiport: replaced <unistd.h> with amiport/unistd.h — open/close/read/write/ssize_t */
#include <amiport/unistd.h>
/* amiport: added amiport/getopt.h — getopt not in libnix */
#include <amiport/getopt.h>
/* amiport: added amiport/glob.h — argv wildcard expansion for AmigaOS shell */
#include <amiport/glob.h>

/* amiport: pledge/unveil not available on AmigaOS — stubbed as macros */
#define pledge(p, e) (0)

/* amiport: DEFFILEMODE — POSIX 0666 creation mask; unused by amiport_open() */
#define DEFFILEMODE 0666

/* amiport: Amiga version string — upstream OpenBSD tee v1.15 */
static const char *verstag = "$VER: tee 1.15 (22.03.2026)";

/* amiport: stack cookie — Amiga default 4KB is too small for ported software */
long __stack = 32768;

/* amiport: reduced from 64KB to 8KB — 64KB wastes 12.5% of A500 Chip RAM;
 * AmigaDOS Read() returns when data is available, not when buffer is full,
 * and FFS/OFS cache at 512-byte blocks, so >4KB yields no throughput gain */
#define BSIZE (8 * 1024)

struct list {
	SLIST_ENTRY(list) next;
	int fd;
	char *name;
};
SLIST_HEAD(, list) head;

/* amiport: changed from void to int — caller handles malloc failure cleanup */
static int
add(int fd, char *name)
{
	struct list *p;

	if ((p = malloc(sizeof(*p))) == NULL)
		return -1;
	p->fd = fd;
	p->name = name;
	SLIST_INSERT_HEAD(&head, p, next);
	return 0;
}

int
main(int argc, char *argv[])
{
	struct list *p;
	struct list *tmp;
	int fd;
	ssize_t n, rval, wval;
	int append, ch, exitval;
	char *buf;

	/* amiport: expand wildcard args — Amiga shell doesn't glob like Unix */
	amiport_expand_argv(&argc, &argv);

	/* amiport: pledge() stubbed — no-op on AmigaOS */
	if (pledge("stdio wpath cpath", NULL) == -1)
		/* amiport: RETURN_ERROR — visible to Amiga IF ERROR scripts */
		err(10, "pledge");

	SLIST_INIT(&head);

	append = 0;
	/* amiport: getopt() provided by amiport/getopt.h via macro */
	while ((ch = getopt(argc, argv, "ai")) != -1) {
		switch(ch) {
		case 'a':
			append = 1;
			break;
		case 'i':
			/* amiport: signal()/SIG_IGN provided by amiport/signal.h via macros */
			(void)signal(SIGINT, SIG_IGN);
			break;
		default:
			(void)fprintf(stderr, "usage: tee [-ai] [file ...]\n");
			/* amiport: RETURN_ERROR — visible to Amiga IF ERROR scripts */
			amiport_free_argv();
			return 10;
		}
	}
	argv += optind;
	argc -= optind;

	/* amiport: handle add() failure — close fds and free memory on OOM */
	if (add(STDOUT_FILENO, "stdout") == -1) {
		warn("malloc");
		amiport_free_argv();
		(void)fflush(stdout);
		_exit(10);
	}

	exitval = 0;
	while (*argv) {
		/*
		 * amiport: replaced open() with amiport_open() — 2-arg form only.
		 * DEFFILEMODE (0666) creation mask dropped: AmigaOS does not use
		 * POSIX permission bits; protection bits are set separately.
		 */
		if ((fd = amiport_open(*argv,
		    O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC))) == -1) {
			warn("%s", *argv);
			/* amiport: RETURN_ERROR — visible to Amiga IF ERROR scripts */
			exitval = 10;
		} else if (add(fd, *argv) == -1) {
			/* amiport: close leaked fd on malloc failure */
			warn("malloc");
			amiport_close(fd);
			exitval = 10;
		}
		argv++;
	}

	/* amiport: pledge() stubbed — no-op on AmigaOS */
	if (pledge("stdio", NULL) == -1)
		/* amiport: RETURN_ERROR — visible to Amiga IF ERROR scripts */
		err(10, "pledge");

	buf = malloc(BSIZE);
	if (buf == NULL)
		/* amiport: RETURN_ERROR — visible to Amiga IF ERROR scripts */
		err(10, NULL);
	/* amiport: replaced read() with amiport_read() — maps to AmigaDOS Read() */
	while ((rval = amiport_read(STDIN_FILENO, buf, BSIZE)) != 0 && rval != -1) {
		/* amiport: check Ctrl-C between reads — AmigaOS has no SIGINT delivery */
		if (amiport_check_break()) {
			(void)fflush(stdout);
			break;
		}
		SLIST_FOREACH(p, &head, next) {
			for (n = 0; n < rval; n += wval) {
				/* amiport: replaced write() with amiport_write() — maps to AmigaDOS Write() */
				wval = amiport_write(p->fd, buf + n, rval - n);
				if (wval == -1) {
					warn("%s", p->name);
					/* amiport: RETURN_ERROR — visible to Amiga IF ERROR scripts */
					exitval = 10;
					break;
				}
			}
		}
	}
	free(buf);
	if (rval == -1) {
		warn("read");
		/* amiport: RETURN_ERROR — visible to Amiga IF ERROR scripts */
		exitval = 10;
	}

	SLIST_FOREACH(p, &head, next) {
		/* amiport: replaced close() with amiport_close() — maps to AmigaDOS Close() */
		if (amiport_close(p->fd) == -1) {
			warn("%s", p->name);
			/* amiport: RETURN_ERROR — visible to Amiga IF ERROR scripts */
			exitval = 10;
		}
	}

	/*
	 * amiport: free linked list nodes before exit — AmigaOS has no automatic
	 * process memory cleanup with -noixemul; all allocations must be freed.
	 */
	while (!SLIST_EMPTY(&head)) {
		tmp = SLIST_FIRST(&head);
		SLIST_REMOVE_HEAD(&head, next);
		free(tmp);
	}

	/* amiport: free expanded argv before exit */
	amiport_free_argv();

	/* amiport: fflush + _exit to avoid libnix exit() hang (crash-patterns #9) */
	(void)fflush(stdout);
	_exit(exitval);

	return exitval; /* unreachable — satisfies C89 compiler */
}
