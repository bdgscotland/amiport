/*	$OpenBSD: input.c,v 1.19 2017/08/13 02:12:16 tedu Exp $	*/
/*    $NetBSD: input.c,v 1.3 1996/02/06 22:47:33 jtc Exp $    */

/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek and Darren F. Provine.
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
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *	@(#)input.c	8.1 (Berkeley) 5/31/93
 */

/*
 * Tetris input -- AmigaOS port.
 *
 * amiport-redesign: Complete redesign of timing mechanism (Tier 3).
 * Original: ppoll() + clock_gettime(CLOCK_MONOTONIC) on POSIX
 * AmigaOS:  WaitForChar(Input(), timeout_us) via dos.library
 *
 * WaitForChar() blocks up to the given timeout for console input.
 * Return value: TRUE (non-zero) if input is available, FALSE (0) on timeout.
 * Granularity: ~1/50s (PAL Amiga, 50 Hz timer.device) ~= 20ms.
 * This is more than sufficient for game timing at the nanosecond->microsecond
 * resolution of fallrate (1 billion / level).
 *
 * The timespec interface is preserved so callers (tgetchar, tsleep) are
 * unchanged. We convert tv_sec/tv_nsec to microseconds for WaitForChar.
 */

/* amiport: removed #include <sys/time.h> -- not available on AmigaOS */
/* amiport: removed #include <poll.h> -- not available on AmigaOS */

#include <time.h>
#include <errno.h>

/* amiport: AmigaOS headers for WaitForChar and Input() */
#include <proto/dos.h>
#include <amiport/signal.h>

#include "input.h"
#include "tetris.h"

/*
 * BSD sys/time.h macros -- not available on AmigaOS libnix, defined locally.
 * amiport: timespecclear and timespecsub defined locally (BSD sys/time.h macros)
 */
#define timespecclear(ts) ((ts)->tv_sec = (ts)->tv_nsec = 0)
#define timespecsub(a, b, result) \
    do { \
        (result)->tv_sec  = (a)->tv_sec  - (b)->tv_sec; \
        (result)->tv_nsec = (a)->tv_nsec - (b)->tv_nsec; \
        if ((result)->tv_nsec < 0) { \
            (result)->tv_sec--; \
            (result)->tv_nsec += 1000000000L; \
        } \
    } while (0)

/* return true iff the given timespec is positive */
#define	TS_POS(ts) \
	((ts)->tv_sec > 0 || ((ts)->tv_sec == 0 && (ts)->tv_nsec > 0))

/*
 * Do a `read wait': wait for input from stdin with timeout *limit.
 * On return, clear *limit to zero (WaitForChar does not return elapsed time;
 * we conservatively zero the limit after the call so callers do not loop).
 *
 * If limit is NULL, wait forever with Ctrl-C check.
 *
 * Return 0 => no input (timeout), 1 => input available, -1 => Ctrl-C
 *
 * amiport: replaced ppoll()+clock_gettime() with WaitForChar(Input(), us)
 * WaitForChar granularity ~20ms (50Hz timer). Elapsed time is not tracked --
 * timeout is consumed fully on each call. Callers (tsleep) loop until TS_POS
 * goes false; since we clear limit after each call, the loop runs at most once
 * per tsleep() invocation. tgetchar() sets timeleft to fallrate once and calls
 * rwait() once per game tick, which matches the original behaviour.
 */
int
rwait(struct timespec *limit)
{
	BPTR fh;
	LONG timeout_us;

	fh = Input();

	if (limit == NULL) {
		/* amiport: wait forever, polling in 500ms slices for Ctrl-C */
		while (!WaitForChar(fh, 500000L)) {
			if (amiport_check_break()) {
				return (-1);
			}
		}
		return (1);
	}

	/* amiport: convert timespec to microseconds for WaitForChar */
	timeout_us = (LONG)(limit->tv_sec * 1000000L + limit->tv_nsec / 1000L);
	if (timeout_us <= 0) {
		timespecclear(limit);
		return (0);
	}

	/* amiport: check Ctrl-C before blocking */
	if (amiport_check_break()) {
		timespecclear(limit);
		return (-1);
	}

	if (WaitForChar(fh, timeout_us)) {
		/*
		 * Input is available. Conservatively zero the remaining time
		 * so the caller's TS_POS loop terminates.
		 */
		timespecclear(limit);
		return (1);
	}

	/* Timeout expired */
	timespecclear(limit);
	return (0);
}

/*
 * `sleep' for the current turn time and eat any
 * input that becomes available.
 * amiport: read(STDIN_FILENO,...) replaced with FGetC(Input())
 */
void
tsleep(void)
{
	struct timespec ts;
	LONG c;
	BPTR fh;

	fh = Input();
	ts.tv_sec = 0;
	ts.tv_nsec = fallrate;
	while (TS_POS(&ts)) {
		if (rwait(&ts)) {
			/* amiport: replaced read(STDIN_FILENO,&c,1) with FGetC(Input()) */
			c = FGetC(fh);
			if (c == -1) /* EOF or error */
				break;
		}
	}
}

/*
 * getchar with timeout.
 * amiport: read(STDIN_FILENO,...) replaced with FGetC(Input())
 */
int
tgetchar(void)
{
	static struct timespec timeleft;
	LONG c;
	BPTR fh;

	fh = Input();

	/*
	 * Reset timeleft to fallrate whenever it is not positive.
	 * In any case, wait to see if there is any input.  If so,
	 * take it, and update timeleft so that the next call to
	 * tgetchar() will not wait as long.  If there is no input,
	 * make timeleft zero or negative, and return -1.
	 *
	 * Most of the hard work is done by rwait().
	 */
	if (!TS_POS(&timeleft)) {
		faster();	/* go faster */
		timeleft.tv_sec = 0;
		timeleft.tv_nsec = fallrate;
	}
	if (!rwait(&timeleft))
		return (-1);
	/* amiport: replaced read(STDIN_FILENO, &c, 1) with FGetC(Input()) */
	c = FGetC(fh);
	if (c == -1)
		stop("end of file, help");
	return ((int)(unsigned char)c);
}
