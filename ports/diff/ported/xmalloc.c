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

void *
xmalloc(size_t size)
{
	void *ptr;

	if (size == 0)
		errx(2, "xmalloc: zero size");
	ptr = malloc(size);
	if (ptr == NULL)
		err(2, "xmalloc: allocating %lu bytes", (unsigned long)size); /* amiport: %zu -> %lu */
	return ptr;
}

void *
xcalloc(size_t nmemb, size_t size)
{
	void *ptr;

	ptr = calloc(nmemb, size);
	if (ptr == NULL)
		err(2, "xcalloc: allocating %lu * %lu bytes",
		    (unsigned long)nmemb, (unsigned long)size); /* amiport: %zu -> %lu */
	return ptr;
}

void *
xreallocarray(void *ptr, size_t nmemb, size_t size)
{
	void *new_ptr;

	/* amiport: replaced reallocarray() with overflow check + realloc() */
	if (nmemb != 0 && size != 0 && nmemb > SIZE_MAX / size)
		errx(2, "xreallocarray: overflow %lu * %lu",
		    (unsigned long)nmemb, (unsigned long)size);
	new_ptr = realloc(ptr, nmemb * size);
	if (new_ptr == NULL)
		err(2, "xreallocarray: allocating %lu * %lu bytes",
		    (unsigned long)nmemb, (unsigned long)size); /* amiport: %zu -> %lu */
	return new_ptr;
}

char *
xstrdup(const char *str)
{
	char *cp;

	if ((cp = strdup(str)) == NULL)
		err(2, "xstrdup");
	return cp;
}

int
xasprintf(char **ret, const char *fmt, ...)
{
	va_list ap;
	int i;
	char buf[1024]; /* amiport: local buffer for vsnprintf */

	va_start(ap, fmt);
	i = vsnprintf(buf, sizeof(buf), fmt, ap); /* amiport: replaced vasprintf() with vsnprintf() */
	va_end(ap);

	if (i < 0)
		err(2, "xasprintf");

	*ret = xmalloc((size_t)i + 1);
	memcpy(*ret, buf, (size_t)i + 1);

	return i;
}
