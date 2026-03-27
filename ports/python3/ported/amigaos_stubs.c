/*
 * amigaos_stubs.c -- Stub functions for CPython 3.11 on AmigaOS
 *
 * amiport: Provides missing POSIX functions and CPython build-generated
 * stubs that aren't available on AmigaOS or without the CPython build system.
 *
 * ALL comments are pure ASCII -- bebbo-gcc (GCC 6.5.0b) silently eats
 * functions following UTF-8 bytes in source, even inside comments.
 */

#include "Python.h"
#include "pycore_initconfig.h"  /* _PyStatus_OK(), PyStatus */
#include <proto/dos.h>
#include <time.h>    /* struct timespec, time_t */
#include <math.h>

/* -------------------------------------------------------------------
 * clock_gettime() types and constants -- not in AmigaOS system headers
 *
 * pyconfig.h defines HAVE_CLOCK_GETTIME=1, which causes pytime.c to
 * compile pytime_fromtimespec() and _PyTime_AsTimespec(). CPython
 * needs those whenever HAVE_NANOSLEEP is also set (the HAVE_NANOSLEEP
 * path in timemodule.c calls _PyTime_AsTimespec to fill a timespec).
 * ------------------------------------------------------------------- */
#ifndef CLOCK_REALTIME
typedef int clockid_t;
#define CLOCK_REALTIME  0
#define CLOCK_MONOTONIC 1
#endif

/* -------------------------------------------------------------------
 * nanosleep -- CPython's pysleep() uses this
 * amiport: Approximated via AmigaDOS Delay() (1/50s = 20ms granularity)
 * ------------------------------------------------------------------- */
int nanosleep(const struct timespec *req, struct timespec *rem)
{
    unsigned long ticks;
    if (!req) return -1;
    ticks = (unsigned long)req->tv_sec * 50;
    ticks += (unsigned long)req->tv_nsec / 20000000; /* 20ms per tick */
    if (ticks == 0 && (req->tv_sec > 0 || req->tv_nsec > 0))
        ticks = 1;
    Delay(ticks);
    if (rem) {
        rem->tv_sec = 0;
        rem->tv_nsec = 0;
    }
    return 0;
}

/* -------------------------------------------------------------------
 * exp2 -- libnix math doesn't have exp2()
 * ------------------------------------------------------------------- */
double exp2(double x)
{
    return pow(2.0, x);
}

/* -------------------------------------------------------------------
 * clock_gettime -- AmigaOS implementation via DateStamp()
 *
 * Both CLOCK_REALTIME and CLOCK_MONOTONIC use DateStamp() since
 * AmigaOS does not change the clock during normal operation, making
 * DateStamp monotonic in practice. Resolution is 1/50s (20ms).
 *
 * CLOCK_REALTIME: DateStamp converted to Unix epoch via the Amiga->Unix
 * epoch offset (1978-01-01 vs 1970-01-01 = 252460800 seconds).
 *
 * CLOCK_MONOTONIC: seconds since Amiga epoch (1978-01-01), always
 * increasing, suitable for timeout and interval calculations.
 *
 * amiport: HAVE_CLOCK_GETTIME must be set so pytime.c compiles
 * pytime_fromtimespec() and _PyTime_AsTimespec(), which CPython
 * requires whenever HAVE_NANOSLEEP is also defined.
 * ------------------------------------------------------------------- */

/* AmigaOS epoch is 1978-01-01; Unix epoch is 1970-01-01 */
#define AMIGA_EPOCH_OFFSET 252460800UL

int clock_gettime(clockid_t clk_id, struct timespec *tp)
{
    struct DateStamp ds;

    if (!tp) {
        return -1;
    }

    DateStamp(&ds);

    tp->tv_sec = (time_t)(ds.ds_Days * 86400UL
                        + ds.ds_Minute * 60UL
                        + ds.ds_Tick / 50UL);
    tp->tv_nsec = (long)((ds.ds_Tick % 50UL) * 20000000L); /* 20ms per tick */

    if (clk_id == CLOCK_REALTIME) {
        tp->tv_sec += (time_t)AMIGA_EPOCH_OFFSET;
    }
    /* CLOCK_MONOTONIC: leave as seconds-since-Amiga-epoch (always increasing) */

    (void)clk_id; /* suppress unused warning for other clock IDs */
    return 0;
}

/* -------------------------------------------------------------------
 * _PyImport_FindSharedFuncptr -- no dynamic loading on AmigaOS
 *
 * dl_funcptr is defined in Python/importdl.h as:
 *   typedef void (*dl_funcptr)(void);   (non-Windows)
 * We inline the typedef here to avoid the path dependency.
 * ------------------------------------------------------------------- */
typedef void (*dl_funcptr)(void);

dl_funcptr
_PyImport_FindSharedFuncptr(const char *prefix,
                            const char *shortname,
                            const char *pathname,
                            FILE *fp)
{
    (void)prefix;
    (void)shortname;
    (void)pathname;
    (void)fp;
    return NULL;
}

/* Deepfreeze and frozen module stubs removed -- now provided by
 * ported/Python/deepfreeze/deepfreeze.c (generated from host Python build) */
