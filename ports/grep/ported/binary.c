/*	$OpenBSD: binary.c,v 1.20 2021/12/15 19:22:44 tb Exp $	*/
/* amiport: Ported to AmigaOS 3.x
 *   - Removed gzbin_file() (no gzip/zlib support, NOZ defined)
 *   - Removed mmbin_file() (no mmap support)
 */

/*-
 * Copyright (c) 1999 James Howard and Dag-Erling Coidan Smorgrav
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* amiport: replaced system includes with grep.h */
/* amiport: removed <zlib.h> — no gzip support (NOZ) */
#include "grep.h"

static int
isbinary(const char *buf, size_t n)
{
	return (memchr(buf, '\0', n) != NULL);
}

int
bin_file(FILE *f)
{
	/* amiport: static to avoid stack pressure — bin_file() is
	 * single-threaded and not reentrant on AmigaOS */
	static char	buf[BUFSIZ];
	size_t		m;
	int		ret = 0;

	if (fseek(f, 0L, SEEK_SET) == -1)
		return 0;

	if ((m = fread(buf, 1, BUFSIZ, f)) == 0)
		return 0;

	if (isbinary(buf, m))
		ret = 1;

	rewind(f);
	return ret;
}

/* amiport: removed gzbin_file() — no gzip support (NOZ defined) */
/* amiport: removed mmbin_file() — no mmap support on AmigaOS */
