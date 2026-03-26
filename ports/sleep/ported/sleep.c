/*	$OpenBSD: sleep.c,v 1.29 2020/02/25 15:46:15 cheloha Exp $	*/
/*	$NetBSD: sleep.c,v 1.8 1995/03/21 09:11:11 cgd Exp $	*/

/*
 * Copyright (c) 1988, 1993, 1994
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

/* amiport: Amiga version string */
static const char *verstag = "$VER: sleep 1.29 (26.03.2026)";

/* amiport: stack size cookie -- Amiga default 4KB is too small */
long __stack = 8192;

/* amiport: replaced <sys/time.h> with <proto/dos.h> for Delay().
 * struct timespec / timespecclear / timespecisset not available on AmigaOS --
 * replaced with plain long variables for tv_sec and tv_nsec. */
#include <proto/dos.h>

#include <ctype.h>
/* amiport: replaced <err.h> with <amiport/err.h> */
#include <amiport/err.h>
/* amiport: removed <signal.h> -- SIGALRM does not exist on AmigaOS.
 * The signal handler (alarmh) and signal(SIGALRM, alarmh) call are removed. */
#include <stdio.h>
/* amiport: replaced <stdlib.h> with <amiport/stdlib.h> (activates exit() macro) */
#include <amiport/stdlib.h>
#include <time.h>
/* amiport: replaced <unistd.h> with <amiport/unistd.h> */
#include <amiport/unistd.h>
/* amiport: included for getopt(), optind (libnix getopt_long is broken) */
#include <amiport/getopt.h>
/* amiport: included for getprogname() macro (uses __progname) */
#include <amiport/utsname.h>
/* amiport: included for amiport_expand_argv() / amiport_free_argv() */
#include <amiport/glob.h>

/* amiport: pledge/unveil not available on AmigaOS -- stubbed */
#define pledge(p, e) (0)

void usage(void);

/* amiport: cleanup for atexit() -- frees expanded argv and flushes stdout */
static void
cleanup(void)
{
    amiport_free_argv();
    (void)fflush(stdout);
}

int
main(int argc, char *argv[])
{
    /* amiport: replaced struct timespec with plain long variables.
     * tv_nsec holds nanoseconds (0..999999999). */
    long tv_sec;
    long tv_nsec;
    time_t t;
    char *cp;
    int ch, i;
    /* amiport: ticks for Delay() (1 tick = 1/50 second) */
    long ticks;

    /* amiport: expand wildcard args -- Amiga shell doesn't glob */
    amiport_expand_argv(&argc, &argv);
    /* amiport: register cleanup for all exit paths including err()/errx() */
    atexit(cleanup);

    if (pledge("stdio", NULL) == -1)
        err(10, "pledge"); /* amiport: RETURN_ERROR */

    /* amiport: removed signal(SIGALRM, alarmh) -- SIGALRM does not exist on AmigaOS */

    while ((ch = getopt(argc, argv, "")) != -1) {
        switch(ch) {
        default:
            usage();
        }
    }
    argc -= optind;
    argv += optind;

    if (argc != 1)
        usage();

    /* amiport: replaced timespecclear(&rqtp) with explicit zero init */
    tv_sec = 0;
    tv_nsec = 0;

    /* Handle whole seconds. */
    for (cp = argv[0]; *cp != '\0' && *cp != '.'; cp++) {
        if (!isdigit((unsigned char)*cp))
            errx(10, "seconds is invalid: %s", argv[0]); /* amiport: RETURN_ERROR */
        t = (tv_sec * 10) + (*cp - '0');
        if (t / 10 != tv_sec)  /* overflow */
            errx(10, "seconds is too large: %s", argv[0]); /* amiport: RETURN_ERROR */
        tv_sec = t;
    }

    /*
     * Handle fractions of a second.  The multiplier divides to zero
     * after nine digits so anything more precise than a nanosecond is
     * validated but not used.
     */
    if (*cp == '.') {
        i = 100000000;
        for (cp++; *cp != '\0'; cp++) {
            if (!isdigit((unsigned char)*cp))
                errx(10, "seconds is invalid: %s", argv[0]); /* amiport: RETURN_ERROR */
            tv_nsec += (*cp - '0') * i;
            i /= 10;
        }
    }

    /* amiport: replaced timespecisset() + nanosleep() with Delay().
     * Delay() takes ticks (1/50 second each).
     * Whole seconds: seconds * 50 ticks.
     * Fractional part: tv_nsec nanoseconds -> (tv_nsec / 20000000) ticks
     * (1 tick = 20,000,000 ns). Add 1 tick for any remaining nanoseconds
     * so that e.g. "sleep 0.1" waits at least 5 ticks (100ms) rather than
     * being truncated to 0 and returning immediately. */
    if (tv_sec > 0 || tv_nsec > 0) {
        ticks = tv_sec * 50L;
        if (tv_nsec > 0) {
            ticks += tv_nsec / 20000000L;
            if (tv_nsec % 20000000L != 0)
                ticks += 1; /* round up to next tick */
        }
        if (ticks > 0) {
            Delay(ticks);
        }
    }

    return 0;
}

void
usage(void)
{
    fprintf(stderr, "usage: %s seconds\n", getprogname());
    exit(10); /* amiport: RETURN_ERROR */
}

/* amiport: removed alarmh() signal handler -- SIGALRM does not exist on AmigaOS.
 * POSIX.1 says sleep(1) may exit with status zero upon receipt of SIGALRM.
 * On AmigaOS this is irrelevant; Delay() is not interruptible by signals. */
