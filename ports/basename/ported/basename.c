/*	$OpenBSD: basename.c,v 1.14 2016/10/28 07:22:59 schwarze Exp $	*/
/*	$NetBSD: basename.c,v 1.9 1995/09/02 05:29:46 jtc Exp $	*/

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

/* amiport: replaced <err.h> with <amiport/err.h> */
#include <amiport/err.h>
#include <libgen.h>
#include <stdio.h>
/* amiport: replaced <stdlib.h> with <amiport/stdlib.h> -- activates exit() macro */
#include <amiport/stdlib.h>
#include <string.h>
/* amiport: replaced <unistd.h> with <amiport/unistd.h> */
#include <amiport/unistd.h>
/* amiport: getopt/optind/optarg via amiport implementation (libnix getopt_long is broken) */
#include <amiport/getopt.h>
/* amiport: argv wildcard expansion for AmigaOS shell */
#include <amiport/glob.h>

/* amiport: version string */
static const char *verstag = "$VER: basename 1.14 (25.03.2026)";

/* amiport: stack cookie -- Amiga default 4KB is too small */
long __stack = 16384;

/* amiport: __progname defined in libamiport.a (argv_expand.o) as a strong symbol */
/* amiport: initialized from argv[0] by amiport_expand_argv() -- do not redefine here */

/* amiport: pledge() not available on AmigaOS -- stubbed */
#define pledge(p, e) (0)

static void usage(void);

static void cleanup(void)
{
    /* amiport: free expanded argv and flush stdout on all exit paths */
    amiport_free_argv();
    (void)fflush(stdout);
}

int
main(int argc, char *argv[])
{
    int ch;
    char *p;

    /* amiport: expand wildcard args -- AmigaOS shell does not glob */
    amiport_expand_argv(&argc, &argv);
    /* amiport: register cleanup for all exit paths including err()/errx() */
    atexit(cleanup);

    if (pledge("stdio", NULL) == -1)
        err(10, "pledge"); /* amiport: RETURN_ERROR */

    /* amiport: getopt() macro from <amiport/unistd.h> routes to amiport_getopt() */
    while ((ch = getopt(argc, argv, "")) != -1) {
        switch (ch) {
        default:
            usage();
        }
    }
    argc -= optind;
    argv += optind;

    if (argc != 1 && argc != 2)
        usage();

    if (**argv == '\0') {
        (void)puts("");
        return 0;
    }
    p = basename(*argv);
    if (p == NULL)
        err(10, "%s", *argv); /* amiport: RETURN_ERROR */
    /*
     * If the suffix operand is present, is not identical to the
     * characters remaining in string, and is identical to a suffix
     * of the characters remaining in string, the suffix suffix
     * shall be removed from string.
     */
    if (*++argv) {
        size_t suffixlen, stringlen, off;

        suffixlen = strlen(*argv);
        stringlen = strlen(p);

        if (suffixlen < stringlen) {
            off = stringlen - suffixlen;
            if (!strcmp(p + off, *argv))
                p[off] = '\0';
        }
    }
    (void)puts(p);
    return 0;
}

/* amiport: removed 'extern char *__progname' -- defined directly above */
/* amiport: removed __dead attribute -- OpenBSD-specific, not available on AmigaOS */
static void
usage(void)
{
    (void)fprintf(stderr, "usage: %s string [suffix]\n", __progname);
    exit(10); /* amiport: RETURN_ERROR */
}
