/* See LICENSE file for copyright and license details. */
/* amiport: exit(1) -> exit(10) for Amiga error return code */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* amiport: replaced <stdlib.h> with amiport/stdlib.h for exit() -> amiport_exit() */
#include <amiport/stdlib.h>

/* amiport: removed #include "../util.h" (pulls in <regex.h> unnecessarily)
 * replaced with sponge-util.h which only declares what sponge uses */
#include "sponge-util.h"

char *argv0;

static void xvprintf(const char *, va_list);

void
eprintf(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    xvprintf(fmt, ap);
    va_end(ap);

    /* amiport: exit(1) -> exit(10) -- POSIX exit(1) is invisible to Amiga shells;
     * exit(10) maps to RETURN_ERROR which AmigaDOS IF WARN/IF ERROR scripts detect */
    exit(10);
}

void
enprintf(int status, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    xvprintf(fmt, ap);
    va_end(ap);

    exit(status);
}

void
weprintf(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    xvprintf(fmt, ap);
    va_end(ap);
}

static void
xvprintf(const char *fmt, va_list ap)
{
    if (argv0 && strncmp(fmt, "usage", strlen("usage")))
        fprintf(stderr, "%s: ", argv0);

    vfprintf(stderr, fmt, ap);

    if (fmt[0] && fmt[strlen(fmt)-1] == ':') {
        fputc(' ', stderr);
        perror(NULL);
    }
}
