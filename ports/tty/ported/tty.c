/*	$OpenBSD: tty.c,v 1.14 2024/05/06 16:49:46 cheloha Exp $	*/
/*	$NetBSD: tty.c,v 1.4 1994/12/07 00:46:57 jtc Exp $	*/

/*
 * Copyright (c) 1988, 1993
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

#include <stdio.h>
/* amiport: replaced <stdlib.h> with <amiport/stdlib.h> -- activates exit() macro */
#include <amiport/stdlib.h>
/* amiport: replaced <unistd.h> with <amiport/unistd.h> */
#include <amiport/unistd.h>
/* amiport: removed <paths.h> -- OpenBSD-specific; _PATH_DEVDB not needed (unveil stubbed) */
#include <amiport/err.h>
/* amiport: replaced <getopt.h> with <amiport/getopt.h> -- libnix getopt_long is broken */
#include <amiport/getopt.h>
/* amiport: AmigaOS dos.library for IsInteractive() / Input() */
#include <proto/dos.h>
/* amiport: argv wildcard expansion */
#include <amiport/glob.h>

/* amiport: pledge/unveil not available on AmigaOS -- stubbed as no-ops */
#define pledge(p, e) (0)
#define unveil(p, f) (0)

/* amiport: Amiga version string */
static const char *verstag = "$VER: tty 1.14 (26.03.2026)";

/* amiport: stack size cookie -- Amiga default 4KB is too small */
long __stack = 16384;

static void cleanup(void);
static void usage(void);

static void
cleanup(void)
{
    /* amiport: free expanded argv on all exit paths including err()/errx() */
    amiport_free_argv();
    (void)fflush(stdout);
}

int
main(int argc, char *argv[])
{
    int ch, sflag;
    /* amiport: t is the tty name -- on AmigaOS we use IsInteractive(Input()) */
    const char *t;

    /* amiport: expand wildcard args -- Amiga shell doesn't glob */
    amiport_expand_argv(&argc, &argv);
    /* amiport: register cleanup for all exit paths including err()/errx() */
    atexit(cleanup);

    sflag = 0;
    while ((ch = getopt(argc, argv, "s")) != -1) {
        switch(ch) {
        case 's':
            sflag = 1;
            break;
        default:
            usage();
            /* NOTREACHED */
        }
    }

    /* amiport: pledge/unveil stubbed -- no-ops on AmigaOS */
    if (unveil(NULL, "r") == -1)
        err(20, "unveil");
    if (pledge("stdio rpath", NULL) == -1)
        err(20, "pledge");

    /*
     * amiport: ttyname(STDIN_FILENO) replaced with IsInteractive(Input()).
     * AmigaOS has no /dev/tty device and no ttyname() function.
     * IsInteractive() tests whether the process Input() handle is connected
     * to an interactive console (CON:, WINDOW:, or "*") vs a file or pipe.
     * We report "CON:" when interactive (the canonical AmigaOS console name)
     * and NULL when not interactive, matching POSIX semantics (NULL = not a tty).
     */
    if (IsInteractive(Input())) {
        t = "CON:"; /* amiport: AmigaOS interactive console */
    } else {
        t = NULL;   /* amiport: not connected to a console */
    }

    if (!sflag)
        puts(t ? t : "not a tty");
    /* amiport: exit(1) -> exit(10) RETURN_ERROR; exit(0) unchanged */
    exit(t ? 0 : 10);
}

static void
usage(void)
{
    fprintf(stderr, "usage: tty [-s]\n");
    /* amiport: exit(2) -> exit(20) RETURN_FAIL */
    exit(20);
}
