/*
 * amiport/profile.h -- ReadEClock-based function profiler for AmigaOS
 *
 * Usage:
 *   #include <amiport/profile.h>
 *
 *   // In main(), before any profiled code:
 *   amiport_profile_init();
 *   atexit(amiport_profile_summary);
 *
 *   // Around code to measure:
 *   AMIPORT_PROFILE_BEGIN("readhash");
 *   ... code ...
 *   AMIPORT_PROFILE_END("readhash");
 *
 * Build with -DAMIPORT_PROFILE to enable. Without it, macros expand
 * to nothing (zero overhead in production builds).
 *
 * Resolution: ~1.4 us per tick (709 KHz PAL / 716 KHz NTSC).
 * Overhead: ~20 us per BEGIN/END pair (two ReadEClock calls + table update).
 *
 * Reference: ADCD 2.1, timer.device/ReadEClock (V36+)
 */

#ifndef AMIPORT_PROFILE_H
#define AMIPORT_PROFILE_H

#ifdef AMIPORT_PROFILE

#include <exec/types.h>

/* Maximum number of distinct profiling points */
#define AMIPORT_PROFILE_MAX_ENTRIES 64

/* Initialize the profiler -- opens timer.device UNIT_ECLOCK.
 * Call once at program startup. Returns 0 on success, -1 on failure. */
int amiport_profile_init(void);

/* Print sorted profiling summary to stdout.
 * Register with atexit() for automatic output. */
void amiport_profile_summary(void);

/* Internal: record a timing sample */
void amiport_profile_record(const char *name, ULONG ticks);

/* Internal: read E-Clock low word (fast inline) */
ULONG amiport_profile_eclock(void);

/* Profiling macros -- simple statements, not do/while.
 * Supports multiple END points and early returns between BEGIN/END.
 * _ap_t0 is declared in function scope -- one BEGIN per function. */
#define AMIPORT_PROFILE_BEGIN(name) \
    ULONG _ap_t0 = amiport_profile_eclock();

#define AMIPORT_PROFILE_END(name) \
    amiport_profile_record(name, amiport_profile_eclock() - _ap_t0);

#else /* !AMIPORT_PROFILE */

/* No-op when profiling is disabled -- zero overhead, no variables */
#define AMIPORT_PROFILE_BEGIN(name) /* nothing */
#define AMIPORT_PROFILE_END(name)   /* nothing */

/* Stubs that compile to nothing */
#define amiport_profile_init()    (0)
#define amiport_profile_summary() ((void)0)

#endif /* AMIPORT_PROFILE */

#endif /* AMIPORT_PROFILE_H */
