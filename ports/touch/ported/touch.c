/*	$OpenBSD: touch.c,v 1.27 2022/01/29 00:06:26 cheloha Exp $	*/
/*	$NetBSD: touch.c,v 1.11 1995/08/31 22:10:06 jtc Exp $	*/

/*
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

/* amiport: AmigaOS 3.x port of touch 1.27 */

/* amiport: Amiga boilerplate -- stack cookie and version string */
long __stack = 16384;
static const char *verstag = "$VER: touch 1.27 (11.04.2026)";

#include <sys/types.h>
/* amiport: fcntl.h included before amiport/sys/stat.h to prevent macro
 * collision -- fcntl.h pulls in system sys/stat.h which would see the
 * #define stat amiport_stat macro and cause redefinition errors */
#include <fcntl.h>
/* amiport: replaced <sys/stat.h> with <amiport/sys/stat.h> */
#include <amiport/sys/stat.h>
/* amiport: removed <sys/time.h> -- not needed; time functions via <time.h> */

#include <ctype.h>
/* amiport: replaced <err.h> with <amiport/err.h> */
#include <amiport/err.h>
/* amiport: <amiport/glob.h> provides amiport_expand_argv() / amiport_free_argv() */
#include <amiport/glob.h>
/* amiport: <amiport/sys/time.h> provides amiport_strptime() */
#include <amiport/sys/time.h>
#include <errno.h>
#include <stdio.h>
/* amiport: replaced <stdlib.h> with <amiport/stdlib.h> (exit -> amiport_exit) */
#include <amiport/stdlib.h>
#include <string.h>
#include <time.h>
/* amiport: replaced <unistd.h> with <amiport/unistd.h>
 *   provides: open/close macros, utimensat, futimens, UTIME_NOW/UTIME_OMIT,
 *   AT_FDCWD, timegm */
#include <amiport/unistd.h>
/* amiport: added <amiport/getopt.h> -- libnix getopt_long is broken (crash-patterns #17) */
#include <amiport/getopt.h>

/* amiport: pledge() is a no-op on AmigaOS */
#define pledge(p, e) (0)

/* amiport: __dead is an OpenBSD extension marking no-return functions */
#ifndef __dead
#define __dead  /* no-op -- no __attribute__((noreturn)) needed for C89 */
#endif

/* amiport: DEFFILEMODE is an OpenBSD extension for default file permissions */
#ifndef DEFFILEMODE
#define DEFFILEMODE 0666
#endif

/* amiport: struct timespec provided by <time.h> (via sys/_timespec.h in
 * bebbo-gcc/sys-include). Do not redefine -- it is already declared. */

void		stime_arg1(char *, struct timespec *);
void		stime_arg2(char *, int, struct timespec *);
void		stime_argd(char *, struct timespec *);
void		stime_file(char *, struct timespec *);
static void __dead usage(void);

static void
cleanup(void)
{
    /* amiport: free expanded argv on all exit paths (err/errx/exit) */
    amiport_free_argv();
    (void)fflush(stdout);
}

int
main(int argc, char *argv[])
{
	struct timespec	 ts[2];
	int		 aflag, cflag, mflag, ch, fd, len, rval, timeset;
	char		*p;

	/* amiport: expand wildcards in argv (AmigaOS shell does not glob) */
	amiport_expand_argv(&argc, &argv);
	atexit(cleanup);

	/* amiport: pledge() is a no-op on AmigaOS */
	if (pledge("stdio rpath wpath cpath fattr", NULL) == -1)
		err(10, "pledge");

	aflag = cflag = mflag = timeset = 0;
	/* amiport: getopt() provided by <amiport/getopt.h> */
	while ((ch = getopt(argc, argv, "acd:fmr:t:")) != -1)
		switch (ch) {
		case 'a':
			aflag = 1;
			break;
		case 'c':
			cflag = 1;
			break;
		case 'd':
			timeset = 1;
			stime_argd(optarg, ts);
			break;
		case 'f':
			break;
		case 'm':
			mflag = 1;
			break;
		case 'r':
			timeset = 1;
			stime_file(optarg, ts);
			break;
		case 't':
			timeset = 1;
			stime_arg1(optarg, ts);
			break;
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	/* Default is both -a and -m. */
	if (aflag == 0 && mflag == 0)
		aflag = mflag = 1;

	/*
	 * If no -r or -t flag, at least two operands, the first of which
	 * is an 8 or 10 digit number, use the obsolete time specification.
	 */
	if (!timeset && argc > 1) {
		(void)strtol(argv[0], &p, 10);
		len = p - argv[0];
		if (*p == '\0' && (len == 8 || len == 10)) {
			timeset = 1;
			stime_arg2(*argv++, len == 10, ts);
		}
	}

	/* Otherwise use the current time of day. */
	if (!timeset)
		ts[0].tv_nsec = ts[1].tv_nsec = UTIME_NOW;

	if (!aflag)
		ts[0].tv_nsec = UTIME_OMIT;
	if (!mflag)
		ts[1].tv_nsec = UTIME_OMIT;

	if (*argv == NULL)
		usage();

	for (rval = 0; *argv; ++argv) {
		/* amiport: restructured for AmigaOS. The original code calls
		 * utimensat() first, then checks errno==ENOENT to decide
		 * whether to create the file. But errno from the amiport shim
		 * is not reliably visible to calling code due to a linker
		 * symbol conflict (see known-pitfalls.md: errno weak symbol).
		 * Instead, we stat first to check existence, then create if
		 * needed, then set the timestamp. */
		{
			struct amiport_stat sb;
			int file_exists = (amiport_stat(*argv, &sb) == 0);

			if (!file_exists) {
				if (cflag)
					continue;
				/* Create the file using libnix native open/close.
				 * Must #undef to bypass amiport macros -- separate
				 * fd namespaces (crash-patterns #12). */
#undef open
#undef close
				fd = open(*argv, O_WRONLY | O_CREAT, 0666);
				if (fd == -1) {
					rval = 1;
					warn("%s", *argv);
#define open(p, f) amiport_open(p, f)
#define close(f) amiport_close(f)
					continue;
				}
				(void)close(fd);
#define open(p, f) amiport_open(p, f)
#define close(f) amiport_close(f)
			}

			/* Set timestamp on the (now existing) file */
			if (utimensat(AT_FDCWD, *argv, ts, 0) == -1) {
				warn("%s", *argv);
				rval = 1;
			}
		}
	}
	/* amiport: exit code -- return 10 on error (RETURN_ERROR) not 1
	 * (AmigaOS shells test IF ERROR/IF WARN, not unix exit 1) */
	return rval ? 10 : 0;
}

#define	ATOI2(s)	((s) += 2, ((s)[-2] - '0') * 10 + ((s)[-1] - '0'))

void
stime_arg1(char *arg, struct timespec *tsp)
{
	struct tm	*lt;
	time_t		 tmptime;
	int		 yearset;
	char		*dot, *p;
					/* Start with the current time. */
	tmptime = time(NULL);
	if ((lt = localtime(&tmptime)) == NULL)
		/* amiport: err(1,...) -> err(10,...) */
		err(10, "localtime");
					/* [[CC]YY]MMDDhhmm[.SS] */
	for (p = arg, dot = NULL; *p != '\0'; p++) {
		if (*p == '.' && dot == NULL)
			dot = p;
		else if (!isdigit((unsigned char)*p))
			goto terr;
	}
	if (dot == NULL)
		lt->tm_sec = 0;		/* Seconds defaults to 0. */
	else {
		*dot++ = '\0';
		if (strlen(dot) != 2)
			goto terr;
		lt->tm_sec = ATOI2(dot);
		if (lt->tm_sec > 61)	/* Could be leap second. */
			goto terr;
	}

	yearset = 0;
	switch (strlen(arg)) {
	case 12:			/* CCYYMMDDhhmm */
		lt->tm_year = (ATOI2(arg) * 100) - 1900;
		yearset = 1;
		/* FALLTHROUGH */
	case 10:			/* YYMMDDhhmm */
		if (yearset) {
			yearset = ATOI2(arg);
			lt->tm_year += yearset;
		} else {
			yearset = ATOI2(arg);
			/* POSIX logic: [00,68]=>20xx, [69,99]=>19xx */
			lt->tm_year = yearset;
			if (yearset < 69)
				lt->tm_year += 100;
		}
		/* FALLTHROUGH */
	case 8:				/* MMDDhhmm */
		lt->tm_mon = ATOI2(arg);
		if (lt->tm_mon > 12 || lt->tm_mon == 0)
			goto terr;
		--lt->tm_mon;		/* Convert from 01-12 to 00-11 */
		lt->tm_mday = ATOI2(arg);
		if (lt->tm_mday > 31 || lt->tm_mday == 0)
			goto terr;
		lt->tm_hour = ATOI2(arg);
		if (lt->tm_hour > 23)
			goto terr;
		lt->tm_min = ATOI2(arg);
		if (lt->tm_min > 59)
			goto terr;
		break;
	default:
		goto terr;
	}

	lt->tm_isdst = -1;		/* Figure out DST. */
	tsp[0].tv_sec = tsp[1].tv_sec = mktime(lt);
	if (tsp[0].tv_sec == -1)
	/* amiport: errx(1,...) -> errx(10,...) */
terr:		errx(10,
	"out of range or illegal time specification: [[CC]YY]MMDDhhmm[.SS]");

	tsp[0].tv_nsec = tsp[1].tv_nsec = 0;
}

void
stime_arg2(char *arg, int year, struct timespec *tsp)
{
	struct tm	*lt;
	time_t		 tmptime;
					/* Start with the current time. */
	tmptime = time(NULL);
	if ((lt = localtime(&tmptime)) == NULL)
		/* amiport: err(1,...) -> err(10,...) */
		err(10, "localtime");

	lt->tm_mon = ATOI2(arg);	/* MMDDhhmm[YY] */
	if (lt->tm_mon > 12 || lt->tm_mon == 0)
		goto terr;
	--lt->tm_mon;			/* Convert from 01-12 to 00-11 */
	lt->tm_mday = ATOI2(arg);
	if (lt->tm_mday > 31 || lt->tm_mday == 0)
		goto terr;
	lt->tm_hour = ATOI2(arg);
	if (lt->tm_hour > 23)
		goto terr;
	lt->tm_min = ATOI2(arg);
	if (lt->tm_min > 59)
		goto terr;
	if (year) {
		year = ATOI2(arg);
		/* POSIX logic: [00,68]=>20xx, [69,99]=>19xx */
		lt->tm_year = year;
		if (year < 69)
			lt->tm_year += 100;
	}
	lt->tm_sec = 0;

	lt->tm_isdst = -1;		/* Figure out DST. */
	tsp[0].tv_sec = tsp[1].tv_sec = mktime(lt);
	if (tsp[0].tv_sec == -1)
	/* amiport: errx(1,...) -> errx(10,...) */
terr:		errx(10,
	"out of range or illegal time specification: MMDDhhmm[YY]");

	tsp[0].tv_nsec = tsp[1].tv_nsec = 0;
}

void
stime_file(char *fname, struct timespec *tsp)
{
	struct amiport_stat	sb;

	/* amiport: stat() mapped to amiport_stat() via macro in amiport/sys/stat.h */
	if (stat(fname, &sb))
		/* amiport: err(1,...) -> err(10,...) */
		err(10, "%s", fname);
	/* amiport: replaced sb.st_atim / sb.st_mtim (struct timespec fields,
	 * Linux/BSD) with scalar sb.st_atime / sb.st_mtime (ULONG seconds).
	 * amiport_stat() returns Unix timestamps in these fields. */
	tsp[0].tv_sec = sb.st_atime;
	tsp[0].tv_nsec = 0;
	tsp[1].tv_sec = sb.st_mtime;
	tsp[1].tv_nsec = 0;
}

void
stime_argd(char *arg, struct timespec *tsp)
{
	/* amiport: struct tm from <time.h>. amiport_strptime() takes
	 * struct amiport_tm*, which is layout-identical to struct tm -- cast
	 * is safe on single-threaded AmigaOS. strptime macro undef'd here to
	 * call amiport_strptime directly with the cast. */
	struct tm	tm;
	char		*frac, *p;
	int		utc = 0;

	/* accept YYYY-MM-DD(T| )hh:mm:ss[(.|,)frac][Z] */
	memset(&tm, 0, sizeof(tm));
	/* amiport: call amiport_strptime with cast -- struct tm and
	 * struct amiport_tm are layout-identical (same fields, same order) */
	p = amiport_strptime(arg, "%F", (struct amiport_tm *)&tm);
	if (p == NULL || (*p != 'T' && *p != ' '))
		goto terr;
	p = amiport_strptime(p + 1, "%T", (struct amiport_tm *)&tm);
	if (p == NULL)
		goto terr;
	tsp[0].tv_nsec = 0;
	if (*p == '.' || *p == ',') {
		frac = ++p;
		while (isdigit((unsigned char)*p)) {
			if (p - frac < 9) {
				tsp[0].tv_nsec = tsp[0].tv_nsec * 10 +
				    *p - '0';
			}
			p++;
		}
		if (p == frac)
			goto terr;

		/* fill in the trailing zeros */
		while (p - frac-- < 9)
			tsp[0].tv_nsec *= 10;
	}
	if (*p == 'Z') {
		utc = 1;
		p++;
	}
	if (*p != '\0')
		goto terr;

	tm.tm_isdst = -1;
	/* amiport: timegm() mapped to amiport_timegm() via macro in
	 * amiport/unistd.h */
	tsp[0].tv_sec = utc ? timegm(&tm) : mktime(&tm);
	if (tsp[0].tv_sec == -1)
	/* amiport: errx(1,...) -> errx(10,...) */
terr:		errx(10,
  "out of range or illegal time specification: YYYY-MM-DDThh:mm:ss[.frac][Z]");
	tsp[1] = tsp[0];
}

static void __dead
usage(void)
{
	(void)fprintf(stderr,
"usage: touch [-acm] [-d ccyy-mm-ddTHH:MM:SS[.frac][Z]] [-r file]\n"
"             [-t [[cc]yy]mmddHHMM[.SS]] file ...\n");
	/* amiport: exit(1) -> exit(10) (RETURN_ERROR for AmigaOS shell scripts) */
	exit(10);
}
