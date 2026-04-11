/*      $OpenBSD: special.c,v 1.8 2018/03/05 16:53:39 cheloha Exp $      */
/*      $NetBSD: special.c,v 1.2 1995/09/08 03:23:00 tls Exp $      */

/*-
 * Copyright (c) 1991, 1993, 1994
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

#include <sys/types.h>

/* amiport: replaced <err.h> with <amiport/err.h> -- bare <err.h> missing in bebbo-gcc libnix */
#include <amiport/err.h>
/* amiport: replaced <stdlib.h> with <amiport/stdlib.h> -- activates exit()->amiport_exit() macro */
#include <amiport/stdlib.h>
#include <stdio.h>
#include <string.h>

#include "extern.h"

/*
 * amiport: signature changed from fd-based to path-based.
 * fdopen() on amiport_open() fds silently fails (crash-patterns #12).
 * path1/path2 == NULL means stdin (fd==0 case from caller).
 * amiport: replaced fdopen() with fopen()/stdin -- avoids fd namespace mismatch.
 */
void
c_special(const char *path1, char *file1, off_t skip1,
    const char *path2, char *file2, off_t skip2)
{
	int ch1, ch2;
	off_t byte, line;
	FILE *fp1, *fp2;
	int dfound;
	int close1, close2;

	/* amiport: use fopen() for named files; use stdin for "-" argument */
	if (path1 == NULL) {
		fp1 = stdin;
		close1 = 0;
	} else if ((fp1 = fopen(path1, "r")) == NULL) {
		fatal("%s", file1);
	} else {
		close1 = 1;
	}
	if (path2 == NULL) {
		fp2 = stdin;
		close2 = 0;
	} else if ((fp2 = fopen(path2, "r")) == NULL) {
		fatal("%s", file2);
	} else {
		close2 = 1;
	}

	dfound = 0;
	while (skip1--)
		if (getc(fp1) == EOF)
			goto eof;
	while (skip2--)
		if (getc(fp2) == EOF)
			goto eof;

	for (byte = line = 1;; ++byte) {
		ch1 = getc(fp1);
		ch2 = getc(fp2);
		if (ch1 == EOF || ch2 == EOF)
			break;
		if (ch1 != ch2) {
			if (lflag) {
				dfound = 1;
				/* amiport: off_t is long (32-bit) on AmigaOS -- use %ld not %lld */
			(void)printf("%6ld %3o %3o\n", (long)byte,
				    ch1, ch2);
			} else
				diffmsg(file1, file2, byte, line);
				/* NOTREACHED */
		}
		if (ch1 == '\n')
			++line;
	}

eof:	if (ferror(fp1))
		fatal("%s", file1);
	if (ferror(fp2))
		fatal("%s", file2);
	if (feof(fp1)) {
		if (!feof(fp2))
			eofmsg(file1);
	} else
		if (feof(fp2))
			eofmsg(file2);
	/* amiport: close files we opened (not stdin) */
	if (close1) fclose(fp1);
	if (close2) fclose(fp2);
	if (dfound)
		exit(DIFF_EXIT);
}
