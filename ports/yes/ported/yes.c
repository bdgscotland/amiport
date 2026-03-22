/*	$OpenBSD: yes.c,v 1.9 2015/10/13 07:03:26 doug Exp $	*/
/*	$NetBSD: yes.c,v 1.3 1994/11/14 04:56:15 jtc Exp $	*/

/*
 * Copyright (c) 1987, 1993
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

/* amiport: replaced <err.h> with <amiport/err.h> for amiport_err() */
#include <amiport/err.h>
#include <stdio.h>
/* amiport: replaced <unistd.h> with <amiport/unistd.h> */
#include <amiport/unistd.h>
/* amiport: added <amiport/signal.h> for amiport_check_break() */
#include <amiport/signal.h>

/* amiport: stub OpenBSD pledge/unveil — no Amiga equivalent */
#define pledge(p, e) (0)
#define unveil(p, f) (0)

/* amiport: stack cookie — Amiga default is 4KB */
long __stack = 8192;

/* amiport: Amiga version string using upstream OpenBSD rev 1.9 */
static const char *verstag = "$VER: yes 1.9 (21.03.2026)";

int
main(int argc, char *argv[])
{
	if (pledge("stdio", NULL) == -1)
		/* amiport: changed exit code 1 to 10 (RETURN_ERROR) */
		err(10, "pledge");

	if (argc > 1)
		for (;;) {
			/* amiport: added Ctrl-C check — no SIGINT on AmigaOS */
			if (amiport_check_break()) {
				(void)fflush(stdout);
				return 0;
			}
			puts(argv[1]);
		}
	else
		for (;;) {
			/* amiport: added Ctrl-C check — no SIGINT on AmigaOS */
			if (amiport_check_break()) {
				(void)fflush(stdout);
				return 0;
			}
			puts("y");
		}
}
