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

/*
 * amiport_tm — broken-down time structure (mirrors POSIX struct tm).
 *
 * All fields follow the same conventions as POSIX struct tm:
 *   tm_year  — years since 1900 (e.g. 126 for 2026)
 *   tm_mon   — months since January, 0-11
 *   tm_mday  — day of month, 1-31
 *   tm_hour  — hours since midnight, 0-23
 *   tm_min   — minutes, 0-59
 *   tm_sec   — seconds, 0-60 (60 allowed for leap second)
 *   tm_wday  — days since Sunday, 0-6 (not set by strptime)
 *   tm_yday  — days since January 1, 0-365 (not set by strptime)
 *   tm_isdst — daylight saving flag (not set by strptime)
 */
struct amiport_tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};

/*
 * strptime — parse a date/time string according to a format
 *
 * Supported specifiers: %Y %m %d %H %M %S %n %t %%
 * Returns pointer to first unprocessed character, or NULL on error.
 * tm_wday, tm_yday, tm_isdst are NOT set — call mktime() if needed.
 */
char *amiport_strptime(const char *s, const char *format,
    struct amiport_tm *tm);

/* Convenience macros */
#ifndef AMIPORT_NO_TIME_MACROS
#define strptime(s, f, t)  amiport_strptime(s, f, t)
#endif

#endif /* AMIPORT_SYS_TIME_H */
