/*
 * amiport profile.c -- ReadEClock-based function profiler
 *
 * Uses timer.device UNIT_ECLOCK for sub-microsecond timing.
 * E-Clock frequency: ~709,379 Hz (PAL) / ~715,909 Hz (NTSC).
 * Each tick is approximately 1.4 microseconds.
 *
 * Reference: ADCD 2.1
 *   - autodocs-3.5/timer-device-readeclock-2.md
 *   - devices/13-timer-device-device-interface.md
 *   - devices/13-timer-device-e-clock-time-and-its-relationship-to-actual.md
 */

#ifdef AMIPORT_PROFILE

#include <exec/types.h>
#include <exec/memory.h>
#include <devices/timer.h>
#include <proto/exec.h>
#include <proto/timer.h>

#include <stdio.h>
#include <string.h>

#include "amiport/profile.h"

/* Timer device globals */
static struct timerequest *prof_timer_io = NULL;
/* TimerBase is declared extern in proto/timer.h -- we define it here */
struct Device *TimerBase = NULL;
static ULONG prof_eclock_freq = 0; /* E-Clock tics per second */
static int prof_initialized = 0;

/* Profiling data table */
struct prof_entry {
    const char *name;       /* function/section name (string literal, not copied) */
    ULONG total_ticks;      /* accumulated E-Clock ticks */
    ULONG call_count;       /* number of BEGIN/END pairs */
    ULONG max_ticks;        /* worst-case single call */
};

static struct prof_entry prof_table[AMIPORT_PROFILE_MAX_ENTRIES];
static int prof_count = 0;

/*
 * amiport_profile_init -- open timer.device and calibrate E-Clock.
 *
 * The timer device must be opened with UNIT_ECLOCK to use ReadEClock().
 * We allocate the timerequest with AllocMem (not CreateExtIO) because
 * we don't need a message port -- ReadEClock is a direct library call.
 *
 * ADCD reference: devices/13-device-interface-opening-the-timer-device.md
 */
int
amiport_profile_init(void)
{
    struct EClockVal ecv;

    if (prof_initialized)
        return 0;

    /* Allocate timerequest -- MEMF_PUBLIC|MEMF_CLEAR as per ADCD example */
    prof_timer_io = (struct timerequest *)AllocMem(
        sizeof(struct timerequest), MEMF_PUBLIC | MEMF_CLEAR);
    if (!prof_timer_io)
        return -1;

    /* Open timer.device with UNIT_ECLOCK */
    if (OpenDevice(TIMERNAME, UNIT_ECLOCK,
                   (struct IORequest *)prof_timer_io, 0) != 0) {
        FreeMem(prof_timer_io, sizeof(struct timerequest));
        prof_timer_io = NULL;
        return -1;
    }

    /* Get TimerBase for ReadEClock library call.
     * proto/timer.h declares TimerBase as struct Device*, not struct Library*. */
    TimerBase = prof_timer_io->tr_node.io_Device;

    /* Read E-Clock frequency (returned by ReadEClock) */
    prof_eclock_freq = ReadEClock(&ecv);

    prof_initialized = 1;
    memset(prof_table, 0, sizeof(prof_table));
    prof_count = 0;

    return 0;
}

/*
 * amiport_profile_eclock -- read low 32 bits of E-Clock.
 *
 * Only uses ev_lo for simplicity. This wraps every ~6060 seconds
 * (2^32 / 709379 Hz). For intervals < 1 hour, ev_lo subtraction
 * handles wrap correctly via unsigned arithmetic.
 */
ULONG
amiport_profile_eclock(void)
{
    struct EClockVal ecv;

    if (!prof_initialized)
        return 0;
    ReadEClock(&ecv);
    return ecv.ev_lo;
}

/*
 * amiport_profile_record -- accumulate a timing sample.
 *
 * Lookup is linear (max 64 entries). For typical profiling with
 * 10-20 instrumented points, this is negligible overhead.
 */
void
amiport_profile_record(const char *name, ULONG ticks)
{
    int i;

    /* Find existing entry */
    for (i = 0; i < prof_count; i++) {
        if (prof_table[i].name == name) { /* pointer compare -- string literals */
            prof_table[i].total_ticks += ticks;
            prof_table[i].call_count++;
            if (ticks > prof_table[i].max_ticks)
                prof_table[i].max_ticks = ticks;
            return;
        }
    }

    /* New entry */
    if (prof_count < AMIPORT_PROFILE_MAX_ENTRIES) {
        prof_table[prof_count].name = name;
        prof_table[prof_count].total_ticks = ticks;
        prof_table[prof_count].call_count = 1;
        prof_table[prof_count].max_ticks = ticks;
        prof_count++;
    }
}

/*
 * amiport_profile_summary -- print sorted results and clean up.
 *
 * Output format designed to be grep-friendly and readable on a 640x200
 * Amiga console (80 columns). Sorted by total_ticks descending.
 * Uses integer arithmetic only (no floating point on 68000).
 */
void
amiport_profile_summary(void)
{
    int i, j;
    struct prof_entry tmp;
    ULONG grand_total = 0;

    if (!prof_initialized || prof_count == 0) {
        if (prof_timer_io) {
            CloseDevice((struct IORequest *)prof_timer_io);
            FreeMem(prof_timer_io, sizeof(struct timerequest));
        }
        return;
    }

    /* Insertion sort by total_ticks descending */
    for (i = 1; i < prof_count; i++) {
        tmp = prof_table[i];
        j = i - 1;
        while (j >= 0 && prof_table[j].total_ticks < tmp.total_ticks) {
            prof_table[j + 1] = prof_table[j];
            j--;
        }
        prof_table[j + 1] = tmp;
    }

    for (i = 0; i < prof_count; i++)
        grand_total += prof_table[i].total_ticks;

    /* Header */
    fprintf(stderr, "\n=== amiport profiler (%lu Hz) ===\n", prof_eclock_freq);
    fprintf(stderr, "%-20s %8s %10s %8s %8s %5s\n",
            "Function", "Calls", "Total(ms)", "Avg(us)", "Max(us)", "%");

    /* Rows */
    for (i = 0; i < prof_count; i++) {
        ULONG calls = prof_table[i].call_count;
        ULONG total = prof_table[i].total_ticks;
        ULONG maxt = prof_table[i].max_ticks;

        /* Convert ticks to milliseconds: total * 1000 / freq
         * Use 64-bit intermediate to avoid overflow on large totals */
        ULONG total_ms = (total / (prof_eclock_freq / 1000));

        /* Average in microseconds: (total / calls) * 1000000 / freq */
        ULONG avg_us = 0;
        if (calls > 0)
            avg_us = (total / calls) * 1000 / (prof_eclock_freq / 1000);

        /* Max in microseconds */
        ULONG max_us = maxt * 1000 / (prof_eclock_freq / 1000);

        /* Percentage: total * 100 / grand_total */
        ULONG pct = 0;
        if (grand_total > 0)
            pct = total * 100 / grand_total;

        fprintf(stderr, "%-20s %8lu %10lu %8lu %8lu %4lu%%\n",
                prof_table[i].name, calls, total_ms, avg_us, max_us, pct);
    }

    /* Footer */
    {
        ULONG gt_ms = grand_total / (prof_eclock_freq / 1000);
        fprintf(stderr, "Total measured: %lu ms\n\n", gt_ms);
    }

    /* Cleanup timer device */
    CloseDevice((struct IORequest *)prof_timer_io);
    FreeMem(prof_timer_io, sizeof(struct timerequest));
    prof_timer_io = NULL;
    TimerBase = NULL;
    prof_initialized = 0;
}

#endif /* AMIPORT_PROFILE */
