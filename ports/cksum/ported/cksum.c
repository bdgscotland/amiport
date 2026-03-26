/*-
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * James W. Williams of NASA Goddard Space Flight Center.
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

/* amiport: replaced <err.h> with amiport shim for err/warn/warnx */
#include <amiport/err.h>
/* amiport: removed <fcntl.h> -- O_RDONLY provided by <amiport/unistd.h> */
#include <stdio.h>
/* amiport: replaced <stdlib.h> with amiport shim -- activates exit() macro */
#include <amiport/stdlib.h>
#include <string.h>
/* amiport: replaced <unistd.h> with amiport shim -- provides open/read/close macros */
#include <amiport/unistd.h>
/* amiport: replaced <getopt.h> with amiport shim -- libnix getopt_long is broken */
#include <amiport/getopt.h>
/* amiport: argv wildcard expansion for AmigaOS shell */
#include <amiport/glob.h>

#include "extern.h"

/* amiport: Amiga version string -- __attribute__((used)) prevents linker removal */
static const char * __attribute__((used)) verstag = "$VER: cksum 1.0 (26.03.2026)";

/* amiport: stack cookie -- Amiga default 4KB is too small */
LONG __stack = 65536;

/* amiport: __dead2 (FreeBSD noreturn attribute) not defined in libnix.
 * Replace with GCC __attribute__((noreturn)) which bebbo-gcc supports. */
static void usage(void) __attribute__((noreturn));

/* amiport: atexit cleanup -- frees expanded argv on all exit paths */
static void
cleanup(void)
{
    amiport_free_argv();
    (void)fflush(stdout);
}

int
main(int argc, char **argv)
{
    uint32_t val;
    int ch, fd, rval;
    off_t len;
    char *fn, *p;
    int (*cfncn)(int, uint32_t *, off_t *);
    void (*pfncn)(char *, uint32_t, off_t);

    /* amiport: expand wildcard args -- Amiga shell does not glob */
    amiport_expand_argv(&argc, &argv);
    /* amiport: register cleanup for all exit paths including err()/errx() */
    atexit(cleanup);

    if ((p = strrchr(argv[0], '/')) == NULL)
        p = argv[0];
    else
        ++p;
    if (!strcmp(p, "sum")) {
        cfncn = csum1;
        pfncn = psum1;
        ++argv;
    } else {
        cfncn = crc;
        pfncn = pcrc;

        while ((ch = getopt(argc, argv, "o:")) != -1)
            switch (ch) {
            case 'o':
                if (!strcmp(optarg, "1")) {
                    cfncn = csum1;
                    pfncn = psum1;
                } else if (!strcmp(optarg, "2")) {
                    cfncn = csum2;
                    pfncn = psum2;
                } else if (!strcmp(optarg, "3")) {
                    cfncn = crc32;
                    pfncn = pcrc;
                } else {
                    warnx("illegal argument to -o option");
                    usage();
                }
                break;
            case '?':
            default:
                usage();
            }
        argc -= optind;
        argv += optind;
    }

    /* amiport: open() -> amiport_open() via macro from <amiport/unistd.h> */
    fd = STDIN_FILENO;
    fn = NULL;
    rval = 0;
    do {
        if (*argv) {
            fn = *argv++;
            if ((fd = open(fn, O_RDONLY)) < 0) { // nosemgrep: cpp.lang.security.filesystem.path-manipulation.path-manipulation -- fn is a CLI argv path
                warn("%s", fn);
                rval = 10; /* amiport: RETURN_ERROR -- visible to Amiga IF ERROR scripts */
                continue;
            }
        }
        if (cfncn(fd, &val, &len)) {
            warn("%s", fn ? fn : "stdin");
            rval = 10; /* amiport: RETURN_ERROR */
        } else
            pfncn(fn, val, len);
        /* amiport: close() -> amiport_close() via macro from <amiport/unistd.h> */
        (void)close(fd);
    } while (*argv);
    exit(rval);
}

static void
usage(void)
{
    (void)fprintf(stderr, "usage: cksum [-o 1 | 2 | 3] [file ...]\n");
    (void)fprintf(stderr, "       sum [file ...]\n");
    exit(10); /* amiport: RETURN_ERROR -- replaced exit(1) */
    __builtin_unreachable(); /* amiport: satisfies noreturn attribute when exit() is macro'd */
}
