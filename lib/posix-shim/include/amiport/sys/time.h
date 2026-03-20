/*
 * amiport/sys/time.h — Time functions shim for AmigaOS
 *
 * Provides basic time operations using AmigaDOS DateStamp.
 */

#ifndef AMIPORT_SYS_TIME_H
#define AMIPORT_SYS_TIME_H

#include <exec/types.h>

/* Seconds between Unix epoch (1970) and Amiga epoch (1978) */
#define AMIGA_EPOCH_OFFSET  252460800L

/* Simplified timeval */
struct amiport_timeval {
    LONG tv_sec;
    LONG tv_usec;
};

/* Get current time (approximate — Amiga DateStamp resolution is 1/50s) */
int amiport_gettimeofday(struct amiport_timeval *tv);

/* Get time as Unix timestamp */
LONG amiport_time(LONG *tloc);

/* Sleep for specified number of microseconds.
 * Resolution limited to 1/50s (20ms) when using Delay(),
 * or finer with timer.device if available. */
int amiport_usleep(ULONG usec);

#endif /* AMIPORT_SYS_TIME_H */
