/*
 * time_funcs.c — Time functions for AmigaOS
 *
 * Uses DateStamp() for basic time operations.
 * Note: AmigaOS epoch is Jan 1, 1978 (vs Unix Jan 1, 1970).
 */

#include <amiport/sys/time.h>

#include <proto/dos.h>
#include <dos/dos.h>

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

int amiport_usleep(ULONG usec)
{
    ULONG ticks;

    /* Convert microseconds to Amiga ticks (1 tick = 20ms = 20000us on PAL).
     * Minimum delay is 1 tick if usec > 0. */
    ticks = usec / 20000;
    if (ticks == 0 && usec > 0) {
        ticks = 1;
    }

    if (ticks > 0) {
        Delay(ticks);
    }

    return 0;
}

/*
 * strptime — parse a time string according to a format
 *
 * amiport: minimal implementation supporting the most common format
 * specifiers used by OpenBSD and POSIX tools.  Unsupported specifiers
 * cause an immediate NULL return so callers can detect the gap.
 *
 * Supported specifiers:
 *   %Y  — 4-digit year (e.g. 2026)
 *   %m  — month 01-12
 *   %d  — day of month 01-31
 *   %H  — hour 00-23
 *   %M  — minute 00-59
 *   %S  — second 00-60 (allow 60 for leap second)
 *   %n  — any whitespace (consumes one or more whitespace chars)
 *   %t  — any whitespace (same as %n)
 *   %%  — literal '%'
 *
 * Returns a pointer to the first unprocessed character in s, or NULL
 * on parse error or unsupported specifier.
 *
 * Note: tm_wday, tm_yday, and tm_isdst are NOT set — callers that need
 * these must call mktime() on the result.
 */
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/*
 * parse_int — consume up to max_digits decimal digits from *p,
 * storing the result in *out.  Returns 1 on success, 0 if no digits.
 */
static int parse_int(const char **p, int *out, int max_digits)
{
    int n = 0;
    int count = 0;

    while (count < max_digits && isdigit((unsigned char)**p)) {
        n = n * 10 + (**p - '0');
        (*p)++;
        count++;
    }

    if (count == 0)
        return 0;

    *out = n;
    return 1;
}

char *
amiport_strptime(const char *s, const char *format, struct amiport_tm *tm)
{
    const char *fp = format;
    const char *sp = s;
    int val;

    if (!s || !format || !tm)
        return NULL;

    while (*fp) {
        if (*fp != '%') {
            /* Literal character — must match exactly */
            if (*sp != *fp)
                return NULL;
            sp++;
            fp++;
            continue;
        }

        fp++; /* skip '%' */

        switch (*fp) {
        case 'Y':
            /* 4-digit year */
            if (!parse_int(&sp, &val, 4))
                return NULL;
            tm->tm_year = val - 1900; /* struct tm stores year-1900 */
            break;

        case 'm':
            /* Month 01-12 */
            if (!parse_int(&sp, &val, 2))
                return NULL;
            if (val < 1 || val > 12)
                return NULL;
            tm->tm_mon = val - 1; /* struct tm months are 0-based */
            break;

        case 'd':
            /* Day 01-31 */
            if (!parse_int(&sp, &val, 2))
                return NULL;
            if (val < 1 || val > 31)
                return NULL;
            tm->tm_mday = val;
            break;

        case 'H':
            /* Hour 00-23 */
            if (!parse_int(&sp, &val, 2))
                return NULL;
            if (val < 0 || val > 23)
                return NULL;
            tm->tm_hour = val;
            break;

        case 'M':
            /* Minute 00-59 */
            if (!parse_int(&sp, &val, 2))
                return NULL;
            if (val < 0 || val > 59)
                return NULL;
            tm->tm_min = val;
            break;

        case 'S':
            /* Second 00-60 (allow 60 for leap second) */
            if (!parse_int(&sp, &val, 2))
                return NULL;
            if (val < 0 || val > 60)
                return NULL;
            tm->tm_sec = val;
            break;

        case 'n':
        case 't':
            /* Whitespace — consume one or more whitespace chars */
            if (!isspace((unsigned char)*sp))
                return NULL;
            while (isspace((unsigned char)*sp))
                sp++;
            break;

        case '%':
            /* Literal '%' */
            if (*sp != '%')
                return NULL;
            sp++;
            break;

        default:
            /* Unsupported specifier — return NULL so caller can detect gap */
            return NULL;
        }

        fp++;
    }

    return (char *)sp;
}
