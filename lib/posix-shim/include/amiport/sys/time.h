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
 * amiport_utimes -- set file access and modification times by path
 *
 * POSIX takes a struct timeval times[2]: times[0]=atime, times[1]=mtime.
 * AmigaOS filesystems have no distinct atime concept (SetFileDate only
 * sets the single modification timestamp), so only times[1] is honored.
 *
 * If times_ptr is NULL, the current DateStamp is used (POSIX requires
 * current time when times==NULL).
 *
 * AmigaOS file time resolution is 1 second on SFS/PFS3, 2 seconds on
 * FFS (even-second alignment). Sub-second precision is discarded.
 *
 * Maps to: dos.library/SetFileDate (ADCD autodocs-3.5).
 *
 * times_ptr is typed void* so callers can pass the libnix/BSD
 * struct timeval array directly -- its layout (long tv_sec, long
 * tv_usec) is identical to struct amiport_timeval on 68k.
 *
 * Returns 0 on success, -1 on failure (errno set via
 * amiport_map_errno).
 */
int amiport_utimes(const char *path, const void *times_ptr);

/*
 * amiport_futimes -- set file times by file descriptor
 *
 * POSIX takes (fd, times[2]). Delegates to amiport_futimens, which
 * recovers the path from the fd's BPTR via NameFromFH() and calls
 * SetFileDate(). Only works for fds in the amiport fd table
 * (amiport_open). Passing a libnix fd (open()/fopen()) returns -1
 * with errno=EBADF, which is POSIX-correct for an invalid
 * descriptor -- libnix fds are a separate namespace from amiport's.
 *
 * If times_ptr is NULL, the current DateStamp is used.
 *
 * Maps to: dos.library/SetFileDate via amiport_futimens (ADCD
 * autodocs-3.5). See file_io.c for the NameFromFH implementation.
 *
 * Returns 0 on success, -1 on failure (errno set).
 */
int amiport_futimes(int fd, const void *times_ptr);

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
#define utimes(p, t)       amiport_utimes((p), (const void *)(t))
#define futimes(f, t)      amiport_futimes((f), (const void *)(t))
#endif

#endif /* AMIPORT_SYS_TIME_H */
