/*
 * amiport/sys/time.h — Time functions shim for AmigaOS
 *
 * Provides basic time operations using AmigaDOS DateStamp.
 */

#ifndef AMIPORT_SYS_TIME_H
#define AMIPORT_SYS_TIME_H

#include <exec/types.h>

/* Simplified timeval */
struct amiport_timeval {
    LONG tv_sec;
    LONG tv_usec;
};

/* Get current time (approximate — Amiga DateStamp resolution is 1/50s) */
int amiport_gettimeofday(struct amiport_timeval *tv);

/* Get time as Unix timestamp */
LONG amiport_time(LONG *tloc);

#endif /* AMIPORT_SYS_TIME_H */
