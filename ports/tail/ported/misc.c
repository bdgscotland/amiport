/*	$OpenBSD: misc.c,v 1.9 2015/11/19 17:50:04 tedu Exp $	*/

/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Edward Sze-Tyan Wang.
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

/* amiport: removed #include <sys/types.h> — types provided by Amiga headers */
/* amiport: replaced #include <sys/stat.h> with amiport wrapper */
#include <amiport/sys/stat.h>

/* amiport: replaced #include <err.h> with amiport wrapper */
#include <amiport/err.h>
#include <stdio.h>

#include "extern.h"

void
ierr(const char *fname)
{
	warn("%s", fname);
	/*
	 * amiport: rval = 1 -> rval = 5 (RETURN_WARN) so AmigaOS shell
	 * IF WARN checks can detect non-fatal input errors (e.g., unreadable file).
	 */
	rval = 5;
}

void
oerr(void)
{
	/* amiport: err(1,...) -> err(10,...) for AmigaOS RETURN_ERROR */
	err(10, "stdout");
}

void printfname(const char *fname)
{
	static int first = 1;
	(void)printf("%s==> %s <==\n", first ? "" : "\n", fname);
	first = 0;
	(void)fflush(stdout);
}
