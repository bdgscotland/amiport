/*	$OpenBSD: nl.c,v 1.8 2022/12/04 23:50:49 cheloha Exp $ */
/*	$NetBSD: nl.c,v 1.11 2011/08/16 12:00:46 christos Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Klaus Klein.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* amiport: Amiga version string */
static const char *verstag __attribute__((used)) = "$VER: nl 1.8 (26.03.2026)";

/* amiport: stack cookie -- Amiga default 4KB is too small */
long __stack = 16384;

/* amiport: suppress wildcard expansion -- program takes pattern arguments (-b p<regex>) */
int __nowild = 1;

/* amiport: err/warn/strtonum via posix-shim */
#include <amiport/err.h>
#include <errno.h>
#include <limits.h>
/* amiport: removed <locale.h> -- setlocale() no-op on AmigaOS, stub below */
/* amiport: regex emulation via posix-emu -- Tier 2 */
/* amiport-emu: regex emulated -- no locale collation, no [:class:], max 9 groups, backtracking NFA */
#include <amiport-emu/regex.h>
#include <stdio.h>
#include <amiport/stdlib.h>   /* amiport: replaced <stdlib.h> -- activates exit() -> amiport_exit() */
#include <string.h>
#include <amiport/unistd.h>   /* amiport: replaced <unistd.h> */
/* amiport: removed <wchar.h> -- no wchar support on AmigaOS 3.x; multibyte paths guarded below */

/* amiport: stdio_ext for getline() shim */
#include <amiport/stdio_ext.h>

/* amiport: glob.h for amiport_expand_argv() and __progname */
#include <amiport/glob.h>

/* amiport: getopt/optarg/optind from posix-shim */
#include <amiport/getopt.h>

/* amiport: pledge/unveil not available on AmigaOS -- stubbed */
#define pledge(p, e) (0)

/* amiport: setlocale() -- already provided via <amiport/unistd.h>, no redefinition needed */

/* amiport: __dead attribute not available on AmigaOS -- define as empty */
#ifndef __dead
#define __dead
#endif

/* amiport: NL_TEXTMAX not defined in libnix limits.h -- define fallback */
#ifndef NL_TEXTMAX
#define NL_TEXTMAX 256
#endif

/* amiport: getprogname() via __progname from <amiport/glob.h> */
#ifndef getprogname
#define getprogname() (__progname)
#endif

typedef enum {
	number_all,		/* number all lines */
	number_nonempty,	/* number non-empty lines */
	number_none,		/* no line numbering */
	number_regex		/* number lines matching regular expression */
} numbering_type;

struct numbering_property {
	const char * const	name;		/* for diagnostics */
	numbering_type		type;		/* numbering type */
	regex_t			expr;		/* for type == number_regex */
};

/* line numbering formats */
#define FORMAT_LN	"%-*d"	/* left justified, leading zeros suppressed */
#define FORMAT_RN	"%*d"	/* right justified, leading zeros suppressed */
#define FORMAT_RZ	"%0*d"	/* right justified, leading zeros kept */

#define FOOTER		0
#define BODY		1
#define HEADER		2
#define NP_LAST		HEADER

/* amiport: zero-initialize regex_t (amiport_emu_regex_t) -- first field is array,
 * so { 0, 0, 0, 0 } causes "missing braces" warnings. Use { {0} } to silence them. */
static struct numbering_property numbering_properties[NP_LAST + 1] = {
	{ "footer",	number_none,	{ {0}, 0, 0, 0 } },
	{ "body",	number_nonempty, { {0}, 0, 0, 0 } },
	{ "header",	number_none,	{ {0}, 0, 0, 0 } },
};

void		filter(void);
void		parse_numbering(const char *, int);
__dead void	usage(void);

/*
 * Delimiter characters that indicate the start of a logical page section.
 * amiport: MB_LEN_MAX is 1 on AmigaOS (no multibyte) -- use fixed size 2
 */
static char delim[2];
static int delimlen;

/*
 * Configurable parameters.
 */

/* line numbering format */
static const char *format = FORMAT_RN;

/* amiport: perf -- precomputed format string and padding (avoids runtime %*d dispatch per line) */
static char concrete_fmt[16];  /* e.g. "%6d", "%-6d", "%06d" */
static char pad_str[32];       /* width spaces for unnumbered lines */

/* increment value used to number logical page lines */
static int incr = 1;

/* number of adjacent blank lines to be considered (and numbered) as one */
static unsigned int nblank = 1;

/* whether to restart numbering at logical page delimiters */
static int restart = 1;

/* characters used in separating the line number and the corrsp. text line */
static const char *sep = "\t";

/* initial value used to number logical page lines */
static int startnum = 1;

/* number of characters to be used for the line number */
/* should be unsigned but required signed by `*' precision conversion */
static int width = 6;

/* amiport: getline buffer tracked for atexit cleanup -- prevent leak on err() paths */
static char *getline_buf = NULL;

static void
cleanup(void)
{
	free(getline_buf);
	getline_buf = NULL; /* amiport: prevent double-free in atexit cleanup */
	amiport_free_argv();
	(void)fflush(stdout);
}

int
main(int argc, char *argv[])
{
	int c;
	/* amiport: removed multibyte clen/delim variables -- no wchar on AmigaOS */
	char delim1 = '\\';
	char delim2 = ':';
	const char *errstr;

	/* amiport: expand wildcard args -- Amiga shell doesn't glob */
	amiport_expand_argv(&argc, &argv);
	/* amiport: register cleanup for all exit paths including err()/errx() */
	atexit(cleanup);

	/* amiport: setlocale() stubbed to no-op */
	(void)setlocale(LC_ALL, "");

	/* amiport: pledge() stubbed to no-op */
	if (pledge("stdio rpath", NULL) == -1)
		err(10, "pledge"); /* amiport: RETURN_ERROR */

	while ((c = getopt(argc, argv, "pb:d:f:h:i:l:n:s:v:w:")) != -1) {
		switch (c) {
		case 'p':
			restart = 0;
			break;
		case 'b':
			parse_numbering(optarg, BODY);
			break;
		case 'd':
			/*
			 * amiport: AmigaOS is single-byte only -- no mbrlen/MB_CUR_MAX.
			 * Accept up to 2 delimiter characters as plain ASCII bytes.
			 * The original multibyte path using mbrlen() is disabled.
			 */
			if (optarg[0] != '\0') {
				delim1 = optarg[0];
				if (optarg[1] != '\0') {
					if (optarg[2] != '\0') {
						errx(10, /* amiport: RETURN_ERROR */
						    "invalid delimiter: %s",
						    optarg);
					}
					delim2 = optarg[1];
				}
			}
			break;
		case 'f':
			parse_numbering(optarg, FOOTER);
			break;
		case 'h':
			parse_numbering(optarg, HEADER);
			break;
		case 'i':
			incr = strtonum(optarg, INT_MIN, INT_MAX, &errstr);
			if (errstr)
				errx(10, "increment value is %s: %s", /* amiport: RETURN_ERROR */
				    errstr, optarg);
			break;
		case 'l':
			nblank = strtonum(optarg, 0, UINT_MAX, &errstr);
			if (errstr)
				errx(10, /* amiport: RETURN_ERROR */
				    "blank line value is %s: %s",
				    errstr, optarg);
			break;
		case 'n':
			if (strcmp(optarg, "ln") == 0) {
				format = FORMAT_LN;
			} else if (strcmp(optarg, "rn") == 0) {
				format = FORMAT_RN;
			} else if (strcmp(optarg, "rz") == 0) {
				format = FORMAT_RZ;
			} else
				errx(10, /* amiport: RETURN_ERROR */
				    "illegal format -- %s", optarg);
			break;
		case 's':
			sep = optarg;
			break;
		case 'v':
			startnum = strtonum(optarg, INT_MIN, INT_MAX, &errstr);
			if (errstr)
				errx(10, /* amiport: RETURN_ERROR */
				    "initial logical page value is %s: %s",
				    errstr, optarg);
			break;
		case 'w':
			width = strtonum(optarg, 1, INT_MAX, &errstr);
			if (errstr)
				errx(10, "width is %s: %s", errstr, /* amiport: RETURN_ERROR */
				    optarg);
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	switch (argc) {
	case 0:
		break;
	case 1:
		if (strcmp(argv[0], "-") != 0 &&
		    freopen(argv[0], "r", stdin) == NULL)
			err(10, "%s", argv[0]); /* amiport: RETURN_ERROR */
		break;
	default:
		usage();
		/* NOTREACHED */
	}

	/* amiport: pledge() stubbed to no-op */
	if (pledge("stdio", NULL) == -1)
		err(10, "pledge"); /* amiport: RETURN_ERROR */

	/* Generate the delimiter sequence -- amiport: ASCII single-byte only */
	delim[0] = delim1;
	delim[1] = delim2;
	delimlen = 2;

	/* amiport: perf -- precompute format string and padding once, not per-line */
	if (format == FORMAT_LN)
		snprintf(concrete_fmt, sizeof(concrete_fmt), "%%-%dd", width);
	else if (format == FORMAT_RZ)
		snprintf(concrete_fmt, sizeof(concrete_fmt), "%%0%dd", width);
	else
		snprintf(concrete_fmt, sizeof(concrete_fmt), "%%%dd", width);
	snprintf(pad_str, sizeof(pad_str), "%*s", width, "");

	/* Do the work. */
	filter();

	exit(EXIT_SUCCESS);
}

void
filter(void)
{
	char *buffer;
	size_t buffersize;
	long linelen; /* amiport: replaced ssize_t with long (C89 on 68k) */
	int line;		/* logical line number */
	int section;		/* logical page section */
	unsigned int adjblank;	/* adjacent blank lines */
	int donumber = 0, idx;

	adjblank = 0;
	line = startnum;
	section = BODY;

	buffer = NULL;
	buffersize = 0;
	/* amiport: getline() maps to amiport_getline() via macro in <amiport/stdio_ext.h> */
	while ((linelen = getline(&buffer, &buffersize, stdin)) > 0) {
		getline_buf = buffer; /* amiport: track for atexit cleanup */
		for (idx = FOOTER; idx <= NP_LAST; idx++) {
			/* Does it look like a delimiter? */
			if (delimlen * (idx + 1) > linelen)
				break;
			if (memcmp(buffer + delimlen * idx, delim,
			    delimlen) != 0)
				break;
			/* Was this the whole line? */
			if (buffer[delimlen * (idx + 1)] == '\n') {
				section = idx;
				adjblank = 0;
				if (restart)
					line = startnum;
				goto nextline;
			}
		}

		switch (numbering_properties[section].type) {
		case number_all:
			/*
			 * Doing this for number_all only is disputable, but
			 * the standard expresses an explicit dependency on
			 * `-b a' etc.
			 */
			if (buffer[0] == '\n' && ++adjblank < nblank)
				donumber = 0;
			else
				donumber = 1, adjblank = 0;
			break;
		case number_nonempty:
			donumber = (buffer[0] != '\n');
			break;
		case number_none:
			donumber = 0;
			break;
		case number_regex:
			/* amiport-emu: regexec() via posix-emu -- Tier 2 emulation */
			donumber =
			    (regexec(&numbering_properties[section].expr,
			    buffer, 0, NULL, 0) == 0);
			break;
		}

		if (donumber) {
			/* amiport: perf -- use precomputed concrete_fmt (no runtime %*d dispatch) */
			(void)printf(concrete_fmt, line);
			line += incr;
			(void)fputs(sep, stdout);
		} else {
			/* amiport: perf -- use precomputed pad_str (no printf for spaces) */
			(void)fputs(pad_str, stdout);
		}
		(void)fwrite(buffer, linelen, 1, stdout);

		if (ferror(stdout))
			err(10, "output error"); /* amiport: RETURN_ERROR */
nextline:
		;
	}

	if (ferror(stdin))
		err(10, "input error"); /* amiport: RETURN_ERROR */

	free(buffer);
	getline_buf = NULL; /* amiport: prevent double-free in atexit cleanup */
}

/*
 * Various support functions.
 */

void
parse_numbering(const char *argstr, int section)
{
	int error;
	char errorbuf[NL_TEXTMAX];

	switch (argstr[0]) {
	case 'a':
		numbering_properties[section].type = number_all;
		break;
	case 'n':
		numbering_properties[section].type = number_none;
		break;
	case 't':
		numbering_properties[section].type = number_nonempty;
		break;
	case 'p':
		/* If there was a previous expression, throw it away. */
		if (numbering_properties[section].type == number_regex)
			regfree(&numbering_properties[section].expr);
		else
			numbering_properties[section].type = number_regex;

		/* Compile/validate the supplied regular expression. */
		/* amiport-emu: regcomp() via posix-emu -- Tier 2, no POSIX char classes */
		if ((error = regcomp(&numbering_properties[section].expr,
		    &argstr[1], REG_NEWLINE|REG_NOSUB)) != 0) {
			(void)regerror(error,
			    &numbering_properties[section].expr,
			    errorbuf, sizeof(errorbuf));
			errx(10, /* amiport: RETURN_ERROR */
			    "%s expr: %s -- %s",
			    numbering_properties[section].name, errorbuf,
			    &argstr[1]);
		}
		break;
	default:
		errx(10, /* amiport: RETURN_ERROR */
		    "illegal %s line numbering type -- %s",
		    numbering_properties[section].name, argstr);
	}
}

__dead void
usage(void)
{
	(void)fprintf(stderr, "usage: %s [-p] [-b type] [-d delim] [-f type] "
	    "[-h type] [-i incr] [-l num]\n\t[-n format] [-s sep] "
	    "[-v startnum] [-w width] [file]\n", getprogname());
	exit(10); /* amiport: RETURN_ERROR (was EXIT_FAILURE) */
}
