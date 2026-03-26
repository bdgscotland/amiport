/*	$OpenBSD: jot.c,v 1.56 2021/08/13 12:37:28 jmc Exp $	*/
/*	$NetBSD: jot.c,v 1.3 1994/12/02 20:29:43 pk Exp $	*/

/*-
 * Copyright (c) 1993
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

/*
 * jot - print sequential or random data
 *
 * Author:  John Kunze, Office of Comp. Affairs, UCB
 */

/* amiport: Amiga version string */
static const char *verstag = "$VER: jot 1.56 (26.03.2026)";

/* amiport: stack cookie -- Amiga default 4KB is too small */
long __stack = 32768;

/* amiport: pledge/unveil not available on AmigaOS -- stubbed */
#define pledge(p, e) (0)

/* amiport: replaced <stdlib.h> with <amiport/stdlib.h> -- activates exit() -> amiport_exit() */
#include <amiport/stdlib.h>
/* amiport: replaced <unistd.h> with <amiport/unistd.h> */
#include <amiport/unistd.h>
/* amiport: replaced <err.h> with <amiport/err.h> -- provides err/errx/warn/warnx/strtonum */
#include <amiport/err.h>
/* amiport: added <amiport/stdio_ext.h> for asprintf() */
#include <amiport/stdio_ext.h>
/* amiport: added <amiport/glob.h> for amiport_expand_argv() */
#include <amiport/glob.h>
/* amiport: added <amiport/getopt.h> -- libnix getopt is broken, use amiport version */
#include <amiport/getopt.h>

#include <ctype.h>
#include <limits.h>
#include <math.h>
/* amiport: removed <stdbool.h> -- C99 header, not available in C89; bool/true/false defined below */
/* amiport: removed <stdint.h> -- C99 header; uint32_t replaced with unsigned long below */
#include <stdio.h>
#include <string.h>

/* amiport: C89 bool emulation -- replaces <stdbool.h> */
#ifndef bool
#define bool  int
#define true  1
#define false 0
#endif

#define	REPS	1
#define	BEGIN	2
#define	ENDER	4
#define	STEP	8

#define	is_default(s)	(strcmp((s), "-") == 0)

static long	reps	= 100;
static double	begin	= 1;
static double	ender	= 100;
static double	step	= 1;

static char	*format = "";
static char	*sepstring = "\n";
static int	prec = -1;
static bool	boring;
static bool	chardata;
static bool	finalnl = true;
/* amiport: renamed 'infinity' -> 'inf_loop' -- conflicts with math.h infinity() function declaration */
static bool	inf_loop;
static bool	intdata;
static bool	longdata;
static bool	nosign;
static bool	randomize;
static bool	word;

static void	getformat(void);
static int	getprec(char *);
static int	putdata(double, bool);
/* amiport: removed __dead attribute -- OpenBSD-specific, not available in C89/bebbo-gcc */
static void	usage(void);

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
	double		x;
	double		y;
	long		i;
	unsigned int	mask = 0;
	int		n = 0;
	int		ch;
	const char	*errstr;

	/* amiport: expand wildcard args -- Amiga shell doesn't glob */
	amiport_expand_argv(&argc, &argv);
	/* amiport: register cleanup for all exit paths including err()/errx() */
	atexit(cleanup);

	/* amiport: pledge() stubbed -- not available on AmigaOS */
	if (pledge("stdio", NULL) == -1)
		err(10, "pledge"); /* amiport: RETURN_ERROR */

	while ((ch = getopt(argc, argv, "b:cnp:rs:w:")) != -1) {
		switch (ch) {
		case 'b':
			boring = true;
			chardata = word = false;
			format = optarg;
			break;
		case 'c':
			chardata = true;
			boring = word = false;
			format = "";
			break;
		case 'n':
			finalnl = false;
			break;
		case 'p':
			prec = strtonum(optarg, 0, INT_MAX, &errstr);
			if (errstr != NULL)
				errx(10, "bad precision value, %s: %s", errstr, /* amiport: RETURN_ERROR */
					optarg);
			break;
		case 'r':
			randomize = true;
			break;
		case 's':
			sepstring = optarg;
			break;
		case 'w':
			word = true;
			boring = chardata = false;
			format = optarg;
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	switch (argc) {	/* examine args right to left, falling thru cases */
	case 4:
		if (!is_default(argv[3])) {
			if (!sscanf(argv[3], "%lf", &step))
				errx(10, "Bad s value: %s", argv[3]); /* amiport: RETURN_ERROR */
			mask |= STEP;
			if (randomize)
				warnx("random seeding not supported");
		}
	case 3:
		if (!is_default(argv[2])) {
			if (!sscanf(argv[2], "%lf", &ender))
				ender = argv[2][strlen(argv[2])-1];
			mask |= ENDER;
			if (prec == -1)
				n = getprec(argv[2]);
		}
	case 2:
		if (!is_default(argv[1])) {
			if (!sscanf(argv[1], "%lf", &begin))
				begin = argv[1][strlen(argv[1])-1];
			mask |= BEGIN;
			if (prec == -1)
				prec = getprec(argv[1]);
			if (n > prec)		/* maximum precision */
				prec = n;
		}
	case 1:
		if (!is_default(argv[0])) {
			reps = strtonum(argv[0], 0, LONG_MAX, &errstr);
			if (errstr != NULL)
				errx(10, "Bad reps value, %s: %s", errstr, /* amiport: RETURN_ERROR */
				    argv[0]);
			mask |= REPS;
			if (reps == 0)
				inf_loop = true;
		}
	case 0:
		if (prec == -1)
			prec = 0;
		break;
	default:
		errx(10, "Too many arguments.  What do you mean by %s?", /* amiport: RETURN_ERROR */
		    argv[4]);
	}

	if (!boring)
		getformat();

	if (!randomize) {
		/*
		 * Consolidate the values of reps, begin, ender, step:
		 * The formula ender - begin == (reps - 1) * step shows that any
		 * three determine the fourth (unless reps == 1 or step == 0).
		 * The manual states the following rules:
		 * 1. If four are specified, compare the given and the computed
		 *    value of reps and take the smaller of the two.
		 * 2. If steps was omitted, it takes the default, unless both
		 *    begin and ender were specified.
		 * 3. Assign defaults to omitted values for reps, begin, ender,
		 *    from left to right.
		 */
		switch (mask) { /* Four cases involve both begin and ender. */
		case REPS | BEGIN | ENDER | STEP:
			if (inf_loop)
				errx(10, /* amiport: RETURN_ERROR */
				    "Can't specify end of infinite sequence");
			if (step != 0.0) {
				long t = (ender - begin + step) / step;
				if (t <= 0)
					errx(10, "Impossible stepsize"); /* amiport: RETURN_ERROR */
				if (t < reps)
					reps = t;
			}
			break;
		case REPS | BEGIN | ENDER:
			if (inf_loop)
				errx(10, /* amiport: RETURN_ERROR */
				    "Can't specify end of infinite sequence");
			if (reps == 1)
				step = 0.0;
			else
				step = (ender - begin) / (reps - 1);
			break;
		case BEGIN | ENDER:
			step = ender > begin ? 1 : -1; /* FreeBSD's behavior. */
			/* FALLTHROUGH */
		case BEGIN | ENDER | STEP:
			if (step == 0.0) {
				reps = 0;
				inf_loop = true;
				break;
			}
			reps = (ender - begin + step) / step;
			if (reps <= 0)
				errx(10, "Impossible stepsize"); /* amiport: RETURN_ERROR */
			break;
		case ENDER:		/* Four cases involve only ender. */
		case ENDER | STEP:
		case REPS | ENDER:
		case REPS | ENDER | STEP:
			if (inf_loop)
				errx(10, /* amiport: RETURN_ERROR */
				    "Must specify start of infinite sequence");
			begin = ender - reps * step + step;
			break;
		default:
			/*
			 * The remaining eight cases omit ender.  We don't need
			 * to compute anything because only reps, begin, step
			 * are used for producing output below.  Rules 2. and 3.
			 * together imply that ender will be set last.
			 */
			break;
		}

		for (i = 1, x = begin; i <= reps || inf_loop; i++, x += step)
			if (putdata(x, reps == i && !inf_loop))
				errx(10, "range error in conversion: %f", x); /* amiport: RETURN_ERROR */
	} else { /* Random output: use defaults for omitted values. */
		/* amiport: replaced bool with int, uint32_t with unsigned long -- C89 compat */
		int		use_unif = 0;
		unsigned long	pow10 = 1;
		unsigned long	uintx = 0; /* Initialized to make gcc happy. */

		if (prec > 9)	/* pow(10, prec) > UINT32_MAX */
			errx(10, "requested precision too large"); /* amiport: RETURN_ERROR */

		if (ender < begin) {
			x = begin;
			begin = ender;
			ender = x;
		}
		x = ender - begin;

		if (prec > 0 || (fmod(ender, 1) == 0 && fmod(begin, 1) == 0)) {
			double range;

			while (prec-- > 0)
				pow10 *= 10;

			range = pow10 * (ender - begin);

			/* If range is an integer, use rand() % uintx. */
			if (fmod(range, 1) == 0) {
				if (range >= (double)0xffffffffUL)
					errx(10, "requested range too large"); /* amiport: RETURN_ERROR */
				use_unif = 1;
				uintx = (unsigned long)(range + 1);
			}
		}

		for (i = 1; i <= reps || inf_loop; i++) {
			double v;

			if (use_unif) {
				/* amiport: replaced arc4random_uniform(uintx) with rand() % uintx
				 * arc4random not available on AmigaOS; rand() is sufficient */
				y = (double)(rand() % (int)uintx) / (double)pow10;
				v = y + begin;
			} else {
				/* amiport: replaced arc4random() with rand()
				 * RAND_MAX is at least 32767; scale to [0,1) range */
				y = rand() / ((double)RAND_MAX + 1.0);
				v = y * x + begin;
			}
			if (putdata(v, reps == i && !inf_loop))
				errx(10, "range error in conversion: %f", v); /* amiport: RETURN_ERROR */
		}
	}

	if (finalnl)
		putchar('\n');

	return 0;
}

static int
putdata(double x, bool last)
{
	if (boring)
		printf("%s", format);
	else if (longdata && nosign) {
		if (x < 0.0 || x > (double)ULONG_MAX)
			return 1;
		printf(format, (unsigned long)x);
	} else if (longdata) {
		if (x < (double)LONG_MIN || x > (double)LONG_MAX)
			return 1;
		printf(format, (long)x);
	} else if (chardata || (intdata && !nosign)) {
		if (x < (double)INT_MIN || x > (double)INT_MAX)
			return 1;
		printf(format, (int)x);
	} else if (intdata) {
		if (x < 0.0 || x > (double)UINT_MAX)
			return 1;
		printf(format, (unsigned int)x);
	} else
		printf(format, x);
	if (!last)
		fputs(sepstring, stdout);

	return 0;
}

/* amiport: removed __dead attribute -- OpenBSD-specific, not available in C89/bebbo-gcc */
static void
usage(void)
{
	(void)fprintf(stderr, "usage: jot [-cnr] [-b word] [-p precision] "
	    "[-s string] [-w word]\n"
	    "	   [reps [begin [end [step]]]]\n");
	exit(10); /* amiport: RETURN_ERROR */
}

static int
getprec(char *s)
{
	if ((s = strchr(s, '.')) == NULL)
		return 0;
	return strspn(s + 1, "0123456789");
}

static void
getformat(void)
{
	char	*p;

	p = format;
	while ((p = strchr(p, '%')) != NULL && p[1] == '%')
		p += 2;

	if (p == NULL && !chardata) {
		/* amiport: asprintf() provided by <amiport/stdio_ext.h> macro */
		if (asprintf(&format, "%s%%.%df", format, prec) == -1)
			err(10, NULL); /* amiport: RETURN_ERROR */
	} else if (p == NULL && chardata) {
		if (asprintf(&format, "%s%%c", format) == -1)
			err(10, NULL); /* amiport: RETURN_ERROR */
	} else if (p[1] == '\0') {
		/* cannot end in single '%' */
		if (asprintf(&format, "%s%%", format) == -1)
			err(10, NULL); /* amiport: RETURN_ERROR */
	} else {
		/*
		 * Allow conversion format specifiers of the form
		 * %[#][ ][{+,-}][0-9]*[.[0-9]*]? where ? must be one of
		 * [l]{d,i,o,u,x} or {f,e,g,F,E,G,d,o,x,D,O,U,X,c,u}
		 */
		char	*fmt;
		int	dot, hash, space, sign, numbers;

		fmt = p++;
		dot = hash = space = sign = numbers = 0;
		while (!isalpha((unsigned char)*p)) {
			if (isdigit((unsigned char)*p)) {
				numbers++;
				p++;
			} else if ((*p == '#' && !(numbers|dot|sign|space|
			    hash++)) ||
			    (*p == ' ' && !(numbers|dot|space++)) ||
			    ((*p == '+' || *p == '-') && !(numbers|dot|sign++))
			    || (*p == '.' && !(dot++)))
				p++;
			else
				goto fmt_broken;
		}
		if (*p == 'l') {
			longdata = true;
			if (*++p == 'l') {
				p++;
				goto fmt_broken;
			}
		}
		switch (*p) {
		case 'd':
		case 'i':
			intdata = true;
			break;
		case 'o':
		case 'u':
		case 'x':
		case 'X':
			intdata = nosign = true;
			break;
		case 'D':
			if (longdata)
				goto fmt_broken;
			longdata = intdata = true; /* same as %ld */
			break;
		case 'O':
		case 'U':
			if (longdata)
				goto fmt_broken;
			longdata = intdata = nosign = true; /* same as %l[ou] */
			break;
		case 'c':
			if (longdata)
				goto fmt_broken;
			chardata = true;
			break;
		case 'e':
		case 'E':
		case 'f':
		case 'F':
		case 'g':
		case 'G':
			if (longdata)
				goto fmt_broken;
			/* No cast needed for printing in putdata() */
			break;
		default:
fmt_broken:
			errx(10, "illegal or unsupported format '%.*s'", /* amiport: RETURN_ERROR */
			    (int)(p + 1 - fmt), fmt);
		}

		while ((p = strchr(p, '%')) != NULL && p[1] == '%')
			p += 2;

		if (p != NULL) {
			if (p[1] != '\0')
				errx(10, "too many conversions"); /* amiport: RETURN_ERROR */
			/* cannot end in single '%' */
			if (asprintf(&format, "%s%%", format) == -1)
				err(10, NULL); /* amiport: RETURN_ERROR */
		}
	}
}
