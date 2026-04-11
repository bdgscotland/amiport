/*	$OpenBSD: seq.c,v 1.8 2023/06/13 21:10:41 millert Exp $	*/

/*-
 * Copyright (c) 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Brian Ginsbach.
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

/*
 * AmigaOS 3.x port of OpenBSD seq 1.8
 * Transformed for AmigaOS compatibility.
 */

/* amiport: AmigaOS version string */
static const char *verstag = "$VER: seq 1.8 (11.04.2026)";

/* amiport: Stack cookie — AmigaOS default stack is 4KB, seq uses recursion-free
 * but snprintf/printf need stack headroom. 8192 is sufficient. */
long __stack = 8192;

#include <ctype.h>
/* amiport: replaced <err.h> with <amiport/err.h> — bare err.h absent from libnix */
#include <amiport/err.h>
#include <errno.h>
/* amiport: replaced <getopt.h> with <amiport/getopt.h> — libnix getopt_long
 * returns '?' for ALL options (crash-patterns #17) */
#include <amiport/getopt.h>
#include <math.h>
/* amiport: locale.h kept — localeconv() is in libnix. On AmigaOS without locale
 * support the decimal_point will be empty or "."; the existing guard handles it. */
#include <locale.h>
#include <stdio.h>
/* amiport: replaced <stdlib.h> with <amiport/stdlib.h> — activates exit() macro */
#include <amiport/stdlib.h>
#include <string.h>
/* amiport: replaced <unistd.h> with <amiport/unistd.h> */
#include <amiport/unistd.h>
/* amiport: asprintf() via <amiport/stdio_ext.h> — maps to amiport_asprintf() */
#include <amiport/stdio_ext.h>
/* amiport: getprogname() macro via <amiport/utsname.h> */
#include <amiport/utsname.h>
/* amiport: amiport_expand_argv / amiport_free_argv for wildcard expansion */
#include <amiport/glob.h>

#define VERSION	"1.0"
#define ZERO	'0'
#define SPACE	' '

#define MAXIMUM(a, b)	(((a) < (b))? (b) : (a))
#define ISSIGN(c)	((int)(c) == '-' || (int)(c) == '+')
#define ISEXP(c)	((int)(c) == 'e' || (int)(c) == 'E')
#define ISODIGIT(c)	((int)(c) >= '0' && (int)(c) <= '7')

/* amiport: pledge() is a no-op on AmigaOS — no kernel sandbox */
#define pledge(promises, execpromises) (0)

/* Globals */

static const char *decimal_point = ".";	/* default */
static char default_format[] = { "%g" };	/* default */

/* amiport: getopt long_opts uses amiport_getopt_long — table is unchanged */
static const struct option long_opts[] = {
	{"format",	required_argument,	NULL, 'f'},
	{"help",	no_argument,		NULL, 'h'},
	{"separator",	required_argument,	NULL, 's'},
	{"version",	no_argument,		NULL, 'v'},
	{"equal-width",	no_argument,		NULL, 'w'},
	{NULL,		no_argument,		NULL, 0}
};

/* amiport: atexit cleanup tracking for argv expansion */
static char *asprintf_cur   = NULL;
static char *asprintf_last  = NULL;
static char *asprintf_prev  = NULL;

/* Prototypes */

static double e_atof(const char *);

static int decimal_places(const char *);
static int numeric(const char *);
static int valid_format(const char *);

static char *generate_format(double, double, double, int, char);

static void usage(int error);

/* amiport: atexit cleanup — free argv expansion and asprintf buffers */
static void
cleanup(void)
{
	free(asprintf_cur);
	asprintf_cur = NULL;
	free(asprintf_last);
	asprintf_last = NULL;
	free(asprintf_prev);
	asprintf_prev = NULL;
	/* amiport: flush stdout before exit on AmigaOS */
	fflush(stdout);
	amiport_free_argv();
}

/*
 * The seq command will print out a numeric sequence from 1, the default,
 * to a user specified upper limit by 1.  The lower bound and increment
 * maybe indicated by the user on the command line.  The sequence can
 * be either whole, the default, or decimal numbers.
 */
int
main(int argc, char *argv[])
{
	int c = 0;
	int equalize = 0;
	double first = 1.0;
	double last = 0.0;
	double incr = 0.0;
	double prev = 0.0;
	double cur, step;
	struct lconv *locale;
	char *fmt = NULL;
	const char *sep = "\n";
	const char *term = "\n";
	char pad = ZERO;

	/* amiport: expand wildcard arguments (AmigaDOS does not glob) */
	amiport_expand_argv(&argc, &argv);

	/* amiport: register cleanup for atexit — runs on all exit paths
	 * including err()/errx() */
	atexit(cleanup);

	/* amiport: pledge() is a no-op macro on AmigaOS */
	if (pledge("stdio", NULL) == -1)
		err(1, "pledge");

	/*
	 * amiport: Determine the locale's decimal point.
	 * localeconv() is available in libnix but AmigaOS has no locale
	 * support — decimal_point will typically be "." or empty.
	 * The existing guard (non-empty decimal_point[0]) handles both cases.
	 */
	locale = localeconv();
	if (locale && locale->decimal_point && locale->decimal_point[0] != '\0')
		decimal_point = locale->decimal_point;

	/*
	 * Process options, but handle negative numbers separately
	 * least they trip up getopt(3).
	 */
	/* amiport: getopt_long -> amiport_getopt_long via macro in amiport/getopt.h */
	while ((amiport_optind < argc) && !numeric(argv[amiport_optind]) &&
	    (c = amiport_getopt_long(argc, argv, "+f:s:w", long_opts, NULL)) != -1) {

		switch (c) {
		case 'f':	/* format (plan9/GNU) */
			fmt = amiport_optarg;
			equalize = 0;
			break;
		case 's':	/* separator (GNU) */
			sep = amiport_optarg;
			break;
		case 'v':	/* version (GNU) */
			printf("seq version %s\n", VERSION);
			return 0;
		case 'w':	/* equal width (plan9/GNU) */
			if (fmt == NULL) {
				if (equalize++)
					pad = SPACE;
			}
			break;
		case 'h':	/* help (GNU) */
			usage(0);
			break;
		default:
			/* amiport: exit(1) -> exit(10) — AmigaOS shell tests
			 * exit codes as WARN(>=5)/ERROR(>=10)/FAIL(>=20) */
			usage(10);
			break;
		}
	}

	argc -= amiport_optind;
	argv += amiport_optind;
	if (argc < 1 || argc > 3) {
		/* amiport: exit(1) -> exit(10) */
		usage(10);
	}

	last = e_atof(argv[argc - 1]);

	if (argc > 1)
		first = e_atof(argv[0]);

	if (argc > 2) {
		incr = e_atof(argv[1]);
		/* Plan 9/GNU don't do zero */
		if (incr == 0.0)
			/* amiport: errx(1,...) -> errx(10,...) */
			errx(10, "zero %screment", (first < last) ? "in" : "de");
	}

	/* default is one for Plan 9/GNU work alike */
	if (incr == 0.0)
		incr = (first < last) ? 1.0 : -1.0;

	if (incr <= 0.0 && first < last)
		/* amiport: errx(1,...) -> errx(10,...) */
		errx(10, "needs positive increment");

	if (incr >= 0.0 && first > last)
		/* amiport: errx(1,...) -> errx(10,...) */
		errx(10, "needs negative decrement");

	if (fmt != NULL) {
		if (!valid_format(fmt))
			/* amiport: errx(1,...) -> errx(10,...) */
			errx(10, "invalid format string: `%s'", fmt);
		/*
		 * XXX to be bug for bug compatible with Plan 9 add a
		 * newline if none found at the end of the format string.
		 */
	} else
		fmt = generate_format(first, incr, last, equalize, pad);

	for (step = 1, cur = first; incr > 0 ? cur <= last : cur >= last;
	    cur = first + incr * step++) {
		if (cur != first)
			fputs(sep, stdout);
		/*
		 * amiport: fmt is user-supplied only when validated by
		 * valid_format() above, which enforces exactly one floating-
		 * point conversion specifier. When not user-supplied, fmt
		 * comes from generate_format() which produces a known-safe
		 * format string. Format string injection is not possible.
		 * nosemgrep: format-string-injection
		 */
		printf(fmt, cur); /* nosemgrep */
		prev = cur;
	}

	/*
	 * Did we miss the last value of the range in the loop above?
	 *
	 * We might have, so check if the printable version of the last
	 * computed value ('cur') and desired 'last' value are equal.  If
	 * they are equal after formatting truncation, but 'cur' and 'prev'
	 * are different, it means the exit condition of the loop held true
	 * due to a rounding error and we still need to print 'last'.
	 */
	/* amiport: asprintf() -> amiport_asprintf() via macro in amiport/stdio_ext.h
	 * Results tracked in globals so atexit cleanup can free them. */
	if (asprintf(&asprintf_cur, fmt, cur) == -1 ||
	    asprintf(&asprintf_last, fmt, last) == -1 ||
	    asprintf(&asprintf_prev, fmt, prev) == -1)
		/* amiport: err(1,...) -> err(10,...) */
		err(10, "asprintf");
	if (strcmp(asprintf_cur, asprintf_last) == 0 &&
	    strcmp(asprintf_cur, asprintf_prev) != 0) {
		if (cur != first)
			fputs(sep, stdout);
		fputs(asprintf_last, stdout);
	}
	/* amiport: free asprintf buffers here (normal path); atexit cleanup
	 * handles the error paths. Set to NULL after free to prevent
	 * double-free in atexit (crash-patterns: atexit double-free). */
	free(asprintf_cur);
	asprintf_cur = NULL;
	free(asprintf_last);
	asprintf_last = NULL;
	free(asprintf_prev);
	asprintf_prev = NULL;

	fputs(term, stdout);

	return 0;
}

/*
 * numeric - verify that string is numeric
 */
static int
numeric(const char *s)
{
	int seen_decimal_pt, decimal_pt_len;

	/* skip any sign */
	if (ISSIGN((unsigned char)*s))
		s++;

	seen_decimal_pt = 0;
	decimal_pt_len = strlen(decimal_point);
	while (*s) {
		if (!isdigit((unsigned char)*s)) {
			if (!seen_decimal_pt &&
			    strncmp(s, decimal_point, decimal_pt_len) == 0) {
				s += decimal_pt_len;
				seen_decimal_pt = 1;
				continue;
			}
			if (ISEXP((unsigned char)*s)) {
				s++;
				if (ISSIGN((unsigned char)*s) ||
				    isdigit((unsigned char)*s)) {
					s++;
					continue;
				}
			}
			break;
		}
		s++;
	}
	return *s == '\0';
}

/*
 * valid_format - validate user specified format string
 */
static int
valid_format(const char *fmt)
{
	unsigned conversions = 0;

	while (*fmt != '\0') {
		/* scan for conversions */
		if (*fmt != '%') {
			fmt++;
			continue;
		}
		fmt++;

		/* allow %% but not things like %10% */
		if (*fmt == '%') {
			fmt++;
			continue;
		}

		/* flags */
		while (*fmt != '\0' && strchr("#0- +'", *fmt)) {
			fmt++;
		}

		/* field width */
		while (*fmt != '\0' && strchr("0123456789", *fmt)) {
			fmt++;
		}

		/* precision */
		if (*fmt == '.') {
			fmt++;
			while (*fmt != '\0' && strchr("0123456789", *fmt)) {
				fmt++;
			}
		}

		/* conversion */
		switch (*fmt) {
		case 'A':
		case 'a':
		case 'E':
		case 'e':
		case 'F':
		case 'f':
		case 'G':
		case 'g':
			/* floating point formats are accepted */
			conversions++;
			break;
		default:
			/* anything else is not */
			return 0;
		}
	}

	/* PR 236347 -- user format strings must have a conversion */
	return conversions == 1;
}

/*
 * e_atof - convert an ASCII string to a double
 *	exit if string is not a valid double, or if converted value would
 *	cause overflow or underflow
 */
static double
e_atof(const char *num)
{
	char *endp;
	double dbl;

	errno = 0;
	dbl = strtod(num, &endp);

	if (errno == ERANGE)
		/* under or overflow */
		/* amiport: err(2,...) kept as-is — exit code 2 is a valid
		 * AmigaOS RETURN_WARN (>=5 for scripts, but 2 is still
		 * distinguishable for seq-specific error handling) */
		err(2, "%s", num);
	else if (*endp != '\0')
		/* "junk" left in number */
		errx(2, "invalid floating point argument: %s", num);

	/* zero shall have no sign */
	if (dbl == -0.0)
		dbl = 0;
	return dbl;
}

/*
 * decimal_places - count decimal places in a number (string)
 */
static int
decimal_places(const char *number)
{
	int places = 0;
	char *dp;

	/* look for a decimal point */
	if ((dp = strstr(number, decimal_point))) {
		dp += strlen(decimal_point);

		while (isdigit((unsigned char)*dp++))
			places++;
	}
	return places;
}

/*
 * generate_format - create a format string
 *
 * XXX to be bug for bug compatible with Plan9 and GNU return "%g"
 * when "%g" prints as "%e" (this way no width adjustments are made)
 */
static char *
generate_format(double first, double incr, double last, int equalize, char pad)
{
	static char buf[256];
	char cc = '\0';
	int precision, width1, width2, places;

	if (equalize == 0)
		return default_format;

	/* figure out "last" value printed */
	if (first > last)
		last = first - incr * floor((first - last) / incr);
	else
		last = first + incr * floor((last - first) / incr);

	snprintf(buf, sizeof(buf), "%g", incr);
	if (strchr(buf, 'e'))
		cc = 'e';
	precision = decimal_places(buf);

	width1 = snprintf(buf, sizeof(buf), "%g", first);
	if (strchr(buf, 'e'))
		cc = 'e';
	if ((places = decimal_places(buf)))
		width1 -= (places + strlen(decimal_point));

	precision = MAXIMUM(places, precision);

	width2 = snprintf(buf, sizeof(buf), "%g", last);
	if (strchr(buf, 'e'))
		cc = 'e';
	if ((places = decimal_places(buf)))
		width2 -= (places + strlen(decimal_point));

	/* XXX if incr is floating point fix the precision */
	if (precision) {
		snprintf(buf, sizeof(buf), "%%%c%d.%d%c", pad,
		    MAXIMUM(width1, width2) + (int)strlen(decimal_point) +
		    precision, precision, (cc) ? cc : 'f');
	} else {
		snprintf(buf, sizeof(buf), "%%%c%d%c", pad,
		    MAXIMUM(width1, width2), (cc) ? cc : 'g');
	}

	return buf;
}

static void
usage(int error)
{
	fprintf(stderr,
	    "usage: %s [-w] [-f format] [-s string] [first [incr]] last\n",
	    getprogname());
	exit(error);
}
