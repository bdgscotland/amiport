/*
 * amiport/err.h -- BSD err/warn family and strtonum for AmigaOS
 *
 * Provides err(), errx(), warn(), warnx(), and strtonum()
 * as inline/macro implementations for ported software.
 */

#ifndef AMIPORT_ERR_H
#define AMIPORT_ERR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

/*
 * warn/warnx -- print warning message to stderr
 */
static void
amiport_warn(const char *fmt)
{
	if (fmt != NULL)
		(void)fprintf(stderr, "%s", fmt);
	(void)fprintf(stderr, ": %s\n", strerror(errno));
}

static void
amiport_warnx(const char *fmt)
{
	if (fmt != NULL)
		(void)fprintf(stderr, "%s", fmt);
	(void)fprintf(stderr, "\n");
}

/*
 * err/errx -- print error message to stderr, then exit
 */
static void
amiport_err(int eval, const char *fmt)
{
	amiport_warn(fmt);
	exit(eval);
}

static void
amiport_errx(int eval, const char *fmt)
{
	amiport_warnx(fmt);
	exit(eval);
}

/*
 * strtonum -- OpenBSD safe string-to-number conversion
 *
 * Converts string to long long within [minval, maxval].
 * On error, sets *errstrp to a human-readable error string and returns 0.
 * On success, sets *errstrp to NULL and returns the value.
 */
static long long
amiport_strtonum(const char *numstr, long long minval, long long maxval,
    const char **errstrp)
{
	long val;
	char *ep;

	if (minval > maxval) {
		if (errstrp != NULL)
			*errstrp = "invalid";
		return (0);
	}

	errno = 0;
	val = strtol(numstr, &ep, 10);
	if (numstr == ep || *ep != '\0') {
		if (errstrp != NULL)
			*errstrp = "invalid";
		return (0);
	}
	if (errno == ERANGE || val < (long)minval || val > (long)maxval) {
		if (errstrp != NULL) {
			if (val < (long)minval)
				*errstrp = "too small";
			else
				*errstrp = "too large";
		}
		return (0);
	}

	if (errstrp != NULL)
		*errstrp = NULL;
	return ((long long)val);
}

/* Convenience macros for drop-in replacement */
#ifndef AMIPORT_NO_ERR_MACROS
#define err    amiport_err
#define errx   amiport_errx
#define warn   amiport_warn
#define warnx  amiport_warnx
#define strtonum amiport_strtonum
#endif

#endif /* AMIPORT_ERR_H */
