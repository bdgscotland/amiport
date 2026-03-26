/*	$OpenBSD: dirname.c,v 1.17 2019/01/25 00:19:26 millert Exp $	*/

/*
 * Copyright (c) 1997 Todd C. Miller <millert@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* amiport: replaced <err.h> with <amiport/err.h> */
#include <amiport/err.h>
/* amiport: removed <libgen.h> -- libnix dirname() has bugs with 2-char paths
 * ("/a" returns "/a" instead of "/") and double slashes. Using local
 * POSIX-correct implementation instead. */
#include <stdio.h>
#include <string.h>
/* amiport: replaced <stdlib.h> with <amiport/stdlib.h> -- activates exit() macro */
#include <amiport/stdlib.h>
/* amiport: replaced <unistd.h> with <amiport/unistd.h> */
#include <amiport/unistd.h>
/* amiport: glob.h provides amiport_expand_argv/amiport_free_argv and __progname */
#include <amiport/glob.h>
/* amiport: getopt.h provides amiport_getopt; macros alias getopt/optind */
#include <amiport/getopt.h>

/* amiport: AmigaOS version string */
static const char *verstag = "$VER: dirname 1.17 (25.03.2026)";

/* amiport: stack cookie -- Amiga default 4KB is too small */
long __stack = 16384;

/* amiport: __progname provided by argv_expand.o in libamiport.a via amiport_expand_argv() */
/* Do NOT define __progname here -- argv_expand.o has a strong definition that conflicts */

/* amiport: pledge/unveil not available on AmigaOS -- stubbed as macros */
#define pledge(p, e) (0)
#define unveil(p, f) (0)

/* amiport: __dead attribute not available on AmigaOS -- removed */
static void usage(void);

/*
 * amiport: local POSIX-correct dirname() to replace buggy libnix version.
 * libnix dirname() fails on 2-char paths ("/a" -> "/a" instead of "/")
 * and doesn't collapse consecutive slashes ("/usr//bin" -> "/usr/" instead
 * of "/usr"). This implementation follows POSIX.1-2008 step-by-step.
 * Uses a static buffer (safe -- AmigaOS is single-threaded).
 */
static char dirname_buf[1024];

static char *
amiport_dirname(char *path)
{
    char *endp;
    size_t len;

    /* Empty or NULL -> "." */
    if (path == NULL || *path == '\0') {
        dirname_buf[0] = '.';
        dirname_buf[1] = '\0';
        return dirname_buf;
    }

    len = strlen(path);
    if (len >= sizeof(dirname_buf))
        len = sizeof(dirname_buf) - 1;
    memcpy(dirname_buf, path, len);
    dirname_buf[len] = '\0';
    endp = dirname_buf + len - 1;

    /* Strip trailing slashes */
    while (endp > dirname_buf && *endp == '/')
        endp--;

    /* If all slashes, return "/" */
    if (endp == dirname_buf && *endp == '/') {
        dirname_buf[1] = '\0';
        return dirname_buf;
    }

    /* Strip trailing non-slash component */
    while (endp > dirname_buf && *endp != '/')
        endp--;

    /* If no slash found, return "." */
    if (endp == dirname_buf && *endp != '/') {
        dirname_buf[0] = '.';
        dirname_buf[1] = '\0';
        return dirname_buf;
    }

    /* Strip trailing slashes from directory part */
    while (endp > dirname_buf && *endp == '/')
        endp--;

    /* If only root slash remains */
    if (endp == dirname_buf && *endp == '/') {
        dirname_buf[1] = '\0';
        return dirname_buf;
    }

    *(endp + 1) = '\0';
    return dirname_buf;
}

/* amiport: atexit cleanup -- flushes stdout and frees expanded argv */
static void
cleanup(void)
{
    amiport_free_argv();
    (void)fflush(stdout);
}

int
main(int argc, char *argv[])
{
    int ch;
    char *dir;

    /* amiport: expand wildcard args -- Amiga shell does not glob */
    amiport_expand_argv(&argc, &argv);
    /* amiport: register cleanup for all exit paths including err()/errx() */
    atexit(cleanup);

    /* amiport: pledge() stubbed -- no-op on AmigaOS */
    if (pledge("stdio", NULL) == -1)
        err(10, "pledge"); /* amiport: RETURN_ERROR */

    /* amiport: getopt() aliased to amiport_getopt() via <amiport/getopt.h> */
    while ((ch = getopt(argc, argv, "")) != -1) {
        switch (ch) {
        default:
            usage();
        }
    }
    argc -= optind;
    argv += optind;

    if (argc != 1)
        usage();

    /* amiport: using local amiport_dirname() instead of libnix dirname() --
     * libnix has bugs with 2-char paths and double slashes. Our implementation
     * uses a static buffer, so no input corruption concern. */
    if ((dir = amiport_dirname(argv[0])) == NULL)
        err(10, "%s", argv[0]); /* amiport: RETURN_ERROR */
    puts(dir);
    return 0;
}

/* amiport: __dead attribute removed -- not available on AmigaOS */
static void
usage(void)
{
    (void)fprintf(stderr, "usage: %s pathname\n", __progname);
    exit(10); /* amiport: RETURN_ERROR */
}
