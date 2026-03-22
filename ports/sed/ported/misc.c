/*	$OpenBSD: misc.c,v 1.13 2024/07/17 20:57:16 millert Exp $	*/

/*-
 * Copyright (c) 1992 Diomidis Spinellis.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Diomidis Spinellis of Imperial College, University of London.
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

#include <sys/types.h>

/* amiport: <err.h> provided by amiport/err.h via compat.h */
/* amiport: <regex.h> provided by amiport-emu/regex.h via compat.h */
#include <stdio.h>
#include <amiport/stdlib.h> /* amiport: exit() override prevents libnix hang */
#include <string.h>
#include <stdarg.h>

#include "defs.h"
#include "extern.h"

/*
 * malloc with result test
 */
void *
xmalloc(size_t size)
{
	void *p;

	if ((p = malloc(size)) == NULL)
		/* amiport: err(1, ...) -> err(10, ...) for Amiga RETURN_ERROR */
		err(10, NULL);
	return (p);
}

void *
xreallocarray(void *o, size_t nmemb, size_t size)
{
	void *p;

	/* amiport: reallocarray(ptr, 0, size) returns NULL without error,
	 * so we only treat it as an error if nmemb and size are both nonzero */
	if (nmemb != 0 && size != 0) {
		if ((p = reallocarray(o, nmemb, size)) == NULL)
			/* amiport: err(1, ...) -> err(10, ...) */
			err(10, NULL);
	} else {
		p = reallocarray(o, nmemb, size);
	}
	return (p);
}

/*
 * realloc with result test
 */
void *
xrealloc(void *p, size_t size)
{

	if ((p = realloc(p, size)) == NULL)
		/* amiport: err(1, ...) -> err(10, ...) */
		err(10, NULL);
	return (p);
}

/*
 * Return a string for a regular expression error passed.  This is a overkill,
 * because of the silly semantics of regerror (we can never know the size of
 * the buffer).
 */
char *
strregerror(int errcode, regex_t *preg)
{
	static char *oe;
	size_t s;

	free(oe);
	s = regerror(errcode, preg, "", 0);
	oe = xmalloc(s);
	(void)regerror(errcode, preg, oe, s);
	return (oe);
}

/*
 * Error reporting function
 */
/* amiport: __dead provided via compat.h */
__dead void
error(const char *fmt, ...)
{
	va_list ap;

	/* amiport: %lu format for Amiga LONG linenum */
	(void)fprintf(stderr, "sed: %lu: %s: ", linenum, fname);
	va_start(ap, fmt);
	(void)vfprintf(stderr, fmt, ap);
	va_end(ap);
	(void)fprintf(stderr, "\n");
	/* amiport: exit(1) -> exit(10) for Amiga RETURN_ERROR */
	exit(10);
}

void
warning(const char *fmt, ...)
{
	va_list ap;

	/* amiport: %lu format for Amiga LONG linenum */
	(void)fprintf(stderr, "sed: %lu: %s: ", linenum, fname);
	va_start(ap, fmt);
	(void)vfprintf(stderr, fmt, ap);
	va_end(ap);
	(void)fprintf(stderr, "\n");
}
