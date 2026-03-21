/* $OpenBSD: xmalloc.c,v 1.10 2019/06/28 05:44:09 deraadt Exp $ */
/*
 * Author: Tatu Ylonen <ylo@cs.hut.fi>
 * Copyright (c) 1995 Tatu Ylonen <ylo@cs.hut.fi>, Espoo, Finland
 *                    All rights reserved
 * Versions of malloc and friends that check their results, and never return
 * failure (they call fatal if they encounter an error).
 *
 * As far as I am concerned, the code I have written for this software
 * can be used freely for any purpose.  Any derived versions of this
 * software must be clearly marked as such, and if the derived work is
 * incompatible with the protocol description in the RFC file, it must be
 * called by a name other than "ssh" or "Secure Shell".
 */

#include <amiport/err.h> /* amiport: replaced <err.h> */
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xmalloc.h"

/* amiport: all err()/errx() calls use exit code 10 (RETURN_ERROR) — POSIX exit(2)
 * is invisible to AmigaOS shell IF WARN/IF ERROR checks. */

void *
xmalloc(size_t size)
{
	void *ptr;

	if (size == 0)
		errx(10, "xmalloc: zero size");
	ptr = malloc(size);
	if (ptr == NULL)
		err(10, "xmalloc: allocating %lu bytes", (unsigned long)size); /* amiport: %zu -> %lu */
	return ptr;
}

void *
xcalloc(size_t nmemb, size_t size)
{
	void *ptr;

	ptr = calloc(nmemb, size);
	if (ptr == NULL)
		err(10, "xcalloc: allocating %lu * %lu bytes",
		    (unsigned long)nmemb, (unsigned long)size); /* amiport: %zu -> %lu */
	return ptr;
}

void *
xreallocarray(void *ptr, size_t nmemb, size_t size)
{
	void *new_ptr;

	/* amiport: replaced reallocarray() with overflow check + realloc() */
	if (nmemb != 0 && size != 0 && nmemb > SIZE_MAX / size)
		errx(10, "xreallocarray: overflow %lu * %lu",
		    (unsigned long)nmemb, (unsigned long)size);
	new_ptr = realloc(ptr, nmemb * size);
	if (new_ptr == NULL)
		err(10, "xreallocarray: allocating %lu * %lu bytes",
		    (unsigned long)nmemb, (unsigned long)size); /* amiport: %zu -> %lu */
	return new_ptr;
}

char *
xstrdup(const char *str)
{
	char *cp;

	if ((cp = strdup(str)) == NULL)
		err(10, "xstrdup");
	return cp;
}

int
xasprintf(char **ret, const char *fmt, ...)
{
	va_list ap;
	va_list ap2;
	int i;

	/* amiport: replaced vasprintf() — two-pass vsnprintf to avoid
	 * fixed-buffer truncation. First pass measures, second allocates. */
	va_start(ap, fmt);
	va_copy(ap2, ap);
	{
		char probe[1024];
		i = vsnprintf(probe, sizeof(probe), fmt, ap); /* amiport: libnix vsnprintf does not support NULL destination (C99) */
	}
	va_end(ap);

	if (i < 0) {
		va_end(ap2);
		err(10, "xasprintf"); /* amiport: exit(2) -> exit(10) RETURN_ERROR */
	}

	*ret = xmalloc((size_t)i + 1);
	vsnprintf(*ret, (size_t)i + 1, fmt, ap2);
	va_end(ap2);

	return i;
}
