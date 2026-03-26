/*	$OpenBSD: look.c,v 1.27 2022/12/04 23:50:48 cheloha Exp $	*/
/*	$NetBSD: look.c,v 1.7 1995/08/31 22:41:02 jtc Exp $	*/

/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * David Hitz of Auspex Systems, Inc.
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

/*
 * look -- find lines in a sorted list.
 *
 * The man page said that TABs and SPACEs participate in -d comparisons.
 * In fact, they were ignored.  This implements historic practice, not
 * the manual page.
 */

/* amiport: Amiga version string */
static const char *verstag = "$VER: look 1.27-2 (26.03.2026)";

/* amiport: stack cookie -- Amiga default 4KB is too small */
long __stack = 16384;

/* amiport: pledge/unveil not available on AmigaOS -- stubbed */
#define pledge(p, e) (0)

/* amiport: <sys/types.h> removed -- not needed; types provided by headers below */
/* amiport: removed <sys/mman.h> -- no mmap on AmigaOS; using amiport_emu_mmap() */
/* amiport: stat.h macros suppressed -- fcntl.h pulls system stat.h first;
 * use struct amiport_stat and amiport_fstat() explicitly instead. */
#define AMIPORT_NO_STAT_MACROS
#include <amiport/sys/stat.h>       /* amiport: replaced <sys/stat.h> */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
/* amiport: removed <stdint.h> -- C99 header; SIZE_MAX check replaced below */
#include <stdio.h>
#include <amiport/stdlib.h>         /* amiport: replaced <stdlib.h> -- activates amiport_exit() */
#include <string.h>
#include <unistd.h>                 /* amiport: system unistd.h for getopt/optarg/optind */
#include <amiport/unistd.h>         /* amiport: open/close/fstat macros */
#include <amiport/err.h>            /* amiport: err/errx/errc via shim */
#include <amiport/glob.h>           /* amiport: amiport_expand_argv, amiport_free_argv */
#include <amiport-emu/mmap.h>       /* amiport-emu: mmap() emulated via AllocMem+Read */

#include "pathnames.h"

#define	EQUAL		0
#define	GREATER		1
#define	LESS		(-1)

int dflag, fflag;

char	*binary_search(char *, char *, char *);
int	 compare(char *, char *, char *);
char	*linear_search(char *, char *, char *);
int	 look(char *, char *, char *);
void	 print_from(char *, char *, char *);
void	 usage(void);

/* amiport: atexit cleanup -- free expanded argv and flush stdout */
static void
cleanup(void)
{
	amiport_free_argv();
	(void)fflush(stdout);
}

int
main(int argc, char *argv[])
{
	struct amiport_stat sb;                /* amiport: struct stat -> struct amiport_stat */
	int ch, fd, termchar;
	char *back, *file, *front, *string, *p;
	int look_result;

	/* amiport: expand wildcard args -- Amiga shell doesn't glob */
	amiport_expand_argv(&argc, &argv);
	/* amiport: register cleanup for all exit paths including err()/errx() */
	atexit(cleanup);

	/* amiport: pledge() stubbed as macro returning 0 */
	if (pledge("stdio rpath", NULL) == -1)
		err(20, "pledge");                 /* amiport: RETURN_FAIL */

	file = _PATH_WORDS;
	termchar = '\0';
	while ((ch = getopt(argc, argv, "dft:")) != -1)
		switch(ch) {
		case 'd':
			dflag = 1;
			break;
		case 'f':
			fflag = 1;
			break;
		case 't':
			termchar = *optarg;
			break;
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	switch (argc) {
	case 2:				/* Don't set -df for user. */
		string = *argv++;
		file = *argv;
		break;
	case 1:				/* But set -df by default. */
		dflag = fflag = 1;
		string = *argv;
		break;
	default:
		usage();
	}

	if (termchar != '\0' && (p = strchr(string, termchar)) != NULL)
		*++p = '\0';

	/* amiport: open() macro from <amiport/unistd.h>; amiport_fstat() called
	 * directly because AMIPORT_NO_STAT_MACROS suppresses the fstat macro */
	if ((fd = open(file, O_RDONLY)) == -1 || amiport_fstat(fd, &sb) == -1)
		err(20, "%s", file);              /* amiport: RETURN_FAIL */

	/*
	 * amiport: removed SIZE_MAX check (from <stdint.h>, C99).
	 * On 32-bit AmigaOS, st_size is a signed LONG -- any valid file fits.
	 * Guard against negative (overflow) st_size as a sanity check.
	 */
	if (sb.st_size < 0) {
		errc(20, EFBIG, "%s", file);      /* amiport: RETURN_FAIL */
	}

	/* amiport: pledge() stubbed -- second call is a no-op */
	if (pledge("stdio", NULL) == -1)
		err(20, "pledge");                /* amiport: RETURN_FAIL */

	/*
	 * amiport-emu: mmap() emulated via AllocMem()+Read() -- entire file
	 * loaded upfront, no lazy paging, snapshot semantics only.
	 * AMIPORT_EMU_PROT_READ / AMIPORT_EMU_MAP_PRIVATE from <amiport-emu/mmap.h>.
	 * fd is an amiport fd (from amiport_open via open() macro).
	 */
	front = (char *)amiport_emu_mmap(NULL,
	    (unsigned long)sb.st_size,
	    AMIPORT_EMU_PROT_READ, AMIPORT_EMU_MAP_PRIVATE, fd, (long)0);
	if (front == (char *)AMIPORT_EMU_MAP_FAILED)
		err(20, "%s", file);              /* amiport: RETURN_FAIL */

	back = front + sb.st_size;

	look_result = look(string, front, back);

	amiport_emu_munmap((void *)front, (unsigned long)sb.st_size);

	/*
	 * amiport: exit code mapping --
	 *   look() returns 0 (found) or 1 (not found).
	 *   exit(0) stays 0 (RETURN_OK).
	 *   exit(1) -> exit(5) (RETURN_WARN) so Amiga scripts can test
	 *   "IF WARN" for no-match vs "IF ERROR" for real errors.
	 */
	if (look_result == 0)
		exit(0);
	else
		exit(5);                          /* amiport: RETURN_WARN -- no match found */
}

int
look(char *string, char *front, char *back)
{
	int ch;
	char *readp, *writep;

	/* Reformat string to avoid doing it multiple times later. */
	for (readp = writep = string; (ch = *readp++);) {
		if (fflag)
			ch = tolower((unsigned char)ch);
		if (!dflag || isalnum((unsigned char)ch))
			*(writep++) = ch;
	}
	*writep = '\0';

	front = binary_search(string, front, back);
	front = linear_search(string, front, back);

	if (front)
		print_from(string, front, back);
	return (front ? 0 : 1);
}


/*
 * Binary search for "string" in memory between "front" and "back".
 *
 * This routine is expected to return a pointer to the start of a line at
 * *or before* the first word matching "string".  Relaxing the constraint
 * this way simplifies the algorithm.
 *
 * Invariants:
 *	front points to the beginning of a line at or before the first
 *	matching string.
 *
 *	back points to the beginning of a line at or after the first
 *	matching line.
 *
 * Base of the Invariants.
 *	front = NULL;
 *	back = EOF;
 *
 * Advancing the Invariants:
 *
 *	p = first newline after halfway point from front to back.
 *
 *	If the string at "p" is not greater than the string to match,
 *	p is the new front.  Otherwise it is the new back.
 *
 * Termination:
 *
 *	The definition of the routine allows it return at any point,
 *	since front is always at or before the line to print.
 *
 *	In fact, it returns when the chosen "p" equals "back".  This
 *	implies that there exists a string is least half as long as
 *	(back - front), which in turn implies that a linear search will
 *	be no more expensive than the cost of simply printing a string or two.
 *
 *	Trying to continue with binary search at this point would be
 *	more trouble than it's worth.
 */
#define	SKIP_PAST_NEWLINE(p, back) \
	while (p < back && *p++ != '\n');

char *
binary_search(char *string, char *front, char *back)
{
	char *p;

	p = front + (back - front) / 2;
	SKIP_PAST_NEWLINE(p, back);

	/*
	 * If the file changes underneath us, make sure we don't
	 * infinitely loop.
	 */
	while (p < back && back > front) {
		if (compare(string, p, back) == GREATER)
			front = p;
		else
			back = p;
		p = front + (back - front) / 2;
		SKIP_PAST_NEWLINE(p, back);
	}
	return (front);
}

/*
 * Find the first line that starts with string, linearly searching from front
 * to back.
 *
 * Return NULL for no such line.
 *
 * This routine assumes:
 *
 *	o front points at the first character in a line.
 *	o front is before or at the first line to be printed.
 */
char *
linear_search(char *string, char *front, char *back)
{
	while (front < back) {
		switch (compare(string, front, back)) {
		case EQUAL:		/* Found it. */
			return (front);
			break;
		case LESS:		/* No such string. */
			return (NULL);
			break;
		case GREATER:		/* Keep going. */
			break;
		}
		SKIP_PAST_NEWLINE(front, back);
	}
	return (NULL);
}

/*
 * Print as many lines as match string, starting at front.
 */
void
print_from(char *string, char *front, char *back)
{
	/* amiport: perf -- fwrite whole lines instead of putchar per byte (10-30x on 68k) */
	char *p;
	for (; front < back && compare(string, front, back) == EQUAL; front = p + 1) {
		for (p = front; p < back && *p != '\n'; ++p)
			;
		if (fwrite(front, 1, p - front, stdout) != (size_t)(p - front))
			err(10, "stdout");
		if (putchar('\n') == EOF)
			err(10, "stdout");
		if (p >= back)
			break;
	}
}

/*
 * Return LESS, GREATER, or EQUAL depending on how the string1 compares with
 * string2 (s1 ??? s2).
 *
 *	o Matches up to len(s1) are EQUAL.
 *	o Matches up to len(s2) are GREATER.
 *
 * Compare understands about the -f and -d flags, and treats comparisons
 * appropriately.
 *
 * The string "s1" is null terminated.  The string s2 is '\n' terminated (or
 * "back" terminated).
 */
int
compare(char *s1, char *s2, char *back)
{
	int ch;

	for (; *s1 && s2 < back && *s2 != '\n'; ++s1, ++s2) {
		ch = *s2;
		if (fflag)
			ch = tolower((unsigned char)ch);
		if (dflag && !isalnum((unsigned char)ch)) {
			++s2;		/* Ignore character in comparison. */
			continue;
		}
		if (*s1 != ch)
			return (*s1 < ch ? LESS : GREATER);
	}
	return (*s1 ? GREATER : EQUAL);
}

void
usage(void)
{
	(void)fprintf(stderr,
	    "usage: look [-df] [-t termchar] string [file]\n");
	exit(20);                                /* amiport: RETURN_FAIL */
}
