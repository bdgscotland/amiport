/*
 * amiport/err.h -- BSD err/warn family and strtonum for AmigaOS
 *
 * Provides err(), errx(), warn(), warnx(), warnc() and strtonum()
 * as inline implementations for ported software.
 *
 * All functions are variadic to match the BSD/POSIX signatures exactly:
 *   void err(int eval, const char *fmt, ...)
 *   void errx(int eval, const char *fmt, ...)
 *   void warn(const char *fmt, ...)
 *   void warnx(const char *fmt, ...)
 *   void warnc(int code, const char *fmt, ...)
 */

#ifndef AMIPORT_ERR_H
#define AMIPORT_ERR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>

/*
 * warn -- print "progname: fmt ...: strerror" to stderr
 */
static void
amiport_warn(const char *fmt, ...)
{
	va_list ap;
	if (fmt != NULL) {
		va_start(ap, fmt);
		(void)vfprintf(stderr, fmt, ap);
		va_end(ap);
		(void)fprintf(stderr, ": ");
	}
	(void)fprintf(stderr, "%s\n", strerror(errno));
}

/*
 * warnx -- print "progname: fmt ..." to stderr (no errno)
 */
static void
amiport_warnx(const char *fmt, ...)
{
	va_list ap;
	if (fmt != NULL) {
		va_start(ap, fmt);
		(void)vfprintf(stderr, fmt, ap);
		va_end(ap);
	}
	(void)fprintf(stderr, "\n");
}

/*
 * err -- print warning then exit
 */
static void
amiport_err(int eval, const char *fmt, ...)
{
	va_list ap;
	if (fmt != NULL) {
		va_start(ap, fmt);
		(void)vfprintf(stderr, fmt, ap);
		va_end(ap);
		(void)fprintf(stderr, ": ");
	}
	(void)fprintf(stderr, "%s\n", strerror(errno));
	exit(eval);
}

/*
 * errx -- print message then exit (no errno)
 */
static void
amiport_errx(int eval, const char *fmt, ...)
{
	va_list ap;
	if (fmt != NULL) {
		va_start(ap, fmt);
		(void)vfprintf(stderr, fmt, ap);
		va_end(ap);
	}
	(void)fprintf(stderr, "\n");
	exit(eval);
}

/*
 * warnc -- print warning message with explicit error code to stderr
 */
static void
amiport_warnc(int code, const char *fmt, ...)
{
	va_list ap;
	if (fmt != NULL) {
		va_start(ap, fmt);
		(void)vfprintf(stderr, fmt, ap);
		va_end(ap);
		(void)fprintf(stderr, ": ");
	}
	(void)fprintf(stderr, "%s\n", strerror(code));
}

/*
 * errc -- print error message with explicit error code, then exit
 *
 * Like err(), but uses the explicit error code instead of errno.
 * Used by OpenBSD tools (look, etc.).
 */
static void
amiport_errc(int eval, int code, const char *fmt, ...)
{
	va_list ap;
	if (fmt != NULL) {
		va_start(ap, fmt);
		(void)vfprintf(stderr, fmt, ap);
		va_end(ap);
		(void)fprintf(stderr, ": ");
	}
	(void)fprintf(stderr, "%s\n", strerror(code));
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
	long long val;
	char *ep;

	if (minval > maxval) {
		if (errstrp != NULL)
			*errstrp = "invalid";
		return (0);
	}

	errno = 0;
	val = strtoll(numstr, &ep, 10);
	if (numstr == ep || *ep != '\0') {
		if (errstrp != NULL)
			*errstrp = "invalid";
		return (0);
	}
	if (errno == ERANGE || val < minval || val > maxval) {
		if (errstrp != NULL) {
			if (val < minval)
				*errstrp = "too small";
			else
				*errstrp = "too large";
		}
		return (0);
	}

	if (errstrp != NULL)
		*errstrp = NULL;
	return (val);
}

/* Convenience macros for drop-in replacement */
#ifndef AMIPORT_NO_ERR_MACROS
#define err    amiport_err
#define errx   amiport_errx
#define errc   amiport_errc
#define warn   amiport_warn
#define warnc  amiport_warnc
#define warnx  amiport_warnx
#define strtonum amiport_strtonum
#endif

#endif /* AMIPORT_ERR_H */
