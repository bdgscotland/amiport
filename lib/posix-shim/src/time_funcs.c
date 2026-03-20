/*
 * time_funcs.c — Time functions for AmigaOS
 *
 * Uses DateStamp() for basic time operations.
 * Note: AmigaOS epoch is Jan 1, 1978 (vs Unix Jan 1, 1970).
 */

#include <amiport/sys/time.h>

#include <proto/dos.h>
#include <dos/dos.h>

/* Seconds between Unix epoch (1970) and Amiga epoch (1978) */
#define AMIGA_EPOCH_OFFSET  252460800L

int amiport_gettimeofday(struct amiport_timeval *tv)
{
    struct DateStamp ds;

    if (!tv) return -1;

    DateStamp(&ds);

    /* Convert DateStamp to Unix-style seconds */
    tv->tv_sec = (ds.ds_Days * 86400L) +
                 (ds.ds_Minute * 60L) +
                 (ds.ds_Tick / TICKS_PER_SECOND) +
                 AMIGA_EPOCH_OFFSET;

    /* Convert remaining ticks to microseconds */
    tv->tv_usec = (ds.ds_Tick % TICKS_PER_SECOND) * (1000000L / TICKS_PER_SECOND);

    return 0;
}

LONG amiport_time(LONG *tloc)
{
    struct amiport_timeval tv;

    amiport_gettimeofday(&tv);

    if (tloc) {
        *tloc = tv.tv_sec;
    }

    return tv.tv_sec;
}
